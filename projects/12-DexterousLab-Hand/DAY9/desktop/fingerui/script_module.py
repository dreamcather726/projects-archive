#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
script_module.py（脚本模块）

模块职责：
1) ScriptRunner：后台线程执行脚本（解析指令、延时、发送、日志/完成信号）
2) ScriptService：对外提供“启动/停止脚本”的稳定接口，屏蔽线程管理细节
3) 不依赖 UI：不引用任何 QWidget/控件，只依赖 serial_module 的串口接口

对外接口清单：
- ScriptRunner(serial_client, commands).start() / stop()
- ScriptRunner.log_signal(str) / finished_signal()
- ScriptService.start(commands: list[str]) -> None
- ScriptService.stop() -> None
- ScriptService.is_running -> bool
- ScriptService.log_signal(str) / finished_signal()
"""

# 启用注解类型支持（允许类内部使用自身类型注解）
from __future__ import annotations

# 导入时间模块，用于延时
import time
# 导入类型注解工具：列表、可选类型
from typing import List, Optional

# 导入PyQt线程、信号、基对象
from PyQt5.QtCore import QObject, QThread, pyqtSignal

# 导入依赖的串口模块客户端（本模块不实现串口，只调用接口）
from serial_module import SerialPortClient


class ScriptRunner(QThread):
    """
    脚本执行线程类
    作用：在后台执行脚本指令，不卡住UI界面
    继承自QThread，重写run()方法实现线程逻辑
    """
    # 信号：向外输出日志字符串
    log_signal = pyqtSignal(str)
    # 信号：脚本执行完成
    finished_signal = pyqtSignal()

    def __init__(self, serial_client: SerialPortClient, commands: List[str]):
        """
        构造函数
        :param serial_client: 串口客户端，提供发送接口
        :param commands: 脚本指令列表
        """
        super().__init__()  # 初始化父类QThread
        self.serial_client = serial_client  # 注入串口客户端
        self.commands = commands            # 要执行的指令
        self.running = True                 # 线程运行标志位

    def run(self) -> None:
        """
        线程入口函数
        自动被调用，遍历执行所有脚本指令
        """
        # 遍历每一行指令
        for cmd in self.commands:
            # 如果运行标志为False，终止执行
            if not self.running:
                break

            # 去除首尾空白字符
            cmd = cmd.strip()
            # 空行或#注释行 → 跳过
            if not cmd or cmd.startswith("#"):
                continue

            # 指令1：控制所有手指
            if cmd.startswith("ALL:"):
                self.send_command(cmd)
            # 指令2：控制单个手指
            elif cmd.startswith("FINGER"):
                self.send_command(cmd)
            # 指令3：延时
            elif cmd.startswith("DELAY"):
                try:
                    # 分割指令，取出毫秒数
                    delay_ms = int(cmd.split(":")[1])
                    self.log_signal.emit(f"等待 {delay_ms}ms")
                    time.sleep(delay_ms / 1000)  # 转成秒
                except Exception:
                    self.log_signal.emit(f"无效的DELAY命令: {cmd}")
            # 未知指令
            else:
                self.log_signal.emit(f"未知命令: {cmd}")

        # 全部执行完毕 → 发完成信号
        self.finished_signal.emit()

    def send_command(self, command: str) -> None:
        """
        发送串口指令函数
        :param command: 要发送的指令字符串（不含换行）
        """
        # 检查串口客户端是否已连接
        if self.serial_client and self.serial_client.is_connected:
            try:
                # 调用串口客户端发送指令
                self.serial_client.send_command(command)
                # 输出日志
                self.log_signal.emit(f"发送: {command}")
                # 短延时，保证硬件处理
                time.sleep(0.05)
            except Exception as e:
                self.log_signal.emit(f"发送失败: {str(e)}")

    def stop(self) -> None:
        """停止线程执行"""
        self.running = False


class ScriptService(QObject):
    """
    脚本服务（线程管理外观模式）
    作用：
    - 封装ScriptRunner，对外提供简单接口
    - 上层只调用 start/stop，不关心线程细节
    - 统一转发日志/完成信号
    """

    log_signal = pyqtSignal(str)        # 日志信号
    finished_signal = pyqtSignal()     # 完成信号

    def __init__(self, serial_client: SerialPortClient):
        super().__init__()
        self._serial_client = serial_client   # 串口客户端
        self._runner: Optional[ScriptRunner] = None  # 当前执行线程

    @property
    def is_running(self) -> bool:
        """查询是否正在运行脚本"""
        if not self._runner:
            return False
        return bool(self._runner.isRunning())

    def start(self, commands: List[str]) -> None:
        """
        启动脚本执行
        每次创建新线程，与原代码逻辑保持一致
        """
        # 创建新线程
        runner = ScriptRunner(self._serial_client, commands)
        # 信号转发：线程 → 服务层
        runner.log_signal.connect(self.log_signal.emit)
        runner.finished_signal.connect(self.finished_signal.emit)
        # 保存线程引用
        self._runner = runner
        # 启动线程
        runner.start()

    def stop(self) -> None:
        """停止当前脚本"""
        if self._runner and self._runner.isRunning():
            self._runner.stop()

    def wait(self) -> None:
        """等待线程结束（程序退出时用）"""
        if self._runner and self._runner.isRunning():
            self._runner.wait()


if __name__ == "__main__":
    """
    独立测试脚本：
    1) mock 串口客户端，验证脚本解析、延时、线程执行流程
    2) 通过 Qt 事件循环等待 finished_signal，输出日志与发送记录
    """

    import sys
    from PyQt5.QtCore import QCoreApplication, QEventLoop, QTimer

    # ===================== 模拟串口客户端（Mock） =====================
    class _MockSerialClient(SerialPortClient):
        def __init__(self) -> None:
            self.sent: List[str] = []  # 记录发送的指令

        @property
        def is_connected(self) -> bool:
            return True  # 永远返回已连接

        def send_command(self, command: str) -> None:
            self.sent.append(command)  # 记录指令，不真实发送

    # ===================== 测试环境初始化 =====================
    app = QCoreApplication(sys.argv)
    serial_client = _MockSerialClient()
    service = ScriptService(serial_client)  # type: ignore[arg-type]

    logs: List[str] = []
    service.log_signal.connect(logs.append)  # 收集日志

    loop = QEventLoop()
    service.finished_signal.connect(loop.quit)  # 执行完退出循环

    # 测试用指令（包含正常、错误、注释、空行）
    commands = [
        "# 注释行应跳过",
        "",
        "FINGER1:90",
        "DELAY:10",
        "ALL:180",
        "UNKNOWN:1",
        "DELAY:BAD",
    ]

    # 启动测试
    service.start(commands)

    # 超时保护：2秒强制结束
    QTimer.singleShot(2000, loop.quit)
    loop.exec_()

    # ===================== 测试断言 =====================
    assert "FINGER1:90" in serial_client.sent
    assert "ALL:180" in serial_client.sent
    assert any("未知命令" in x for x in logs)
    assert any("无效的DELAY命令" in x for x in logs)
    print("script_module 独立测试通过")