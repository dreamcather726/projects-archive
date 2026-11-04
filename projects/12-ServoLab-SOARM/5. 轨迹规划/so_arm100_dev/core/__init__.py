# -*- coding: utf-8 -*-
"""
核心模块
包含电机总线通信、校准、机械臂控制、轨迹规划等核心功能
"""

from .motor_bus import MotorBus
from .motor_setup import MotorSetup
from .calibration import MotorCalibration
from .robot import SO100Robot
from .kinematics import SO100Kinematics, Pose
from .trajectory import (
    Trajectory,
    TrajectoryPoint,
    JointTrajectoryPlanner,
    CartesianTrajectoryPlanner,
    CubicSplineInterpolator,
    TrapezoidalVelocityProfile
)
from .motion_controller import (
    MotionController,
    MotionState,
    MotionStatus,
    TrajectoryRecorder,
    MotionPrimitives
)

__all__ = [
    # 基础模块
    'MotorBus', 
    'MotorSetup', 
    'MotorCalibration', 
    'SO100Robot',
    # 运动学
    'SO100Kinematics',
    'Pose',
    # 轨迹规划
    'Trajectory',
    'TrajectoryPoint',
    'JointTrajectoryPlanner',
    'CartesianTrajectoryPlanner',
    'CubicSplineInterpolator',
    'TrapezoidalVelocityProfile',
    # 运动控制
    'MotionController',
    'MotionState',
    'MotionStatus',
    'TrajectoryRecorder',
    'MotionPrimitives',
]
