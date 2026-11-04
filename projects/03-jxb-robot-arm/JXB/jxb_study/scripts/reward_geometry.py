import math

import numpy as np


# ===== 常用参数配置 =====
# 这个值用来避免除以 0，或者对零向量归一化时出现数值不稳定。
EPSILON = 1e-8

# 朝向奖励权重：越大，模型越重视“末端是否朝向目标”。
# 注意：这里不是直接加分，而是作为距离奖励的乘法增益。
DIRECTION_WEIGHT = 1.0

# 距离衰减权重：越大，距离稍微变远时奖励下降越快。
DISTANCE_WEIGHT = 15.0

# 末端默认朝向轴：表示末端局部坐标系里哪个方向算“正前方”。
# (1, 0, 0) 表示 X 轴，(0, 1, 0) 表示 Y 轴，(0, 0, 1) 表示 Z 轴。
DEFAULT_LOCAL_FORWARD_AXIS = (1.0, 0.0, 0.0)


def distance_to_target(end_position, target_position):
    """计算末端执行器到目标点之间的直线距离。"""
    # 先把输入统一转成 numpy 数组，方便后面做向量减法。
    end_position = np.asarray(end_position, dtype=np.float32)
    target_position = np.asarray(target_position, dtype=np.float32)

    # target_position - end_position 得到“末端指向目标”的向量。
    # np.linalg.norm(...) 求这个向量的长度，也就是两点之间的距离。
    return float(np.linalg.norm(target_position - end_position))


def quaternion_to_matrix(quaternion):
    """把 PyBullet 风格四元数 [x, y, z, w] 转成 3x3 旋转矩阵。"""
    x, y, z, w = np.asarray(quaternion, dtype=np.float32)

    # 四元数最好先归一化，否则数值误差会影响旋转矩阵的准确性。
    norm = math.sqrt(float(x * x + y * y + z * z + w * w))
    if norm < EPSILON:
        raise ValueError("orientation_quat must not be a zero quaternion")

    x, y, z, w = x / norm, y / norm, z / norm, w / norm

    # 这是标准的“四元数 -> 旋转矩阵”公式。
    # 得到的矩阵可以把局部坐标系方向转换到世界坐标系方向。
    return np.array(
        [
            [1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y - z * w), 2.0 * (x * z + y * w)],
            [2.0 * (x * y + z * w), 1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z - x * w)],
            [2.0 * (x * z - y * w), 2.0 * (y * z + x * w), 1.0 - 2.0 * (x * x + y * y)],
        ],
        dtype=np.float32,
    )


def local_axis_in_world(orientation_quat, local_axis=DEFAULT_LOCAL_FORWARD_AXIS):
    """把末端自身坐标系里的某个轴，转换成世界坐标系中的方向。"""
    # 末端的朝向通常用四元数表示，不方便直接和目标方向做点乘。
    # 所以先把四元数转换成旋转矩阵。
    rotation = quaternion_to_matrix(orientation_quat)

    # local_axis 表示“末端自己的哪个方向算正前方”。
    local_axis = np.asarray(local_axis, dtype=np.float32)
    axis_norm = np.linalg.norm(local_axis)
    if axis_norm < EPSILON:
        raise ValueError("local_axis must not be a zero vector")

    # 旋转矩阵乘以局部方向向量，就能得到这个方向在世界坐标系里的方向。
    world_axis = rotation @ (local_axis / axis_norm)

    # 再归一化一次，保证返回的是单位方向向量，长度约等于 1。
    return world_axis / (np.linalg.norm(world_axis) + EPSILON)


def direction_alignment_cos(
    end_position,
    end_orientation_quat,
    target_position,
    local_forward_axis=DEFAULT_LOCAL_FORWARD_AXIS,
):
    """计算末端朝向和目标方向之间夹角的 cos 值。

    cos 越接近 1，说明末端越朝向目标；
    cos 接近 0，说明末端方向和目标方向接近垂直；
    cos 接近 -1，说明末端背对目标。
    """
    end_position = np.asarray(end_position, dtype=np.float32)
    target_position = np.asarray(target_position, dtype=np.float32)

    # 这个向量表示从末端位置出发，指向目标点的方向。
    to_target = target_position - end_position
    distance = np.linalg.norm(to_target)

    # 如果末端已经几乎和目标重合，就认为方向已经对准，直接返回 1。
    # 这样可以避免除以一个接近 0 的距离。
    if distance < EPSILON:
        return 1.0

    # 把“末端 -> 目标”的向量变成单位向量，只保留方向，不保留距离大小。
    target_direction = to_target / distance

    # 根据末端四元数，求出末端当前“正前方”在世界坐标系里的方向。
    tool_direction = local_axis_in_world(end_orientation_quat, local_forward_axis)

    # 两个单位向量的点乘结果就是夹角的 cos 值。
    cos_value = float(np.dot(tool_direction, target_direction))

    # 由于浮点误差，结果可能会略微超过 [-1, 1]，这里手动限制一下。
    return max(-1.0, min(1.0, cos_value))


def angle_to_target_rad(
    end_position,
    end_orientation_quat,
    target_position,
    local_forward_axis=DEFAULT_LOCAL_FORWARD_AXIS,
):
    """返回末端朝向和目标方向之间的夹角，单位是弧度。"""
    # 先算 cos 值，再通过 arccos 反推出真实夹角。
    cos_value = direction_alignment_cos(
        end_position,
        end_orientation_quat,
        target_position,
        local_forward_axis,
    )
    return math.acos(cos_value)


def position_and_direction_terms(
    end_position,
    end_orientation_quat,
    target_position,
    local_forward_axis=DEFAULT_LOCAL_FORWARD_AXIS,
):
    """一次性返回奖励设计里最基础的两个量：距离和朝向 cos。"""
    # 距离项：越小越好。
    distance = distance_to_target(end_position, target_position)

    # 朝向项：越接近 1 越好。
    alignment_cos = direction_alignment_cos(
        end_position,
        end_orientation_quat,
        target_position,
        local_forward_axis,
    )
    return {
        "distance": distance,
        "alignment_cos": alignment_cos,
    }


def calculate_reach_reward(
    end_position,
    end_orientation_quat,
    target_position,
    local_forward_axis=DEFAULT_LOCAL_FORWARD_AXIS,
):
    """根据“末端朝向目标”和“末端接近目标”计算一个基础奖励。"""
    terms = position_and_direction_terms(
        end_position,
        end_orientation_quat,
        target_position,
        local_forward_axis,
    )

    # 距离分数：距离越小越接近 1，距离越大越接近 0。
    # 这里使用指数衰减，而不是简单的 -distance。
    distance_reward = math.exp(-DISTANCE_WEIGHT * terms["distance"])

    # 朝向分数：把 cos 从 [-1, 1] 转换到 [0, 1]。
    # 0 表示背对目标，0.5 表示垂直，1 表示正对目标。
    direction_score = (terms["alignment_cos"] + 1.0) / 2.0
    direction_reward = DIRECTION_WEIGHT * direction_score#范围是 [0, DIRECTION_WEIGHT]，当完全朝向目标时达到最大值。

    # 总奖励不再使用“朝向奖励 + 距离奖励”的直接相加。
    # 这里让朝向作为距离奖励的增益：只有距离本身不错时，朝向好才更有价值。
    reward = distance_reward * (0.50 + direction_reward)#1.0 是基础奖励，保证即使朝向奖励为 0，距离奖励也能发挥作用。

    return {
        "reward": reward,
        "distance": terms["distance"],
        "alignment_cos": terms["alignment_cos"],
        "direction_score": direction_score,
        "direction_reward": direction_reward,
        "distance_reward": distance_reward,
    }
