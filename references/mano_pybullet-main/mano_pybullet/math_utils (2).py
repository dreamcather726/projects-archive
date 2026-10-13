"""数学转换工具函数。"""

import numpy as np
from transforms3d.axangles import axangle2mat, mat2axangle
from transforms3d.euler import euler2mat, mat2euler
from transforms3d.quaternions import mat2quat, quat2mat

__all__ = ('mat2rvec', 'rvec2mat', 'joint2mat', 'mat2joint', 'mat2pb', 'pb2mat')


def mat2rvec(mat):
    """将旋转矩阵转换为旋转向量。"""
    axis, angle = mat2axangle(mat, unit_thresh=1e-05)
    return axis * angle


def rvec2mat(rvec):
    """将旋转向量转换为旋转矩阵。"""
    angle = np.linalg.norm(rvec)
    axis = rvec if angle != 0.0 else [0.0, 0.0, 1.0]
    mat = axangle2mat(axis, angle)
    return mat


def joint2mat(axes, angles):
    """组合多自由度关节的旋转矩阵。

    参数:
        axes {str} -- 关节轴
        angles {list} -- 关节角度

    返回:
        array -- 旋转矩阵
    """
    rest = ''.join([i for i in 'xyz' if i not in axes])
    euler = [0.0, 0.0, 0.0]
    euler[:len(axes)] = angles[:len(axes)]
    return euler2mat(*euler, 'r' + axes + rest)


def mat2joint(mat, axes):
    """分解多自由度关节的旋转矩阵。

    参数:
        mat {mat3} -- 旋转矩阵
        axes {str} -- 关节轴

    返回:
        list -- 绕关节轴的旋转角度
    """
    rest = ''.join([i for i in 'xyz' if i not in axes])
    euler = mat2euler(mat, 'r' + axes + rest)
    return euler[:len(axes)]


def mat2pb(mat):
    """将旋转矩阵转换为 PyBullet 格式的四元数。"""
    quat = mat2quat(mat)
    return quat[1], quat[2], quat[3], quat[0]


def pb2mat(orn):
    """将 PyBullet 格式的四元数转换为旋转矩阵。"""
    quat = orn[3], orn[0], orn[1], orn[2]
    return quat2mat(quat)
