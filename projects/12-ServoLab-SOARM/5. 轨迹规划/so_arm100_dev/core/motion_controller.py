# -*- coding: utf-8 -*-
"""
SO-ARM100机械臂运动控制器
负责轨迹执行、跟踪控制和安全监控

功能：
- 轨迹执行（阻塞/非阻塞）
- PD跟踪控制
- 速度/加速度限制
- 紧急停止
- 运动状态监控
"""

import time
import threading
import logging
from typing import Dict, Optional, Callable, List
from dataclasses import dataclass
from enum import Enum

from .trajectory import Trajectory, JointTrajectoryPlanner

logger = logging.getLogger(__name__)


class MotionState(Enum):
    """运动状态"""
    IDLE = "idle"           # 空闲
    RUNNING = "running"     # 执行中
    PAUSED = "paused"       # 暂停
    STOPPED = "stopped"     # 停止
    ERROR = "error"         # 错误


@dataclass
class MotionStatus:
    """运动状态信息"""
    state: MotionState
    progress: float          # 进度 [0, 1]
    elapsed_time: float      # 已用时间
    remaining_time: float    # 剩余时间
    current_position: Dict[str, float]  # 当前位置
    target_position: Dict[str, float]   # 目标位置
    tracking_error: float    # 跟踪误差


class MotionController:
    """
    运动控制器
    
    负责轨迹的执行和跟踪控制
    """
    
    def __init__(self, 
                 robot,
                 control_frequency: float = 50.0,
                 position_tolerance: float = 0.02,
                 max_tracking_error: float = 0.2):
        """
        初始化运动控制器
        
        Args:
            robot: SO100Robot实例
            control_frequency: 控制频率（Hz）
            position_tolerance: 位置到达容差（归一化）
            max_tracking_error: 最大允许跟踪误差
        """
        self.robot = robot
        self.control_frequency = control_frequency
        self.dt = 1.0 / control_frequency
        self.position_tolerance = position_tolerance
        self.max_tracking_error = max_tracking_error
        
        # 运动状态
        self._state = MotionState.IDLE
        self._current_trajectory: Optional[Trajectory] = None
        self._start_time = 0
        self._pause_time = 0
        
        # 线程控制
        self._motion_thread: Optional[threading.Thread] = None
        self._stop_event = threading.Event()
        self._pause_event = threading.Event()
        
        # 回调函数
        self._on_complete: Optional[Callable] = None
        self._on_error: Optional[Callable] = None
        self._on_progress: Optional[Callable] = None
        
        # 轨迹规划器
        self.planner = JointTrajectoryPlanner()
        
        # 性能统计
        self._iteration_count = 0
        self._total_error = 0.0
    
    @property
    def state(self) -> MotionState:
        """获取当前运动状态"""
        return self._state
    
    @property
    def is_moving(self) -> bool:
        """是否正在运动"""
        return self._state == MotionState.RUNNING
    
    def get_status(self) -> MotionStatus:
        """获取详细运动状态"""
        current_pos = {}
        target_pos = {}
        progress = 0.0
        elapsed = 0.0
        remaining = 0.0
        error = 0.0
        
        if self.robot.is_connected:
            try:
                current_pos = self.robot.get_observation()
            except:
                pass
        
        if self._current_trajectory and self._state == MotionState.RUNNING:
            elapsed = time.time() - self._start_time
            duration = self._current_trajectory.duration
            progress = min(1.0, elapsed / duration) if duration > 0 else 1.0
            remaining = max(0, duration - elapsed)
            target_pos = self._current_trajectory.sample(elapsed)
            
            # 计算跟踪误差
            if current_pos and target_pos:
                errors = [abs(current_pos.get(j, 0) - target_pos.get(j, 0)) 
                         for j in target_pos.keys()]
                error = max(errors) if errors else 0
        
        return MotionStatus(
            state=self._state,
            progress=progress,
            elapsed_time=elapsed,
            remaining_time=remaining,
            current_position=current_pos,
            target_position=target_pos,
            tracking_error=error
        )
    
    def move_to_position(self, 
                        target: Dict[str, float],
                        duration: float = None,
                        blocking: bool = True) -> bool:
        """
        移动到目标位置
        
        Args:
            target: 目标关节位置（归一化 [-1, 1]）
            duration: 运动时长（秒），None表示自动计算
            blocking: 是否阻塞等待完成
        
        Returns:
            bool: 是否成功启动（阻塞模式下返回是否成功完成）
        """
        if not self.robot.is_connected:
            logger.error("机械臂未连接")
            return False
        
        if self._state == MotionState.RUNNING:
            logger.warning("正在执行其他运动，请先停止")
            return False
        
        # 获取当前位置
        try:
            current = self.robot.get_observation()
        except Exception as e:
            logger.error(f"读取当前位置失败: {e}")
            return False
        
        # 补全目标位置
        full_target = current.copy()
        full_target.update(target)
        
        # 规划轨迹
        trajectory = self.planner.plan_point_to_point(current, full_target, duration)
        
        # 执行轨迹
        return self.execute_trajectory(trajectory, blocking=blocking)
    
    def move_through_waypoints(self,
                              waypoints: List[Dict[str, float]],
                              duration: float = None,
                              timestamps: List[float] = None,
                              blocking: bool = True) -> bool:
        """
        经过多个路径点
        
        Args:
            waypoints: 路径点列表
            duration: 总时长
            timestamps: 时间戳列表
            blocking: 是否阻塞
        
        Returns:
            bool: 是否成功
        """
        if not self.robot.is_connected:
            logger.error("机械臂未连接")
            return False
        
        if self._state == MotionState.RUNNING:
            logger.warning("正在执行其他运动")
            return False
        
        # 规划轨迹
        trajectory = self.planner.plan_via_waypoints(waypoints, timestamps, duration)
        
        return self.execute_trajectory(trajectory, blocking=blocking)
    
    def execute_trajectory(self, 
                          trajectory: Trajectory,
                          blocking: bool = True,
                          on_complete: Callable = None,
                          on_error: Callable = None) -> bool:
        """
        执行轨迹
        
        Args:
            trajectory: 轨迹对象
            blocking: 是否阻塞等待完成
            on_complete: 完成回调
            on_error: 错误回调
        
        Returns:
            bool: 阻塞模式返回是否成功完成，非阻塞返回是否成功启动
        """
        if not self.robot.is_connected:
            logger.error("机械臂未连接")
            return False
        
        if self._state == MotionState.RUNNING:
            logger.warning("正在执行其他运动")
            return False
        
        self._current_trajectory = trajectory
        self._on_complete = on_complete
        self._on_error = on_error
        self._stop_event.clear()
        self._pause_event.clear()
        
        if blocking:
            return self._execute_trajectory_sync()
        else:
            self._motion_thread = threading.Thread(
                target=self._execute_trajectory_async,
                daemon=True
            )
            self._motion_thread.start()
            return True
    
    def _execute_trajectory_sync(self) -> bool:
        """同步执行轨迹"""
        self._state = MotionState.RUNNING
        self._start_time = time.time()
        self._iteration_count = 0
        self._total_error = 0.0
        
        trajectory = self._current_trajectory
        
        logger.info(f"开始执行轨迹，时长: {trajectory.duration:.2f}s")
        
        try:
            while True:
                loop_start = time.perf_counter()
                
                # 检查停止信号
                if self._stop_event.is_set():
                    logger.info("轨迹执行被停止")
                    self._state = MotionState.STOPPED
                    return False
                
                # 检查暂停
                while self._pause_event.is_set():
                    time.sleep(0.1)
                    if self._stop_event.is_set():
                        self._state = MotionState.STOPPED
                        return False
                
                # 计算当前时间
                elapsed = time.time() - self._start_time
                
                # 检查是否完成
                if elapsed >= trajectory.duration:
                    # 发送最终位置
                    final_pos = trajectory.sample(trajectory.duration)
                    self.robot.send_action(final_pos)
                    
                    self._state = MotionState.IDLE
                    avg_error = self._total_error / max(1, self._iteration_count)
                    logger.info(f"轨迹执行完成，平均跟踪误差: {avg_error:.4f}")
                    
                    if self._on_complete:
                        self._on_complete()
                    
                    return True
                
                # 采样目标位置
                target_pos = trajectory.sample(elapsed)
                
                # 发送动作
                self.robot.send_action(target_pos)
                
                # 计算跟踪误差（可选）
                self._iteration_count += 1
                
                # 频率控制
                loop_time = time.perf_counter() - loop_start
                sleep_time = self.dt - loop_time
                if sleep_time > 0:
                    time.sleep(sleep_time)
                elif self._iteration_count % 100 == 0:
                    actual_freq = 1.0 / loop_time if loop_time > 0 else 0
                    logger.debug(f"控制循环超时: {loop_time*1000:.1f}ms, 实际频率: {actual_freq:.1f}Hz")
        
        except Exception as e:
            logger.error(f"轨迹执行错误: {e}")
            self._state = MotionState.ERROR
            if self._on_error:
                self._on_error(e)
            return False
    
    def _execute_trajectory_async(self):
        """异步执行轨迹"""
        self._execute_trajectory_sync()
    
    def pause(self) -> None:
        """暂停运动"""
        if self._state == MotionState.RUNNING:
            self._pause_event.set()
            self._pause_time = time.time()
            self._state = MotionState.PAUSED
            logger.info("运动已暂停")
    
    def resume(self) -> None:
        """继续运动"""
        if self._state == MotionState.PAUSED:
            pause_duration = time.time() - self._pause_time
            self._start_time += pause_duration  # 调整开始时间
            self._pause_event.clear()
            self._state = MotionState.RUNNING
            logger.info("运动已继续")
    
    def stop(self) -> None:
        """停止运动"""
        self._stop_event.set()
        self._pause_event.clear()
        
        # 等待线程结束
        if self._motion_thread and self._motion_thread.is_alive():
            self._motion_thread.join(timeout=1.0)
        
        self._state = MotionState.STOPPED
        logger.info("运动已停止")
    
    def emergency_stop(self) -> None:
        """紧急停止"""
        self.stop()
        
        # 关闭扭矩
        if self.robot.is_connected:
            try:
                self.robot.disable_torque_all()
                logger.warning("紧急停止：扭矩已关闭")
            except Exception as e:
                logger.error(f"紧急停止失败: {e}")
    
    def wait_for_completion(self, timeout: float = None) -> bool:
        """
        等待运动完成
        
        Args:
            timeout: 超时时间（秒）
        
        Returns:
            bool: 是否成功完成
        """
        if self._motion_thread is None:
            return self._state == MotionState.IDLE
        
        self._motion_thread.join(timeout=timeout)
        return self._state == MotionState.IDLE
    
    def move_home(self, duration: float = 3.0, blocking: bool = True) -> bool:
        """
        移动到零位（所有关节归零）
        
        Args:
            duration: 运动时长
            blocking: 是否阻塞
        
        Returns:
            bool: 是否成功
        """
        home_position = {joint: 0.0 for joint in self.planner.joint_names}
        return self.move_to_position(home_position, duration, blocking)
    
    def jog(self, 
           joint_name: str, 
           direction: int, 
           speed: float = 0.5,
           duration: float = 0.5) -> bool:
        """
        点动控制（单关节微调）
        
        Args:
            joint_name: 关节名称
            direction: 方向（1=正向, -1=反向）
            speed: 速度（归一化单位/秒）
            duration: 持续时间
        
        Returns:
            bool: 是否成功
        """
        if not self.robot.is_connected:
            return False
        
        try:
            current = self.robot.get_observation()
            current_pos = current.get(joint_name, 0.0)
            
            # 计算目标位置
            delta = direction * speed * duration
            target_pos = max(-1.0, min(1.0, current_pos + delta))
            
            target = {joint_name: target_pos}
            return self.move_to_position(target, duration, blocking=True)
        
        except Exception as e:
            logger.error(f"点动失败: {e}")
            return False


class TrajectoryRecorder:
    """
    轨迹记录器
    
    记录手动拖动（遥操作）过程中的轨迹数据
    """
    
    def __init__(self, robot, frequency: float = 50.0):
        """
        Args:
            robot: SO100Robot实例
            frequency: 采样频率
        """
        self.robot = robot
        self.frequency = frequency
        self.dt = 1.0 / frequency
        
        self._recording = False
        self._record_thread: Optional[threading.Thread] = None
        self._data: List[Dict] = []
        self._start_time = 0
    
    def start_recording(self) -> None:
        """开始记录"""
        if self._recording:
            logger.warning("已在记录中")
            return
        
        self._data = []
        self._recording = True
        self._start_time = time.time()
        
        self._record_thread = threading.Thread(
            target=self._record_loop,
            daemon=True
        )
        self._record_thread.start()
        logger.info("开始记录轨迹")
    
    def stop_recording(self) -> List[Dict]:
        """
        停止记录
        
        Returns:
            List[Dict]: 记录的数据
        """
        self._recording = False
        
        if self._record_thread:
            self._record_thread.join(timeout=1.0)
        
        logger.info(f"记录完成，共 {len(self._data)} 个数据点")
        return self._data
    
    def _record_loop(self) -> None:
        """记录循环"""
        while self._recording:
            loop_start = time.perf_counter()
            
            try:
                if self.robot.is_connected:
                    obs = self.robot.get_observation()
                    timestamp = time.time() - self._start_time
                    
                    self._data.append({
                        'timestamp': timestamp,
                        'positions': obs.copy()
                    })
            except Exception as e:
                logger.debug(f"记录失败: {e}")
            
            # 频率控制
            elapsed = time.perf_counter() - loop_start
            sleep_time = self.dt - elapsed
            if sleep_time > 0:
                time.sleep(sleep_time)
    
    def get_trajectory(self) -> Optional[Trajectory]:
        """
        将记录的数据转换为轨迹
        
        Returns:
            Trajectory: 轨迹对象
        """
        if not self._data:
            return None
        
        waypoints = [d['positions'] for d in self._data]
        timestamps = [d['timestamp'] for d in self._data]
        
        planner = JointTrajectoryPlanner(interpolation_method='cubic_spline')
        return planner.plan_via_waypoints(waypoints, timestamps)
    
    def save_to_file(self, filepath: str) -> None:
        """保存数据到文件"""
        import json
        
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump({
                'frequency': self.frequency,
                'data': self._data
            }, f, indent=2)
        
        logger.info(f"轨迹已保存到: {filepath}")
    
    @classmethod
    def load_from_file(cls, filepath: str) -> List[Dict]:
        """从文件加载数据"""
        import json
        
        with open(filepath, 'r', encoding='utf-8') as f:
            content = json.load(f)
        
        return content.get('data', [])


class MotionPrimitives:
    """
    运动原语
    
    提供常用的基础运动模式
    """
    
    def __init__(self, controller: MotionController):
        """
        Args:
            controller: MotionController实例
        """
        self.controller = controller
        self.robot = controller.robot
    
    def wave(self, 
            amplitude: float = 0.3,
            frequency: float = 1.0,
            duration: float = 5.0,
            joint: str = 'wrist_roll') -> bool:
        """
        挥手动作
        
        Args:
            amplitude: 幅度
            frequency: 频率（Hz）
            duration: 持续时间
            joint: 运动关节
        """
        import numpy as np
        
        n_points = int(duration * 50)
        timestamps = np.linspace(0, duration, n_points)
        
        # 获取当前位置
        current = self.robot.get_observation()
        center = current.get(joint, 0.0)
        
        waypoints = []
        for t in timestamps:
            pos = current.copy()
            pos[joint] = center + amplitude * np.sin(2 * np.pi * frequency * t)
            waypoints.append(pos)
        
        trajectory = self.controller.planner.plan_via_waypoints(
            waypoints, timestamps.tolist()
        )
        
        return self.controller.execute_trajectory(trajectory, blocking=True)
    
    def nod(self, 
           amplitude: float = 0.2,
           count: int = 3,
           duration_per_nod: float = 0.5) -> bool:
        """
        点头动作
        
        Args:
            amplitude: 幅度
            count: 次数
            duration_per_nod: 每次时间
        """
        current = self.robot.get_observation()
        center = current.get('wrist_flex', 0.0)
        
        waypoints = [current.copy()]
        timestamps = [0.0]
        
        for i in range(count):
            t_base = i * duration_per_nod * 2
            
            # 下
            pos = current.copy()
            pos['wrist_flex'] = center + amplitude
            waypoints.append(pos)
            timestamps.append(t_base + duration_per_nod)
            
            # 上
            pos = current.copy()
            pos['wrist_flex'] = center - amplitude * 0.5
            waypoints.append(pos)
            timestamps.append(t_base + duration_per_nod * 2)
        
        # 回到原位
        waypoints.append(current.copy())
        timestamps.append(timestamps[-1] + duration_per_nod)
        
        trajectory = self.controller.planner.plan_via_waypoints(
            waypoints, timestamps
        )
        
        return self.controller.execute_trajectory(trajectory, blocking=True)
    
    def pick_and_place(self,
                      pick_position: Dict[str, float],
                      place_position: Dict[str, float],
                      approach_height: float = 0.3,
                      grasp_position: float = 0.8,
                      release_position: float = -0.8,
                      move_duration: float = 2.0,
                      grasp_duration: float = 0.5) -> bool:
        """
        抓取和放置动作
        
        Args:
            pick_position: 抓取位置（关节空间）
            place_position: 放置位置（关节空间）
            approach_height: 接近高度偏移
            grasp_position: 夹爪闭合位置
            release_position: 夹爪张开位置
            move_duration: 移动时间
            grasp_duration: 夹爪动作时间
        
        Returns:
            bool: 是否成功
        """
        current = self.robot.get_observation()
        
        # 1. 张开夹爪
        open_gripper = current.copy()
        open_gripper['gripper'] = release_position
        if not self.controller.move_to_position(open_gripper, grasp_duration):
            return False
        
        # 2. 移动到抓取位置上方
        approach_pick = pick_position.copy()
        if 'shoulder_lift' in approach_pick:
            approach_pick['shoulder_lift'] -= approach_height
        if not self.controller.move_to_position(approach_pick, move_duration):
            return False
        
        # 3. 下降到抓取位置
        if not self.controller.move_to_position(pick_position, move_duration * 0.5):
            return False
        
        # 4. 闭合夹爪
        grasp = pick_position.copy()
        grasp['gripper'] = grasp_position
        if not self.controller.move_to_position(grasp, grasp_duration):
            return False
        
        # 5. 提升
        if not self.controller.move_to_position(approach_pick, move_duration * 0.5):
            return False
        
        # 6. 移动到放置位置上方
        approach_place = place_position.copy()
        if 'shoulder_lift' in approach_place:
            approach_place['shoulder_lift'] -= approach_height
        approach_place['gripper'] = grasp_position
        if not self.controller.move_to_position(approach_place, move_duration):
            return False
        
        # 7. 下降到放置位置
        place_pos = place_position.copy()
        place_pos['gripper'] = grasp_position
        if not self.controller.move_to_position(place_pos, move_duration * 0.5):
            return False
        
        # 8. 释放
        release = place_position.copy()
        release['gripper'] = release_position
        if not self.controller.move_to_position(release, grasp_duration):
            return False
        
        # 9. 提升
        approach_place['gripper'] = release_position
        if not self.controller.move_to_position(approach_place, move_duration * 0.5):
            return False
        
        logger.info("抓取放置完成")
        return True
