# -*- coding: utf-8 -*-
"""
SO-ARM100机械臂轨迹规划模块
提供关节空间和笛卡尔空间的轨迹规划功能

支持的插值方法：
- 线性插值
- 三次样条插值
- 五次多项式插值
- 梯形速度曲线
"""

import numpy as np
from typing import Dict, List, Optional, Callable, Tuple, Union
from dataclasses import dataclass, field
from abc import ABC, abstractmethod
import logging

logger = logging.getLogger(__name__)


@dataclass
class TrajectoryPoint:
    """轨迹点"""
    time: float                          # 时间戳（秒）
    positions: Dict[str, float]          # 关节位置（归一化）
    velocities: Optional[Dict[str, float]] = None   # 关节速度
    accelerations: Optional[Dict[str, float]] = None  # 关节加速度


@dataclass
class Trajectory:
    """
    轨迹数据结构
    
    包含完整的轨迹信息，支持在任意时刻采样
    """
    duration: float                       # 总时长（秒）
    waypoints: List[TrajectoryPoint]     # 路径点列表
    joint_names: List[str]               # 关节名称列表
    
    # 插值函数（每个关节一个）
    _position_interpolators: Dict[str, Callable] = field(default_factory=dict)
    _velocity_interpolators: Dict[str, Callable] = field(default_factory=dict)
    _acceleration_interpolators: Dict[str, Callable] = field(default_factory=dict)
    
    def sample(self, t: float) -> Dict[str, float]:
        """
        在时刻t采样关节位置
        
        Args:
            t: 时间（秒），范围[0, duration]
        
        Returns:
            Dict[str, float]: {关节名称: 位置}
        """
        t = np.clip(t, 0, self.duration)
        
        positions = {}
        for name in self.joint_names:
            if name in self._position_interpolators:
                positions[name] = float(self._position_interpolators[name](t))
        return positions
    
    def sample_velocity(self, t: float) -> Dict[str, float]:
        """在时刻t采样关节速度"""
        t = np.clip(t, 0, self.duration)
        
        velocities = {}
        for name in self.joint_names:
            if name in self._velocity_interpolators:
                velocities[name] = float(self._velocity_interpolators[name](t))
        return velocities
    
    def sample_acceleration(self, t: float) -> Dict[str, float]:
        """在时刻t采样关节加速度"""
        t = np.clip(t, 0, self.duration)
        
        accelerations = {}
        for name in self.joint_names:
            if name in self._acceleration_interpolators:
                accelerations[name] = float(self._acceleration_interpolators[name](t))
        return accelerations
    
    def sample_all(self, t: float) -> TrajectoryPoint:
        """在时刻t采样完整轨迹点"""
        return TrajectoryPoint(
            time=t,
            positions=self.sample(t),
            velocities=self.sample_velocity(t) if self._velocity_interpolators else None,
            accelerations=self.sample_acceleration(t) if self._acceleration_interpolators else None
        )
    
    def to_array(self, dt: float = 0.02) -> Tuple[np.ndarray, np.ndarray]:
        """
        转换为时间序列数组
        
        Args:
            dt: 采样间隔（秒）
        
        Returns:
            Tuple[times, positions]: 时间数组和位置数组
        """
        times = np.arange(0, self.duration + dt, dt)
        positions = np.zeros((len(times), len(self.joint_names)))
        
        for i, t in enumerate(times):
            pos = self.sample(t)
            for j, name in enumerate(self.joint_names):
                positions[i, j] = pos.get(name, 0.0)
        
        return times, positions
    
    def get_max_velocity(self) -> Dict[str, float]:
        """获取每个关节的最大速度"""
        if not self._velocity_interpolators:
            return {}
        
        dt = 0.01
        times = np.arange(0, self.duration + dt, dt)
        
        max_velocities = {name: 0.0 for name in self.joint_names}
        for t in times:
            velocities = self.sample_velocity(t)
            for name, vel in velocities.items():
                max_velocities[name] = max(max_velocities[name], abs(vel))
        
        return max_velocities
    
    def get_max_acceleration(self) -> Dict[str, float]:
        """获取每个关节的最大加速度"""
        if not self._acceleration_interpolators:
            return {}
        
        dt = 0.01
        times = np.arange(0, self.duration + dt, dt)
        
        max_accelerations = {name: 0.0 for name in self.joint_names}
        for t in times:
            accelerations = self.sample_acceleration(t)
            for name, acc in accelerations.items():
                max_accelerations[name] = max(max_accelerations[name], abs(acc))
        
        return max_accelerations


class Interpolator(ABC):
    """插值器基类"""
    
    @abstractmethod
    def fit(self, times: np.ndarray, values: np.ndarray) -> None:
        """拟合插值函数"""
        pass
    
    @abstractmethod
    def __call__(self, t: float) -> float:
        """在时刻t插值"""
        pass
    
    @abstractmethod
    def derivative(self, t: float, order: int = 1) -> float:
        """计算导数"""
        pass


class LinearInterpolator(Interpolator):
    """线性插值器"""
    
    def __init__(self):
        self.times = None
        self.values = None
    
    def fit(self, times: np.ndarray, values: np.ndarray) -> None:
        self.times = np.array(times)
        self.values = np.array(values)
    
    def __call__(self, t: float) -> float:
        return float(np.interp(t, self.times, self.values))
    
    def derivative(self, t: float, order: int = 1) -> float:
        if order == 1:
            # 数值微分
            epsilon = 1e-6
            return (self(t + epsilon) - self(t - epsilon)) / (2 * epsilon)
        return 0.0


class CubicSplineInterpolator(Interpolator):
    """
    三次样条插值器
    
    保证位置、速度、加速度连续
    """
    
    def __init__(self, boundary_condition: str = 'natural'):
        """
        Args:
            boundary_condition: 边界条件
                - 'natural': 自然边界（二阶导数为0）
                - 'clamped': 固定边界（指定首尾速度）
                - 'not-a-knot': 非节点条件
        """
        self.boundary_condition = boundary_condition
        self.times = None
        self.values = None
        self.coefficients = None  # 样条系数
    
    def fit(self, times: np.ndarray, values: np.ndarray, 
            start_velocity: float = 0.0, end_velocity: float = 0.0) -> None:
        """
        拟合三次样条
        
        Args:
            times: 时间点数组
            values: 值数组
            start_velocity: 起始速度（clamped边界条件使用）
            end_velocity: 结束速度（clamped边界条件使用）
        """
        self.times = np.array(times)
        self.values = np.array(values)
        n = len(times)
        
        if n < 2:
            raise ValueError("至少需要2个点进行插值")
        
        if n == 2:
            # 只有两个点，使用线性插值
            self.coefficients = self._linear_coefficients()
            return
        
        # 计算h值（区间长度）
        h = np.diff(self.times)
        
        # 构建三对角矩阵方程 Ax = b 求解二阶导数
        A = np.zeros((n, n))
        b = np.zeros(n)
        
        # 内部节点
        for i in range(1, n - 1):
            A[i, i-1] = h[i-1]
            A[i, i] = 2 * (h[i-1] + h[i])
            A[i, i+1] = h[i]
            b[i] = 3 * ((self.values[i+1] - self.values[i]) / h[i] - 
                       (self.values[i] - self.values[i-1]) / h[i-1])
        
        # 边界条件
        if self.boundary_condition == 'natural':
            A[0, 0] = 1
            A[n-1, n-1] = 1
            b[0] = 0
            b[n-1] = 0
        elif self.boundary_condition == 'clamped':
            A[0, 0] = 2 * h[0]
            A[0, 1] = h[0]
            b[0] = 3 * ((self.values[1] - self.values[0]) / h[0] - start_velocity)
            
            A[n-1, n-2] = h[n-2]
            A[n-1, n-1] = 2 * h[n-2]
            b[n-1] = 3 * (end_velocity - (self.values[n-1] - self.values[n-2]) / h[n-2])
        else:  # not-a-knot
            A[0, 0] = h[1]
            A[0, 1] = -(h[0] + h[1])
            A[0, 2] = h[0]
            b[0] = 0
            
            A[n-1, n-3] = h[n-2]
            A[n-1, n-2] = -(h[n-3] + h[n-2])
            A[n-1, n-1] = h[n-3]
            b[n-1] = 0
        
        # 求解二阶导数
        try:
            c = np.linalg.solve(A, b)
        except np.linalg.LinAlgError:
            # 如果矩阵奇异，回退到线性插值
            logger.warning("三次样条矩阵奇异，回退到线性插值")
            self.coefficients = self._linear_coefficients()
            return
        
        # 计算样条系数 [a, b, c, d] for each segment
        # S(t) = a + b*(t-ti) + c*(t-ti)^2 + d*(t-ti)^3
        self.coefficients = []
        for i in range(n - 1):
            a = self.values[i]
            b_coef = (self.values[i+1] - self.values[i]) / h[i] - h[i] * (2*c[i] + c[i+1]) / 3
            c_coef = c[i]
            d = (c[i+1] - c[i]) / (3 * h[i])
            self.coefficients.append((a, b_coef, c_coef, d))
    
    def _linear_coefficients(self):
        """生成线性插值系数"""
        coeffs = []
        for i in range(len(self.times) - 1):
            a = self.values[i]
            h = self.times[i+1] - self.times[i]
            b = (self.values[i+1] - self.values[i]) / h if h > 0 else 0
            coeffs.append((a, b, 0, 0))
        return coeffs
    
    def _find_segment(self, t: float) -> int:
        """找到t所在的区间索引"""
        t = np.clip(t, self.times[0], self.times[-1])
        idx = np.searchsorted(self.times, t, side='right') - 1
        return max(0, min(idx, len(self.coefficients) - 1))
    
    def __call__(self, t: float) -> float:
        """在时刻t插值位置"""
        if self.coefficients is None:
            return float(self.values[0])
        
        i = self._find_segment(t)
        a, b, c, d = self.coefficients[i]
        dt = t - self.times[i]
        
        return float(a + b*dt + c*dt**2 + d*dt**3)
    
    def derivative(self, t: float, order: int = 1) -> float:
        """计算导数（速度或加速度）"""
        if self.coefficients is None:
            return 0.0
        
        i = self._find_segment(t)
        a, b, c, d = self.coefficients[i]
        dt = t - self.times[i]
        
        if order == 1:  # 速度
            return float(b + 2*c*dt + 3*d*dt**2)
        elif order == 2:  # 加速度
            return float(2*c + 6*d*dt)
        else:
            return 0.0


class QuinticPolynomialInterpolator(Interpolator):
    """
    五次多项式插值器
    
    保证位置、速度、加速度连续，适用于点到点运动
    """
    
    def __init__(self):
        self.coefficients = None
        self.t0 = 0
        self.tf = 1
    
    def fit(self, times: np.ndarray, values: np.ndarray,
            start_velocity: float = 0.0, end_velocity: float = 0.0,
            start_acceleration: float = 0.0, end_acceleration: float = 0.0) -> None:
        """
        拟合五次多项式（仅支持两点）
        
        q(t) = a0 + a1*t + a2*t^2 + a3*t^3 + a4*t^4 + a5*t^5
        """
        if len(times) != 2:
            raise ValueError("五次多项式插值仅支持两点")
        
        self.t0 = times[0]
        self.tf = times[1]
        T = self.tf - self.t0
        
        q0, qf = values[0], values[1]
        v0, vf = start_velocity, end_velocity
        a0, af = start_acceleration, end_acceleration
        
        # 求解五次多项式系数
        # 使用矩阵方程 M * [a0,a1,a2,a3,a4,a5]^T = [q0,v0,a0,qf,vf,af]^T
        M = np.array([
            [1, 0, 0, 0, 0, 0],
            [0, 1, 0, 0, 0, 0],
            [0, 0, 2, 0, 0, 0],
            [1, T, T**2, T**3, T**4, T**5],
            [0, 1, 2*T, 3*T**2, 4*T**3, 5*T**4],
            [0, 0, 2, 6*T, 12*T**2, 20*T**3]
        ])
        
        b = np.array([q0, v0, a0, qf, vf, af])
        self.coefficients = np.linalg.solve(M, b)
    
    def __call__(self, t: float) -> float:
        if self.coefficients is None:
            return 0.0
        
        t = np.clip(t, self.t0, self.tf)
        dt = t - self.t0
        
        result = 0.0
        for i, coef in enumerate(self.coefficients):
            result += coef * dt**i
        return float(result)
    
    def derivative(self, t: float, order: int = 1) -> float:
        if self.coefficients is None:
            return 0.0
        
        t = np.clip(t, self.t0, self.tf)
        dt = t - self.t0
        
        if order == 1:  # 速度
            return float(self.coefficients[1] + 
                        2*self.coefficients[2]*dt + 
                        3*self.coefficients[3]*dt**2 + 
                        4*self.coefficients[4]*dt**3 + 
                        5*self.coefficients[5]*dt**4)
        elif order == 2:  # 加速度
            return float(2*self.coefficients[2] + 
                        6*self.coefficients[3]*dt + 
                        12*self.coefficients[4]*dt**2 + 
                        20*self.coefficients[5]*dt**3)
        return 0.0


class TrapezoidalVelocityProfile:
    """
    梯形速度曲线
    
    三阶段：加速 - 匀速 - 减速
    """
    
    def __init__(self, max_velocity: float = 1.0, max_acceleration: float = 2.0):
        """
        Args:
            max_velocity: 最大速度（归一化单位/秒）
            max_acceleration: 最大加速度（归一化单位/秒²）
        """
        self.max_velocity = max_velocity
        self.max_acceleration = max_acceleration
        
        # 规划参数
        self.t_accel = 0
        self.t_cruise = 0
        self.t_decel = 0
        self.duration = 0
        self.start_pos = 0
        self.end_pos = 0
        self.actual_max_vel = 0
    
    def plan(self, start_pos: float, end_pos: float, 
             duration: float = None) -> float:
        """
        规划梯形速度曲线
        
        Args:
            start_pos: 起始位置
            end_pos: 结束位置
            duration: 期望时长（可选）
        
        Returns:
            float: 实际时长
        """
        self.start_pos = start_pos
        self.end_pos = end_pos
        distance = abs(end_pos - start_pos)
        direction = 1 if end_pos >= start_pos else -1
        
        if distance < 1e-10:
            self.duration = 0
            return 0
        
        # 计算最小时间（三角形速度曲线）
        t_min = 2 * np.sqrt(distance / self.max_acceleration)
        
        if duration is None or duration < t_min:
            duration = t_min
        
        # 计算加速时间
        # v_max可能受限于距离
        v_max_possible = np.sqrt(distance * self.max_acceleration)
        self.actual_max_vel = min(self.max_velocity, v_max_possible) * direction
        
        self.t_accel = abs(self.actual_max_vel) / self.max_acceleration
        self.t_decel = self.t_accel
        
        # 匀速阶段距离
        accel_distance = 0.5 * self.max_acceleration * self.t_accel**2
        cruise_distance = distance - 2 * accel_distance
        
        if cruise_distance > 0:
            self.t_cruise = cruise_distance / abs(self.actual_max_vel)
        else:
            # 没有匀速阶段（三角形）
            self.t_cruise = 0
            self.t_accel = np.sqrt(distance / self.max_acceleration)
            self.t_decel = self.t_accel
            self.actual_max_vel = self.max_acceleration * self.t_accel * direction
        
        self.duration = self.t_accel + self.t_cruise + self.t_decel
        return self.duration
    
    def position(self, t: float) -> float:
        """在时刻t获取位置"""
        t = np.clip(t, 0, self.duration)
        
        if self.duration < 1e-10:
            return self.start_pos
        
        direction = 1 if self.end_pos >= self.start_pos else -1
        a = self.max_acceleration * direction
        v_max = self.actual_max_vel
        
        if t <= self.t_accel:
            # 加速阶段
            return self.start_pos + 0.5 * a * t**2
        elif t <= self.t_accel + self.t_cruise:
            # 匀速阶段
            dt = t - self.t_accel
            pos_after_accel = self.start_pos + 0.5 * a * self.t_accel**2
            return pos_after_accel + v_max * dt
        else:
            # 减速阶段
            dt = t - self.t_accel - self.t_cruise
            pos_after_cruise = (self.start_pos + 
                               0.5 * a * self.t_accel**2 + 
                               v_max * self.t_cruise)
            return pos_after_cruise + v_max * dt - 0.5 * a * dt**2
    
    def velocity(self, t: float) -> float:
        """在时刻t获取速度"""
        t = np.clip(t, 0, self.duration)
        
        if self.duration < 1e-10:
            return 0.0
        
        direction = 1 if self.end_pos >= self.start_pos else -1
        a = self.max_acceleration * direction
        
        if t <= self.t_accel:
            return a * t
        elif t <= self.t_accel + self.t_cruise:
            return self.actual_max_vel
        else:
            dt = t - self.t_accel - self.t_cruise
            return self.actual_max_vel - a * dt
    
    def acceleration(self, t: float) -> float:
        """在时刻t获取加速度"""
        t = np.clip(t, 0, self.duration)
        
        if self.duration < 1e-10:
            return 0.0
        
        direction = 1 if self.end_pos >= self.start_pos else -1
        a = self.max_acceleration * direction
        
        if t <= self.t_accel:
            return a
        elif t <= self.t_accel + self.t_cruise:
            return 0.0
        else:
            return -a


class JointTrajectoryPlanner:
    """
    关节空间轨迹规划器
    
    支持多种插值方法和速度限制
    """
    
    # 默认关节名称（SO-ARM100）
    DEFAULT_JOINT_NAMES = [
        'shoulder_pan', 'shoulder_lift', 'elbow_flex',
        'wrist_flex', 'wrist_roll', 'gripper'
    ]
    
    def __init__(self, 
                 joint_names: List[str] = None,
                 interpolation_method: str = 'cubic_spline',
                 max_velocity: float = 1.0,
                 max_acceleration: float = 2.0):
        """
        Args:
            joint_names: 关节名称列表
            interpolation_method: 插值方法
                - 'linear': 线性插值
                - 'cubic_spline': 三次样条（推荐）
                - 'quintic': 五次多项式
                - 'trapezoidal': 梯形速度曲线
            max_velocity: 最大速度（归一化单位/秒）
            max_acceleration: 最大加速度（归一化单位/秒²）
        """
        self.joint_names = joint_names or self.DEFAULT_JOINT_NAMES
        self.interpolation_method = interpolation_method
        self.max_velocity = max_velocity
        self.max_acceleration = max_acceleration
    
    def plan_point_to_point(self, 
                           start: Dict[str, float],
                           goal: Dict[str, float],
                           duration: float = None) -> Trajectory:
        """
        点到点轨迹规划
        
        Args:
            start: 起始关节位置（归一化）
            goal: 目标关节位置（归一化）
            duration: 运动时长（秒），None表示自动计算
        
        Returns:
            Trajectory: 轨迹对象
        """
        # 确定关节列表
        joints = list(set(start.keys()) | set(goal.keys()))
        joints = [j for j in self.joint_names if j in joints]
        
        # 计算最小时间（基于最大位移）
        max_displacement = 0
        for joint in joints:
            s = start.get(joint, 0.0)
            g = goal.get(joint, 0.0)
            max_displacement = max(max_displacement, abs(g - s))
        
        if duration is None:
            # 基于梯形速度曲线计算最小时间
            if max_displacement > 0:
                t_min = max_displacement / self.max_velocity + \
                        self.max_velocity / self.max_acceleration
                duration = max(t_min, 0.5)  # 至少0.5秒
            else:
                duration = 0.1
        
        # 创建路径点
        waypoints = [
            TrajectoryPoint(time=0, positions=start),
            TrajectoryPoint(time=duration, positions=goal)
        ]
        
        # 为每个关节创建插值器
        position_interpolators = {}
        velocity_interpolators = {}
        acceleration_interpolators = {}
        
        for joint in joints:
            s = start.get(joint, 0.0)
            g = goal.get(joint, 0.0)
            times = np.array([0, duration])
            values = np.array([s, g])
            
            if self.interpolation_method == 'linear':
                interp = LinearInterpolator()
                interp.fit(times, values)
            elif self.interpolation_method == 'cubic_spline':
                interp = CubicSplineInterpolator(boundary_condition='clamped')
                interp.fit(times, values, start_velocity=0, end_velocity=0)
            elif self.interpolation_method == 'quintic':
                interp = QuinticPolynomialInterpolator()
                interp.fit(times, values)
            elif self.interpolation_method == 'trapezoidal':
                trap = TrapezoidalVelocityProfile(self.max_velocity, self.max_acceleration)
                trap.plan(s, g, duration)
                interp = trap
            else:
                interp = CubicSplineInterpolator()
                interp.fit(times, values)
            
            position_interpolators[joint] = (
                interp.position if hasattr(interp, 'position') else interp
            )
            
            if hasattr(interp, 'velocity'):
                velocity_interpolators[joint] = interp.velocity
            elif hasattr(interp, 'derivative'):
                velocity_interpolators[joint] = lambda t, i=interp: i.derivative(t, 1)
            
            if hasattr(interp, 'acceleration'):
                acceleration_interpolators[joint] = interp.acceleration
            elif hasattr(interp, 'derivative'):
                acceleration_interpolators[joint] = lambda t, i=interp: i.derivative(t, 2)
        
        trajectory = Trajectory(
            duration=duration,
            waypoints=waypoints,
            joint_names=joints
        )
        trajectory._position_interpolators = position_interpolators
        trajectory._velocity_interpolators = velocity_interpolators
        trajectory._acceleration_interpolators = acceleration_interpolators
        
        return trajectory
    
    def plan_via_waypoints(self,
                          waypoints: List[Dict[str, float]],
                          timestamps: List[float] = None,
                          duration: float = None) -> Trajectory:
        """
        多路径点轨迹规划
        
        Args:
            waypoints: 路径点列表 [{关节名: 位置}, ...]
            timestamps: 时间戳列表 [t0, t1, ...], 与waypoints对应
            duration: 总时长（如果timestamps未指定）
        
        Returns:
            Trajectory: 轨迹对象
        """
        n_waypoints = len(waypoints)
        if n_waypoints < 2:
            raise ValueError("至少需要2个路径点")
        
        # 确定关节列表
        joints = set()
        for wp in waypoints:
            joints.update(wp.keys())
        joints = [j for j in self.joint_names if j in joints]
        
        # 确定时间戳
        if timestamps is None:
            if duration is None:
                duration = n_waypoints - 1  # 默认每个段1秒
            timestamps = np.linspace(0, duration, n_waypoints).tolist()
        else:
            duration = timestamps[-1]
        
        times = np.array(timestamps)
        
        # 创建路径点对象
        trajectory_points = [
            TrajectoryPoint(time=timestamps[i], positions=waypoints[i])
            for i in range(n_waypoints)
        ]
        
        # 为每个关节创建插值器
        position_interpolators = {}
        velocity_interpolators = {}
        acceleration_interpolators = {}
        
        for joint in joints:
            values = np.array([wp.get(joint, 0.0) for wp in waypoints])
            
            if self.interpolation_method == 'linear':
                interp = LinearInterpolator()
                interp.fit(times, values)
            elif self.interpolation_method == 'cubic_spline':
                interp = CubicSplineInterpolator(boundary_condition='natural')
                interp.fit(times, values)
            elif self.interpolation_method == 'quintic':
                # 五次多项式只支持两点，改用三次样条
                interp = CubicSplineInterpolator()
                interp.fit(times, values)
            else:
                interp = CubicSplineInterpolator()
                interp.fit(times, values)
            
            position_interpolators[joint] = interp
            velocity_interpolators[joint] = lambda t, i=interp: i.derivative(t, 1)
            acceleration_interpolators[joint] = lambda t, i=interp: i.derivative(t, 2)
        
        trajectory = Trajectory(
            duration=duration,
            waypoints=trajectory_points,
            joint_names=joints
        )
        trajectory._position_interpolators = position_interpolators
        trajectory._velocity_interpolators = velocity_interpolators
        trajectory._acceleration_interpolators = acceleration_interpolators
        
        return trajectory
    
    def plan_circular_joint_motion(self,
                                  center: Dict[str, float],
                                  radius: Dict[str, float],
                                  start_angle: float = 0,
                                  end_angle: float = 2 * np.pi,
                                  duration: float = 5.0,
                                  n_points: int = 50) -> Trajectory:
        """
        关节空间圆周运动（用于演示）
        
        Args:
            center: 中心关节位置
            radius: 各关节的运动半径
            start_angle: 起始角度（弧度）
            end_angle: 结束角度（弧度）
            duration: 时长
            n_points: 采样点数
        
        Returns:
            Trajectory: 轨迹对象
        """
        angles = np.linspace(start_angle, end_angle, n_points)
        timestamps = np.linspace(0, duration, n_points)
        
        waypoints = []
        for angle in angles:
            wp = {}
            for joint in center.keys():
                c = center.get(joint, 0.0)
                r = radius.get(joint, 0.0)
                wp[joint] = c + r * np.sin(angle)
            waypoints.append(wp)
        
        return self.plan_via_waypoints(waypoints, timestamps.tolist())


class CartesianTrajectoryPlanner:
    """
    笛卡尔空间轨迹规划器
    
    在笛卡尔空间规划轨迹，然后通过逆运动学转换到关节空间
    """
    
    def __init__(self, kinematics=None):
        """
        Args:
            kinematics: 运动学对象（SO100Kinematics）
        """
        self.kinematics = kinematics
        self.joint_planner = JointTrajectoryPlanner()
    
    def plan_linear_path(self,
                        start_pose: np.ndarray,
                        end_pose: np.ndarray,
                        current_joints: Dict[str, float],
                        duration: float = 2.0,
                        n_points: int = 20) -> Optional[Trajectory]:
        """
        笛卡尔空间直线运动
        
        Args:
            start_pose: 起始位姿 [x, y, z, roll, pitch, yaw]
            end_pose: 结束位姿
            current_joints: 当前关节位置（用于IK初始化）
            duration: 时长
            n_points: 插值点数
        
        Returns:
            Trajectory: 轨迹对象，如果IK失败返回None
        """
        if self.kinematics is None:
            logger.error("未设置运动学对象")
            return None
        
        from .kinematics import Pose
        
        # 在笛卡尔空间线性插值
        waypoints = []
        timestamps = np.linspace(0, duration, n_points)
        prev_joints = current_joints.copy()
        
        for i, t in enumerate(timestamps):
            alpha = t / duration
            pose_arr = (1 - alpha) * start_pose + alpha * end_pose
            pose = Pose.from_array(pose_arr)
            
            # 逆运动学
            joints = self.kinematics.inverse_kinematics(pose, prev_joints)
            if joints is None:
                logger.error(f"IK失败于点 {i}/{n_points}")
                return None
            
            waypoints.append(joints)
            prev_joints = joints
        
        return self.joint_planner.plan_via_waypoints(waypoints, timestamps.tolist())
    
    def plan_circular_arc(self,
                         center: np.ndarray,
                         radius: float,
                         start_angle: float,
                         end_angle: float,
                         plane: str = 'xy',
                         height: float = 0.0,
                         current_joints: Dict[str, float] = None,
                         duration: float = 5.0,
                         n_points: int = 50) -> Optional[Trajectory]:
        """
        圆弧轨迹
        
        Args:
            center: 圆心 [x, y, z]
            radius: 半径
            start_angle: 起始角度（弧度）
            end_angle: 结束角度（弧度）
            plane: 运动平面 ('xy', 'xz', 'yz')
            height: 固定高度（对于平面外的坐标）
            current_joints: 当前关节位置
            duration: 时长
            n_points: 采样点数
        
        Returns:
            Trajectory: 轨迹对象
        """
        if self.kinematics is None:
            logger.error("未设置运动学对象")
            return None
        
        from .kinematics import Pose
        
        angles = np.linspace(start_angle, end_angle, n_points)
        timestamps = np.linspace(0, duration, n_points)
        
        waypoints = []
        prev_joints = current_joints
        
        for i, angle in enumerate(angles):
            if plane == 'xy':
                x = center[0] + radius * np.cos(angle)
                y = center[1] + radius * np.sin(angle)
                z = center[2] if len(center) > 2 else height
            elif plane == 'xz':
                x = center[0] + radius * np.cos(angle)
                y = center[1] if len(center) > 1 else height
                z = (center[2] if len(center) > 2 else 0) + radius * np.sin(angle)
            else:  # yz
                x = center[0] if len(center) > 0 else height
                y = center[1] + radius * np.cos(angle)
                z = (center[2] if len(center) > 2 else 0) + radius * np.sin(angle)
            
            pose = Pose(x=x, y=y, z=z, roll=0, pitch=np.pi/2, yaw=0)
            
            joints = self.kinematics.inverse_kinematics(pose, prev_joints)
            if joints is None:
                logger.warning(f"IK失败于角度 {np.degrees(angle):.1f}°")
                continue
            
            waypoints.append(joints)
            prev_joints = joints
        
        if len(waypoints) < 2:
            logger.error("有效路径点不足")
            return None
        
        # 调整时间戳
        valid_timestamps = timestamps[:len(waypoints)].tolist()
        return self.joint_planner.plan_via_waypoints(waypoints, valid_timestamps)
