"""
SO-ARM100机械臂控制示例程序
演示如何使用Robot类控制机械臂
"""

import sys
import time
import logging
from pathlib import Path

# 添加项目根目录到路径
sys.path.insert(0, str(Path(__file__).parent.parent))

from core.robot import SO100Robot


# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)


def demo_basic_control():
    """基础控制演示"""
    print("\n" + "="*60)
    print("SO-ARM100 基础控制演示")
    print("="*60)
    
    # 输入串口
    port = input("\n请输入串口名称 (默认 COM3): ").strip() or "COM3"
    
    # 创建机械臂实例
    robot = SO100Robot(
        port=port,
        baudrate=1_000_000,
        calibration_dir="./calibration",
        max_relative_target=1,  # 限制单步最大移动10%范围
    )
    
    try:
        # 连接
        print("\n正在连接机械臂...")
        robot.connect()
        
        # 使能扭矩
        print("\n使能所有舵机扭矩...")
        robot.enable_torque_all()
        time.sleep(0.5)
        
        # 读取当前观测
        print("\n读取当前关节位置（归一化值 [-1, 1]）:")
        obs = robot.get_observation()
        for motor, value in obs.items():
            print(f"  {motor:15s}: {value:7.3f}")
        
        # 发送测试动作
        print("\n发送测试动作...")
        print("将所有关节移动到中间位置（0.0）")
        
        action = {motor: 0.0 for motor in obs.keys()}
        actual_action = robot.send_action(action)
        
        print("\n实际发送的动作:")
        for motor, value in actual_action.items():
            print(f"  {motor:15s}: {value:7.3f}")
        
        # 等待运动完成
        time.sleep(2.0)
        
        # 读取新位置
        print("\n读取新位置:")
        new_obs = robot.get_observation()
        for motor, value in new_obs.items():
            print(f"  {motor:15s}: {value:7.3f}")
        
    except Exception as e:
        print(f"\n✗ 错误: {e}")
    finally:
        # 断开连接
        print("\n断开连接...")
        robot.disconnect()


def demo_interactive_control():
    """交互式控制演示"""
    import logging
    
    # 设置详细日志
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    
    print("\n" + "="*60)
    print("SO-ARM100 交互式控制")
    print("="*60)
    
    # 输入串口
    port = input("\n请输入串口名称 (默认 COM3): ").strip() or "COM3"
    
    # 创建机械臂实例
    robot = SO100Robot(
        port=port,
        baudrate=1_000_000,
        calibration_dir="./calibration",
        max_relative_target=1,
    )
    
    try:
        # 连接
        robot.connect()
        robot.enable_torque_all()
        
        print("\n" + "="*60)
        print("交互式控制（输入目标位置，范围 [-1, 1]）")
        print("输入 'q' 退出")
        print("="*60)
        
        while True:
            # 显示当前位置
            print("\n当前位置:")
            obs = robot.get_observation()
            for motor, value in obs.items():
                print(f"  {motor:15s}: {value:7.3f}")
            
            # 输入控制指令
            print("\n输入舵机名称和目标位置 (例如: shoulder_pan 0.5)")
            print("或输入 'all 0.0' 控制所有舵机")
            user_input = input("> ").strip()
            
            if user_input.lower() == 'q':
                break
            
            try:
                parts = user_input.split()
                if len(parts) != 2:
                    print("格式错误！请输入: <舵机名称> <目标位置>")
                    continue
                
                motor_name, target_str = parts
                target = float(target_str)
                
                if target < -1 or target > 1:
                    print("目标位置超出范围 [-1, 1]")
                    continue
                
                # 发送动作
                if motor_name.lower() == 'all':
                    action = {m: target for m in obs.keys()}
                else:
                    if motor_name not in obs:
                        print(f"未知舵机: {motor_name}")
                        continue
                    action = {motor_name: target}
                
                robot.send_action(action)
                print(f"✓ 已发送动作")
                time.sleep(0.5)
                
            except ValueError:
                print("无效的数值！")
            except Exception as e:
                print(f"错误: {e}")
    
    except Exception as e:
        print(f"\n✗ 错误: {e}")
    finally:
        robot.disconnect()


def demo_motor_info():
    """显示舵机详细信息"""
    print("\n" + "="*60)
    print("SO-ARM100 舵机信息查询")
    print("="*60)
    
    port = input("\n请输入串口名称 (默认 COM3): ").strip() or "COM3"
    
    robot = SO100Robot(port=port)
    
    try:
        robot.connect()
        
        print("\n" + "="*60)
        print("舵机详细信息")
        print("="*60)
        
        info = robot.get_motor_info()
        
        for motor_name, motor_info in info.items():
            print(f"\n{motor_name}:")
            print(f"  ID:          {motor_info['id']}")
            print(f"  Position:    {motor_info['position']}")
            print(f"  Load:        {motor_info['load']}")
            print(f"  Voltage:     {motor_info['voltage'] / 10:.1f}V" if motor_info['voltage'] else "  Voltage:     N/A")
            print(f"  Temperature: {motor_info['temperature']}°C" if motor_info['temperature'] else "  Temperature: N/A")
    
    except Exception as e:
        print(f"\n✗ 错误: {e}")
    finally:
        robot.disconnect()


def main():
    """主函数"""
    print("\n" + "╔" + "="*58 + "╗")
    print("║" + " "*15 + "SO-ARM100 机械臂控制工具" + " "*19 + "║")
    print("╚" + "="*58 + "╝")
    
    while True:
        print("\n" + "="*60)
        print("主菜单")
        print("="*60)
        print("1. 基础控制演示")
        print("2. 交互式控制")
        print("3. 查询舵机信息")
        print("0. 退出")
        
        choice = input("\n请选择操作: ").strip()
        
        if choice == '1':
            demo_basic_control()
        elif choice == '2':
            demo_interactive_control()
        elif choice == '3':
            demo_motor_info()
        elif choice == '0':
            print("\n再见！")
            break
        else:
            print("\n无效选择！")


if __name__ == "__main__":
    main()
