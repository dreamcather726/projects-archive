#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
main.py（主入口）

模块职责：
1) 整合 4 个模块：ui_module / interaction_module / serial_module / script_module
2) 统一绑定所有信号：UI 控件信号 -> 交互逻辑；脚本信号 -> 日志/完成回调
3) 初始化并启动程序，处理窗口关闭清理

依赖接口清单：
- ui_module.FingerControllerUI：提供控件
- serial_module.SerialPortClient：串口扫描/连接/发送
- script_module.ScriptService：脚本后台执行（内部线程）
- interaction_module.InteractionController：用户交互逻辑
"""

from __future__ import annotations

import sys

from PyQt5.QtWidgets import QApplication

from interaction_module import InteractionController
from script_module import ScriptService
from serial_module import SerialPortClient
from ui_module import FingerControllerUI


class FingerControllerApp(FingerControllerUI):
    """整合后的主窗口（仅在主入口中做信号绑定与关闭清理）"""

    def __init__(self):
        super().__init__()

        self.serial_client = SerialPortClient()
        self.script_service = ScriptService(self.serial_client)
        self.controller = InteractionController(self, self.serial_client, self.script_service)

        self._bind_signals()
        self.controller.refresh_ports()

    def _bind_signals(self) -> None:
        """集中绑定全部信号（UI 模块不做任何绑定）"""
        self.refresh_btn.clicked.connect(self.controller.refresh_ports)
        self.connect_btn.clicked.connect(self.controller.toggle_connection)

        for idx, slider in enumerate(self.finger_sliders, start=1):
            slider.valueChanged.connect(lambda value, i=idx: self.controller.on_slider_changed(i, value))

        for name, btn in self.preset_buttons.items():
            btn.clicked.connect(lambda _, n=name: self.controller.apply_preset_by_name(n))

        self.load_btn.clicked.connect(self.controller.load_script)
        self.save_btn.clicked.connect(self.controller.save_script)
        self.run_btn.clicked.connect(self.controller.run_script)
        self.stop_btn.clicked.connect(self.controller.stop_script)

        self.script_service.log_signal.connect(self.script_log.append)
        self.script_service.finished_signal.connect(self.controller.on_script_finished)

    def closeEvent(self, event) -> None:
        """窗口关闭事件：停止脚本线程、关闭串口（保持原程序退出时的资源释放行为）"""
        self.controller.close_cleanup()
        event.accept()


if __name__ == "__main__":
    """
    整体整合测试：
    - 直接运行 main.py 启动完整上位机
    - 界面功能应与原 finger_controller.py 一致
    """

    app = QApplication(sys.argv)
    window = FingerControllerApp()
    window.show()
    sys.exit(app.exec_())
