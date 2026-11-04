# -*- coding: utf-8 -*-
"""
串口查找工具
用于自动识别SO-ARM100机械臂连接的串口
"""

import platform
import time
from pathlib import Path


def find_available_ports():
    """查找系统中所有可用的串口"""
    from serial.tools import list_ports
    
    if platform.system() == "Windows":
        # Windows系统列出COM端口
        ports = [port.device for port in list_ports.comports()]
    else:  # Linux/macOS
        # Unix系统列出/dev/tty*端口
        ports = [str(path) for path in Path("/dev").glob("tty*")]
    
    return ports


def find_motor_bus_port():
    """
    通过拔插USB线的方式自动识别机械臂串口
    
    Returns:
        str: 检测到的串口名称
    """
    print("=" * 50)
    print("SO-ARM100机械臂串口查找工具")
    print("=" * 50)
    
    print("\n[步骤1] 正在扫描当前连接的串口...")
    ports_before = find_available_ports()
    print(f"已连接的串口: {ports_before}")
    
    print("\n[步骤2] 请拔出机械臂的USB线，然后按回车键...")
    input()
    
    time.sleep(0.5)  # 等待端口释放
    ports_after = find_available_ports()
    ports_diff = list(set(ports_before) - set(ports_after))
    
    if len(ports_diff) == 1:
        port = ports_diff[0]
        print(f"\n✓ 检测到机械臂串口: {port}")
        print("\n请重新插入USB线...")
        input("插入后按回车键继续...")
        return port
    elif len(ports_diff) == 0:
        raise RuntimeError("错误：未检测到端口变化，请确认USB线已拔出")
    else:
        raise RuntimeError(f"错误：检测到多个端口变化 ({ports_diff})，请确保只连接机械臂")


def list_all_ports():
    """列出所有可用串口"""
    print("系统中所有可用的串口:")
    ports = find_available_ports()
    if ports:
        for i, port in enumerate(ports, 1):
            print(f"  {i}. {port}")
    else:
        print("  未检测到任何串口")
    return ports


if __name__ == "__main__":
    try:
        # 查找机械臂串口
        port = find_motor_bus_port()
        print(f"\n串口识别完成: {port}")
        print("\n将此串口配置到你的代码中即可使用")
    except Exception as e:
        print(f"\n错误: {e}")
