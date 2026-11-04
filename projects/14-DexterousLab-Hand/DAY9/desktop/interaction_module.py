#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
interaction_module.py（交互模块）

模块职责：
1) 处理所有用户交互逻辑：滑块、按钮、预设、文件操作、状态提示、日志追加
2) 不直接操作串口：仅调用 serial_module 的接口
3) 不创建线程：仅调用 script_module 的接口（线程由脚本模块内部管理）
4) 允许依赖 UI 对象（控件属性），但不负责绑定信号（由 main.py 统一绑定）

对外接口清单：
- InteractionController(ui, serial_client, script_service)
  - refresh_ports()
  - toggle_connection()
  - on_slider_changed(finger_num, value)
  - apply_preset(angles)
  - apply_preset_by_name(name)
  - load_script() / save_script()
  - run_script() / stop_script()
  - on_script_finished()
  - close_cleanup()
"""

from __future__ import annotations

from typing import Dict, List, Optional

from PyQt5.QtWidgets import QFileDialog

from serial_module import SerialPortClient
from script_module import ScriptService


class InteractionController:
    """
    用户交互控制器

    说明：
    - ui：来自 ui_module 的纯控件对象（不要求是特定类，只要包含所需属性）
    - serial_client：来自 serial_module 的串口客户端
    - script_service：来自 script_module 的脚本服务（内部管理线程）
    """

    def __init__(self, ui, serial_client: SerialPortClient, script_service: ScriptService):
        self.ui = ui
        self.serial_client = serial_client
        self.script_service = script_service

        self._presets: Dict[str, List[int]] = {
            "张开": [90, 90, 90, 90],
            "握拳": [180, 180, 180, 180],
            "OK手势": [180, 180, 90, 90],
            "指示": [90, 180, 180, 180],
        }

    def refresh_ports(self) -> None:
        """刷新可用串口列表"""
        self.ui.port_combo.clear()
        ports = self.serial_client.scan_ports()
        for port in ports:
            self.ui.port_combo.addItem(port)

    def toggle_connection(self) -> None:
        """连接/断开串口（按钮点击触发）"""
        if self.serial_client.is_connected:
            self.serial_client.disconnect()
            self.ui.connect_btn.setText("连接")
            self.ui.status_bar.showMessage("已断开")
            return

        port = self.ui.port_combo.currentText()
        if not port:
            self.ui.status_bar.showMessage("请选择串口")
            return

        try:
            self.serial_client.connect(port)
            self.ui.connect_btn.setText("断开")
            self.ui.status_bar.showMessage(f"已连接到 {port}")
        except Exception as e:
            self.ui.status_bar.showMessage(f"连接失败: {str(e)}")

    def on_slider_changed(self, finger_num: int, value: int) -> None:
        """
        滑动条数值改变时调用
        :param finger_num: 手指编号 1-4
        :param value: 角度值
        """
        self.ui.finger_labels[finger_num - 1].setText(f"手指{finger_num}: {value}°")
        self.send_finger_command(finger_num, value)

    def send_finger_command(self, finger_num: int, angle: int) -> None:
        """发送单个手指控制指令"""
        if not self.serial_client.is_connected:
            return

        command = f"FINGER{finger_num}:{angle}\n"
        try:
            self.serial_client.send_raw_text(command)
            self.ui.status_bar.showMessage(f"发送指令: {command.strip()}")
        except Exception as e:
            self.ui.status_bar.showMessage(f"发送失败: {str(e)}")

    def apply_preset(self, angles: List[int]) -> None:
        """应用预设动作，批量设置4个手指角度"""
        for i, angle in enumerate(angles):
            self.ui.finger_sliders[i].setValue(angle)

    def apply_preset_by_name(self, name: str) -> None:
        """按预设名称应用动作（供 main 绑定按钮点击）"""
        angles = self._presets.get(name)
        if angles is None:
            return
        self.apply_preset(angles)

    def load_script_from_path(self, file_path: str) -> None:
        """从指定路径加载脚本到编辑区（便于独立测试；load_script() 仍保持原交互行为）"""
        with open(file_path, "r", encoding="utf-8") as f:
            self.ui.script_editor.setText(f.read())
        self.ui.script_log.append(f"已加载脚本: {file_path}")

    def load_script(self) -> None:
        """加载脚本文件到编辑区"""
        file_path, _ = QFileDialog.getOpenFileName(
            self.ui, "选择脚本文件", "", "Text Files (*.txt);;All Files (*)"
        )
        if not file_path:
            return
        try:
            self.load_script_from_path(file_path)
        except Exception as e:
            self.ui.script_log.append(f"加载失败: {str(e)}")

    def save_script_to_path(self, file_path: str) -> None:
        """保存编辑区脚本到指定路径（便于独立测试；save_script() 仍保持原交互行为）"""
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(self.ui.script_editor.toPlainText())
        self.ui.script_log.append(f"已保存脚本: {file_path}")

    def save_script(self) -> None:
        """保存编辑区脚本到文件"""
        file_path, _ = QFileDialog.getSaveFileName(
            self.ui, "保存脚本文件", "", "Text Files (*.txt);;All Files (*)"
        )
        if not file_path:
            return
        try:
            self.save_script_to_path(file_path)
        except Exception as e:
            self.ui.script_log.append(f"保存失败: {str(e)}")

    def run_script(self) -> None:
        """执行脚本"""
        if not self.serial_client.is_connected:
            self.ui.script_log.append("请先连接串口")
            return

        script_text = self.ui.script_editor.toPlainText()
        commands = script_text.split("\n")

        self.ui.script_log.append("开始执行脚本...")
        self.script_service.start(commands)

    def stop_script(self) -> None:
        """停止脚本执行"""
        if self.script_service.is_running:
            self.script_service.stop()
            self.ui.script_log.append("脚本已停止")

    def on_script_finished(self) -> None:
        """脚本执行完成回调"""
        self.ui.script_log.append("脚本执行完成")

    def close_cleanup(self) -> None:
        """
        程序退出清理：
        1) 停止脚本线程并等待结束
        2) 关闭串口连接
        """
        if self.script_service.is_running:
            self.script_service.stop()
            self.script_service.wait()
        if self.serial_client.is_connected:
            self.serial_client.disconnect()


if __name__ == "__main__":
    """
    独立测试脚本：
    1) 模拟串口客户端，验证滑块/预设/文件操作逻辑
    2) 不弹出文件对话框：直接调用 load_script_from_path/save_script_to_path
    """

    import os
    import sys
    import tempfile

    from PyQt5.QtWidgets import QApplication

    from ui_module import FingerControllerUI

    class _MockSerialClient(SerialPortClient):
        def __init__(self):
            self.connected = False
            self.sent: List[str] = []
            self._ports = ["MOCK1", "MOCK2"]

        def scan_ports(self) -> List[str]:
            return list(self._ports)

        @property
        def is_connected(self) -> bool:
            return self.connected

        def connect(self, port: str) -> None:
            if port not in self._ports:
                raise RuntimeError("端口不存在")
            self.connected = True

        def disconnect(self) -> None:
            self.connected = False

        def send_raw_text(self, text: str) -> None:
            self.sent.append(text)

    app = QApplication(sys.argv)
    ui = FingerControllerUI()
    serial_client = _MockSerialClient()
    script_service = ScriptService(serial_client)  # type: ignore[arg-type]
    controller = InteractionController(ui, serial_client, script_service)

    controller.refresh_ports()
    assert ui.port_combo.count() == 2
    ui.port_combo.setCurrentIndex(0)
    controller.toggle_connection()
    assert serial_client.is_connected is True

    controller.on_slider_changed(1, 120)
    assert ui.finger_labels[0].text() == "手指1: 120°"
    assert serial_client.sent[-1] == "FINGER1:120\n"

    controller.apply_preset_by_name("握拳")
    assert all(s.value() == 180 for s in ui.finger_sliders)

    with tempfile.TemporaryDirectory() as d:
        p = os.path.join(d, "test_script.txt")
        ui.script_editor.setText("FINGER1:90\nDELAY:10\n")
        controller.save_script_to_path(p)
        ui.script_editor.setText("")
        controller.load_script_from_path(p)
        assert "FINGER1:90" in ui.script_editor.toPlainText()

    print("interaction_module 交互测试通过")
