# -*- coding: utf-8 -*-
"""
舵机总线通信基础类
提供与STS3215舵机的底层通信功能
"""

import time
import logging
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

logger = logging.getLogger(__name__)


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
        
        # 创建GroupSyncRead和GroupSyncWrite用于批量读写（参考LeRobot）
        self.sync_reader = None
        self.sync_writer = None
        
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
    
    def read_byte(self, motor_id: int, address: int, num_retry: int = 2) -> Optional[int]:
        """
        读取1字节数据（带重试机制，参考LeRobot）
        
        Args:
            motor_id: 舵机ID
            address: 寄存器地址
            num_retry: 重试次数
            
        Returns:
            Optional[int]: 读取的值，失败返回None
        """
        for n_try in range(1 + num_retry):
            value, comm_result, error = self.packet_handler.read1ByteTxRx(
                self.port_handler, motor_id, address
            )
            
            if comm_result == self._comm_success and error == self._no_error:
                return value
            
            if n_try < num_retry:
                logger.debug(
                    f"读取失败 (motor_id={motor_id}, address={address}, try={n_try+1}/{num_retry+1}): "
                    f"{self.packet_handler.getTxRxResult(comm_result)}"
                )
        
        logger.warning(f"读取1字节失败 (motor_id={motor_id}, address={address}) 重试{num_retry}次后仍失败")
        return None
    
    def read_word(self, motor_id: int, address: int, num_retry: int = 2) -> Optional[int]:
        """
        读取2字节数据（带重试）
        
        Args:
            motor_id: 舵机ID
            address: 寄存器地址
            num_retry: 重试次数
            
        Returns:
            Optional[int]: 读取的值，失败返回None
        """
        for n_try in range(1 + num_retry):
            value, comm_result, error = self.packet_handler.read2ByteTxRx(
                self.port_handler, motor_id, address
            )
            
            if comm_result == self._comm_success and error == self._no_error:
                return value
            
            if n_try < num_retry:
                logger.debug(
                    f"读取失败 (motor_id={motor_id}, address={address}, try={n_try+1}/{num_retry+1}): "
                    f"{self.packet_handler.getTxRxResult(comm_result)}"
                )
        
        logger.warning(f"读取2字节失败 (motor_id={motor_id}, address={address}) 重试{num_retry}次后仍失败")
        return None
    
    def write_byte(self, motor_id: int, address: int, value: int, num_retry: int = 2) -> bool:
        """
        写入1字节数据（带重试机制，参考LeRobot）
        
        Args:
            motor_id: 舵机ID
            address: 寄存器地址
            value: 要写入的值
            num_retry: 重试次数
            
        Returns:
            bool: 写入是否成功
        """
        for n_try in range(1 + num_retry):
            comm_result, error = self.packet_handler.write1ByteTxRx(
                self.port_handler, motor_id, address, value
            )
            
            if comm_result == self._comm_success and error == self._no_error:
                return True
            
            if n_try < num_retry:
                logger.debug(
                    f"写入失败 (motor_id={motor_id}, address={address}, value={value}, try={n_try+1}/{num_retry+1}): "
                    f"{self.packet_handler.getTxRxResult(comm_result)}"
                )
        
        logger.warning(f"写入1字节失败 (motor_id={motor_id}, address={address}, value={value}) 重试{num_retry}次后仍失败")
        return False
    
    def write_word(self, motor_id: int, address: int, value: int, num_retry: int = 2) -> bool:
        """
        写入2字节数据（带重试）
        
        注意：如果value是负数，需要转换为无符号16位整数
        
        Args:
            motor_id: 舵机ID
            address: 寄存器地址
            value: 要写入的值（有符号整数）
            num_retry: 重试次数
            
        Returns:
            bool: 写入是否成功
        """
        # 转换有符号整数到无符号16位
        # 如果是负数，转换为 65536 + value
        if value < 0:
            value = 65536 + value
        
        # 确保在0-65535范围内
        value = max(0, min(65535, value))
        
        for n_try in range(1 + num_retry):
            comm_result, error = self.packet_handler.write2ByteTxRx(
                self.port_handler, motor_id, address, value
            )
            
            if comm_result == self._comm_success and error == self._no_error:
                return True
            
            if n_try < num_retry:
                logger.debug(
                    f"写入失败 (motor_id={motor_id}, address={address}, value={value}, try={n_try+1}/{num_retry+1}): "
                    f"{self.packet_handler.getTxRxResult(comm_result)}"
                )
        
        logger.warning(f"写入2字节失败 (motor_id={motor_id}, address={address}, value={value}) 重试{num_retry}次后仍失败")
        return False
    
    def read_control_table(self, motor_id: int, data_name: str, num_retry: int = 2) -> Optional[int]:
        """
        根据控制表名称读取数据
        
        注意：某些寄存器（如Homing_Offset）是有符号数，需要特殊处理
        
        Args:
            motor_id: 舵机ID
            data_name: 控制表中的数据名称
            num_retry: 重试次数
            
        Returns:
            Optional[int]: 读取的值
        """
        if data_name not in CONTROL_TABLE:
            logger.error(f"未知的控制表项: {data_name}")
            return None
        
        address, size = CONTROL_TABLE[data_name]
        
        if size == 1:
            return self.read_byte(motor_id, address, num_retry=num_retry)
        elif size == 2:
            value = self.read_word(motor_id, address, num_retry=num_retry)
            if value is None:
                return None
            
            # 有符号寄存器：需要转换无符号16位到有符号16位
            # Present_Position, Goal_Position也是有符号的（支持负值位置）
            signed_registers = ["Present_Position", "Goal_Position", "Homing_Offset", "Present_Load", "Present_Current"]
            if data_name in signed_registers and value > 32767:
                value = value - 65536
            
            return value
        else:
            logger.error(f"不支持的数据大小: {size}")
            return None
    
    def write_control_table(self, motor_id: int, data_name: str, value: int, num_retry: int = 2) -> bool:
        """
        根据控制表名称写入数据
        
        Args:
            motor_id: 舵机ID
            data_name: 控制表中的数据名称
            value: 要写入的值
            num_retry: 重试次数
            
        Returns:
            bool: 写入是否成功
        """
        if data_name not in CONTROL_TABLE:
            logger.error(f"未知的控制表项: {data_name}")
            return False
        
        address, size = CONTROL_TABLE[data_name]
        
        if size == 1:
            return self.write_byte(motor_id, address, value, num_retry=num_retry)
        elif size == 2:
            return self.write_word(motor_id, address, value, num_retry=num_retry)
        else:
            logger.error(f"不支持的数据大小: {size}")
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
    
    def sync_read(self, data_name: str, motor_names: Optional[List[str]] = None, num_retry: int = 2) -> Dict[str, int]:
        """
        同步读取多个舵机的同一寄存器（参考LeRobot实现）
        使用GroupSyncRead批量读取，提高效率和可靠性
        
        Args:
            data_name: 寄存器名称（如"Present_Position"）
            motor_names: 舵机名称列表，None表示读取所有舵机
            num_retry: 重试次数
            
        Returns:
            Dict[str, int]: {舵机名称: 读取值}
        """
        from config.motor_config import MOTOR_IDS
        
        if motor_names is None:
            motor_names = list(MOTOR_IDS.keys())
        
        if not motor_names:
            return {}
        
        # 获取地址和长度
        if data_name not in CONTROL_TABLE:
            logger.error(f"未知的控制表项: {data_name}")
            return {}
        
        address, size = CONTROL_TABLE[data_name]
        
        # 准备ID列表
        motor_ids = []
        id_to_name = {}
        for motor_name in motor_names:
            if motor_name in MOTOR_IDS:
                motor_id = MOTOR_IDS[motor_name]
                motor_ids.append(motor_id)
                id_to_name[motor_id] = motor_name
        
        if not motor_ids:
            return {}
        
        # 使用GroupSyncRead批量读取
        ids_values = self._sync_read_raw(address, size, motor_ids, num_retry=num_retry)
        
        # 转换为名称映射
        result = {}
        for motor_id, value in ids_values.items():
            if motor_id in id_to_name:
                result[id_to_name[motor_id]] = value
        
        # 检查失败的舵机
        failed_motors = [name for name in motor_names if name not in result]
        if failed_motors:
            logger.warning(f"sync_read失败的舵机: {failed_motors}")
        
        return result
    
    def _sync_read_raw(self, addr: int, length: int, motor_ids: List[int], num_retry: int = 2) -> Dict[int, int]:
        """
        使用GroupSyncRead批量读取（参考LeRobot实现）
        
        Args:
            addr: 寄存器地址
            length: 数据长度
            motor_ids: 舵机ID列表
            num_retry: 重试次数
            
        Returns:
            Dict[int, int]: {舵机ID: 值}
        """
        # 创建或重用sync_reader
        if self.sync_reader is None:
            self.sync_reader = scs.GroupSyncRead(self.port_handler, self.packet_handler, addr, length)
        
        # 设置参数
        self.sync_reader.clearParam()
        self.sync_reader.start_address = addr
        self.sync_reader.data_length = length
        
        # 添加所有舵机ID
        for motor_id in motor_ids:
            success = self.sync_reader.addParam(motor_id)
            if not success:
                logger.warning(f"sync_reader.addParam失败 (motor_id={motor_id})")
        
        # 重试读取
        comm_result = None
        for n_try in range(1 + num_retry):
            comm_result = self.sync_reader.txRxPacket()  # 批量读取
            
            if comm_result == self._comm_success:
                break
            
            if n_try < num_retry:
                logger.debug(
                    f"sync_read失败 (address={addr}, try={n_try+1}/{num_retry+1}): "
                    f"{self.packet_handler.getTxRxResult(comm_result)}"
                )
                time.sleep(0.002)  # 2ms延迟后重试（舵机可能在运动）
        
        # 提取数据
        result = {}
        for motor_id in motor_ids:
            # 检查是否有数据可用
            available = self.sync_reader.isAvailable(motor_id, addr, length)
            if available:
                value = self.sync_reader.getData(motor_id, addr, length)
                result[motor_id] = value
            else:
                logger.debug(f"sync_reader数据不可用 (motor_id={motor_id})")
        
        if comm_result != self._comm_success:
            logger.warning(
                f"sync_read通信失败 (address={addr}) 重试{num_retry}次后仍失败: "
                f"{self.packet_handler.getTxRxResult(comm_result)}"
            )
        
        return result
    
    def sync_write(self, data_name: str, values: Dict[str, int], num_retry: int = 2) -> bool:
        """
        同步写入多个舵机的同一寄存器（参考LeRobot实现）
        使用GroupSyncWrite批量发送，不等待响应
        
        Args:
            data_name: 寄存器名称（如"Goal_Position"）
            values: {舵机名称: 写入值}
            num_retry: 重试次数
            
        Returns:
            bool: 是否全部写入成功
        """
        from config.motor_config import MOTOR_IDS
        
        if not values:
            return True
        
        # 获取地址和长度
        if data_name not in CONTROL_TABLE:
            logger.error(f"未知的控制表项: {data_name}")
            return False
        
        address, size = CONTROL_TABLE[data_name]
        
        # 准备ID和值的映射
        ids_values = {}
        for motor_name, value in values.items():
            if motor_name in MOTOR_IDS:
                ids_values[MOTOR_IDS[motor_name]] = value
        
        if not ids_values:
            return True
        
        # 使用GroupSyncWrite批量写入
        return self._sync_write_raw(address, size, ids_values, num_retry=num_retry)
    
    def _sync_write_raw(self, addr: int, length: int, ids_values: Dict[int, int], num_retry: int = 2) -> bool:
        """
        使用GroupSyncWrite批量写入（参考LeRobot实现）
        
        Args:
            addr: 寄存器地址
            length: 数据长度
            ids_values: {舵机ID: 值}
            num_retry: 重试次数
        """
        # 创建或重用sync_writer
        if self.sync_writer is None:
            self.sync_writer = scs.GroupSyncWrite(self.port_handler, self.packet_handler, addr, length)
        
        # 设置参数
        self.sync_writer.clearParam()
        self.sync_writer.start_address = addr
        self.sync_writer.data_length = length
        
        # 添加所有舵机数据
        for motor_id, value in ids_values.items():
            # 转换为字节数组
            if length == 1:
                data = [scs.SCS_LOBYTE(value)]
            elif length == 2:
                data = [scs.SCS_LOBYTE(value), scs.SCS_HIBYTE(value)]
            else:
                logger.error(f"不支持的数据长度: {length}")
                return False
            
            success = self.sync_writer.addParam(motor_id, data)
            if not success:
                logger.warning(f"addParam失败 (motor_id={motor_id})")
        
        # 重试发送
        for n_try in range(1 + num_retry):
            comm_result = self.sync_writer.txPacket()  # 只发送，不等待响应
            
            if comm_result == self._comm_success:
                return True
            
            if n_try < num_retry:
                logger.debug(
                    f"sync_write失败 (address={addr}, try={n_try+1}/{num_retry+1}): "
                    f"{self.packet_handler.getTxRxResult(comm_result)}"
                )
                time.sleep(0.001)  # 短暂延迟后重试
        
        logger.warning(f"sync_write失败 (address={addr}) 重试{num_retry}次后仍失败")
        return False
    
    def torque_disabled(self):
        """
        上下文管理器：在代码块中禁用扭矩，退出时恢复
        
        用法：
            with bus.torque_disabled():
                # 配置舵机参数
                pass
        """
        from contextlib import contextmanager
        from config.motor_config import MOTOR_IDS
        
        @contextmanager
        def _torque_disabled():
            # 记录原始扭矩状态
            original_states = {}
            for motor_id in MOTOR_IDS.values():
                state = self.read_control_table(motor_id, "Torque_Enable")
                original_states[motor_id] = state
                # 禁用扭矩
                self.disable_torque(motor_id)
            
            try:
                yield
            finally:
                # 恢复原始状态
                for motor_id, state in original_states.items():
                    if state == 1:
                        self.enable_torque(motor_id)
        
        return _torque_disabled()
    
    def __enter__(self):
        """上下文管理器入口"""
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """上下文管理器退出"""
        self.disconnect()
    
    def __del__(self):
        """析构函数，确保串口关闭"""
        if hasattr(self, 'is_connected') and self.is_connected:
            self.disconnect()
