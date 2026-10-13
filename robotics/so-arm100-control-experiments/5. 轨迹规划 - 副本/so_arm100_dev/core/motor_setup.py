# -*- coding: utf-8 -*-
"""
舵机ID设置工具
用于给SO-ARM100机械臂的舵机设置正确的ID
"""

import time
from typing import Optional
from .motor_bus import MotorBus

import sys
from pathlib import Path
sys.path.append(str(Path(__file__).parent.parent))
from config.motor_config import MOTOR_IDS, SCAN_BAUDRATES, BAUDRATE_TABLE


class MotorSetup:
    """
    舵机ID设置工具类
    用于逐个设置机械臂舵机的ID
    """
    
    def __init__(self, port: str):
        """
        初始化设置工具
        
        Args:
            port: 串口名称
        """
        self.port = port
        self.motor_bus = None
    
    def find_motor_baudrate(self) -> Optional[int]:
        """
        自动检测舵机的波特率
        
        Returns:
            Optional[int]: 检测到的波特率，如果失败返回None
        """
        print("\n正在自动检测舵机波特率...")
        
        for baudrate in SCAN_BAUDRATES:
            print(f"  尝试波特率: {baudrate}")
            
            try:
                bus = MotorBus(self.port, baudrate)
                if bus.connect():
                    # 尝试扫描ID 1
                    model = bus.ping(1)
                    if model is not None:
                        print(f"✓ 找到舵机! 波特率={baudrate}, 型号={model}")
                        bus.disconnect()
                        return baudrate
                    
                    # 尝试扫描广播ID
                    motors = bus.scan_motors(range(0, 10))
                    if motors:
                        print(f"✓ 找到舵机! 波特率={baudrate}")
                        bus.disconnect()
                        return baudrate
                    
                    bus.disconnect()
            except Exception as e:
                print(f"  波特率 {baudrate} 测试失败: {e}")
        
        print("✗ 未能检测到舵机波特率")
        return None
    
    def setup_single_motor(self, motor_name: str, target_id: int, 
                          current_baudrate: Optional[int] = None) -> bool:
        """
        设置单个舵机的ID
        
        使用方法：
        1. 只连接一个舵机到控制板
        2. 运行此函数设置ID
        3. 断开当前舵机，连接下一个舵机
        4. 重复步骤2-3
        
        Args:
            motor_name: 舵机名称（如"shoulder_pan"）
            target_id: 目标ID
            current_baudrate: 当前波特率（如果已知）
            
        Returns:
            bool: 设置是否成功
        """
        print("\n" + "=" * 60)
        print(f"设置舵机: {motor_name} (目标ID={target_id})")
        print("=" * 60)
        
        # 如果未指定波特率，自动检测
        if current_baudrate is None:
            current_baudrate = self.find_motor_baudrate()
            if current_baudrate is None:
                return False
        
        # 连接到舵机
        self.motor_bus = MotorBus(self.port, current_baudrate)
        if not self.motor_bus.connect():
            print("✗ 连接失败")
            return False
        
        # 扫描当前连接的舵机
        print("\n扫描当前连接的舵机...")
        motors = self.motor_bus.scan_motors(range(0, 254))
        
        if len(motors) == 0:
            print("✗ 未发现舵机，请检查连接")
            self.motor_bus.disconnect()
            return False
        
        if len(motors) > 1:
            print(f"✗ 发现多个舵机 {list(motors.keys())}，请确保只连接一个舵机")
            self.motor_bus.disconnect()
            return False
        
        # 获取当前舵机ID
        current_id = list(motors.keys())[0]
        print(f"\n当前舵机ID: {current_id}")
        
        if current_id == target_id:
            print(f"✓ 舵机ID已经是 {target_id}，无需修改")
            self.motor_bus.disconnect()
            return True
        
        # 修改ID
        print(f"\n正在将ID从 {current_id} 修改为 {target_id}...")
        
        # 关闭扭矩
        if not self.motor_bus.disable_torque(current_id):
            print("✗ 关闭扭矩失败")
            self.motor_bus.disconnect()
            return False
        
        time.sleep(0.2)
        
        # 写入新ID
        if not self.motor_bus.write_control_table(current_id, "ID", target_id):
            print("✗ 写入新ID失败")
            self.motor_bus.disconnect()
            return False
        
        time.sleep(0.5)
        
        # 验证新ID
        print(f"\n验证新ID...")
        new_model = self.motor_bus.ping(target_id)
        if new_model is not None:
            print(f"✓ ID设置成功! 新ID={target_id}")
            self.motor_bus.disconnect()
            return True
        else:
            print("✗ 无法验证新ID，可能设置失败")
            self.motor_bus.disconnect()
            return False
    
    def setup_all_motors(self, start_from: str = None):
        """
        交互式设置所有舵机ID
        
        Args:
            start_from: 从哪个舵机开始（可选）
        """
        print("\n" + "=" * 60)
        print("SO-ARM100机械臂舵机ID设置向导")
        print("=" * 60)
        print("\n说明:")
        print("1. 每次只连接一个舵机到控制板")
        print("2. 按照提示依次设置每个舵机的ID")
        print("3. 设置完一个后，断开该舵机，连接下一个")
        print()
        
        # 确定起始位置
        motor_list = list(MOTOR_IDS.items())
        if start_from:
            start_idx = next((i for i, (name, _) in enumerate(motor_list) 
                            if name == start_from), 0)
            motor_list = motor_list[start_idx:]
        
        # 询问波特率
        print("请选择舵机当前的波特率:")
        print("1. 自动检测（推荐）")
        print("2. 1,000,000 (1Mbps)")
        print("3. 115,200")
        choice = input("请输入选项 (1-3): ").strip()
        
        baudrate = None
        if choice == "2":
            baudrate = 1_000_000
        elif choice == "3":
            baudrate = 115_200
        
        # 逐个设置舵机
        for motor_name, target_id in motor_list:
            print(f"\n{'='*60}")
            print(f"准备设置: {motor_name} (ID={target_id})")
            print(f"{'='*60}")
            input(f"请只连接 '{motor_name}' 舵机，然后按回车键继续...")
            
            success = self.setup_single_motor(motor_name, target_id, baudrate)
            
            if success:
                print(f"\n✓ {motor_name} 设置完成!")
            else:
                print(f"\n✗ {motor_name} 设置失败!")
                retry = input("是否重试? (y/n): ").strip().lower()
                if retry == 'y':
                    success = self.setup_single_motor(motor_name, target_id, baudrate)
                else:
                    print("跳过此舵机...")
            
            print("\n" + "-" * 60)
        
        print("\n" + "=" * 60)
        print("所有舵机ID设置完成!")
        print("=" * 60)
        print("\n接下来可以:")
        print("1. 将所有舵机连接到控制板")
        print("2. 运行测试程序验证所有舵机")
    
    def verify_all_motors(self, baudrate: int = 1_000_000):
        """
        验证所有舵机是否正确连接且ID正确
        
        Args:
            baudrate: 波特率
        """
        print("\n" + "=" * 60)
        print("验证所有舵机")
        print("=" * 60)
        
        self.motor_bus = MotorBus(self.port, baudrate)
        if not self.motor_bus.connect():
            print("✗ 连接失败")
            return
        
        print("\n扫描所有舵机...")
        motors = self.motor_bus.scan_motors(range(1, 7))
        
        print("\n验证结果:")
        all_ok = True
        for motor_name, expected_id in MOTOR_IDS.items():
            if expected_id in motors:
                print(f"  ✓ {motor_name} (ID={expected_id}): 正常")
            else:
                print(f"  ✗ {motor_name} (ID={expected_id}): 未找到")
                all_ok = False
        
        if all_ok:
            print("\n✓ 所有舵机验证通过!")
        else:
            print("\n✗ 部分舵机未找到，请检查连接")
        
        self.motor_bus.disconnect()
