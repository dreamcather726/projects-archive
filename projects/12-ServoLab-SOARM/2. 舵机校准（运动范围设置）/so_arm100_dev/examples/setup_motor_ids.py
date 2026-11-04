# -*- coding: utf-8 -*-
"""
舵机ID设置示例程序
用于给SO-ARM100机械臂的6个舵机设置正确的ID
"""

import sys
from pathlib import Path

# 添加项目路径
project_root = Path(__file__).parent.parent
sys.path.append(str(project_root))

from core.motor_setup import MotorSetup
from utils.port_finder import find_motor_bus_port, list_all_ports


def main():
    print("""
╔══════════════════════════════════════════════════════════╗
║      SO-ARM100 机械臂舵机ID设置工具                      ║
╚══════════════════════════════════════════════════════════╝
    """)
    
    # 选择串口
    print("请选择串口获取方式:")
    print("1. 自动检测（推荐）")
    print("2. 手动输入")
    print("3. 查看所有串口")
    
    choice = input("\n请输入选项 (1-3): ").strip()
    
    if choice == "1":
        try:
            port = find_motor_bus_port()
        except Exception as e:
            print(f"自动检测失败: {e}")
            port = input("请手动输入串口名称 (如 COM3 或 /dev/ttyUSB0): ").strip()
    elif choice == "3":
        list_all_ports()
        port = input("\n请输入要使用的串口名称: ").strip()
    else:
        port = input("请输入串口名称 (如 COM3 或 /dev/ttyUSB0): ").strip()
    
    # 创建设置工具
    setup = MotorSetup(port)
    
    # 选择操作
    print("\n请选择操作:")
    print("1. 设置所有舵机ID（完整向导）")
    print("2. 设置单个舵机ID")
    print("3. 验证所有舵机")
    
    choice = input("\n请输入选项 (1-3): ").strip()
    
    if choice == "1":
        # 完整向导模式
        setup.setup_all_motors()
    
    elif choice == "2":
        # 单个舵机设置
        print("\n可用的舵机:")
        from config.motor_config import MOTOR_IDS
        for i, (name, motor_id) in enumerate(MOTOR_IDS.items(), 1):
            print(f"  {i}. {name} (目标ID={motor_id})")
        
        motor_choice = input("\n请输入要设置的舵机编号: ").strip()
        try:
            motor_idx = int(motor_choice) - 1
            motor_name = list(MOTOR_IDS.keys())[motor_idx]
            target_id = MOTOR_IDS[motor_name]
            
            print(f"\n准备设置: {motor_name} (ID={target_id})")
            input("请确保只连接此舵机，然后按回车继续...")
            
            setup.setup_single_motor(motor_name, target_id)
        except (ValueError, IndexError):
            print("无效的选项")
    
    elif choice == "3":
        # 验证模式
        print("\n请确保所有舵机都已连接到控制板")
        input("按回车开始验证...")
        setup.verify_all_motors()
    
    else:
        print("无效的选项")
    
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
