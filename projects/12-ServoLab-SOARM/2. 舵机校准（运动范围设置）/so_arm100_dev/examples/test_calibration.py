# -*- coding: utf-8 -*-
"""
校准测试程序
快速测试校准数据是否正确加载和应用
"""

import sys
from pathlib import Path

project_root = Path(__file__).parent.parent
sys.path.append(str(project_root))

from core.motor_bus import MotorBus
from core.calibration import MotorCalibration


def test_load_calibration(port: str = "COM3"):
    """
    测试加载校准数据
    
    Args:
        port: 串口名称
    """
    print("="*60)
    print("测试：加载校准数据")
    print("="*60)
    
    # 连接
    bus = MotorBus(port, 1_000_000)
    if not bus.connect():
        print("✗ 连接失败")
        return
    
    # 加载校准数据
    calibrator = MotorCalibration(bus)
    data = calibrator.load_calibration()
    
    if data:
        print("\n✓ 校准数据加载成功")
        calibrator.display_calibration()
    else:
        print("\n✗ 校准数据加载失败")
    
    bus.disconnect()


def test_verify_calibration(port: str = "COM3"):
    """
    测试验证校准数据
    
    Args:
        port: 串口名称
    """
    print("\n" + "="*60)
    print("测试：验证校准数据")
    print("="*60)
    
    # 连接
    bus = MotorBus(port, 1_000_000)
    if not bus.connect():
        print("✗ 连接失败")
        return
    
    # 加载并验证
    calibrator = MotorCalibration(bus)
    if calibrator.load_calibration():
        calibrator.verify_calibration()
    else:
        print("✗ 无法加载校准数据")
    
    bus.disconnect()


def test_read_positions(port: str = "COM3"):
    """
    测试读取所有舵机位置
    
    Args:
        port: 串口名称
    """
    print("\n" + "="*60)
    print("测试：读取所有舵机位置")
    print("="*60)
    
    # 连接
    bus = MotorBus(port, 1_000_000)
    if not bus.connect():
        print("✗ 连接失败")
        return
    
    # 读取位置
    calibrator = MotorCalibration(bus)
    positions = calibrator.read_all_positions()
    
    print("\n当前舵机位置:")
    for motor_name, pos in positions.items():
        print(f"  {motor_name:15s}: {pos:5d}")
    
    bus.disconnect()


def main():
    print("""
╔══════════════════════════════════════════════════════════╗
║      SO-ARM100 校准测试程序                              ║
╚══════════════════════════════════════════════════════════╝
    """)
    
    port = input("请输入串口名称 (默认 COM3): ").strip() or "COM3"
    
    print("\n请选择测试:")
    print("1. 加载校准数据")
    print("2. 验证校准数据")
    print("3. 读取当前位置")
    print("4. 运行所有测试")
    
    choice = input("\n请选择 (1-4): ").strip()
    
    try:
        if choice == "1":
            test_load_calibration(port)
        elif choice == "2":
            test_verify_calibration(port)
        elif choice == "3":
            test_read_positions(port)
        elif choice == "4":
            test_load_calibration(port)
            test_verify_calibration(port)
            test_read_positions(port)
        else:
            print("无效的选项")
    
    except Exception as e:
        print(f"\n✗ 测试失败: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n程序被用户中断")
