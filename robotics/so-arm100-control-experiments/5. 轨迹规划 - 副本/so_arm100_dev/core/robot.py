"""
SO-ARM100机械臂控制类（参考LeRobot实现）
提供高层控制接口，包括连接、校准、读取观测、发送动作等
"""

import json
import logging
import time
from pathlib import Path
from typing import Dict, Optional, Any

from .motor_bus import MotorBus
from .calibration import MotorCalibration
from config.motor_config import MOTOR_IDS, OperatingMode


logger = logging.getLogger(__name__)


class SO100Robot:
    """
    SO-ARM100机械臂控制类
    
    功能：
    - 连接/断开机械臂
    - 自动加载校准数据
    - 读取关节位置（观测）
    - 发送目标位置（动作）
    - 安全限制（最大相对移动距离）
    """
    
    def __init__(self, 
                 port: str = "COM3",
                 baudrate: int = 1_000_000,
                 calibration_dir: str = "./calibration",
                 robot_id: str = "default",
                 max_relative_target: Optional[float] = None,
                 disable_torque_on_disconnect: bool = True):
        """
        初始化机械臂控制器
        
        Args:
            port: 串口名称
            baudrate: 波特率
            calibration_dir: 校准数据目录
            robot_id: 机械臂ID，用于区分不同机械臂的校准文件（如 "leader", "follower"）
            max_relative_target: 最大相对移动距离（归一化值），None表示不限制
            disable_torque_on_disconnect: 断开连接时是否关闭扭矩
        """
        self.port = port
        self.baudrate = baudrate
        self.calibration_dir = Path(calibration_dir)
        self.robot_id = robot_id
        self.max_relative_target = max_relative_target
        self.disable_torque_on_disconnect = disable_torque_on_disconnect
        
        # 创建舵机总线
        self.bus = MotorBus(port, baudrate)
        
        # 校准数据
        self.calibration_data: Optional[Dict] = None
        self._connected = False
    
    @property
    def is_connected(self) -> bool:
        """检查是否已连接"""
        return self._connected and self.bus.is_connected
    
    @property
    def is_calibrated(self) -> bool:
        """检查是否已校准"""
        return self.calibration_data is not None
    
    def connect(self, auto_load_calibration: bool = True) -> None:
        """
        连接机械臂
        
        Args:
            auto_load_calibration: 是否自动加载校准数据
        """
        if self.is_connected:
            logger.warning("机械臂已连接")
            return
        
        # 连接串口
        if not self.bus.connect():
            raise RuntimeError(f"无法连接到串口: {self.port}")
        
        logger.info(f"✓ 已连接到 {self.port}")
        
        # 加载校准数据
        if auto_load_calibration:
            self._load_calibration()
        
        # 配置舵机
        self.configure()
        
        self._connected = True
        logger.info("机械臂已连接并配置完成")
    
    def disconnect(self) -> None:
        """断开连接"""
        if not self.is_connected:
            logger.warning("机械臂未连接")
            return
        
        # 关闭扭矩
        if self.disable_torque_on_disconnect:
            logger.info("关闭所有舵机扭矩...")
            for motor_id in MOTOR_IDS.values():
                self.bus.disable_torque(motor_id)
        
        # 断开串口
        self.bus.disconnect()
        self._connected = False
        logger.info("机械臂已断开连接")
    
    def _load_calibration(self) -> None:
        """加载校准数据（根据robot_id加载对应文件）"""
        calibration_file = self.calibration_dir / f"so_arm100_calibration_{self.robot_id}.json"
        
        if not calibration_file.exists():
            logger.warning(f"未找到校准文件: {calibration_file}")
            logger.warning(f"请先为机械臂 '{self.robot_id}' 运行校准程序")
            return
        
        with open(calibration_file, 'r', encoding='utf-8') as f:
            self.calibration_data = json.load(f)
        
        logger.info(f"✓ 已加载校准数据: {calibration_file} (robot_id={self.robot_id})")
        
        # 写入校准数据到舵机
        self._write_calibration_to_motors()
    
    def _write_calibration_to_motors(self) -> None:
        """将校准数据写入舵机（参考LeRobot实现）"""
        if not self.calibration_data:
            return
        
        logger.info("写入校准数据到舵机...")
        
        for motor_name, calib in self.calibration_data.items():
            if motor_name not in MOTOR_IDS:
                continue
            
            motor_id = MOTOR_IDS[motor_name]
            
            # 写入零点偏移
            homing_offset = calib["homing_offset"]
            self.bus.write_control_table(motor_id, "Homing_Offset", homing_offset)
            
            # 写入位置限制（关键！LeRobot实现）
            range_min = calib["range_min"]
            range_max = calib["range_max"]
            self.bus.write_control_table(motor_id, "Min_Position_Limit", range_min)
            self.bus.write_control_table(motor_id, "Max_Position_Limit", range_max)
            
            logger.debug(
                f"  {motor_name}: Homing_Offset={homing_offset}, "
                f"Position_Limit=[{range_min}, {range_max}]"
            )
            
            time.sleep(0.05)
    
    def configure(self) -> None:
        """配置舵机参数（参考LeRobot实现）"""
        logger.info("配置舵机参数...")
        
        # 使用上下文管理器在扭矩禁用状态下配置
        with self.bus.torque_disabled():
            # 配置每个舵机
            for motor_name, motor_id in MOTOR_IDS.items():
                # 设置位置模式
                self.bus.set_operating_mode(motor_id, OperatingMode.POSITION)
                
                # 夹爪需要更强的保持力；其他关节保持较低P系数避免抖动
                p_coefficient = 32 if motor_name == "gripper" else 16
                self.bus.write_control_table(motor_id, "P_Coefficient", p_coefficient)
                self.bus.write_control_table(motor_id, "I_Coefficient", 0)
                self.bus.write_control_table(motor_id, "D_Coefficient", 32)
                
                # 夹爪特殊配置：提高可用扭矩，同时保留过流/过载保护
                if motor_name == "gripper":
                    self.bus.write_control_table(motor_id, "Max_Torque_Limit", 750)  # 75%最大扭矩
                    self.bus.write_control_table(motor_id, "Protection_Current", 400)  # 80%最大电流
                    self.bus.write_control_table(motor_id, "Overload_Torque", 50)  # 50%过载扭矩
                
                time.sleep(0.05)
        
        logger.info("✓ 舵机配置完成")
    
    def get_observation(self) -> Dict[str, float]:
        """
        读取观测数据（关节位置）
        参考LeRobot实现，使用sync_read
        
        Returns:
            Dict[str, float]: {舵机名称: 归一化位置值 [-1, 1]}
        """
        if not self.is_connected:
            raise RuntimeError("机械臂未连接")
        
        start = time.perf_counter()
        
        # 使用sync_read一次性读取所有位置
        obs_dict = self.bus.sync_read("Present_Position")
        
        # 归一化到[-1, 1]
        observation = {}
        for motor_name, pos in obs_dict.items():
            if self.calibration_data and motor_name in self.calibration_data:
                calib = self.calibration_data[motor_name]
                range_min = calib["range_min"]
                range_max = calib["range_max"]
                
                # 归一化: (pos - min) / (max - min) * 2 - 1
                normalized = (pos - range_min) / (range_max - range_min) * 2 - 1
                observation[motor_name] = normalized
            else:
                # 无校准数据，返回原始值
                observation[motor_name] = float(pos)
        
        dt_ms = (time.perf_counter() - start) * 1000
        logger.debug(f"读取观测: {dt_ms:.1f}ms")
        
        return observation
    
    def send_action(self, action: Dict[str, float]) -> Dict[str, float]:
        """
        发送动作指令（目标位置）
        参考LeRobot实现，使用sync_write
        
        Args:
            action: {舵机名称: 归一化目标位置 [-1, 1]}
        
        Returns:
            Dict[str, float]: 实际发送的动作（可能被限幅）
        """
        if not self.is_connected:
            raise RuntimeError("机械臂未连接")
        
        if not self.calibration_data:
            raise RuntimeError("未加载校准数据，无法发送动作")
        
        # 转换归一化值到原始位置
        goal_positions = {}
        for motor_name, normalized_val in action.items():
            if motor_name not in MOTOR_IDS:
                logger.warning(f"未知舵机: {motor_name}")
                continue

            # 不让夹爪持续顶住校准范围端点，降低触发过载保护的概率
            if motor_name == "gripper":
                normalized_val = max(-0.9, min(0.9, normalized_val))
            
            calib = self.calibration_data[motor_name]
            range_min = calib["range_min"]
            range_max = calib["range_max"]
            
            # 反归一化: (normalized + 1) / 2 * (max - min) + min
            pos = int((normalized_val + 1) / 2 * (range_max - range_min) + range_min)
            pos_before_limit = pos
            
            # 限制在范围内
            pos = max(range_min, min(range_max, pos))
            
            if pos != pos_before_limit:
                logger.warning(
                    f"{motor_name} 目标位置超出校准范围: "
                    f"归一化={normalized_val:.3f}, 原始={pos_before_limit}, "
                    f"限制后={pos}, 范围=[{range_min}, {range_max}]"
                )
            
            logger.info(
                f"  → {motor_name}: 归一化目标={normalized_val:.3f}, "
                f"原始位置={pos}, 范围=[{range_min}, {range_max}]"
            )
            
            goal_positions[motor_name] = pos
        
        # 安全限制：检查最大相对移动距离（在归一化空间限制，参考LeRobot）
        if self.max_relative_target is not None:
            # 读取当前归一化位置
            current_obs = self.get_observation()
            
            # 限制每个关节的移动距离（在归一化空间[-1, 1]）
            for motor_name in list(action.keys()):
                if motor_name in current_obs:
                    current_norm = current_obs[motor_name]
                    goal_norm = action[motor_name]
                    
                    # 计算归一化空间的移动距离
                    delta_norm = goal_norm - current_norm
                    
                    # 限制移动距离
                    if abs(delta_norm) > self.max_relative_target:
                        # 限制到最大允许移动距离
                        limited_norm = current_norm + self.max_relative_target * (1 if delta_norm > 0 else -1)
                        logger.info(
                            f"{motor_name} 移动距离被安全限制: "
                            f"当前={current_norm:.3f}, 目标={goal_norm:.3f}, "
                            f"delta={delta_norm:.3f}, 限制到={limited_norm:.3f}"
                        )
                        
                        # 重新计算原始位置
                        calib = self.calibration_data[motor_name]
                        range_min = calib["range_min"]
                        range_max = calib["range_max"]
                        limited_pos = int((limited_norm + 1) / 2 * (range_max - range_min) + range_min)
                        limited_pos = max(range_min, min(range_max, limited_pos))
                        goal_positions[motor_name] = limited_pos
        
        # 使用sync_write一次性发送所有目标位置
        logger.info(f"正在发送Goal_Position: {goal_positions}")
        start = time.perf_counter()
        success = self.bus.sync_write("Goal_Position", goal_positions)
        dt_ms = (time.perf_counter() - start) * 1000
        logger.info(f"发送动作完成: {dt_ms:.1f}ms, 成功={success}")
        
        # 返回归一化后的实际发送值
        actual_action = {}
        for motor_name, goal_pos in goal_positions.items():
            calib = self.calibration_data[motor_name]
            range_min = calib["range_min"]
            range_max = calib["range_max"]
            normalized = (goal_pos - range_min) / (range_max - range_min) * 2 - 1
            actual_action[motor_name] = normalized
        
        return actual_action
    
    def enable_torque_all(self) -> None:
        """使能所有舵机扭矩"""
        for motor_name, motor_id in MOTOR_IDS.items():
            if self.bus.enable_torque(motor_id):
                logger.debug(f"✓ {motor_name} 扭矩已使能")
    
    def disable_torque_all(self) -> None:
        """关闭所有舵机扭矩"""
        for motor_name, motor_id in MOTOR_IDS.items():
            if self.bus.disable_torque(motor_id):
                logger.debug(f"✓ {motor_name} 扭矩已关闭")
    
    def get_motor_info(self) -> Dict[str, Dict]:
        """
        获取所有舵机信息
        
        Returns:
            Dict[str, Dict]: {舵机名称: {id, position, load, voltage, temperature}}
        """
        info = {}
        for motor_name, motor_id in MOTOR_IDS.items():
            motor_info = {
                "id": motor_id,
                "position": self.bus.get_present_position(motor_id),
                "load": self.bus.read_control_table(motor_id, "Present_Load"),
                "voltage": self.bus.read_control_table(motor_id, "Present_Voltage"),
                "temperature": self.bus.read_control_table(motor_id, "Present_Temperature"),
            }
            info[motor_name] = motor_info
        return info
    
    def __enter__(self):
        """上下文管理器支持"""
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """上下文管理器支持"""
        self.disconnect()
    
    def __repr__(self):
        return f"SO100Robot(port={self.port}, connected={self.is_connected})"
