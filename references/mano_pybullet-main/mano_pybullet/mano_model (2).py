"""此模块描述 ManoModel 类。"""

import functools
import os
import pickle
import warnings

import numpy as np

from .math_utils import rvec2mat

__all__ = ('ManoModel')


class ManoModel:
    """用于操作 MANO 手部模型的辅助类。"""

    def __init__(self, left_hand=False, models_dir=None):
        """从 pickle 文件加载手部模型。

        关键字参数:
            left_hand {bool} -- 创建左手模型 (默认: {False})
            models_dir {str} -- pickle 模型文件的路径 (默认: {None})
        """
        if models_dir is None:
            models_dir = os.path.expandvars('$MANO_MODELS_DIR')

        fname = f'MANO_{["RIGHT", "LEFT"][left_hand]}.pkl'
        self._model = self._load(os.path.join(models_dir, fname))
        self._is_left_hand = left_hand

    @staticmethod
    @functools.lru_cache(maxsize=2)
    def _load(path):
        """从磁盘加载模型。

        参数:
            path {str} -- pickle 模型文件的路径

        返回:
            chumpy array -- MANO 模型
        """
        with open(path, 'rb') as pick_file:
            with warnings.catch_warnings():  # 抑制 chumpy 警告
                warnings.filterwarnings("ignore", category=DeprecationWarning)
                return pickle.load(pick_file, encoding='latin1')

    @property
    def is_left_hand(self):
        """这是左手模型。

        返回:
            bool -- 左手标志
        """
        return self._is_left_hand

    @property
    def faces(self):
        """手部网格面的索引。

        返回:
            np.ndarray -- 矩阵 Nf x 3，其中 Nf 是面的数量
        """
        return self._model.get('f')

    @property
    def weights(self):
        """顶点权重。

        返回:
            array -- 矩阵 Nv x Nl，其中 Nv 是顶点数，Nl 是链接数
        """
        return self._model.get('weights')

    @property
    def kintree_table(self):
        """运动学树。

        返回:
            array -- 矩阵 2 x Nl，其中 Nl 是链接数
        """
        return np.int32(self._model.get('kintree_table'))

    @property
    def shapedirs(self):
        """形状映射矩阵。

        返回:
            array -- 矩阵 Nv x 3 x Nb，其中 Nv 是顶点数，Nb 是形状系数数量
        """
        return self._model.get('shapedirs')

    @property
    def posedirs(self):
        """姿态映射矩阵。

        返回:
            array -- 矩阵 Nv x 3 x ((Nl-1)*9)，其中 Nv 是顶点数，Nl 是链接数
        """
        return self._model.get('posedirs')

    @property
    def link_names(self):
        """人类可读的链接名称。

        返回:
            list -- 链接名称列表，大小为 Nl，其中 Nl 是链接数
        """
        fingers = ('index', 'middle', 'pinky', 'ring', 'thumb')
        return ['palm'] + ['{}{}'.format(f, i) for f in fingers for i in range(1, 4)]

    @property
    def tip_links(self):
        """指尖链接索引。

        返回:
            list -- 指尖链接索引列表
        """
        return [3, 6, 12, 9, 15]

    def origins(self, betas=None, pose=None, trans=None):
        """关节原点。

        关键字参数:
            betas {array} -- 形状系数，向量 1 x 10 (默认: {None})
            pose {array} -- 手部姿态，矩阵 Nl x 3 (默认: {None})
            trans {array} -- 平移，向量 1 x 3 (默认: {None})

        返回:
            array -- 矩阵 Nl x 3，其中 Nl 是链接数
        """
        origins = self._model.get('J')
        if betas is not None:
            regressor = self._model.get('J_regressor')
            origins = regressor.dot(self.vertices(betas=betas))
        if pose is not None:
            raise NotImplementedError
        if trans is not None:
            origins = origins + trans
        return origins

    def vertices(self, betas=None, pose=None, trans=None):
        """手部网格顶点。

        关键字参数:
            betas {array} -- 形状系数，向量 1 x 10 (默认: {None})
            pose {array} -- 手部姿态，矩阵 Nl x 3 (默认: {None})
            trans {array} -- 平移，向量 1 x 3 (默认: {None})

        返回:
            array -- 矩阵 Nv x 3，其中 Nv 是顶点数
        """
        vertices = self._model.get('v_template')
        if betas is not None:
            vertices = vertices + np.dot(self.shapedirs, betas)
        if pose is not None:
            pose = np.ravel([rvec2mat(rvec) - np.eye(3) for rvec in pose[1:]])
            vertices = vertices + np.dot(self.posedirs, pose)
            raise NotImplementedError
        if trans is not None:
            vertices = vertices + trans
        return vertices
