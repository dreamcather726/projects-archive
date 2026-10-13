"""GUI 关节控制测试应用程序。"""

import argparse

import numpy as np
import pybullet as pb
from pybullet_utils.bullet_client import BulletClient

from ..hand_body import HandBody
from ..hand_model import HandModel20, HandModel45

parser = argparse.ArgumentParser('GUI 调试工具')
parser.add_argument('--dofs', type=int, default=20,
                    help='自由度数量 (20 或 45)')

parser.add_argument('--left-hand', dest='left_hand', action='store_true',
                    help='显示左手')
parser.add_argument('--right-hand', dest='left_hand', action='store_false',
                    help='显示右手')
parser.set_defaults(left_hand=False)

parser.add_argument('--visual-shapes', dest='visual', action='store_true',
                    help='显示视觉形状')
parser.add_argument('--no-visual-shapes', dest='visual', action='store_false',
                    help='隐藏视觉形状')
parser.set_defaults(visual=True)

parser.add_argument('--self-collisions', dest='self_collisions', action='store_true',
                    help='启用自碰撞')
parser.add_argument('--no-self-collisions', dest='self_collisions', action='store_false',
                    help='禁用自碰撞')
parser.set_defaults(self_collisions=True)


def main(args):
    """测试 GUI 应用程序。"""
    client = BulletClient(pb.GUI)
    client.setGravity(0, 0, -10)

    client.resetDebugVisualizerCamera(
        cameraDistance=0.5,
        cameraYaw=-40.0,
        cameraPitch=-40.0,
        cameraTargetPosition=[0.0, 0.0, 0.0]
    )

    if args.dofs == 20:
        hand_model = HandModel20(left_hand=args.left_hand)
    elif args.dofs == 45:
        hand_model = HandModel45(left_hand=args.left_hand)
    else:
        raise ValueError('仅支持 20 和 45 自由度的模型。')

    flags = sum([
        HandBody.FLAG_ENABLE_COLLISION_SHAPES,
        HandBody.FLAG_ENABLE_VISUAL_SHAPES * args.visual,
        HandBody.FLAG_JOINT_LIMITS,
        HandBody.FLAG_DYNAMICS,
        HandBody.FLAG_USE_SELF_COLLISION * args.self_collisions])

    client.configureDebugVisualizer(pb.COV_ENABLE_RENDERING, 0)
    hand = HandBody(client, hand_model, flags=flags)
    client.configureDebugVisualizer(pb.COV_ENABLE_RENDERING, 1)

    slider_ids = []
    for coord in ('X', 'Y', 'Z'):
        uid = client.addUserDebugParameter(f'base_{coord}', -0.5, 0.5, 0)
        slider_ids.append(uid)
    for coord in ('R', 'P', 'Y'):
        uid = client.addUserDebugParameter(f'base_{coord}', -3.14, 3.14, 0)
        slider_ids.append(uid)
    for i, joint in enumerate(hand_model.joints):
        name = hand_model.link_names[i]
        for axis, (lower, upper) in zip(joint.axes, joint.limits):
            uid = client.addUserDebugParameter(f'{name}[{axis}]', lower, upper, 0)
            slider_ids.append(uid)

    client.setRealTimeSimulation(True)

    try:
        while client.isConnected():
            values = [client.readUserDebugParameter(uid) for uid in slider_ids]

            position = values[0:3]
            rotation = client.getQuaternionFromEuler(values[3:6])
            angles = values[6:]

            hand.set_target(position, rotation, angles)
    except pb.error as err:
        if str(err) not in ['未连接到物理服务器。', '读取参数失败。']:
            raise


if __name__ == '__main__':
    main(parser.parse_args())
