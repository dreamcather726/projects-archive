# -*- coding: utf-8 -*-
"""
舵机校准示例程序
用于校准SO-ARM100机械臂的零点位置和运动范围
"""

import sys
from pathlib import Path

# 添加项目路径
project_root = Path(__file__).parent.parent
sys.path.append(str(project_root))

from core.motor_bus import MotorBus
from core.calibration import MotorCalibration
from utils.port_finder import find_motor_bus_port, list_all_ports


def main():
    print("""
╔══════════════════════════════════════════════════════════╗
║      SO-ARM100 机械臂舵机校准工具（LeRobot方式）         ║
╚══════════════════════════════════════════════════════════╝
    """)
    
    # 输入机械臂ID
    print("提示：如果有多个机械臂，请为每个机械臂指定唯一ID")
    print("例如：leader, follower, robot1, robot2 等")
    robot_id = input("\n请输入机械臂ID (默认 default): ").strip() or "default"
    
    # 选择串口
    port = input("请输入串口名称 (默认 COM3): ").strip() or "COM3"
    
    # 连接舵机总线
    print(f"\n正在连接到串口: {port}")
    motor_bus = MotorBus(port, 1_000_000)
    
    if not motor_bus.connect():
        print("✗ 连接失败，程序退出")
        return
    
    # 创建校准工具
    calibrator = MotorCalibration(motor_bus)
    
    # 主菜单
    while True:
        print("\n" + "="*60)
        print("校准工具主菜单")
        print("="*60)
        print("1. 完整校准流程（推荐）")
        print("2. 加载校准数据")
        print("3. 显示校准数据")
        print("4. 保存校准数据")
        print("5. 验证校准数据")
        print("6. 读取当前位置")
        print("0. 退出")
        
        choice = input("\n请选择操作: ").strip()
        
        if choice == "1":
            # 完整校准流程（LeRobot方式）
            calibration_data = calibrator.calibrate_all_motors()
            
            # 自动保存
            if calibration_data:
                calibrator.save_calibration(robot_id=robot_id)
                print(f"\n提示：校准数据已保存 (robot_id={robot_id})")
        
        elif choice == "2":
            # 加载校准数据
            filename = input("\n请输入校准文件名 (默认: so_arm100_calibration.json): ").strip()
            if not filename:
                filename = "so_arm100_calibration.json"
            
            calibrator.load_calibration(filename)
        
        elif choice == "3":
            # 显示校准数据
            calibrator.display_calibration()
        
        elif choice == "4":
            # 保存校准数据
            save_id = input(f"\n请输入保存的robot_id (默认 {robot_id}): ").strip() or robot_id
            calibrator.save_calibration(robot_id=save_id)
        
        elif choice == "5":
            # 验证校准数据
            calibrator.verify_calibration()
        
        elif choice == "6":
            # 读取当前位置
            print("\n读取所有舵机当前位置...")
            positions = calibrator.read_all_positions()
            
            print("\n舵机位置:")
            for motor_name, pos in positions.items():
                print(f"  {motor_name}: {pos}")
        
        elif choice == "0":
            # 退出
            print("\n退出校准工具")
            break
        
        else:
            print("✗ 无效的选项")
    
    # 断开连接
    motor_bus.disconnect()
    print("\n程序结束")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n程序被用户中断")
    except Exception as e:
        print(f"\n发生错误: {e}")
        import traceback
        traceback.print_exc()
