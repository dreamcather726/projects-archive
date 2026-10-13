"""网格操作工具函数。"""

import numpy as np

__all__ = ('filter_mesh', 'save_mesh_obj')


def filter_mesh(vertices, faces, vertex_mask):
    """使用顶点掩码从网格中获取子网格。

    参数:
        vertices {array} -- 完整网格顶点
        faces {array} -- 完整网格面
        vertex_mask {boolean array} -- 顶点过滤器
    """
    index_map = np.cumsum(vertex_mask) - 1
    faces_mask = np.all(vertex_mask[faces], axis=1)
    vertices_sub = vertices[vertex_mask]
    faces_sub = index_map[faces[faces_mask]]
    return vertices_sub, faces_sub


def save_mesh_obj(filename, vertices, faces):
    """将网格保存为 obj 文件。

    参数:
        filename {str} -- 输出文件名
        vertices {array} -- 网格顶点
        faces {array} -- 网格面
    """
    with open(filename, 'w') as obj:
        for vert in vertices:
            obj.write('v {:f} {:f} {:f}\n'.format(*vert))
        for face in faces + 1:  # 在 obj 文件中，面索引从 1 开始，不是从 0 开始
            obj.write('f {:d} {:d} {:d}\n'.format(*face))
