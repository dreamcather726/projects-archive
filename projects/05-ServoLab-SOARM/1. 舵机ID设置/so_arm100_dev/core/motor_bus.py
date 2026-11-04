# -*- coding: utf-8 -*-
"""
舵机总线通信基础类
提供与STS3215舵机的底层通信功能
"""

import time
from typing import Optional, Dict, List

try:
    import scservo_sdk as scs
except ImportError:
    import feetech_servo_sdk as scs

# 导入配置
import sys
from pathlib import Path
sys.path.append(str(Path(__file__).parent.parent))
from config.motor_config import (
    PROTOCOL_VERSION,
    DEFAULT_BAUDRATE,
    TIMEOUT_MS,
    CONTROL_TABLE,
    BAUDRATE_TABLE,
    SCAN_BAUDRATES,
    OperatingMode,
    TorqueMode,
)


class MotorBus:
    """
    舵机总线通信类
    封装了与STS3215舵机通信的底层操作
    """
    
    def __init__(self, port: str, baudrate: int = DEFAULT_BAUDRATE):
        """
        初始化舵机总线
        
        Args:
            port: 串口名称（如 "COM3" 或 "/dev/ttyUSB0"）
            baudrate: 波特率（默认1000000）
        """
        self.port = port
        self.baudrate = baudrate
        self.protocol_version = PROTOCOL_VERSION
        
        # 创建端口和数据包处理器
        self.port_handler = scs.PortHandler(port)
        self.packet_handler = scs.PacketHandler(PROTOCOL_VERSION)
        
        # 通信状态码
        self._comm_success = scs.COMM_SUCCESS
        self._no_error = 0x00
        
        self.is_connected = False
    
    def connect(self) -> bool:
        """
        连接到舵机总线
        
        Returns:
            bool: 连接是否成功
        """
        try:
            if self.port_handler.openPort():
                print(f"✓ 串口打开成功: {self.port}")
            else:
                print(f"✗ 无法打开串口: {self.port}")
                return False
            
            if self.port_handler.setBaudRate(self.baudrate):
                print(f"✓ 波特率设置成功: {self.baudrate}")
            else:
                print(f"✗ 无法设置波特率: {self.baudrate}")
                return False
            
            self.is_connected = True
            return True
            
        except Exception as e:
            print(f"✗ 连接失败: {e}")
            return False
    
    def disconnect(self):
        """断开舵机总线连接"""
        if self.is_connected:
            self.port_handler.closePort()
            self.is_connected = False
            print("✓ 串口已关闭")
    
    def set_baudrate(self, baudrate: int) -> bool:
        """
        设置波特率
        
        Args:
            baudrate: 新的波特率
            
        Returns:
            bool: 设置是否成功
        """
        if self.port_handler.setBaudRate(baudrate):
            self.baudrate = baudrate
            return True
        return False
    
    def ping(self, motor_id: int) -> Optional[int]:
        """
        Ping单个舵机，检查是否在线
        
        Args:
            motor_id: 舵机ID
            
        Returns:
            Optional[int]: 舵机型号编号，如果失败返回None
        """
        model_number, comm_result, error = self.packet_handler.ping(
            self.port_handler, motor_id
        )
        
        if comm_result == self._comm_success and error == self._no_error:
            return model_number
        return None
    
    def scan_motors(self, id_range: range = range(0, 254)) -> Dict[int, int]:
        """
        扫描总线上的所有舵机
        
        Args:
            id_range: 要扫描的ID范围
            
        Returns:
            Dict[int, int]: {舵机ID: 型号编号}
        """
        found_motors = {}
        print(f"\n正在扫描ID范围 {id_range.start} 到 {id_range.stop}...")
        
        for motor_id in id_range:
            model = self.ping(motor_id)
            if model is not None:
                found_motors[motor_id] = model
                print(f"  发现舵机: ID={motor_id}, 型号={model}")
        
        if not found_motors:
            print("  未发现任何舵机")
        
        return found_motors
    
    def read_byte(self, motor_id: int, address: int) -> Optional[int]:
        """
        读取1字节数据
        
        Args:
            motor_id: 舵机ID
            address: 寄存器地址
            
        Returns:
            Optional[int]: 读取的值，失败返回None
        """
        value, comm_result, error = self.packet_handler.read1ByteTxRx(
            self.port_handler, motor_id, address
        )
        
        if comm_result == self._comm_success and error == self._no_error:
            return value
        return None
    
    def read_word(self, motor_id: int, address: int) -> Optional[int]:
        """
        读取2字节数据
        
        Args:
            motor_id: 舵机ID
            address: 寄存器地址
            
        Returns:
            Optional[int]: 读取的值，失败返回None
        """
        value, comm_result, error = self.packet_handler.read2ByteTxRx(
            self.port_handler, motor_id, address
        )
        
        if comm_result == self._comm_success and error == self._no_error:
            return value
        return None
    
    def write_byte(self, motor_id: int, address: int, value: int) -> bool:
        """
        写入1字节数据
        
        Args:
            motor_id: 舵机ID
            address: 寄存器地址
            value: 要写入的值
            
        Returns:
            bool: 写入是否成功
        """
        comm_result, error = self.packet_handler.write1ByteTxRx(
            self.port_handler, motor_id, address, value
        )
        
        return comm_result == self._comm_success and error == self._no_error
    
    def write_word(self, motor_id: int, address: int, value: int) -> bool:
        """
        写入2字节数据
        
        Args:
            motor_id: 舵机ID
            address: 寄存器地址
            value: 要写入的值
            
        Returns:
            bool: 写入是否成功
        """
        comm_result, error = self.packet_handler.write2ByteTxRx(
            self.port_handler, motor_id, address, value
        )
        
        return comm_result == self._comm_success and error == self._no_error
    
    def read_control_table(self, motor_id: int, data_name: str) -> Optional[int]:
        """
        根据控制表名称读取数据
        
        Args:
            motor_id: 舵机ID
            data_name: 控制表中的数据名称
            
        Returns:
            Optional[int]: 读取的值
        """
        if data_name not in CONTROL_TABLE:
            print(f"✗ 未知的控制表项: {data_name}")
            return None
        
        address, size = CONTROL_TABLE[data_name]
        
        if size == 1:
            return self.read_byte(motor_id, address)
        elif size == 2:
            return self.read_word(motor_id, address)
        else:
            print(f"✗ 不支持的数据大小: {size}")
            return None
    
    def write_control_table(self, motor_id: int, data_name: str, value: int) -> bool:
        """
        根据控制表名称写入数据
        
        Args:
            motor_id: 舵机ID
            data_name: 控制表中的数据名称
            value: 要写入的值
            
        Returns:
            bool: 写入是否成功
        """
        if data_name not in CONTROL_TABLE:
            print(f"✗ 未知的控制表项: {data_name}")
            return False
        
        address, size = CONTROL_TABLE[data_name]
        
        if size == 1:
            return self.write_byte(motor_id, address, value)
        elif size == 2:
            return self.write_word(motor_id, address, value)
        else:
            print(f"✗ 不支持的数据大小: {size}")
            return False
    
    def enable_torque(self, motor_id: int) -> bool:
        """使能舵机扭矩"""
        return self.write_control_table(motor_id, "Torque_Enable", TorqueMode.ENABLED)
    
    def disable_torque(self, motor_id: int) -> bool:
        """关闭舵机扭矩"""
        return self.write_control_table(motor_id, "Torque_Enable", TorqueMode.DISABLED)
    
    def set_operating_mode(self, motor_id: int, mode: int) -> bool:
        """
        设置舵机操作模式
        
        Args:
            motor_id: 舵机ID
            mode: 操作模式（使用OperatingMode类）
        """
        # 需要先关闭扭矩才能修改操作模式
        self.disable_torque(motor_id)
        time.sleep(0.1)
        success = self.write_control_table(motor_id, "Operating_Mode", mode)
        time.sleep(0.1)
        return success
    
    def get_present_position(self, motor_id: int) -> Optional[int]:
        """获取当前位置"""
        return self.read_control_table(motor_id, "Present_Position")
    
    def set_goal_position(self, motor_id: int, position: int) -> bool:
        """设置目标位置"""
        return self.write_control_table(motor_id, "Goal_Position", position)
    
    def __enter__(self):
        """上下文管理器入口"""
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """上下文管理器退出"""
        self.disconnect()
