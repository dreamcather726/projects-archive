# -*- coding: utf-8 -*-
"""
SO-ARM100机械臂运动学模块
提供正运动学、逆运动学和雅可比矩阵计算

基于DH参数建模
"""

import numpy as np
from typing import Dict, Optional, Tuple, List
from dataclasses import dataclass
import logging

logger = logging.getLogger(__name__)


@dataclass
class DHParameters:
    """DH参数（改进型DH参数）"""
    alpha: float  # 绕x轴旋转角度（弧度）
    a: float      # 沿x轴平移距离（米）
    d: float      # 沿z轴平移距离（米）
    theta: float  # 绕z轴旋转角度（弧度）- 对于旋转关节这是变量


@dataclass
class Pose:
    """笛卡尔空间位姿"""
    x: float
    y: float
    z: float
    roll: float   # 绕x轴旋转（弧度）
    pitch: float  # 绕y轴旋转（弧度）
    yaw: float    # 绕z轴旋转（弧度）
    
    def to_array(self) -> np.ndarray:
        """转换为数组 [x, y, z, roll, pitch, yaw]"""
        return np.array([self.x, self.y, self.z, self.roll, self.pitch, self.yaw])
    
    @classmethod
    def from_array(cls, arr: np.ndarray) -> 'Pose':
        """从数组创建"""
        return cls(x=arr[0], y=arr[1], z=arr[2], 
                   roll=arr[3], pitch=arr[4], yaw=arr[5])
    
    def to_homogeneous_matrix(self) -> np.ndarray:
        """转换为4x4齐次变换矩阵"""
        return pose_to_matrix(self.x, self.y, self.z, 
                             self.roll, self.pitch, self.yaw)


def rotation_matrix_x(angle: float) -> np.ndarray:
    """绕X轴旋转矩阵"""
    c, s = np.cos(angle), np.sin(angle)
    return np.array([
        [1, 0, 0],
        [0, c, -s],
        [0, s, c]
    ])


def rotation_matrix_y(angle: float) -> np.ndarray:
    """绕Y轴旋转矩阵"""
    c, s = np.cos(angle), np.sin(angle)
    return np.array([
        [c, 0, s],
        [0, 1, 0],
        [-s, 0, c]
    ])


def rotation_matrix_z(angle: float) -> np.ndarray:
    """绕Z轴旋转矩阵"""
    c, s = np.cos(angle), np.sin(angle)
    return np.array([
        [c, -s, 0],
        [s, c, 0],
        [0, 0, 1]
    ])


def euler_to_rotation_matrix(roll: float, pitch: float, yaw: float) -> np.ndarray:
    """欧拉角转旋转矩阵（ZYX顺序）"""
    Rz = rotation_matrix_z(yaw)
    Ry = rotation_matrix_y(pitch)
    Rx = rotation_matrix_x(roll)
    return Rz @ Ry @ Rx


def rotation_matrix_to_euler(R: np.ndarray) -> Tuple[float, float, float]:
    """旋转矩阵转欧拉角（ZYX顺序）"""
    sy = np.sqrt(R[0, 0]**2 + R[1, 0]**2)
    
    singular = sy < 1e-6
    
    if not singular:
        roll = np.arctan2(R[2, 1], R[2, 2])
        pitch = np.arctan2(-R[2, 0], sy)
        yaw = np.arctan2(R[1, 0], R[0, 0])
    else:
        roll = np.arctan2(-R[1, 2], R[1, 1])
        pitch = np.arctan2(-R[2, 0], sy)
        yaw = 0
    
    return roll, pitch, yaw


def pose_to_matrix(x: float, y: float, z: float, 
                   roll: float, pitch: float, yaw: float) -> np.ndarray:
    """位姿转齐次变换矩阵"""
    T = np.eye(4)
    T[:3, :3] = euler_to_rotation_matrix(roll, pitch, yaw)
    T[:3, 3] = [x, y, z]
    return T


def matrix_to_pose(T: np.ndarray) -> Tuple[float, float, float, float, float, float]:
    """齐次变换矩阵转位姿"""
    x, y, z = T[:3, 3]
    roll, pitch, yaw = rotation_matrix_to_euler(T[:3, :3])
    return x, y, z, roll, pitch, yaw


def dh_transform(alpha: float, a: float, d: float, theta: float) -> np.ndarray:
    """
    计算DH变换矩阵（改进型DH参数）
    
    Args:
        alpha: 绕x轴旋转角度
        a: 沿x轴平移距离
        d: 沿z轴平移距离
        theta: 绕z轴旋转角度
    
    Returns:
        4x4齐次变换矩阵
    """
    ca, sa = np.cos(alpha), np.sin(alpha)
    ct, st = np.cos(theta), np.sin(theta)
    
    return np.array([
        [ct, -st, 0, a],
        [st * ca, ct * ca, -sa, -d * sa],
        [st * sa, ct * sa, ca, d * ca],
        [0, 0, 0, 1]
    ])


class SO100Kinematics:
    """
    SO-ARM100机械臂运动学
    
    6自由度机械臂：
    - Joint 1: shoulder_pan（底座旋转）
    - Joint 2: shoulder_lift（肩部俯仰）
    - Joint 3: elbow_flex（肘部）
    - Joint 4: wrist_flex（腕部俯仰）
    - Joint 5: wrist_roll（腕部旋转）
    - Joint 6: gripper（夹爪）- 不参与运动学计算
    """
    
    # SO-ARM100 DH参数（单位：米，弧度）
    # 注意：这些参数需要根据实际机械臂测量调整
    # 默认值基于SO-ARM100标准尺寸估计
    DEFAULT_DH_PARAMS = [
        # (alpha, a, d, theta_offset)
        (0, 0, 0.045, 0),           # Joint 1: shoulder_pan
        (-np.pi/2, 0, 0, -np.pi/2), # Joint 2: shoulder_lift
        (0, 0.104, 0, 0),           # Joint 3: elbow_flex
        (0, 0.088, 0, 0),           # Joint 4: wrist_flex
        (-np.pi/2, 0, 0, 0),        # Joint 5: wrist_roll
    ]
    
    # 关节名称到索引映射（不包括gripper）
    JOINT_NAMES = ['shoulder_pan', 'shoulder_lift', 'elbow_flex', 
                   'wrist_flex', 'wrist_roll']
    
    def __init__(self, dh_params: List[Tuple] = None):
        """
        初始化运动学
        
        Args:
            dh_params: DH参数列表，每个元素为(alpha, a, d, theta_offset)
        """
        self.dh_params = dh_params or self.DEFAULT_DH_PARAMS
        self.num_joints = len(self.dh_params)
        
        # 关节限制（归一化空间 [-1, 1]）
        self.joint_limits_normalized = {
            'shoulder_pan': (-1.0, 1.0),
            'shoulder_lift': (-1.0, 1.0),
            'elbow_flex': (-1.0, 1.0),
            'wrist_flex': (-1.0, 1.0),
            'wrist_roll': (-1.0, 1.0),
        }
    
    def forward_kinematics(self, joint_positions: Dict[str, float]) -> Pose:
        """
        正运动学：关节角度 -> 末端位姿
        
        Args:
            joint_positions: {关节名称: 归一化位置 [-1, 1]}
        
        Returns:
            Pose: 末端执行器位姿
        """
        # 转换归一化值到弧度（假设[-1,1]对应[-π, π]）
        joint_angles = self._normalized_to_radians(joint_positions)
        
        # 计算正运动学
        T = np.eye(4)
        for i, (alpha, a, d, theta_offset) in enumerate(self.dh_params):
            if i < len(joint_angles):
                theta = joint_angles[i] + theta_offset
            else:
                theta = theta_offset
            T = T @ dh_transform(alpha, a, d, theta)
        
        # 转换为位姿
        x, y, z, roll, pitch, yaw = matrix_to_pose(T)
        return Pose(x=x, y=y, z=z, roll=roll, pitch=pitch, yaw=yaw)
    
    def inverse_kinematics(self, target_pose: Pose, 
                          current_joints: Dict[str, float] = None,
                          max_iterations: int = 100,
                          tolerance: float = 1e-4) -> Optional[Dict[str, float]]:
        """
        逆运动学：末端位姿 -> 关节角度（数值解法）
        
        使用阻尼最小二乘法（Damped Least Squares / Levenberg-Marquardt）
        
        Args:
            target_pose: 目标位姿
            current_joints: 当前关节位置（作为初始猜测）
            max_iterations: 最大迭代次数
            tolerance: 收敛容差
        
        Returns:
            Dict[str, float]: 关节位置（归一化），如果无解返回None
        """
        # 初始猜测
        if current_joints is None:
            current_joints = {name: 0.0 for name in self.JOINT_NAMES}
        
        joints = np.array([current_joints.get(name, 0.0) for name in self.JOINT_NAMES])
        target = target_pose.to_array()
        
        # 阻尼因子
        damping = 0.1
        
        for iteration in range(max_iterations):
            # 当前位姿
            current_pose = self.forward_kinematics(
                {name: joints[i] for i, name in enumerate(self.JOINT_NAMES)}
            )
            current = current_pose.to_array()
            
            # 误差
            error = target - current
            
            # 角度误差需要归一化到[-π, π]
            for i in range(3, 6):
                while error[i] > np.pi:
                    error[i] -= 2 * np.pi
                while error[i] < -np.pi:
                    error[i] += 2 * np.pi
            
            error_norm = np.linalg.norm(error)
            
            if error_norm < tolerance:
                logger.debug(f"IK收敛于第{iteration}次迭代，误差={error_norm:.6f}")
                return {name: float(joints[i]) for i, name in enumerate(self.JOINT_NAMES)}
            
            # 计算雅可比矩阵
            J = self._compute_jacobian(joints)
            
            # 阻尼最小二乘
            JT = J.T
            delta_joints = JT @ np.linalg.inv(J @ JT + damping**2 * np.eye(6)) @ error
            
            # 更新关节角度
            joints = joints + delta_joints[:self.num_joints]
            
            # 限制在归一化范围内
            joints = np.clip(joints, -1.0, 1.0)
        
        logger.warning(f"IK未收敛，最终误差={error_norm:.6f}")
        return None
    
    def _compute_jacobian(self, joints: np.ndarray) -> np.ndarray:
        """
        计算雅可比矩阵（数值方法）
        
        Args:
            joints: 关节角度数组（归一化）
        
        Returns:
            6xN雅可比矩阵
        """
        epsilon = 1e-6
        J = np.zeros((6, self.num_joints))
        
        # 当前位姿
        current_pose = self.forward_kinematics(
            {name: joints[i] for i, name in enumerate(self.JOINT_NAMES)}
        ).to_array()
        
        # 数值微分
        for i in range(self.num_joints):
            joints_plus = joints.copy()
            joints_plus[i] += epsilon
            
            pose_plus = self.forward_kinematics(
                {name: joints_plus[j] for j, name in enumerate(self.JOINT_NAMES)}
            ).to_array()
            
            J[:, i] = (pose_plus - current_pose) / epsilon
        
        return J
    
    def _normalized_to_radians(self, joint_positions: Dict[str, float]) -> List[float]:
        """
        归一化位置转弧度
        
        假设归一化范围[-1, 1]对应[-π, π]
        """
        angles = []
        for name in self.JOINT_NAMES:
            normalized = joint_positions.get(name, 0.0)
            # [-1, 1] -> [-π, π]
            angle = normalized * np.pi
            angles.append(angle)
        return angles
    
    def _radians_to_normalized(self, angles: List[float]) -> Dict[str, float]:
        """弧度转归一化位置"""
        return {
            name: float(angles[i] / np.pi)
            for i, name in enumerate(self.JOINT_NAMES)
        }
    
    def check_reachability(self, pose: Pose) -> bool:
        """
        检查目标位姿是否可达
        
        Args:
            pose: 目标位姿
        
        Returns:
            bool: 是否可达
        """
        result = self.inverse_kinematics(pose)
        return result is not None
    
    def get_workspace_bounds(self) -> Dict[str, Tuple[float, float]]:
        """
        获取工作空间边界（近似）
        
        Returns:
            Dict: {轴: (最小值, 最大值)}
        """
        # 通过采样估计工作空间
        samples = []
        for _ in range(1000):
            joints = {name: np.random.uniform(-1, 1) for name in self.JOINT_NAMES}
            pose = self.forward_kinematics(joints)
            samples.append(pose.to_array()[:3])
        
        samples = np.array(samples)
        return {
            'x': (float(samples[:, 0].min()), float(samples[:, 0].max())),
            'y': (float(samples[:, 1].min()), float(samples[:, 1].max())),
            'z': (float(samples[:, 2].min()), float(samples[:, 2].max())),
        }
