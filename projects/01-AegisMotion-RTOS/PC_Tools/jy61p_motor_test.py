#!/usr/bin/env python3
"""JY61P 数据显示与 Emm_V5 多电机控制测试工具。

STM32 通过同一个板载 CH340 串口完成两件事：
1. 周期输出 JY61P 的加速度、角速度、YPR、温度和版本号文本；
2. 接收本程序发送的带 CRC16 二进制帧，并控制 USART6 上的 Emm_V5 电机。

运行前安装依赖：python -m pip install pyserial

源码顺序：协议常量/编码 -> 自检 -> 串口工作线程 -> Tk界面及命令回调。
只有Tk主线程操作控件；后台线程传递完整帧或最新IMU快照。
本工具用于人工架空调试，不是自动驾驶主控；操作步骤见 PC_Tools/README.md。
"""

from __future__ import annotations

import queue
import re
import struct
import threading
import time
import tkinter as tk
from collections import deque
from functools import partial
from tkinter import messagebox, scrolledtext, ttk
from typing import Optional

try:
    import serial
    from serial.tools import list_ports
except ImportError as exc:  # pragma: no cover - 仅在缺少运行依赖时触发。
    raise SystemExit("缺少 pyserial，请先执行：python -m pip install pyserial") from exc


# STM32 输出的完整传感器文本示例：
# ACC[g]=0.001,0.002,0.998;GYRO[dps]=0.000,0.061,0.000;
# YPR[deg]=1.230,-0.450,0.100;TEMP[C]=26.510;VER=123
NUMBER_PATTERN = r"[-+]?\d+(?:\.\d+)?"
IMU_LINE_PATTERN = re.compile(
    rf"^ACC\[g\]=({NUMBER_PATTERN}),({NUMBER_PATTERN}),({NUMBER_PATTERN});"
    rf"GYRO\[dps\]=({NUMBER_PATTERN}),({NUMBER_PATTERN}),({NUMBER_PATTERN});"
    rf"YPR\[deg\]=({NUMBER_PATTERN}),({NUMBER_PATTERN}),({NUMBER_PATTERN});"
    rf"TEMP\[C\]=({NUMBER_PATTERN});VER=(\d+)$"
)

FRAME_SYNC = 0x5A
FRAME_MIN_SIZE = 7
DEST_HOST = 0x00
DEST_CONTROLLER = 0x01
MOTOR_MAX_RPM = 100
HEARTBEAT_INTERVAL_MS = 200
# 以下调度和容量参数限制单次GUI工作量，不改变STM32侧保护期限。
GUI_POLL_INTERVAL_MS = 20
GUI_MAX_EVENTS_PER_TICK = 64
GUI_EVENT_BUDGET_SECONDS = 0.005
GUI_LOG_MAX_LINES = 500
GUI_LOG_MAX_ENTRY_CHARS = 1024
SERIAL_EVENT_QUEUE_SIZE = 256
SELF_TEST_MOTOR_ADDRESSES = (1, 2, 3, 4)
SELF_TEST_CONTROLLER_TIMEOUT_SECONDS = 6.0
SELF_TEST_IMU_TIMEOUT_SECONDS = 2.0
SELF_TEST_PROBE_INTERVAL_SECONDS = 0.5
SELF_TEST_MOTOR_ATTEMPTS = 2
DEVICE_NAMES = {"controller": "下位机", "imu": "IMU", **{
    f"motor_{address}": f"{address}号电机" for address in SELF_TEST_MOTOR_ADDRESSES
}}
STATUS_PENDING_COLOR = "#b45309"
STATUS_ONLINE_COLOR = "#15803d"
STATUS_ERROR_COLOR = "#dc2626"
STATUS_IDLE_COLOR = "#64748b"

COMMAND_PING = 0x01
COMMAND_STATUS = 0x02
COMMAND_SELF_TEST = 0x03
COMMAND_RENEW = 0x04
COMMAND_ENABLE = 0x10
COMMAND_VELOCITY = 0x11
COMMAND_POSITION = 0x12
COMMAND_STOP = 0x13
COMMAND_EMERGENCY_STOP = 0x14
COMMAND_SYNC = 0x15
COMMAND_ZERO = 0x16
COMMAND_READ = 0x17
COMMAND_HOME = 0x18
COMMAND_CHASSIS = 0x20

# 发送入队和后台实际写入使用相同的急停门禁，包含使能（含失能）和续期。
# 保留现有行为：急停期间仅查询/停止/重新自检等非运动请求可发。
MOTION_GATED_COMMANDS = (
    COMMAND_ENABLE, COMMAND_VELOCITY, COMMAND_POSITION,
    COMMAND_HOME, COMMAND_SYNC, COMMAND_CHASSIS, COMMAND_RENEW,
)

EVENT_READY = 0xE0
EVENT_IMU_TEXT = 0xE1
EVENT_MOTOR_REPLY = 0xE2
EVENT_SELF_TEST = 0xE3
EVENT_CHASSIS_RESULT = 0xE4

ACK_NAMES = {
    0x00: "成功",
    0x01: "长度错误",
    0x02: "CRC错误",
    0x03: "不支持的命令",
    0x04: "参数越界",
    0x05: "系统忙",
    0x06: "电机通信失败",
    0x07: "下位机自检未通过或故障锁定，禁止运动",
}

READ_ITEM_VALUES = {
    "VER": 0,
    "VBUS": 3,
    "CURRENT": 5,
    "SPEED": 9,
    "POSITION": 10,
    "PERR": 11,
    "FLAG": 13,
    "STATE": 15,
    "ORG": 16,
}


def crc16_modbus(data: bytes | bytearray) -> int:
    """计算 CRC16-Modbus：初值 0xFFFF，多项式 0xA001。"""

    crc = 0xFFFF
    for value in data:
        crc ^= value
        for _ in range(8):
            crc = ((crc >> 1) ^ 0xA001) if (crc & 1) else (crc >> 1)
    return crc & 0xFFFF


def build_frame(destination: int, sequence: int, index: int, data: bytes = b"") -> bytes:
    """构造 5A|LEN|DEST|SEQ|INDEX|DATA|CRC16_L|CRC16_H 帧。"""

    if len(data) > 248:
        raise ValueError("帧载荷不能超过 248 字节")
    frame = bytearray((FRAME_SYNC, len(data) + FRAME_MIN_SIZE, destination, sequence, index))
    frame.extend(data)
    crc = crc16_modbus(frame[1:])
    frame.extend((crc & 0xFF, crc >> 8))
    return bytes(frame)


def format_hex(data: bytes | bytearray) -> str:
    """把二进制数据格式化为便于对照的十六进制文本。"""

    return " ".join(f"{value:02X}" for value in data)


class ConnectionSelfTest:
    """新固件只查询STM32自检结果；旧固件才执行逐台只读FLAG探测。

    时间由GUI传入 monotonic() 秒值；active仅表示本次连接检查尚未结束。
    在线不等于已使能/到位，且不能替代STM32自身的运动门禁。
    """

    def __init__(self) -> None:
        self.active = False
        self.controller_online = False
        self.imu_online = False
        self.motor_online: dict[int, Optional[bool]] = {
            address: None for address in SELF_TEST_MOTOR_ADDRESSES
        }
        self._motor_index = 0
        self._motor_attempts = 0
        self._waiting_motor = False
        self._next_probe_at = 0.0
        self._controller_deadline = 0.0
        self._imu_deadline = 0.0
        self.firmware_state: Optional[int] = None
        self._firmware_deadline = 0.0

    def observe_firmware(self, state: int, motor_mask: int, imu: int, now: float) -> None:
        if not self.active:
            return
        self.observe_controller(now)
        if self.firmware_state is None:
            self._firmware_deadline = now + SELF_TEST_CONTROLLER_TIMEOUT_SECONDS
        self.firmware_state = state
        self.imu_online = imu in (1, 2)
        for bit, address in enumerate((2, 1, 3, 4)):
            self.motor_online[address] = True if motor_mask & (1 << bit) else (
                None if state == 1 else False
            )
        if state in (2, 3):
            self.active = False

    @property
    def current_motor(self) -> Optional[int]:
        if self._motor_index < len(SELF_TEST_MOTOR_ADDRESSES):
            return SELF_TEST_MOTOR_ADDRESSES[self._motor_index]
        return None

    def start(self, now: float) -> None:
        self.__init__()
        self.active = True
        self._next_probe_at = now
        self._controller_deadline = now + SELF_TEST_CONTROLLER_TIMEOUT_SECONDS

    def observe_controller(self, now: float) -> None:
        if self.active and not self.controller_online:
            self.controller_online = True
            self._imu_deadline = now + SELF_TEST_IMU_TIMEOUT_SECONDS
            self._next_probe_at = now

    def observe_imu(self) -> None:
        if self.active:
            self.imu_online = True

    def observe_motor(self, payload: bytes, now: float) -> None:
        # READ 的成功 ACK 仅表示命令入队；必须收到该地址的完整 FLAG 回复。
        if (self.active and self.controller_online and self._waiting_motor
                and len(payload) == 4 and payload[0] == self.current_motor
                and payload[1] == 0x3A and payload[-1] == 0x6B):
            self.motor_online[payload[0]] = True
            self._motor_index += 1
            self._motor_attempts = 0
            self._waiting_motor = False
            self._next_probe_at = now + 0.05

    def tick(self, now: float) -> Optional[tuple[int, bytes, str]]:
        if not self.active:
            return None
        if self.firmware_state is not None:
            if now >= self._firmware_deadline:
                self.active = False
            elif now >= self._next_probe_at:
                self._next_probe_at = now + SELF_TEST_PROBE_INTERVAL_SECONDS
                return COMMAND_STATUS, b"", "自检：等待 STM32 自检结果"
            return None
        if not self.controller_online:
            if now >= self._controller_deadline:
                self.active = False
                return None
            if now >= self._next_probe_at:
                self._next_probe_at = now + SELF_TEST_PROBE_INTERVAL_SECONDS
                return COMMAND_STATUS, b"", "自检：查询下位机状态"
            return None

        address = self.current_motor
        if address is not None:
            if now < self._next_probe_at:
                return None
            if self._motor_attempts >= SELF_TEST_MOTOR_ATTEMPTS:
                self.motor_online[address] = False
                self._motor_index += 1
                self._motor_attempts = 0
                self._waiting_motor = False
                address = self.current_motor
            if address is not None:
                self._waiting_motor = True
                self._motor_attempts += 1
                self._next_probe_at = now + SELF_TEST_PROBE_INTERVAL_SECONDS
                return COMMAND_READ, bytes((address, READ_ITEM_VALUES["FLAG"])), f"自检：{address}号电机 FLAG"

        if self.imu_online or now >= self._imu_deadline:
            self.active = False
        return None


class SerialWorker:
    """串口读写在后台，GUI只负责入队和取事件，不等待串口发送完成。

    _write_lock保护发送序号/队列/急停恢复状态，不能持锁执行serial.write。
    _event_lock保护IMU和错误邮箱；普通ACK保持FIFO，快照只保留最新值。
    """

    def __init__(self) -> None:
        self.events: "queue.Queue[tuple[str, object]]" = queue.Queue(maxsize=SERIAL_EVENT_QUEUE_SIZE)
        self._event_lock = threading.Lock()
        self._latest_imu_frame: Optional[bytes] = None
        self._imu_frame_count = 0
        self._protocol_error_message = ""
        self._protocol_error_count = 0
        self._serial: Optional[serial.Serial] = None
        self._reader_thread: Optional[threading.Thread] = None
        self._writer_thread: Optional[threading.Thread] = None
        self._stop_event = threading.Event()
        self._write_lock = threading.Lock()
        self._tx_queue: "queue.Queue[bytes]" = queue.Queue(maxsize=128)
        self._next_sequence = 0
        self._urgent_frame: Optional[bytes] = None
        self._motion_blocked = False
        # 0=未请求恢复；1=已发SELF_TEST待RUNNING；2=已观察RUNNING待PASSED。
        # 合并原两个布尔量，保留“旧PASSED不能解除本地急停”的先后顺序约束。
        self._recovery_state = 0
        # 以完整RENEW帧为键，后台写入时检查期限，不重发运动/相对位置动作。
        self._renew_expiry: dict[bytes, float] = {}

    @property
    def connected(self) -> bool:
        """串口对象存在且保持打开时返回 True。"""

        return self._serial is not None and self._serial.is_open

    def connect(self, port: str, baudrate: int = 115200) -> None:
        """打开串口并启动后台接收线程。"""

        self.disconnect()
        # 先创建未打开的串口对象，再关闭 DTR/RTS 后打开端口。
        # 部分带 CH340 的 STM32 开发板会把 DTR/RTS 接入复位电路；如果沿用
        # pyserial 的默认有效电平，可能出现“软件显示已连接，但单片机一直复位”的现象。
        serial_port = serial.Serial(
            port=None,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.05,
            write_timeout=0.1,
        )
        serial_port.port = port
        serial_port.dtr = False
        serial_port.rts = False
        serial_port.open()

        # 打开后再次明确关闭硬件握手线，兼容部分 Windows CH340 驱动。
        serial_port.dtr = False
        serial_port.rts = False
        self._serial = serial_port
        self._next_sequence = 0
        self._motion_blocked = False
        self._recovery_state = 0
        self._urgent_frame = None
        self._clear_tx_queue()

        # 重连时不把旧连接的样本、应答和错误显示到新连接中。
        self._clear_received_events()

        # 给可能发生过瞬时复位的 STM32 一点恢复时间，然后仅清除发送缓存。
        # 不清空接收缓存，避免丢掉 STM32 刚启动后输出的 READY/IMU 文本。
        time.sleep(0.15)
        serial_port.reset_output_buffer()
        self._stop_event.clear()
        self._reader_thread = threading.Thread(target=self._read_loop, daemon=True)
        self._writer_thread = threading.Thread(target=self._write_loop, daemon=True)
        self._reader_thread.start()
        self._writer_thread.start()

    def _clear_received_events(self) -> None:
        while True:
            try:
                self.events.get_nowait()
            except queue.Empty:
                break
        with self._event_lock:
            self._latest_imu_frame = None
            self._imu_frame_count = 0
            self._protocol_error_message = ""
            self._protocol_error_count = 0

    def _post_event(self, event_type: str, payload: object) -> None:
        """合并高频快照和协议错误；普通应答按 FIFO 保留。"""

        if event_type == "frame" and isinstance(payload, bytes):
            if payload[2] in (DEST_HOST, 0xFF) and payload[4] == EVENT_IMU_TEXT:
                with self._event_lock:
                    self._latest_imu_frame = payload
                    self._imu_frame_count += 1
                return
        if event_type == "protocol_error":
            with self._event_lock:
                self._protocol_error_message = str(payload)[-GUI_LOG_MAX_ENTRY_CHARS:]
                self._protocol_error_count += 1
            return

        # 队列满时只让后台线程等待，关闭连接时可以及时退出。
        while not self._stop_event.is_set():
            try:
                self.events.put((event_type, payload), timeout=0.05)
                return
            except queue.Full:
                continue

    def take_latest_imu(self) -> Optional[tuple[bytes, int]]:
        """一次取走最新 IMU 帧和累计收到的样本数。"""

        with self._event_lock:
            if self._latest_imu_frame is None:
                return None
            result = (self._latest_imu_frame, self._imu_frame_count)
            self._latest_imu_frame = None
            self._imu_frame_count = 0
            return result

    def take_protocol_errors(self) -> tuple[str, int]:
        with self._event_lock:
            result = (self._protocol_error_message, self._protocol_error_count)
            self._protocol_error_message = ""
            self._protocol_error_count = 0
            return result

    def disconnect(self) -> None:
        """停止接收线程并关闭串口。"""

        self._stop_event.set()
        serial_port = self._serial
        self._serial = None
        if serial_port is not None:
            try:
                serial_port.close()
            except serial.SerialException:
                pass

        reader_thread = self._reader_thread
        self._reader_thread = None
        writer_thread = self._writer_thread
        self._writer_thread = None
        if reader_thread is not None and reader_thread is not threading.current_thread():
            reader_thread.join(timeout=0.5)
        if writer_thread is not None and writer_thread is not threading.current_thread():
            writer_thread.join(timeout=0.5)
        self._clear_tx_queue()
        self._clear_received_events()

    def send_frame(self, index: int, data: bytes = b"") -> tuple[int, bytes]:
        """分配SEQ并入队，返回(sequence, frame)；这不是发送完成确认。

        ESTOP进入优先槽并清空旧帧；STOP/SELF_TEST清空普通待发队列。
        新RENEW在250ms内未发送则丢弃，避免GUI停顿后旧缓存继续保活。
        """

        serial_port = self._serial
        if serial_port is None or not serial_port.is_open:
            raise serial.SerialException("串口尚未连接")

        with self._write_lock:
            self._next_sequence = (self._next_sequence + 1) & 0xFF
            frame = build_frame(DEST_CONTROLLER, self._next_sequence, index, data)
            if index == COMMAND_EMERGENCY_STOP:
                self._motion_blocked = True
                self._recovery_state = 0
                self._clear_tx_queue()
                self._urgent_frame = frame
                return self._next_sequence, frame
            if self._motion_blocked and index in MOTION_GATED_COMMANDS:
                raise serial.SerialException("急停已锁定，请等待重新自检通过")
            if index in (COMMAND_SELF_TEST, COMMAND_STOP):
                self._clear_tx_queue()
                if index == COMMAND_SELF_TEST:
                    self._recovery_state = 1
            try:
                self._tx_queue.put_nowait(frame)
                if index == COMMAND_RENEW:
                    self._renew_expiry[frame] = time.monotonic() + 0.25
            except queue.Full as exc:
                raise serial.SerialException("发送队列已满（最多128帧）") from exc
        return self._next_sequence, frame

    def _write_loop(self) -> None:
        """急停槽优先，其余完整帧FIFO；锁外写串口，一帧写完后才取下一帧。"""

        try:
            while not self._stop_event.is_set():
                queued = False
                with self._write_lock:
                    frame = self._urgent_frame
                    self._urgent_frame = None
                    if frame is None:
                        try:
                            frame = self._tx_queue.get_nowait()
                            queued = True
                        except queue.Empty:
                            pass
                    expiry = self._renew_expiry.pop(frame, None)
                if frame is None:
                    self._stop_event.wait(0.01)
                    continue

                serial_port = self._serial
                if serial_port is None or not serial_port.is_open:
                    if queued:
                        self._tx_queue.task_done()
                    break
                try:
                    if expiry is not None and time.monotonic() >= expiry:
                        continue
                    # 已开始写的单帧允许写完，随后优先急停；不再 flush 无限等待驱动。
                    if self._motion_blocked and frame[4] in MOTION_GATED_COMMANDS:
                        continue
                    if serial_port.write(frame) != len(frame):
                        raise serial.SerialException("串口帧未完整发送，已停止发送线程")
                finally:
                    if queued:
                        self._tx_queue.task_done()
                self._post_event("sent", frame)
        except (serial.SerialException, OSError) as exc:
            if not self._stop_event.is_set():
                self._post_event("error", str(exc))

    def _clear_tx_queue(self) -> None:
        """连接切换时清空未发送旧帧，防止重连后误动作。"""

        while True:
            try:
                self._tx_queue.get_nowait()
                self._tx_queue.task_done()
            except queue.Empty:
                self._renew_expiry.clear()
                return

    def observe_firmware_safety(self, state: int) -> None:
        """按新固件自检阶段推进本地恢复；PASSED须由GUI先确认无急停/待停车。

        发送SELF_TEST本身不解锁，必须按序观察RUNNING(1)和PASSED(2)。
        此门禁只是PC侧防误发送，STM32侧的独立故障锁仍是最终执行门禁。
        """
        with self._write_lock:
            if state == 1 and self._recovery_state != 0:
                self._recovery_state = 2
            elif state == 2 and (not self._motion_blocked or self._recovery_state == 2):
                self._motion_blocked = False
                self._recovery_state = 0

    def _read_loop(self) -> None:
        """从任意分包的串口字节流中提取完整二进制帧。"""

        receive_buffer = bytearray()
        try:
            while not self._stop_event.is_set():
                serial_port = self._serial
                if serial_port is None or not serial_port.is_open:
                    break

                # 优先读取驱动中已经缓存的全部字节；没有缓存时至少等待 1 字节。
                raw_data = serial_port.read(max(1, serial_port.in_waiting))
                if not raw_data:
                    continue
                receive_buffer.extend(raw_data)

                self._extract_frames(receive_buffer)
        except (serial.SerialException, OSError) as exc:
            if not self._stop_event.is_set():
                self._post_event("error", str(exc))

    def _extract_frames(self, receive_buffer: bytearray) -> None:
        """处理粘包、拆包、异常长度和 CRC 错误后的重新同步。"""

        while receive_buffer:
            try:
                sync_position = receive_buffer.index(FRAME_SYNC)
            except ValueError:
                self._post_event("protocol_error", f"丢弃非帧数据：{format_hex(receive_buffer[:64])}")
                receive_buffer.clear()
                return

            if sync_position:
                noise = bytes(receive_buffer[:sync_position])
                del receive_buffer[:sync_position]
                self._post_event("protocol_error", f"丢弃非帧数据：{format_hex(noise[:64])}")
            if len(receive_buffer) < 2:
                return

            frame_length = receive_buffer[1]
            if frame_length < FRAME_MIN_SIZE:
                self._post_event("protocol_error", f"非法帧长度：{frame_length}")
                del receive_buffer[0]
                continue
            if len(receive_buffer) < frame_length:
                return

            frame = bytes(receive_buffer[:frame_length])
            received_crc = frame[-2] | (frame[-1] << 8)
            calculated_crc = crc16_modbus(frame[1:-2])
            if received_crc != calculated_crc:
                self._post_event(
                    "protocol_error",
                    f"CRC错误：RX={received_crc:04X} CALC={calculated_crc:04X} "
                    f"FRAME={format_hex(frame)}",
                )
                del receive_buffer[0]
                continue

            del receive_buffer[:frame_length]
            self._post_event("frame", frame)


class TestApplication:
    """Tk主线程拥有所有界面、待应答请求、自检显示和动作授权意图。

    after()限制每轮消费64事件/5ms，日志批量刷新、最多500行。
    _motion_sequence只有匹配成功ACK后才赋值；PING不建立运动授权。
    """

    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.worker = SerialWorker()
        self._pending_log_lines: deque[str] = deque(maxlen=GUI_LOG_MAX_LINES)
        self._self_test = ConnectionSelfTest()
        self._firmware_safety: Optional[tuple[int, ...]] = None
        self._motion_sequence: Optional[int] = None
        self._motion_requests: set[int] = set()
        self._lease_supported = False
        self.continuous_motion_var = tk.BooleanVar(value=False)
        self._self_test_reported = True
        self._self_test_sequences: set[int] = set()
        self.device_status_vars = {
            key: tk.StringVar(value=f"{name}：未连接") for key, name in DEVICE_NAMES.items()
        }
        self.device_status_labels: dict[str, ttk.Label] = {}
        self.self_test_summary_var = tk.StringVar(value="自检：等待连接")

        self.root.title("STM32F407 · JY61P + Emm_V5 测试工具")
        self.root.geometry("1050x900")
        self.root.minsize(900, 800)
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

        # 串口连接相关变量。
        self.port_var = tk.StringVar()
        self.connection_var = tk.StringVar(value="未连接")
        self.show_imu_log_var = tk.BooleanVar(value=False)

        # 传感器数据显示变量。
        self.sensor_vars = {
            "ax": tk.StringVar(value="--"),
            "ay": tk.StringVar(value="--"),
            "az": tk.StringVar(value="--"),
            "gx": tk.StringVar(value="--"),
            "gy": tk.StringVar(value="--"),
            "gz": tk.StringVar(value="--"),
            "yaw": tk.StringVar(value="--"),
            "pitch": tk.StringVar(value="--"),
            "roll": tk.StringVar(value="--"),
            "temp": tk.StringVar(value="--"),
            "version": tk.StringVar(value="--"),
            "count": tk.StringVar(value="0"),
            "yaw_lock": tk.StringVar(value="等待静止"),
        }
        self.sample_count = 0
        self.pending_requests: dict[int, tuple[int, str]] = {}

        # 电机控制参数。地址可在每次命令前修改，从而控制多台电机。
        self.motor_address_var = tk.StringVar(value="1")
        self.motor_direction_var = tk.StringVar(value="0 - CW")
        self.motor_speed_var = tk.StringVar(value="100")
        self.motor_acceleration_var = tk.StringVar(value="10")
        self.motor_pulses_var = tk.StringVar(value="3200")
        self.motor_absolute_var = tk.BooleanVar(value=False)
        self.motor_sync_var = tk.BooleanVar(value=False)
        self.read_item_var = tk.StringVar(value="FLAG")
        self.home_mode_var = tk.StringVar(value="0 - 单圈就近")

        # 麦克纳姆底盘分量：前进、左移、逆时针均为正，范围 -1000~1000。
        self.chassis_forward_var = tk.StringVar(value="0")
        self.chassis_left_var = tk.StringVar(value="0")
        self.chassis_ccw_var = tk.StringVar(value="0")
        self.chassis_speed_var = tk.StringVar(value="100")
        self.chassis_acceleration_var = tk.StringVar(value="10")

        self._build_interface()
        self._refresh_ports()
        self._set_motor_controls_enabled(False)
        self._append_log("提示：先烧录最新固件，再选择板载 CH340 对应的 COM 口。")
        self.root.after(GUI_POLL_INTERVAL_MS, self._poll_serial_events)
        self.root.after(HEARTBEAT_INTERVAL_MS, self._send_heartbeat)

    def _build_interface(self) -> None:
        """创建串口、传感器、电机和日志四个界面区域。"""

        main = ttk.Frame(self.root, padding=10)
        main.pack(fill=tk.BOTH, expand=True)
        main.columnconfigure(0, weight=1)
        main.columnconfigure(1, weight=1)
        main.rowconfigure(3, weight=1)

        self._build_connection_frame(main)
        self._build_sensor_frame(main)
        self._build_motor_frame(main)
        self._build_chassis_frame(main)
        self._build_log_frame(main)

    def _build_connection_frame(self, parent: ttk.Frame) -> None:
        """创建串口选择和连接按钮。"""

        frame = ttk.LabelFrame(parent, text="串口连接", padding=8)
        frame.grid(row=0, column=0, columnspan=2, sticky="ew", pady=(0, 8))
        frame.columnconfigure(1, weight=1)

        ttk.Label(frame, text="COM口：").grid(row=0, column=0, padx=(0, 4))
        self.port_combo = ttk.Combobox(frame, textvariable=self.port_var, state="readonly", width=45)
        self.port_combo.grid(row=0, column=1, sticky="ew")
        ttk.Button(frame, text="刷新", command=self._refresh_ports).grid(row=0, column=2, padx=5)

        self.connect_button = ttk.Button(frame, text="连接", command=self._toggle_connection)
        self.connect_button.grid(row=0, column=3, padx=5)
        ttk.Label(frame, textvariable=self.connection_var).grid(row=0, column=4, padx=(8, 0))
        ttk.Label(frame, text="115200 / 8N1").grid(row=0, column=5, padx=(15, 0))
        ttk.Checkbutton(frame, text="持续授权当前动作（测试用；未勾选将在500ms后停车）",
                        variable=self.continuous_motion_var).grid(
                            row=3, column=0, columnspan=6, sticky="w", pady=(6, 0))

        status_frame = ttk.Frame(frame)
        status_frame.grid(row=1, column=0, columnspan=6, sticky="ew", pady=(8, 0))
        for column, (key, variable) in enumerate(self.device_status_vars.items()):
            label = ttk.Label(status_frame, textvariable=variable, foreground=STATUS_IDLE_COLOR)
            label.grid(row=0, column=column, padx=(0, 12), sticky="w")
            self.device_status_labels[key] = label
        ttk.Label(frame, textvariable=self.self_test_summary_var).grid(
            row=2, column=0, columnspan=5, sticky="w", pady=(6, 0)
        )
        self.self_test_button = ttk.Button(frame, text="重新自检", command=self._start_self_test,
                                          state=tk.DISABLED)
        self.self_test_button.grid(row=2, column=5, sticky="e", pady=(6, 0))

    def _build_sensor_frame(self, parent: ttk.Frame) -> None:
        """创建 JY61P 实时数值显示区域。"""

        frame = ttk.LabelFrame(parent, text="JY61P 实时数据", padding=10)
        frame.grid(row=1, column=0, sticky="nsew", padx=(0, 5), pady=(0, 8))

        headings = ("数据", "X / Yaw", "Y / Pitch", "Z / Roll")
        for column, heading in enumerate(headings):
            ttk.Label(frame, text=heading, anchor="center").grid(
                row=0, column=column, padx=6, pady=4, sticky="ew"
            )
            frame.columnconfigure(column, weight=1)

        self._add_sensor_row(frame, 1, "加速度 (g)", "ax", "ay", "az")
        self._add_sensor_row(frame, 2, "角速度 (°/s)", "gx", "gy", "gz")
        self._add_sensor_row(frame, 3, "姿态角 (°)", "yaw", "pitch", "roll")

        ttk.Label(frame, text="温度 (℃)").grid(row=4, column=0, padx=6, pady=4, sticky="w")
        ttk.Label(frame, textvariable=self.sensor_vars["temp"]).grid(row=4, column=1, pady=4)
        ttk.Label(frame, text="版本号").grid(row=4, column=2, padx=6, pady=4, sticky="e")
        ttk.Label(frame, textvariable=self.sensor_vars["version"]).grid(row=4, column=3, pady=4)

        ttk.Label(frame, text="有效样本数").grid(row=5, column=0, padx=6, pady=4, sticky="w")
        ttk.Label(frame, textvariable=self.sensor_vars["count"]).grid(row=5, column=1, pady=4)
        ttk.Checkbutton(
            frame,
            text="记录显示的 IMU 数据",
            variable=self.show_imu_log_var,
        ).grid(row=5, column=2, columnspan=2, padx=6, pady=4, sticky="e")

        ttk.Label(frame, text="Yaw 静止锁定").grid(row=6, column=0, padx=6, pady=4, sticky="w")
        ttk.Label(frame, textvariable=self.sensor_vars["yaw_lock"]).grid(
            row=6, column=1, pady=4
        )

    def _add_sensor_row(
        self,
        parent: ttk.LabelFrame,
        row: int,
        title: str,
        first_key: str,
        second_key: str,
        third_key: str,
    ) -> None:
        """添加一行三轴数值，统一显示格式。"""

        ttk.Label(parent, text=title).grid(row=row, column=0, padx=6, pady=6, sticky="w")
        for column, key in enumerate((first_key, second_key, third_key), start=1):
            ttk.Label(
                parent,
                textvariable=self.sensor_vars[key],
                width=12,
                anchor="center",
                relief="sunken",
            ).grid(row=row, column=column, padx=4, pady=6, sticky="ew")

    def _build_motor_frame(self, parent: ttk.Frame) -> None:
        """创建带设备地址的 Emm_V5 多电机控制区域。"""

        frame = ttk.LabelFrame(parent, text="Emm_V5 多电机控制", padding=10)
        frame.grid(row=1, column=1, sticky="nsew", padx=(5, 0), pady=(0, 8))
        frame.columnconfigure(1, weight=1)
        frame.columnconfigure(3, weight=1)

        ttk.Label(frame, text="电机地址").grid(row=0, column=0, sticky="w", pady=3)
        address = ttk.Spinbox(
            frame, from_=0, to=255, textvariable=self.motor_address_var, width=10
        )
        address.grid(row=0, column=1, sticky="ew", padx=(4, 10), pady=3)
        self.motor_widgets = [address]

        ttk.Label(frame, text="方向").grid(row=0, column=2, sticky="w", pady=3)
        direction = ttk.Combobox(
            frame,
            textvariable=self.motor_direction_var,
            values=("0 - CW", "1 - CCW"),
            state="readonly",
            width=12,
        )
        direction.grid(row=0, column=3, sticky="ew", padx=(4, 0), pady=3)
        self.motor_widgets.append(direction)

        ttk.Label(frame, text="速度 RPM").grid(row=1, column=0, sticky="w", pady=3)
        speed = ttk.Entry(frame, textvariable=self.motor_speed_var)
        speed.grid(row=1, column=1, sticky="ew", padx=(4, 10), pady=3)
        self.motor_widgets.append(speed)

        ttk.Label(frame, text="加速度 0~255").grid(row=1, column=2, sticky="w", pady=3)
        acceleration = ttk.Entry(frame, textvariable=self.motor_acceleration_var)
        acceleration.grid(row=1, column=3, sticky="ew", padx=(4, 0), pady=3)
        self.motor_widgets.append(acceleration)

        ttk.Label(frame, text="位置脉冲数").grid(row=2, column=0, sticky="w", pady=3)
        pulses = ttk.Entry(frame, textvariable=self.motor_pulses_var)
        pulses.grid(row=2, column=1, sticky="ew", padx=(4, 10), pady=3)
        self.motor_widgets.append(pulses)

        absolute = ttk.Checkbutton(frame, text="绝对位置", variable=self.motor_absolute_var)
        absolute.grid(row=2, column=2, sticky="w", pady=3)
        sync = ttk.Checkbutton(frame, text="等待同步触发", variable=self.motor_sync_var)
        sync.grid(row=2, column=3, sticky="w", pady=3)
        self.motor_widgets.extend((absolute, sync))

        # 常用动作按钮。ESTOP 使用广播地址 0，便于紧急停止整条总线。
        button_specs = (
            ("使能", partial(self._send_enable_state, 1)),
            ("失能", partial(self._send_enable_state, 0)),
            ("速度运行", self._run_velocity),
            ("位置运行", self._run_position),
            ("停止当前", self._stop_motor),
            ("同步触发", self._trigger_sync),
            ("当前位置清零", self._zero_position),
            ("全部急停", self._emergency_stop),
        )
        for index, (text, callback) in enumerate(button_specs):
            if text == "全部急停":
                button = tk.Button(
                    frame, text="全部急停", command=callback,
                    background="#ef233c", foreground="white",
                    activebackground="#b91c1c", activeforeground="white",
                    disabledforeground="#ffd6da", font=("Microsoft YaHei UI", 11, "bold"),
                    relief=tk.RAISED, borderwidth=2, padx=8, pady=5,
                )
                self.emergency_stop_button = button
            else:
                button = ttk.Button(frame, text=text, command=callback)
                self.motor_widgets.append(button)
            button.grid(row=3 + (index // 4), column=index % 4, padx=3, pady=5, sticky="ew")

        ttk.Separator(frame, orient=tk.HORIZONTAL).grid(
            row=5, column=0, columnspan=4, sticky="ew", pady=7
        )

        ttk.Label(frame, text="读取项目").grid(row=6, column=0, sticky="w")
        read_item = ttk.Combobox(
            frame,
            textvariable=self.read_item_var,
            values=("FLAG", "SPEED", "POSITION", "PERR", "VBUS", "CURRENT", "STATE", "ORG", "VER"),
            state="readonly",
        )
        read_item.grid(row=6, column=1, sticky="ew", padx=(4, 10))
        read_button = ttk.Button(frame, text="读取", command=self._read_motor_parameter)
        read_button.grid(row=6, column=2, columnspan=2, sticky="ew", padx=(4, 0))

        ttk.Label(frame, text="回零模式").grid(row=7, column=0, sticky="w", pady=(6, 0))
        home_mode = ttk.Combobox(
            frame,
            textvariable=self.home_mode_var,
            values=("0 - 单圈就近", "1 - 单圈方向", "2 - 无限位碰撞", "3 - 限位开关"),
            state="readonly",
        )
        home_mode.grid(row=7, column=1, sticky="ew", padx=(4, 10), pady=(6, 0))
        home_button = ttk.Button(frame, text="执行回零", command=self._home_motor)
        home_button.grid(row=7, column=2, columnspan=2, sticky="ew", padx=(4, 0), pady=(6, 0))
        self.motor_widgets.extend((read_item, read_button, home_mode, home_button))

    def _build_chassis_frame(self, parent: ttk.Frame) -> None:
        """创建 X 型麦克纳姆轮底盘控制区域。"""

        frame = ttk.LabelFrame(
            parent,
            text="麦克纳姆底盘（1右前 / 2左前 / 3左后 / 4右后）",
            padding=8,
        )
        frame.grid(row=2, column=0, columnspan=2, sticky="ew", pady=(0, 8))

        parameters = (
            ("前进量", self.chassis_forward_var, -1000, 1000),
            ("左移量", self.chassis_left_var, -1000, 1000),
            ("逆时针量", self.chassis_ccw_var, -1000, 1000),
            ("最大 RPM", self.chassis_speed_var, 0, MOTOR_MAX_RPM),
            ("加速度", self.chassis_acceleration_var, 0, 255),
        )
        for column, (label, variable, minimum, maximum) in enumerate(parameters):
            ttk.Label(frame, text=f"{label} {minimum}~{maximum}").grid(
                row=0, column=column, padx=4, sticky="w"
            )
            entry = ttk.Entry(frame, textvariable=variable, width=12)
            entry.grid(row=1, column=column, padx=4, pady=(2, 6), sticky="ew")
            frame.columnconfigure(column, weight=1)
            self.motor_widgets.append(entry)

        run_button = ttk.Button(frame, text="执行分量", command=self._drive_chassis)
        run_button.grid(row=1, column=5, padx=4, pady=(2, 6), sticky="ew")
        self.motor_widgets.append(run_button)

        preset_commands = (
            ("左前", 1000, 1000, 0),
            ("前进", 1000, 0, 0),
            ("右前", 1000, -1000, 0),
            ("逆时针", 0, 0, 1000),
            ("停止", 0, 0, 0),
            ("顺时针", 0, 0, -1000),
            ("左后", -1000, 1000, 0),
            ("后退", -1000, 0, 0),
            ("右后", -1000, -1000, 0),
            ("左移", 0, 1000, 0),
            ("右移", 0, -1000, 0),
        )
        for index, (text, forward, left, ccw) in enumerate(preset_commands):
            button = ttk.Button(
                frame,
                text=text,
                command=lambda x=forward, y=left, z=ccw: self._drive_chassis(x, y, z),
            )
            button.grid(row=2 + (index // 6), column=index % 6, padx=3, pady=3, sticky="ew")
            self.motor_widgets.append(button)

    def _build_log_frame(self, parent: ttk.Frame) -> None:
        """创建收发日志区域。"""

        frame = ttk.LabelFrame(parent, text="通讯日志", padding=8)
        frame.grid(row=3, column=0, columnspan=2, sticky="nsew")
        frame.rowconfigure(0, weight=1)
        frame.columnconfigure(0, weight=1)
        frame.columnconfigure(1, weight=1)
        frame.columnconfigure(2, weight=1)

        self.log_text = scrolledtext.ScrolledText(frame, height=14, state=tk.DISABLED, wrap=tk.WORD)
        self.log_text.grid(row=0, column=0, columnspan=3, sticky="nsew")
        ttk.Button(frame, text="查询状态", command=self._request_status).grid(
            row=1, column=0, sticky="w", pady=(6, 0)
        )
        ttk.Button(frame, text="PING", command=partial(self._send_command, COMMAND_PING, b"", "PING")).grid(
            row=1, column=1, pady=(6, 0)
        )
        ttk.Button(frame, text="清空日志", command=self._clear_log).grid(
            row=1, column=2, sticky="e", pady=(6, 0)
        )

    def _refresh_ports(self) -> None:
        """重新枚举系统串口，并优先保留当前选择。"""

        selected = self.port_var.get()
        ports = sorted(list_ports.comports(), key=lambda item: item.device)
        display_values = [f"{item.device} — {item.description}" for item in ports]
        self.port_combo["values"] = display_values

        matching = next((value for value in display_values if value.startswith(selected + " ")), None)
        if matching is not None:
            self.port_var.set(matching)
        elif display_values:
            self.port_var.set(display_values[0])
        else:
            self.port_var.set("")

    def _toggle_connection(self) -> None:
        """根据当前连接状态执行连接或断开。"""

        if self.worker.connected:
            self.worker.disconnect()
            self.pending_requests.clear()
            self._update_connection_state(False)
            self._append_log("串口已断开。")
            return

        selected = self.port_var.get().strip()
        if not selected:
            messagebox.showwarning("没有串口", "没有检测到可用串口，请检查 CH340 驱动和 USB 连接。")
            return

        # 下拉框格式为“COMx — 描述”，第一个空格以前就是 pyserial 需要的端口名。
        port_name = selected.split()[0]
        try:
            self.worker.connect(port_name, 115200)
        except (serial.SerialException, OSError) as exc:
            messagebox.showerror("连接失败", f"无法打开 {port_name}：\n{exc}")
            return

        self._update_connection_state(True)
        self.pending_requests.clear()
        self._append_log(f"已连接 {port_name}，参数 115200 8N1。")
        self._append_log("已关闭 DTR/RTS 硬件握手，避免 CH340 使 STM32 保持复位。")
        self._append_log("正在请求 STM32 状态；JY61P 的 USART3 固定使用 9600 8N1。")
        self._start_self_test()

    def _update_connection_state(self, connected: bool) -> None:
        """同步更新连接按钮、状态文字和电机控件可用状态。"""

        self.connection_var.set("已连接" if connected else "未连接")
        self.connect_button.configure(text="断开" if connected else "连接")
        self.port_combo.configure(state="disabled" if connected else "readonly")
        if not connected:
            self._clear_motion_authorization()
            self._lease_supported = False
            self._firmware_safety = None
            self._self_test.active = False
            self._self_test_reported = True
            self._clear_self_test_requests()
            self.self_test_summary_var.set("自检：等待连接")
            for key in DEVICE_NAMES:
                self._set_device_status(key, "未连接", STATUS_IDLE_COLOR)
        self.self_test_button.configure(state="normal" if connected and not self._self_test.active else "disabled")
        self._set_motor_controls_enabled(connected and not self._self_test.active)

    def _set_motor_controls_enabled(self, enabled: bool) -> None:
        """未连接时禁止发送电机命令，避免用户误以为命令已经执行。"""

        enabled = enabled and getattr(self.worker, "_motion_blocked", False) is not True
        state = "normal" if enabled else "disabled"
        for widget in self.motor_widgets:
            # 只读下拉框在启用时需要恢复 readonly，而普通控件使用 normal。
            if isinstance(widget, ttk.Combobox):
                widget.configure(state="readonly" if enabled else "disabled")
            else:
                widget.configure(state=state)
        # 自检期间也可以立即发送广播急停。
        self.emergency_stop_button.configure(state="normal" if self.worker.connected else "disabled")

    def _set_device_status(self, key: str, status: str, color: str) -> None:
        text = f"{DEVICE_NAMES[key]}：{status}"
        if self.device_status_vars[key].get() != text:
            self.device_status_vars[key].set(text)
            self.device_status_labels[key].configure(foreground=color)

    def _start_self_test(self) -> None:
        if not self.worker.connected or self._self_test.active:
            return
        self._self_test.start(time.monotonic())
        self._self_test_reported = False
        self._clear_self_test_requests()
        if getattr(self, "_firmware_safety", None) is not None:
            self._send_command(COMMAND_SELF_TEST, b"", "下位机重新自检（先停车）")
        self.self_test_button.configure(state="disabled")
        self._set_motor_controls_enabled(False)
        for key in DEVICE_NAMES:
            self._set_device_status(key, "待检测", STATUS_PENDING_COLOR)
        self._append_log("开始连接自检：等待下位机状态和实时 IMU 数据，依次读取 1～4 号电机 FLAG。")
        self._advance_self_test()

    def _advance_self_test(self) -> None:
        if self._self_test_reported:
            return
        if not self.worker.connected:
            return
        probe = self._self_test.tick(time.monotonic())
        if probe is not None:
            sequence = self._send_command(*probe)
            if sequence is not None:
                self._self_test_sequences.add(sequence)
        if self._self_test_reported:
            return  # 发送失败可能已断开连接。
        test = self._self_test
        self._set_device_status("controller", "在线" if test.controller_online else
                                ("检测中" if test.active else "无应答"),
                                STATUS_ONLINE_COLOR if test.controller_online else
                                (STATUS_PENDING_COLOR if test.active else STATUS_ERROR_COLOR))
        self._set_device_status("imu", "在线" if test.imu_online else
                                ("检测中" if test.active else "未收到数据"),
                                STATUS_ONLINE_COLOR if test.imu_online else
                                (STATUS_PENDING_COLOR if test.active else STATUS_ERROR_COLOR))
        for address, online in test.motor_online.items():
            if online:
                status, color = "在线", STATUS_ONLINE_COLOR
            elif online is None and test.active:
                status = "检测中" if test.controller_online and address == test.current_motor else "待检测"
                color = STATUS_PENDING_COLOR
            else:
                status, color = "未收到回复", STATUS_ERROR_COLOR
            self._set_device_status(f"motor_{address}", status, color)
        if test.active:
            stage = "等待下位机启动" if not test.controller_online else (
                f"检测 {test.current_motor} 号电机" if test.current_motor is not None else "等待 IMU 实时数据"
            )
            summary = f"自检中：{stage}"
            if self.self_test_summary_var.get() != summary:
                self.self_test_summary_var.set(summary)
            return
        self._self_test_reported = True
        self._clear_self_test_requests()
        missing = []
        if not test.controller_online:
            missing.append("下位机")
        if not test.imu_online:
            missing.append("IMU")
        missing.extend(f"{address}号电机" for address, online in test.motor_online.items() if not online)
        summary = "自检通过：下位机、IMU 和 1～4 号电机均在线" if not missing else (
            "自检完成，未检测到：" + "、".join(missing)
        )
        if test.firmware_state is not None and test.firmware_state != 2:
            summary = "STM32 自检未通过/故障锁定，禁止运动；" + summary.replace("自检通过：", "检测到：")
        self.self_test_summary_var.set(summary)
        self._append_log(summary)
        self.self_test_button.configure(state="normal")
        self._set_motor_controls_enabled(test.controller_online and (
            test.firmware_state is None or test.firmware_state == 2
        ))

    def _clear_self_test_requests(self) -> None:
        for sequence in self._self_test_sequences:
            pending = self.pending_requests.get(sequence)
            if pending is not None and pending[1].startswith("自检："):
                self.pending_requests.pop(sequence)
        self._self_test_sequences.clear()

    def _poll_serial_events(self) -> None:
        """在 Tkinter 主线程中消费串口线程产生的事件。"""

        deadline = time.perf_counter() + GUI_EVENT_BUDGET_SECONDS
        try:
            for _ in range(GUI_MAX_EVENTS_PER_TICK):
                if time.perf_counter() >= deadline:
                    break
                try:
                    event_type, payload = self.worker.events.get_nowait()
                except queue.Empty:
                    break
                if event_type == "frame":
                    self._handle_received_frame(payload)
                elif event_type == "sent":
                    frame = payload
                    pending = self.pending_requests.get(frame[3])
                    if frame[4] == COMMAND_PING and pending is None:
                        continue
                    description = pending[1] if pending is not None else f"INDEX=0x{frame[4]:02X}"
                    self._append_log(
                        f"TX  {description}  SEQ={frame[3]:02X} INDEX={frame[4]:02X}  "
                        f"[{format_hex(frame)}]"
                    )
                elif event_type == "protocol_error":
                    self._append_log(f"协议错误：{payload}")
                elif event_type == "error":
                    self._append_log(f"串口错误：{payload}")
                    self.worker.disconnect()
                    self._update_connection_state(False)
            latest_imu = self.worker.take_latest_imu()
            if latest_imu is not None:
                frame, sample_count = latest_imu
                self._handle_received_frame(frame, sample_count)
            error_message, error_count = self.worker.take_protocol_errors()
            if error_count:
                self._append_log(f"协议错误（本轮 {error_count} 条，仅显示最新）：{error_message}")
            self._advance_self_test()
        except Exception as exc:
            self._append_log(f"界面处理异常：{type(exc).__name__}: {exc}")
        finally:
            # 即使解析或日志刷新异常，也安排下一轮，避免数据永久停止更新。
            try:
                self._flush_log()
            finally:
                delay_ms = 1 if not self.worker.events.empty() else GUI_POLL_INTERVAL_MS
                self.root.after(delay_ms, self._poll_serial_events)

    def _handle_received_frame(self, frame: bytes, sample_count: int = 1) -> None:
        """显示原始帧，并按 INDEX 解析应答或异步事件。"""

        destination, sequence, index = frame[2], frame[3], frame[4]
        payload = frame[5:-2]
        # 周期成功应答不刷屏；续租拒绝/缺状态必须进入下面的保护处理。
        if sequence not in self.pending_requests and (
                index == (COMMAND_PING | 0x80) or
                index == (COMMAND_RENEW | 0x80) and payload == b"\x00"):
            return
        if index != EVENT_IMU_TEXT:
            self._append_log(
                f"RX  DEST={destination:02X} SEQ={sequence:02X} INDEX={index:02X}  "
                f"[{format_hex(frame)}]"
            )
        if destination not in (DEST_HOST, 0xFF):
            self._append_log(f"忽略：该帧目标不是上位机（DEST={destination:02X}）")
            return

        if index == (COMMAND_RENEW | 0x80):
            # 自动续租不登记pending；SEQ回绕时不能误删同SEQ的其他命令。
            if self.pending_requests.get(sequence, (None,))[0] == COMMAND_RENEW:
                self.pending_requests.pop(sequence)
            if payload != b"\x00":
                self._clear_motion_authorization()
                reason = (ACK_NAMES.get(payload[0], f"未知状态 0x{payload[0]:02X}")
                          if len(payload) == 1 else
                          "应答长度错误" if payload else "应答缺少状态字节")
                self._append_log(f"运动续租失败：{reason}；已撤销持续授权，请查询状态。")
                self.self_test_summary_var.set("运动续租失败：已撤销持续授权，请查询状态并排障后重新确认")
            return

        if index == EVENT_READY:
            self._append_log("事件：STM32 已就绪。")
            return
        if index == EVENT_IMU_TEXT:
            self._handle_imu_text(payload.decode("ascii", errors="replace").strip(), sample_count)
            return
        if index == EVENT_MOTOR_REPLY:
            self._append_log(f"电机回复：{format_hex(payload)}")
            self._self_test.observe_motor(payload, time.monotonic())
            return
        if index == EVENT_SELF_TEST:
            self._handle_firmware_safety(payload)
            return
        if index == EVENT_CHASSIS_RESULT:
            if len(payload) == 3:
                result_names = {2: "四轮应答通过，同步触发已入发送队列（非到位确认）",
                                3: "电机失败，已请求全车停车并锁定", 4: "控制序列已取消"}
                self._append_log(f"底盘结果：SEQ={payload[0]:02X} "
                                 f"{result_names.get(payload[1], '未知结果')} "
                                 f"故障地址={payload[2]}")
            else:
                self._append_log("底盘结果事件长度错误")
            return
        if not 0x80 <= index <= 0xDF:
            self._append_log(f"未知上报 INDEX=0x{index:02X}，DATA={format_hex(payload)}")
            return

        request_index = index & 0x7F
        pending = self.pending_requests.pop(sequence, None)
        description = pending[1] if pending is not None else f"命令 0x{request_index:02X}"
        if pending is not None and pending[0] != request_index:
            self._append_log(
                f"应答不匹配：SEQ={sequence:02X}，期望 INDEX=0x{pending[0] | 0x80:02X}"
            )
        if not payload:
            self._append_log(f"{description}：应答缺少状态字节。")
            return

        status = payload[0]
        motion_requests = getattr(self, "_motion_requests", set())
        if sequence in motion_requests:
            motion_requests.discard(sequence)
            if pending is not None and pending[0] == request_index and status == 0:
                self._motion_sequence = sequence
        status_text = ACK_NAMES.get(status, f"未知状态 0x{status:02X}")
        if request_index == COMMAND_STATUS and status == 0:
            self._handle_status_payload(payload[1:])
        else:
            self._append_log(f"{description}：{status_text}。")

    def _handle_status_payload(self, payload: bytes) -> None:
        """解析状态应答中的大端序计数器。"""

        if len(payload) not in (30, 43, 53, 57):
            self._append_log(f"状态应答长度错误：期望30、43、53或57，实际{len(payload)}")
            return
        values = struct.unpack(">IBIIIIIIB", payload[:30])
        imu_baud, yaw_lock, valid, checksum, imu_drop, imu_dma, motor_drop, motor_dma, busy = values
        self._self_test.observe_controller(time.monotonic())
        self.sensor_vars["yaw_lock"].set("已锁定" if yaw_lock else "运动/等待")
        self._append_log(
            "状态："
            f"IMU_BAUD={imu_baud} YAW_LOCK={yaw_lock} VALID={valid} CHECKSUM={checksum} "
            f"IMU_DROP={imu_drop} IMU_DMA={imu_dma} MOTOR_DROP={motor_drop} "
            f"MOTOR_DMA={motor_dma} CHASSIS_BUSY={busy}"
        )
        if len(payload) >= 43:
            failsafe_armed, failsafe_count, host_errors, motor_errors = struct.unpack(
                ">BIII", payload[30:43]
            )
            self._append_log(
                "安全："
                f"FAILSAFE_ARMED={failsafe_armed} FAILSAFE_COUNT={failsafe_count} "
                f"HOST_PROTOCOL_ERROR={host_errors} MOTOR_PROTOCOL_ERROR={motor_errors}"
            )
        if len(payload) in (53, 57):
            self._handle_firmware_safety(payload[43:])

    def _handle_firmware_safety(self, payload: bytes) -> None:
        """兼容旧10字节和新14字节安全载荷；字段顺序见HOST_PROTOCOL.md。"""
        if len(payload) not in (10, 14):
            self._append_log("STM32 自检状态长度错误")
            return
        state, motor_mask, imu, reason, address, stop_pending, faults = struct.unpack(
            ">BBBBBBI", payload[:10]
        )
        safety = (state, motor_mask, imu, reason, address, stop_pending, faults)
        if getattr(self, "_firmware_safety", None) != safety:
            self._append_log(f"STM32 自检：{ {1: '检测中', 2: '通过', 3: '失败/锁定'}.get(state, '未知')} "
                             f"MOTOR_MASK={motor_mask:02X} IMU={imu} FAULT={reason} "
                             f"ADDRESS={address} STOP_PENDING={stop_pending} MOTOR_FAULTS={faults}")
        self._firmware_safety = safety
        if len(payload) == 14:
            self._lease_supported = True
            estop, mode, leased, motion_sequence = payload[10:]
            self._append_log(f"控制授权：ESTOP={estop} MODE={mode} LEASE={leased} SEQ={motion_sequence:02X}")
            if not leased:
                self._motion_sequence = None
            if state == 1:
                self.worker.observe_firmware_safety(state)
            if state == 2 and not estop and not stop_pending:
                self.worker.observe_firmware_safety(state)
        if state != 2 or stop_pending:
            self._clear_motion_authorization()
        self._self_test.observe_firmware(state, motor_mask, imu, time.monotonic())
        if not self._self_test.active:
            self._set_motor_controls_enabled(state == 2 and not stop_pending)
        if state == 3:
            self.self_test_summary_var.set("STM32 故障锁定：排查设备后点击“重新自检”，急停仍可使用")

    def _handle_imu_text(self, line: str, sample_count: int = 1) -> None:
        """解析二进制事件帧中携带的 IMU 文本载荷。"""

        match = IMU_LINE_PATTERN.fullmatch(line)
        if match is None:
            self._append_log(f"IMU载荷格式错误：{line}")
            return

        values = match.groups()
        self._self_test.observe_imu()
        keys = ("ax", "ay", "az", "gx", "gy", "gz", "yaw", "pitch", "roll", "temp", "version")
        for key, value in zip(keys, values):
            self.sensor_vars[key].set(value)

        self.sample_count += sample_count
        self.sensor_vars["count"].set(str(self.sample_count))
        if self.show_imu_log_var.get():
            self._append_log(f"IMU {line}")

    def _get_integer(self, variable: tk.StringVar, name: str, minimum: int, maximum: int) -> int:
        """读取并检查界面中的整数参数，非法时抛出 ValueError。"""

        text = variable.get().strip()
        try:
            value = int(text, 10)
        except ValueError as exc:
            raise ValueError(f"{name} 必须是整数") from exc
        if value < minimum or value > maximum:
            raise ValueError(f"{name} 必须在 {minimum}~{maximum} 之间")
        return value

    def _get_motor_common_values(self) -> tuple[int, int, int, int, int]:
        """读取地址、方向、速度、加速度和同步标志。"""

        address = self._get_integer(self.motor_address_var, "电机地址", 0, 255)
        direction = int(self.motor_direction_var.get().split()[0])
        speed = self._get_integer(self.motor_speed_var, "速度", 0, MOTOR_MAX_RPM)
        acceleration = self._get_integer(self.motor_acceleration_var, "加速度", 0, 255)
        sync = 1 if self.motor_sync_var.get() else 0
        return address, direction, speed, acceleration, sync

    def _send_command(self, index: int, data: bytes, description: str) -> Optional[int]:
        """发送二进制命令并记录 SEQ，便于匹配 STM32 应答。"""

        if not self.worker.connected:
            messagebox.showwarning("串口未连接", "请先连接 STM32 的板载 CH340 串口。")
            return

        if index in (COMMAND_STOP, COMMAND_EMERGENCY_STOP, COMMAND_SELF_TEST) or (
                index == COMMAND_ENABLE and len(data) == 3 and data[1] == 0):
            self._clear_motion_authorization()
            self.pending_requests.clear()
        try:
            sequence, frame = self.worker.send_frame(index, data)
        except (serial.SerialException, OSError) as exc:
            self._append_log(f"发送失败：{exc}")
            self.worker.disconnect()
            self._update_connection_state(False)
            return

        self.pending_requests[sequence] = (index, description)
        if self._is_motion_request(index, data):
            self._motion_sequence = None
            if not hasattr(self, "_motion_requests"):
                self._motion_requests = set()
            self._motion_requests.add(sequence)
        self._append_log(
            f"QUEUE  {description}  SEQ={sequence:02X} INDEX={index:02X}  "
            f"[等待发送：{format_hex(frame)}]"
        )
        return sequence

    def _send_heartbeat(self) -> None:
        """PING 只维持链路；显式勾选后使用独立 RENEW，绝不重发位置命令。"""

        if self.worker.connected:
            try:
                self.worker.send_frame(COMMAND_PING)
                sequence = getattr(self, "_motion_sequence", None)
                variable = getattr(self, "continuous_motion_var", None)
                if (getattr(self, "_lease_supported", False) and sequence is not None
                        and variable is not None and variable.get()):
                    self.worker.send_frame(COMMAND_RENEW, bytes((sequence,)))
            except (serial.SerialException, OSError) as exc:
                self._append_log(f"心跳发送失败：{exc}")
                self.worker.disconnect()
                self._update_connection_state(False)

        self.root.after(HEARTBEAT_INTERVAL_MS, self._send_heartbeat)

    @staticmethod
    def _is_motion_request(index: int, data: bytes) -> bool:
        """与STM32布防条件保持一致，尤其绝对位置0仍可能运动。"""
        if index == COMMAND_VELOCITY and len(data) == 6:
            return int.from_bytes(data[2:4], "big") != 0
        if index == COMMAND_POSITION and len(data) == 11:
            return int.from_bytes(data[5:9], "big") != 0 or data[9] != 0
        if index == COMMAND_CHASSIS and len(data) == 9:
            return any(data[:6]) and int.from_bytes(data[6:8], "big") != 0
        return index in (COMMAND_HOME, COMMAND_SYNC)

    def _clear_motion_authorization(self) -> None:
        """撤销PC持续授权意图；真正停车由已发送的STOP或STM32超时完成。"""
        self._motion_sequence = None
        getattr(self, "_motion_requests", set()).clear()
        variable = getattr(self, "continuous_motion_var", None)
        if variable is not None:
            variable.set(False)

    def _request_status(self) -> None:
        """查询 STM32 是否在线，以及 JY61P 和安全状态计数。"""

        if self.worker.connected:
            self._send_command(COMMAND_STATUS, b"", "查询状态")

    def _send_validated(self, builder) -> None:
        """统一捕获参数校验异常，再发送 builder 生成的命令。"""

        try:
            index, data, description = builder()
        except ValueError as exc:
            messagebox.showwarning("参数错误", str(exc))
            return
        self._send_command(index, data, description)

    def _send_enable_state(self, state: int) -> None:
        def build() -> tuple[int, bytes, str]:
            address = self._get_integer(self.motor_address_var, "电机地址", 0, 255)
            sync = 0  # 使能只允许立即执行，避免未触发使能与健康检测冲突。
            return COMMAND_ENABLE, bytes((address, state, sync)), f"EN {address} {state} {sync}"

        self._send_validated(build)

    def _run_velocity(self) -> None:
        def build() -> tuple[int, bytes, str]:
            address, direction, speed, acceleration, sync = self._get_motor_common_values()
            data = struct.pack(">BBHBB", address, direction, speed, acceleration, sync)
            return (
                COMMAND_VELOCITY,
                data,
                f"VEL {address} {direction} {speed} {acceleration} {sync}",
            )

        self._send_validated(build)

    def _drive_chassis(
        self,
        forward: Optional[int] = None,
        left: Optional[int] = None,
        ccw: Optional[int] = None,
    ) -> None:
        """发送底盘三自由度速度；零向量/零最大速度统一走全车STOP。"""

        def build() -> tuple[int, bytes, str]:
            if forward is None or left is None or ccw is None:
                requested_forward = self._get_integer(
                    self.chassis_forward_var, "前进量", -1000, 1000
                )
                requested_left = self._get_integer(self.chassis_left_var, "左移量", -1000, 1000)
                requested_ccw = self._get_integer(
                    self.chassis_ccw_var, "逆时针量", -1000, 1000
                )
            else:
                requested_forward = forward
                requested_left = left
                requested_ccw = ccw
                self.chassis_forward_var.set(str(forward))
                self.chassis_left_var.set(str(left))
                self.chassis_ccw_var.set(str(ccw))

            # 零向量就是全车停止，不让速度/加速度输入错误阻止停车。
            # 共用STOP路径，撤销授权并清空未发送的旧运动/续租帧。
            if not any((requested_forward, requested_left, requested_ccw)):
                return COMMAND_STOP, b"\x00\x00", "STOP 0 0（底盘停止/退出控制模式）"
            speed = self._get_integer(
                self.chassis_speed_var, "底盘最大速度", 0, MOTOR_MAX_RPM
            )
            if speed == 0:
                return COMMAND_STOP, b"\x00\x00", "STOP 0 0（底盘停止/退出控制模式）"
            acceleration = self._get_integer(
                self.chassis_acceleration_var, "底盘加速度", 0, 255
            )
            data = struct.pack(
                ">hhhHB", requested_forward, requested_left, requested_ccw, speed, acceleration
            )
            return (
                COMMAND_CHASSIS,
                data,
                f"CHASSIS {requested_forward} {requested_left} {requested_ccw} "
                f"{speed} {acceleration}",
            )

        self._send_validated(build)

    def _run_position(self) -> None:
        def build() -> tuple[int, bytes, str]:
            address, direction, speed, acceleration, sync = self._get_motor_common_values()
            pulses = self._get_integer(self.motor_pulses_var, "位置脉冲数", 0, 0xFFFFFFFF)
            absolute = 1 if self.motor_absolute_var.get() else 0
            data = struct.pack(
                ">BBHBIBB", address, direction, speed, acceleration, pulses, absolute, sync
            )
            description = (
                f"POS {address} {direction} {speed} {acceleration} {pulses} {absolute} {sync}"
            )
            return COMMAND_POSITION, data, description

        self._send_validated(build)

    def _stop_motor(self) -> None:
        def build() -> tuple[int, bytes, str]:
            return COMMAND_STOP, b"\x00\x00", "STOP 0 0（全车停车/退出控制模式）"

        self._send_validated(build)

    def _emergency_stop(self) -> None:
        # ESTOP 在 STM32 中转换为地址 0 的立即停止命令，用于广播停止。
        self._send_command(COMMAND_EMERGENCY_STOP, b"", "ESTOP")
        self._set_motor_controls_enabled(False)
        self.self_test_summary_var.set("急停锁定：停车后需点击“重新自检”，通过后才能恢复")

    def _trigger_sync(self) -> None:
        # 地址 0 用作广播：先给各电机发送 sync=1 的动作，再统一触发。
        self._send_command(COMMAND_SYNC, b"\x00", "SYNC 0")

    def _zero_position(self) -> None:
        def build() -> tuple[int, bytes, str]:
            address = self._get_integer(self.motor_address_var, "电机地址", 0, 255)
            return COMMAND_ZERO, bytes((address,)), f"ZERO {address}"

        self._send_validated(build)

    def _read_motor_parameter(self) -> None:
        def build() -> tuple[int, bytes, str]:
            address = self._get_integer(self.motor_address_var, "电机地址", 1, 255)
            item_name = self.read_item_var.get()
            item = READ_ITEM_VALUES[item_name]
            return COMMAND_READ, bytes((address, item)), f"READ {address} {item_name}"

        self._send_validated(build)

    def _home_motor(self) -> None:
        def build() -> tuple[int, bytes, str]:
            address = self._get_integer(self.motor_address_var, "电机地址", 0, 255)
            mode = int(self.home_mode_var.get().split()[0])
            sync = 1 if self.motor_sync_var.get() else 0
            return COMMAND_HOME, bytes((address, mode, sync)), f"HOME {address} {mode} {sync}"

        self._send_validated(build)

    def _append_log(self, text: str) -> None:
        """暂存日志，在下一轮界面刷新时批量写入。"""

        if len(text) > GUI_LOG_MAX_ENTRY_CHARS:
            text = text[:GUI_LOG_MAX_ENTRY_CHARS] + "…"
        self._pending_log_lines.append(text)

    def _flush_log(self) -> None:
        if not self._pending_log_lines:
            return

        self.log_text.configure(state=tk.NORMAL)
        try:
            self.log_text.insert(tk.END, "\n".join(self._pending_log_lines) + "\n")
            self._pending_log_lines.clear()
            line_count = int(self.log_text.index("end-1c").split(".")[0]) - 1
            excess_lines = line_count - GUI_LOG_MAX_LINES
            if excess_lines > 0:
                self.log_text.delete("1.0", f"{excess_lines + 1}.0")
            self.log_text.see(tk.END)
        finally:
            self.log_text.configure(state=tk.DISABLED)

    def _clear_log(self) -> None:
        self._pending_log_lines.clear()
        self.log_text.configure(state=tk.NORMAL)
        self.log_text.delete("1.0", tk.END)
        self.log_text.configure(state=tk.DISABLED)

    def _on_close(self) -> None:
        """关闭窗口前停止线程并释放 COM 口。"""

        self.worker.disconnect()
        self.root.destroy()


def main() -> None:
    """程序入口。"""

    root = tk.Tk()
    TestApplication(root)
    root.mainloop()


if __name__ == "__main__":
    main()
