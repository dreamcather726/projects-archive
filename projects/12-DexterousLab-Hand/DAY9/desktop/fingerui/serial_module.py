#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
serial_module.py（串口模块）

模块职责：
1) 纯串口通信：扫描端口、连接/断开、发送指令
2) 与 UI / 脚本完全解耦：不引用任何 UI 组件，不依赖脚本线程
3) 提供清晰、可注入 mock 的接口，便于独立测试

对外接口清单：
- SerialPortClient.scan_ports() -> list[str]
- SerialPortClient.connect(port: str) -> None
- SerialPortClient.disconnect() -> None
- SerialPortClient.is_connected -> bool
- SerialPortClient.send_raw_text(text: str) -> None
- SerialPortClient.send_command(command: str) -> None
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, List, Optional

import serial
import serial.tools.list_ports


@dataclass(frozen=True)
class SerialConfig:
    """串口配置参数"""

    baudrate: int = 115200
    timeout: float = 0.1


class SerialPortClient:
    """
    串口客户端（纯通信封装）

    说明：
    - 默认使用 pyserial 的 serial.Serial 作为串口构造器；
    - 测试时可通过 serial_factory 注入 mock，以实现“无需硬件”的独立测试。
    """

    def __init__(
        self,
        config: SerialConfig | None = None,
        serial_factory: Callable[..., object] = serial.Serial,
    ) -> None:
        self._config = config or SerialConfig()
        self._serial_factory = serial_factory
        self._serial: Optional[object] = None

    def scan_ports(self) -> List[str]:
        """扫描并返回可用串口端口名列表（如：COM3）"""
        ports = serial.tools.list_ports.comports()
        return [p.device for p in ports]

    @property
    def is_connected(self) -> bool:
        """是否已连接（底层串口对象存在且处于打开状态）"""
        if not self._serial:
            return False
        is_open = getattr(self._serial, "is_open", False)
        return bool(is_open)

    def connect(self, port: str) -> None:
        """
        连接指定端口

        :param port: 端口名（如 COM3）
        :raises: 底层串口初始化异常（保持原行为：异常由上层捕获并提示）
        """
        self._serial = self._serial_factory(
            port=port,
            baudrate=self._config.baudrate,
            timeout=self._config.timeout,
        )

    def disconnect(self) -> None:
        """断开连接（若已连接则关闭串口）"""
        if not self._serial:
            return
        close = getattr(self._serial, "close", None)
        if callable(close):
            close()

    def send_raw_text(self, text: str) -> None:
        """
        发送原始文本（按传入内容原样发送，不强制补换行）

        说明：
        - 用于保持原代码行为：滑块发送时已经自带 '\\n'；
        - 同时也方便脚本模块自行决定是否补换行。
        """
        if not self.is_connected:
            return
        write = getattr(self._serial, "write", None)
        if not callable(write):
            raise RuntimeError("底层串口对象不支持 write()")
        write(text.encode("utf-8"))

    def send_command(self, command: str) -> None:
        """
        发送一条命令（保证命令以 '\\n' 结尾）

        :param command: 不含或可含换行的指令字符串，如 'FINGER1:90'
        """
        if command.endswith("\n"):
            self.send_raw_text(command)
        else:
            self.send_raw_text(command + "\n")


if __name__ == "__main__":
    """
    独立测试脚本（无需真实硬件）：
    1) 使用 mock 串口构造器，验证 connect / send / disconnect 行为
    2) 扫描端口 scan_ports 仍会尝试读取本机端口列表（若无端口则返回空）
    """

    class _MockSerial:
        def __init__(self, port: str, baudrate: int, timeout: float) -> None:
            self.port = port
            self.baudrate = baudrate
            self.timeout = timeout
            self.is_open = True
            self.written: List[bytes] = []

        def write(self, data: bytes) -> int:
            self.written.append(data)
            return len(data)

        def close(self) -> None:
            self.is_open = False

    client = SerialPortClient(serial_factory=_MockSerial)
    print("scan_ports():", client.scan_ports())
    client.connect("MOCK_PORT")
    assert client.is_connected is True
    client.send_raw_text("FINGER1:90\n")
    client.send_command("FINGER2:180")
    assert b"FINGER1:90\n" in client._serial.written  # type: ignore[attr-defined]
    assert b"FINGER2:180\n" in client._serial.written  # type: ignore[attr-defined]
    client.disconnect()
    assert client.is_connected is False
    print("serial_module 扫描端口通讯测试通过")
