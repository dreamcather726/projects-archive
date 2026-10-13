"""手部模型。"""

import collections

import numpy as np

from .mano_model import ManoModel
from .math_utils import joint2mat, mat2joint, mat2rvec, rvec2mat

__all__ = ('HandModel', 'HandModel20', 'HandModel45')

Joint = collections.namedtuple('Joint', ['origin', 'basis', 'axes', 'limits'])


class HandModel(ManoModel):
    """基础刚体手部模型。

    该模型提供刚体手部运动学描述和从关节空间到 MANO 模型姿态的数学工具。
    """

    def __init__(self, left_hand=False, models_dir=None):
        """初始化 HandModel。

        关键字参数:
            left_hand {bool} -- 创建左手模型 (默认: {False})
            models_dir {str} -- pickle 模型文件的路径 (默认: {None})
        """
        super().__init__(left_hand=left_hand, models_dir=models_dir)
        self._joints = self._make_joints()
        self._basis = [joint.basis for joint in self._joints]
        self._axes = [joint.axes for joint in self._joints]
        self._dofs = [(u-len(self._axes[i]), u)
                      for i, u in enumerate(np.cumsum([len(joint.axes) for joint in self._joints]))]

        assert len(self._joints) == len(self.origins()), '关节数量错误'
        assert all([len(j.axes) == len(j.limits) for j in self._joints]), '限制数量错误'
        assert not self._joints[0].axes, '手掌关节未固定'

    @property
    def joints(self):
        """关节描述。

        返回:
            list -- Joint 结构列表
        """
        return self._joints

    @property
    def dofs_number(self):
        """自由度数量。

        返回:
            int -- 所有关节的自由度总和
        """
        return sum([len(joint.axes) for joint in self._joints[1:]])

    @property
    def dofs_limits(self):
        """对应自由度的限制。

        返回:
            tuple -- 下限列表，上限列表
        """
        return list(zip(*[limits for joint in self._joints[1:] for limits in joint.limits]))

    def angles_to_mano(self, angles, palm_basis=None):
        """将关节角度转换为 MANO 姿态。

        参数:
            angles {array} -- 刚体模型的自由度角度

        关键字参数:
            palm_basis {mat3} -- 手掌基向量 (默认: {None})

        返回:
            array -- MANO 姿态，大小为 N*3 的数组，其中 N 是链接数
        """
        if len(angles) != self.dofs_number:
            raise ValueError(f'期望 {self.dofs_number} 个角度（得到 {len(angles)} 个）。')

        def joint_to_rvec(i, angles):
            return mat2rvec(self._basis[i] @ joint2mat(self._axes[i], angles) @ self._basis[i].T)

        rvecs = [joint_to_rvec(i, angles[d0:d1]) for i, (d0, d1) in enumerate(self._dofs)]
        if palm_basis is not None:
            rvecs[0] = mat2rvec(palm_basis)
        return np.ravel(rvecs)

    def mano_to_angles(self, mano_pose):
        """将 MANO 姿态转换为刚体模型的关节角度。

        不能保证刚体模型能够完美恢复 MANO 姿态。

        参数:
            mano_pose {array} -- MANO 姿态，大小为 N*3 的数组，其中 N 是链接数

        返回:
            tuple -- 自由度角度，手掌基向量
        """
        rvecs = np.asarray(mano_pose).reshape((-1, 3))
        if rvecs.size != 48:
            raise ValueError(f'MANO 姿态中期望 48 个元素（得到 {rvecs.size} 个）。')

        def rvec_to_joint(i, rvec):
            return mat2joint(self._basis[i].T @ rvec2mat(rvec) @ self._basis[i], self._axes[i])

        angles = [angle for i, rvec in enumerate(rvecs) for angle in rvec_to_joint(i, rvec)]
        palm_basis = rvec2mat(rvecs[0])
        return angles, palm_basis

    def _make_joints(self):
        """计算关节参数。

        返回:
            list -- 关节参数列表
        """
        raise NotImplementedError


class HandModel20(HandModel):
    """具有 20 个自由度的启发式刚体模型。"""

    def _make_joints(self):
        """计算关节参数。

        返回:
            list -- 关节参数列表
        """
        origin = dict(zip(self.link_names, self.origins()))
        basis = {'palm': np.eye(3)}

        def make_basis(yvec, zvec):
            mat = np.vstack([np.cross(yvec, zvec), yvec, zvec])
            return mat.T / np.linalg.norm(mat.T, axis=0)

        zvec = origin['index2'] - origin['index3']
        yvec = np.cross(zvec, [0.0, 0.0, 1.0])
        basis['index'] = make_basis(yvec, zvec)

        zvec = origin['middle2'] - origin['middle3']
        yvec = np.cross(zvec, origin['index1'] - origin['ring1'])
        basis['middle'] = make_basis(yvec, zvec)

        zvec = origin['ring2'] - origin['ring3']
        yvec = np.cross(zvec, origin['middle1'] - origin['ring1'])
        basis['ring'] = make_basis(yvec, zvec)

        zvec = origin['pinky2'] - origin['pinky3']
        yvec = np.cross(zvec, origin['ring1'] - origin['pinky1'])
        basis['pinky'] = make_basis(yvec, zvec)

        yvec = origin['thumb1'] - origin['index1']
        zvec = np.cross(yvec, origin['thumb1'] - origin['thumb2'])
        basis['thumb0'] = make_basis(yvec, zvec)

        zvec = origin['thumb2'] - origin['thumb3']
        yvec = np.cross(zvec, [0, -np.sin(0.96), np.cos(0.96)])
        basis['thumb'] = make_basis(yvec, zvec)

        if self.is_left_hand:
            rot = np.array([[-1, 0, 0], [0, -1, 0], [0, 0, 1]])
            basis = {key: mat @ rot for key, mat in basis.items()}

        return [
            Joint(origin['palm'], basis['palm'], '', []),
            Joint(origin['index1'], basis['index'], 'yx', np.deg2rad([(-20, 20), (-10, 90)])),
            Joint(origin['index2'], basis['index'], 'x', np.deg2rad([(0, 100)])),
            Joint(origin['index3'], basis['index'], 'x', np.deg2rad([(0, 100)])),
            Joint(origin['middle1'], basis['middle'], 'yx', np.deg2rad([(-30, 20), (-10, 90)])),
            Joint(origin['middle2'], basis['middle'], 'x', np.deg2rad([(0, 100)])),
            Joint(origin['middle3'], basis['middle'], 'x', np.deg2rad([(0, 100)])),
            Joint(origin['pinky1'], basis['pinky'], 'yx', np.deg2rad([(-40, 20), (-10, 90)])),
            Joint(origin['pinky2'], basis['pinky'], 'x', np.deg2rad([(0, 100)])),
            Joint(origin['pinky3'], basis['pinky'], 'x', np.deg2rad([(0, 100)])),
            Joint(origin['ring1'], basis['ring'], 'yx', np.deg2rad([(-30, 20), (-10, 90)])),
            Joint(origin['ring2'], basis['ring'], 'x', np.deg2rad([(0, 100)])),
            Joint(origin['ring3'], basis['ring'], 'x', np.deg2rad([(0, 100)])),
            Joint(origin['thumb1'], basis['thumb0'], 'yz', np.deg2rad([(-10, 150), (-40, 40)])),
            Joint(origin['thumb2'], basis['thumb'], 'x', np.deg2rad([(0, 100)])),
            Joint(origin['thumb3'], basis['thumb'], 'x', np.deg2rad([(0, 100)])),
        ]


class HandModel45(HandModel):
    """具有 45 个自由度的刚体模型。"""

    def _make_joints(self):
        """计算关节参数。

        返回:
            list -- 关节参数列表
        """
        origin = dict(zip(self.link_names, self.origins()))
        limits = [(-np.pi, np.pi), (-np.pi/2, np.pi/2), (-np.pi, np.pi)]

        return [
            Joint(origin['palm'], np.eye(3), '', []),
            Joint(origin['index1'], np.eye(3), 'xyz', limits),
            Joint(origin['index2'], np.eye(3), 'xyz', limits),
            Joint(origin['index3'], np.eye(3), 'xyz', limits),
            Joint(origin['middle1'], np.eye(3), 'xyz', limits),
            Joint(origin['middle2'], np.eye(3), 'xyz', limits),
            Joint(origin['middle3'], np.eye(3), 'xyz', limits),
            Joint(origin['pinky1'], np.eye(3), 'xyz', limits),
            Joint(origin['pinky2'], np.eye(3), 'xyz', limits),
            Joint(origin['pinky3'], np.eye(3), 'xyz', limits),
            Joint(origin['ring1'], np.eye(3), 'xyz', limits),
            Joint(origin['ring2'], np.eye(3), 'xyz', limits),
            Joint(origin['ring3'], np.eye(3), 'xyz', limits),
            Joint(origin['thumb1'], np.eye(3), 'xyz', limits),
            Joint(origin['thumb2'], np.eye(3), 'xyz', limits),
            Joint(origin['thumb3'], np.eye(3), 'xyz', limits),
        ]
