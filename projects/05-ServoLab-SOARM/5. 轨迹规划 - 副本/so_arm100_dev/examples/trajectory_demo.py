# -*- coding: utf-8 -*-
"""
SO-ARM100 轨迹规划演示程序

演示内容：
1. 点到点运动
2. 多路径点轨迹
3. 轨迹记录与回放
4. 运动原语
"""

import sys
import time
import logging
from pathlib import Path

# 添加项目根目录到路径
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

from core.robot import SO100Robot
from core.motion_controller import (
    MotionController, 
    TrajectoryRecorder, 
    MotionPrimitives,
    MotionState
)
from core.trajectory import JointTrajectoryPlanner, Trajectory

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


def demo_point_to_point(controller: MotionController):
    """
    演示1：点到点运动
    """
    print("\n" + "="*60)
    print("演示1：点到点运动")
    print("="*60)
    
    # 移动到零位
    print("\n1. 移动到零位...")
    controller.move_home(duration=3.0)
    time.sleep(0.5)
    
    # 移动shoulder_pan
    print("\n2. shoulder_pan 移动到 0.5...")
    controller.move_to_position({'shoulder_pan': 0.5}, duration=2.0)
    time.sleep(0.5)
    
    # 移动多个关节
    print("\n3. 多关节同时运动...")
    target = {
        'shoulder_pan': -0.3,
        'shoulder_lift': 0.3,
        'elbow_flex': -0.2
    }
    controller.move_to_position(target, duration=2.5)
    time.sleep(0.5)
    
    # 回到零位
    print("\n4. 返回零位...")
    controller.move_home(duration=3.0)
    
    print("\n✓ 点到点运动演示完成")


def demo_waypoint_trajectory(controller: MotionController):
    """
    演示2：多路径点轨迹
    """
    print("\n" + "="*60)
    print("演示2：多路径点轨迹")
    print("="*60)
    
    # 定义路径点
    waypoints = [
        {'shoulder_pan': 0.0, 'shoulder_lift': 0.0, 'elbow_flex': 0.0},
        {'shoulder_pan': 0.4, 'shoulder_lift': 0.2, 'elbow_flex': -0.3},
        {'shoulder_pan': 0.0, 'shoulder_lift': 0.4, 'elbow_flex': 0.0},
        {'shoulder_pan': -0.4, 'shoulder_lift': 0.2, 'elbow_flex': -0.3},
        {'shoulder_pan': 0.0, 'shoulder_lift': 0.0, 'elbow_flex': 0.0},
    ]
    
    print(f"\n执行 {len(waypoints)} 个路径点的轨迹...")
    print("路径点:")
    for i, wp in enumerate(waypoints):
        print(f"  {i+1}. {wp}")
    
    # 执行轨迹
    success = controller.move_through_waypoints(
        waypoints, 
        duration=8.0,
        blocking=True
    )
    
    if success:
        print("\n✓ 多路径点轨迹演示完成")
    else:
        print("\n✗ 轨迹执行失败")


def demo_interpolation_methods(controller: MotionController):
    """
    演示3：不同插值方法比较
    """
    print("\n" + "="*60)
    print("演示3：插值方法比较")
    print("="*60)
    
    methods = ['linear', 'cubic_spline', 'trapezoidal']
    
    start = {'shoulder_pan': -0.5}
    goal = {'shoulder_pan': 0.5}
    
    for method in methods:
        print(f"\n使用 {method} 插值...")
        
        # 创建指定插值方法的规划器
        controller.planner = JointTrajectoryPlanner(
            interpolation_method=method,
            max_velocity=0.8,
            max_acceleration=2.0
        )
        
        # 先移动到起点
        controller.move_to_position(start, duration=1.5)
        time.sleep(0.3)
        
        # 执行运动
        controller.move_to_position(goal, duration=2.0)
        time.sleep(0.5)
    
    # 恢复默认
    controller.planner = JointTrajectoryPlanner(interpolation_method='cubic_spline')
    controller.move_home(duration=2.0)
    
    print("\n✓ 插值方法比较演示完成")


def demo_trajectory_recording(robot: SO100Robot, controller: MotionController):
    """
    演示4：轨迹记录与回放
    """
    print("\n" + "="*60)
    print("演示4：轨迹记录与回放")
    print("="*60)
    
    recorder = TrajectoryRecorder(robot, frequency=50.0)
    
    # 关闭扭矩，允许手动拖动
    print("\n1. 关闭扭矩，进入记录模式...")
    robot.disable_torque_all()
    
    print("\n现在可以手动拖动机械臂")
    input("按回车开始记录...")
    
    # 开始记录
    recorder.start_recording()
    print("\n正在记录... (拖动机械臂演示动作)")
    print("按回车停止记录...")
    input()
    
    # 停止记录
    data = recorder.stop_recording()
    
    if len(data) < 10:
        print("记录数据太少，跳过回放")
        return
    
    # 转换为轨迹
    trajectory = recorder.get_trajectory()
    
    if trajectory is None:
        print("无法生成轨迹")
        return
    
    print(f"\n记录了 {len(data)} 个数据点，轨迹时长: {trajectory.duration:.2f}s")
    
    # 回放
    print("\n2. 使能扭矩，准备回放...")
    robot.enable_torque_all()
    time.sleep(0.5)
    
    input("按回车开始回放...")
    
    print("\n正在回放轨迹...")
    controller.execute_trajectory(trajectory, blocking=True)
    
    print("\n✓ 轨迹记录与回放演示完成")
    
    # 询问是否保存
    save = input("\n是否保存轨迹到文件? (y/n): ").strip().lower()
    if save == 'y':
        filepath = "recorded_trajectory.json"
        recorder.save_to_file(filepath)
        print(f"已保存到: {filepath}")


def demo_motion_primitives(controller: MotionController):
    """
    演示5：运动原语
    """
    print("\n" + "="*60)
    print("演示5：运动原语")
    print("="*60)
    
    primitives = MotionPrimitives(controller)
    
    # 挥手
    print("\n1. 挥手动作...")
    primitives.wave(amplitude=0.3, frequency=1.5, duration=3.0)
    time.sleep(0.5)
    
    # 点头
    print("\n2. 点头动作...")
    primitives.nod(amplitude=0.25, count=3)
    time.sleep(0.5)
    
    # 回到零位
    controller.move_home(duration=2.0)
    
    print("\n✓ 运动原语演示完成")


def demo_async_control(controller: MotionController):
    """
    演示6：异步控制
    """
    print("\n" + "="*60)
    print("演示6：异步控制（暂停/继续/停止）")
    print("="*60)
    
    # 规划一个长轨迹
    waypoints = []
    for i in range(10):
        angle = i * 0.1 - 0.5
        waypoints.append({
            'shoulder_pan': angle,
            'shoulder_lift': 0.3 * (i % 2),
        })
    
    print("\n启动异步轨迹执行...")
    
    # 非阻塞执行
    controller.move_through_waypoints(waypoints, duration=10.0, blocking=False)
    
    time.sleep(2.0)
    
    # 暂停
    print("\n暂停运动...")
    controller.pause()
    time.sleep(2.0)
    
    # 继续
    print("继续运动...")
    controller.resume()
    time.sleep(2.0)
    
    # 停止
    print("停止运动...")
    controller.stop()
    
    # 回零位
    time.sleep(0.5)
    controller.move_home(duration=2.0)
    
    print("\n✓ 异步控制演示完成")


def demo_pick_and_place(controller: MotionController):
    """
    演示7：抓取放置
    """
    print("\n" + "="*60)
    print("演示7：抓取放置（模拟）")
    print("="*60)
    
    primitives = MotionPrimitives(controller)
    
    # 定义抓取和放置位置
    pick_pos = {
        'shoulder_pan': 0.3,
        'shoulder_lift': 0.4,
        'elbow_flex': -0.3,
        'wrist_flex': 0.2,
        'wrist_roll': 0.0,
    }
    
    place_pos = {
        'shoulder_pan': -0.3,
        'shoulder_lift': 0.4,
        'elbow_flex': -0.3,
        'wrist_flex': 0.2,
        'wrist_roll': 0.0,
    }
    
    print("\n执行抓取放置动作...")
    primitives.pick_and_place(
        pick_position=pick_pos,
        place_position=place_pos,
        approach_height=0.2,
        move_duration=1.5
    )
    
    # 回零位
    controller.move_home(duration=2.0)
    
    print("\n✓ 抓取放置演示完成")


def main():
    """主函数"""
    print("\n" + "╔" + "="*58 + "╗")
    print("║" + " "*15 + "SO-ARM100 轨迹规划演示" + " "*18 + "║")
    print("╚" + "="*58 + "╝")
    
    # 输入串口
    port = input("\n请输入串口名称 (默认 COM24): ").strip() or "COM24"
    robot_id = input("请输入机械臂ID (默认 follower): ").strip() or "follower"
    
    # 创建机械臂实例
    robot = SO100Robot(
        port=port,
        robot_id=robot_id,
        max_relative_target=None  # 轨迹规划自带平滑，无需限制
    )
    
    try:
        # 连接
        print("\n正在连接机械臂...")
        robot.connect()
        
        # 使能扭矩
        robot.enable_torque_all()
        time.sleep(0.5)
        
        # 创建运动控制器
        controller = MotionController(
            robot,
            control_frequency=50.0,
            position_tolerance=0.02
        )
        
        # 主菜单
        while True:
            print("\n" + "="*60)
            print("轨迹规划演示菜单")
            print("="*60)
            print("1. 点到点运动")
            print("2. 多路径点轨迹")
            print("3. 插值方法比较")
            print("4. 轨迹记录与回放")
            print("5. 运动原语（挥手、点头）")
            print("6. 异步控制（暂停/继续/停止）")
            print("7. 抓取放置（模拟）")
            print("8. 移动到零位")
            print("0. 退出")
            
            choice = input("\n请选择演示项目: ").strip()
            
            if choice == '1':
                demo_point_to_point(controller)
            elif choice == '2':
                demo_waypoint_trajectory(controller)
            elif choice == '3':
                demo_interpolation_methods(controller)
            elif choice == '4':
                demo_trajectory_recording(robot, controller)
            elif choice == '5':
                demo_motion_primitives(controller)
            elif choice == '6':
                demo_async_control(controller)
            elif choice == '7':
                demo_pick_and_place(controller)
            elif choice == '8':
                controller.move_home(duration=3.0)
                print("✓ 已移动到零位")
            elif choice == '0':
                print("\n再见！")
                break
            else:
                print("无效选择，请重试")
    
    except KeyboardInterrupt:
        print("\n\n程序被中断")
    except Exception as e:
        logger.error(f"发生错误: {e}", exc_info=True)
    finally:
        # 断开连接
        if robot.is_connected:
            robot.disconnect()
        print("\n程序结束")


if __name__ == "__main__":
    main()
