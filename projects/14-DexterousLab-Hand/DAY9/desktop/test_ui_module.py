#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
test_ui_module.py（UI测试模块）

模块职责：
1) 提供直观的UI界面，用于测试三个模块的功能
2) 包含串口模块、交互模块和脚本模块的测试用例
3) 显示测试结果和详细日志
4) 支持生成测试报告
"""

from __future__ import annotations

import os
import sys
import tempfile
from typing import List

from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QTextEdit, QLabel, QGroupBox, QTabWidget, QGridLayout,
    QComboBox, QStatusBar
)
from PyQt5.QtCore import Qt, pyqtSignal

from serial_module import SerialPortClient, SerialConfig
from script_module import ScriptService
from interaction_module import InteractionController


class TestSerialClient(SerialPortClient):
    """测试用串口客户端，记录所有操作"""
    def __init__(self):
        super().__init__(serial_factory=self._mock_serial_factory)
        self.operations: List[str] = []
        self.mock_serial = None
    
    def _mock_serial_factory(self, port: str, baudrate: int, timeout: float):
        self.operations.append(f"connect: {port}, baudrate={baudrate}, timeout={timeout}")
        self.mock_serial = _MockSerial(port, baudrate, timeout)
        return self.mock_serial
    
    def scan_ports(self) -> List[str]:
        ports = super().scan_ports()
        self.operations.append(f"scan_ports: {ports}")
        return ports
    
    def connect(self, port: str) -> None:
        super().connect(port)
        self.operations.append(f"connect: {port}")
    
    def disconnect(self) -> None:
        super().disconnect()
        self.operations.append("disconnect")
    
    def send_raw_text(self, text: str) -> None:
        super().send_raw_text(text)
        if self.mock_serial:
            self.operations.append(f"send_raw: {text!r}")
    
    def send_command(self, command: str) -> None:
        super().send_command(command)
        if self.mock_serial:
            self.operations.append(f"send_command: {command!r}")


class _MockSerial:
    """模拟串口对象"""
    def __init__(self, port: str, baudrate: int, timeout: float):
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


class TestUI(QMainWindow):
    """测试界面"""
    def __init__(self):
        super().__init__()
        self.setWindowTitle("灵巧手模块测试工具")
        self.setGeometry(100, 100, 1000, 700)
        
        # 初始化测试客户端
        self.serial_client = TestSerialClient()
        self.script_service = ScriptService(self.serial_client)
        
        # 创建主窗口部件
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # 主布局
        main_layout = QVBoxLayout(central_widget)
        
        # 标签页
        self.tab_widget = QTabWidget()
        main_layout.addWidget(self.tab_widget)
        
        # 串口模块测试标签页
        self.serial_tab = QWidget()
        self.tab_widget.addTab(self.serial_tab, "串口模块测试")
        self.setup_serial_tab()
        
        # 交互模块测试标签页
        self.interaction_tab = QWidget()
        self.tab_widget.addTab(self.interaction_tab, "交互模块测试")
        self.setup_interaction_tab()
        
        # 脚本模块测试标签页
        self.script_tab = QWidget()
        self.tab_widget.addTab(self.script_tab, "脚本模块测试")
        self.setup_script_tab()
        
        # 测试结果标签页
        self.result_tab = QWidget()
        self.tab_widget.addTab(self.result_tab, "测试结果")
        self.setup_result_tab()
        
        # 状态栏
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("就绪")
        
        # 连接信号
        self.script_service.log_signal.connect(self.append_script_log)
        self.script_service.finished_signal.connect(self.on_script_finished)
    
    def setup_serial_tab(self):
        """设置串口模块测试界面"""
        layout = QVBoxLayout(self.serial_tab)
        
        # 端口扫描区域
        scan_group = QGroupBox("端口扫描测试")
        scan_layout = QHBoxLayout()
        self.scan_button = QPushButton("扫描端口")
        self.scan_button.clicked.connect(self.test_scan_ports)
        self.port_list = QTextEdit()
        self.port_list.setReadOnly(True)
        scan_layout.addWidget(self.scan_button)
        scan_layout.addWidget(self.port_list)
        scan_group.setLayout(scan_layout)
        layout.addWidget(scan_group)
        
        # 连接测试区域
        connect_group = QGroupBox("连接测试")
        connect_layout = QGridLayout()
        connect_layout.addWidget(QLabel("端口:"), 0, 0)
        self.port_combo = QComboBox()
        connect_layout.addWidget(self.port_combo, 0, 1)
        self.connect_button = QPushButton("连接")
        self.connect_button.clicked.connect(self.test_connect)
        connect_layout.addWidget(self.connect_button, 1, 0)
        self.disconnect_button = QPushButton("断开")
        self.disconnect_button.clicked.connect(self.test_disconnect)
        connect_layout.addWidget(self.disconnect_button, 1, 1)
        connect_group.setLayout(connect_layout)
        layout.addWidget(connect_group)
        
        # 发送测试区域
        send_group = QGroupBox("发送测试")
        send_layout = QVBoxLayout()
        self.send_text = QTextEdit()
        self.send_text.setPlainText("FINGER1:90")
        send_layout.addWidget(self.send_text)
        send_buttons = QHBoxLayout()
        self.send_raw_button = QPushButton("发送原始文本")
        self.send_raw_button.clicked.connect(self.test_send_raw)
        self.send_command_button = QPushButton("发送命令")
        self.send_command_button.clicked.connect(self.test_send_command)
        send_buttons.addWidget(self.send_raw_button)
        send_buttons.addWidget(self.send_command_button)
        send_layout.addLayout(send_buttons)
        send_group.setLayout(send_layout)
        layout.addWidget(send_group)
        
        # 操作日志
        log_group = QGroupBox("操作日志")
        log_layout = QVBoxLayout()
        self.serial_log = QTextEdit()
        self.serial_log.setReadOnly(True)
        log_layout.addWidget(self.serial_log)
        log_group.setLayout(log_layout)
        layout.addWidget(log_group)
    
    def setup_interaction_tab(self):
        """设置交互模块测试界面"""
        layout = QVBoxLayout(self.interaction_tab)
        
        # 模拟UI对象
        class MockUI:
            def __init__(self):
                self.port_combo = QComboBox()
                self.connect_btn = QPushButton("连接")
                self.status_bar = QStatusBar()
                from PyQt5.QtWidgets import QSlider
                self.finger_sliders = [QSlider(Qt.Horizontal) for i in range(4)]
                for slider in self.finger_sliders:
                    slider.setMinimum(0)
                    slider.setMaximum(180)
                    slider.setValue(90)
                self.finger_labels = [QLabel(f"手指{i+1}: 90°") for i in range(4)]
                self.script_editor = QTextEdit()
                self.script_log = QTextEdit()
                self.script_log.setReadOnly(True)
        
        self.mock_ui = MockUI()
        self.interaction_controller = InteractionController(
            self.mock_ui, self.serial_client, self.script_service
        )
        
        # 滑块测试
        slider_group = QGroupBox("滑块控制测试")
        slider_layout = QVBoxLayout()
        for i in range(4):
            h_layout = QHBoxLayout()
            h_layout.addWidget(self.mock_ui.finger_labels[i])
            h_layout.addWidget(self.mock_ui.finger_sliders[i])
            slider_layout.addLayout(h_layout)
        test_slider_button = QPushButton("测试滑块控制")
        test_slider_button.clicked.connect(self.test_slider)
        slider_layout.addWidget(test_slider_button)
        slider_group.setLayout(slider_layout)
        layout.addWidget(slider_group)
        
        # 预设测试
        preset_group = QGroupBox("预设动作测试")
        preset_layout = QHBoxLayout()
        presets = ["张开", "握拳", "OK手势", "指示"]
        for preset in presets:
            button = QPushButton(preset)
            button.clicked.connect(lambda _, p=preset: self.test_preset(p))
            preset_layout.addWidget(button)
        preset_group.setLayout(preset_layout)
        layout.addWidget(preset_group)
        
        # 文件操作测试
        file_group = QGroupBox("文件操作测试")
        file_layout = QVBoxLayout()
        self.file_editor = QTextEdit()
        self.file_editor.setPlainText("FINGER1:90\nDELAY:100\nFINGER2:180")
        file_layout.addWidget(self.file_editor)
        file_buttons = QHBoxLayout()
        save_button = QPushButton("保存测试")
        save_button.clicked.connect(self.test_save)
        load_button = QPushButton("加载测试")
        load_button.clicked.connect(self.test_load)
        file_buttons.addWidget(save_button)
        file_buttons.addWidget(load_button)
        file_layout.addLayout(file_buttons)
        file_group.setLayout(file_layout)
        layout.addWidget(file_group)
        
        # 脚本日志
        log_group = QGroupBox("脚本日志")
        log_layout = QVBoxLayout()
        log_layout.addWidget(self.mock_ui.script_log)
        log_group.setLayout(log_layout)
        layout.addWidget(log_group)
    
    def setup_script_tab(self):
        """设置脚本模块测试界面"""
        layout = QVBoxLayout(self.script_tab)
        
        # 脚本编辑器
        editor_group = QGroupBox("脚本编辑器")
        editor_layout = QVBoxLayout()
        self.script_editor = QTextEdit()
        self.script_editor.setPlainText("# 测试脚本\nFINGER1:90\nDELAY:100\nFINGER2:180\nDELAY:100\nALL:90")
        editor_layout.addWidget(self.script_editor)
        editor_group.setLayout(editor_layout)
        layout.addWidget(editor_group)
        
        # 控制按钮
        control_group = QGroupBox("控制")
        control_layout = QHBoxLayout()
        self.run_button = QPushButton("运行脚本")
        self.run_button.clicked.connect(self.test_run_script)
        self.stop_button = QPushButton("停止脚本")
        self.stop_button.clicked.connect(self.test_stop_script)
        control_layout.addWidget(self.run_button)
        control_layout.addWidget(self.stop_button)
        control_group.setLayout(control_layout)
        layout.addWidget(control_group)
        
        # 脚本日志
        log_group = QGroupBox("脚本日志")
        log_layout = QVBoxLayout()
        self.script_log = QTextEdit()
        self.script_log.setReadOnly(True)
        log_layout.addWidget(self.script_log)
        log_group.setLayout(log_layout)
        layout.addWidget(log_group)
    
    def setup_result_tab(self):
        """设置测试结果界面"""
        layout = QVBoxLayout(self.result_tab)
        
        # 测试结果显示
        result_group = QGroupBox("测试结果")
        result_layout = QVBoxLayout()
        self.result_text = QTextEdit()
        self.result_text.setReadOnly(True)
        result_layout.addWidget(self.result_text)
        result_group.setLayout(result_layout)
        layout.addWidget(result_group)
        
        # 操作按钮
        button_group = QHBoxLayout()
        self.generate_report_button = QPushButton("生成测试报告")
        self.generate_report_button.clicked.connect(self.generate_report)
        self.clear_result_button = QPushButton("清空结果")
        self.clear_result_button.clicked.connect(self.clear_result)
        button_group.addWidget(self.generate_report_button)
        button_group.addWidget(self.clear_result_button)
        layout.addLayout(button_group)
    
    def test_scan_ports(self):
        """测试端口扫描"""
        ports = self.serial_client.scan_ports()
        self.port_list.setPlainText("\n".join(ports))
        self.port_combo.clear()
        for port in ports:
            self.port_combo.addItem(port)
        self.update_serial_log("端口扫描测试完成")
        self.status_bar.showMessage("端口扫描测试完成")
        self.append_result("✅ 串口模块 - 端口扫描测试通过")
    
    def test_connect(self):
        """测试连接"""
        port = self.port_combo.currentText()
        if not port:
            self.update_serial_log("请选择端口")
            return
        try:
            self.serial_client.connect(port)
            self.update_serial_log(f"连接到 {port} 成功")
            self.status_bar.showMessage(f"连接到 {port} 成功")
            self.append_result(f"✅ 串口模块 - 连接测试通过: {port}")
        except Exception as e:
            self.update_serial_log(f"连接失败: {str(e)}")
            self.status_bar.showMessage(f"连接失败: {str(e)}")
            self.append_result(f"❌ 串口模块 - 连接测试失败: {str(e)}")
    
    def test_disconnect(self):
        """测试断开连接"""
        self.serial_client.disconnect()
        self.update_serial_log("断开连接成功")
        self.status_bar.showMessage("断开连接成功")
        self.append_result("✅ 串口模块 - 断开连接测试通过")
    
    def test_send_raw(self):
        """测试发送原始文本"""
        text = self.send_text.toPlainText()
        try:
            self.serial_client.send_raw_text(text)
            self.update_serial_log(f"发送原始文本: {text!r}")
            self.status_bar.showMessage("发送成功")
            self.append_result(f"✅ 串口模块 - 发送原始文本测试通过: {text!r}")
        except Exception as e:
            self.update_serial_log(f"发送失败: {str(e)}")
            self.status_bar.showMessage(f"发送失败: {str(e)}")
            self.append_result(f"❌ 串口模块 - 发送原始文本测试失败: {str(e)}")
    
    def test_send_command(self):
        """测试发送命令"""
        command = self.send_text.toPlainText()
        try:
            self.serial_client.send_command(command)
            self.update_serial_log(f"发送命令: {command!r}")
            self.status_bar.showMessage("发送成功")
            self.append_result(f"✅ 串口模块 - 发送命令测试通过: {command!r}")
        except Exception as e:
            self.update_serial_log(f"发送失败: {str(e)}")
            self.status_bar.showMessage(f"发送失败: {str(e)}")
            self.append_result(f"❌ 串口模块 - 发送命令测试失败: {str(e)}")
    
    def test_slider(self):
        """测试滑块控制"""
        for i in range(1, 5):
            angle = 90 + i * 15
            self.interaction_controller.on_slider_changed(i, angle)
        self.update_serial_log("滑块控制测试完成")
        self.status_bar.showMessage("滑块控制测试完成")
        self.append_result("✅ 交互模块 - 滑块控制测试通过")
    
    def test_preset(self, preset_name):
        """测试预设动作"""
        self.interaction_controller.apply_preset_by_name(preset_name)
        self.update_serial_log(f"预设动作测试完成: {preset_name}")
        self.status_bar.showMessage(f"预设动作测试完成: {preset_name}")
        self.append_result(f"✅ 交互模块 - 预设动作测试通过: {preset_name}")
    
    def test_save(self):
        """测试保存脚本"""
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "test_script.txt")
            self.mock_ui.script_editor.setText(self.file_editor.toPlainText())
            self.interaction_controller.save_script_to_path(p)
            self.update_serial_log(f"脚本保存测试完成: {p}")
            self.status_bar.showMessage("脚本保存测试完成")
            self.append_result(f"✅ 交互模块 - 脚本保存测试通过")
    
    def test_load(self):
        """测试加载脚本"""
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "test_script.txt")
            with open(p, "w", encoding="utf-8") as f:
                f.write(self.file_editor.toPlainText())
            self.interaction_controller.load_script_from_path(p)
            self.update_serial_log(f"脚本加载测试完成: {p}")
            self.status_bar.showMessage("脚本加载测试完成")
            self.append_result(f"✅ 交互模块 - 脚本加载测试通过")
    
    def test_run_script(self):
        """测试运行脚本"""
        script_text = self.script_editor.toPlainText()
        commands = script_text.split("\n")
        self.script_service.start(commands)
        self.update_serial_log("脚本运行测试开始")
        self.status_bar.showMessage("脚本运行测试开始")
    
    def test_stop_script(self):
        """测试停止脚本"""
        self.script_service.stop()
        self.update_serial_log("脚本停止测试完成")
        self.status_bar.showMessage("脚本停止测试完成")
        self.append_result("✅ 脚本模块 - 脚本停止测试通过")
    
    def on_script_finished(self):
        """脚本执行完成回调"""
        self.update_serial_log("脚本执行完成")
        self.status_bar.showMessage("脚本执行完成")
        self.append_result("✅ 脚本模块 - 脚本执行测试通过")
    
    def append_script_log(self, log):
        """添加脚本日志"""
        self.script_log.append(log)
        self.mock_ui.script_log.append(log)
    
    def update_serial_log(self, message):
        """更新串口日志"""
        self.serial_log.append(message)
        # 显示操作记录
        if self.serial_client.operations:
            last_op = self.serial_client.operations[-1]
            self.serial_log.append(f"操作: {last_op}")
    
    def append_result(self, message):
        """添加测试结果"""
        self.result_text.append(message)
    
    def generate_report(self):
        """生成测试报告"""
        report = "# 灵巧手模块测试报告\n\n"
        report += "## 测试结果\n"
        report += self.result_text.toPlainText()
        report += "\n\n## 串口操作记录\n"
        report += "\n".join(self.serial_client.operations)
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.md', delete=False, encoding='utf-8') as f:
            f.write(report)
            temp_file = f.name
        
        self.update_serial_log(f"测试报告已生成: {temp_file}")
        self.status_bar.showMessage(f"测试报告已生成: {temp_file}")
        self.append_result(f"📄 测试报告已生成: {temp_file}")
    
    def clear_result(self):
        """清空测试结果"""
        self.result_text.clear()
        self.serial_log.clear()
        self.script_log.clear()
        self.mock_ui.script_log.clear()
        self.serial_client.operations.clear()
        self.status_bar.showMessage("测试结果已清空")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    test_ui = TestUI()
    test_ui.show()
    sys.exit(app.exec_())
