#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
手指控制上位机
使用串口控制ESP32-S3上的4个手指
支持自定义预设动作脚本
"""

import sys
import time
import serial
import serial.tools.list_ports
from PyQt5.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, 
    QLabel, QComboBox, QPushButton, QSlider, QStatusBar,
    QFileDialog, QTextEdit, QGroupBox, QGridLayout
)
from PyQt5.QtCore import Qt, QThread, pyqtSignal


class ScriptRunner(QThread):
    """脚本执行线程"""
    log_signal = pyqtSignal(str)
    finished_signal = pyqtSignal()
    
    def __init__(self, serial_port, commands):
        super().__init__()
        self.serial_port = serial_port
        self.commands = commands
        self.running = True
    
    def run(self):
        for cmd in self.commands:
            if not self.running:
                break
            
            cmd = cmd.strip()
            if not cmd or cmd.startswith('#'):
                continue
            
            # 解析命令
            if cmd.startswith('ALL:'):
                self.send_command(cmd)
            elif cmd.startswith('FINGER'):
                self.send_command(cmd)
            elif cmd.startswith('DELAY'):
                try:
                    delay_ms = int(cmd.split(':')[1])
                    self.log_signal.emit(f'等待 {delay_ms}ms')
                    time.sleep(delay_ms / 1000)
                except:
                    self.log_signal.emit(f'无效的DELAY命令: {cmd}')
            else:
                self.log_signal.emit(f'未知命令: {cmd}')
        
        self.finished_signal.emit()
    
    def send_command(self, command):
        if self.serial_port and self.serial_port.is_open:
            try:
                full_cmd = command + '\n'
                self.serial_port.write(full_cmd.encode('utf-8'))
                self.log_signal.emit(f'发送: {command}')
                time.sleep(0.05)  # 小延迟确保指令发送
            except Exception as e:
                self.log_signal.emit(f'发送失败: {str(e)}')
    
    def stop(self):
        self.running = False


class FingerController(QWidget):
    def __init__(self):
        super().__init__()
        self.serial_port = None
        self.script_runner = None
        self.init_ui()
    
    def init_ui(self):
        self.setWindowTitle('手指控制器')
        self.setGeometry(100, 100, 800, 600)
        
        main_layout = QHBoxLayout()
        
        # 左侧控制面板
        left_panel = QVBoxLayout()
        
        # 串口设置
        port_group = QGroupBox('串口设置')
        port_layout = QHBoxLayout()
        port_label = QLabel('串口:')
        self.port_combo = QComboBox()
        self.refresh_ports()
        
        refresh_btn = QPushButton('刷新')
        refresh_btn.clicked.connect(self.refresh_ports)
        
        connect_btn = QPushButton('连接')
        connect_btn.clicked.connect(self.toggle_connection)
        self.connect_btn = connect_btn
        
        port_layout.addWidget(port_label)
        port_layout.addWidget(self.port_combo)
        port_layout.addWidget(refresh_btn)
        port_layout.addWidget(connect_btn)
        port_group.setLayout(port_layout)
        left_panel.addWidget(port_group)
        
        # 手指控制
        finger_group = QGroupBox('手指控制')
        finger_layout = QVBoxLayout()
        
        self.finger_sliders = []
        self.finger_labels = []
        
        for i in range(1, 5):
            finger_hbox = QHBoxLayout()
            finger_label = QLabel(f'手指{i}: 90°')
            self.finger_labels.append(finger_label)
            
            slider = QSlider(Qt.Horizontal)
            slider.setMinimum(90)
            slider.setMaximum(180)
            slider.setValue(90)
            slider.valueChanged.connect(lambda value, idx=i: self.on_slider_changed(idx, value))
            self.finger_sliders.append(slider)
            
            finger_hbox.addWidget(finger_label)
            finger_hbox.addWidget(slider)
            finger_layout.addLayout(finger_hbox)
        
        finger_group.setLayout(finger_layout)
        left_panel.addWidget(finger_group)
        
        # 预设动作按钮
        preset_group = QGroupBox('预设动作')
        preset_layout = QGridLayout()
        
        presets = [
            ('张开', [90, 90, 90, 90]),
            ('握拳', [180, 180, 180, 180]),
            ('OK手势', [180, 180, 90, 90]),
            ('指示', [90, 180, 180, 180]),
        ]
        
        for idx, (name, angles) in enumerate(presets):
            btn = QPushButton(name)
            btn.clicked.connect(lambda _, a=angles: self.apply_preset(a))
            preset_layout.addWidget(btn, idx // 2, idx % 2)
        
        preset_group.setLayout(preset_layout)
        left_panel.addWidget(preset_group)
        
        # 状态栏
        self.status_bar = QStatusBar()
        self.status_bar.showMessage('未连接')
        left_panel.addWidget(self.status_bar)
        
        main_layout.addLayout(left_panel, 1)
        
        # 右侧脚本面板
        right_panel = QVBoxLayout()
        
        script_group = QGroupBox('脚本控制')
        script_layout = QVBoxLayout()
        
        # 脚本编辑区
        self.script_editor = QTextEdit()
        self.script_editor.setPlaceholderText(
            '# 脚本格式示例:\n'
            '# FINGER1:180  # 手指1弯曲\n'
            '# FINGER2:90   # 手指2复位\n'
            '# DELAY:500    # 等待500毫秒\n\n'
            'FINGER1:90\n'
            'FINGER2:90\n'
            'FINGER3:90\n'
            'FINGER4:90\n'
        )
        script_layout.addWidget(self.script_editor)
        
        # 脚本按钮
        script_btn_layout = QHBoxLayout()
        
        load_btn = QPushButton('加载脚本')
        load_btn.clicked.connect(self.load_script)
        
        save_btn = QPushButton('保存脚本')
        save_btn.clicked.connect(self.save_script)
        
        run_btn = QPushButton('执行脚本')
        run_btn.clicked.connect(self.run_script)
        
        stop_btn = QPushButton('停止')
        stop_btn.clicked.connect(self.stop_script)
        
        script_btn_layout.addWidget(load_btn)
        script_btn_layout.addWidget(save_btn)
        script_btn_layout.addWidget(run_btn)
        script_btn_layout.addWidget(stop_btn)
        
        script_layout.addLayout(script_btn_layout)
        
        # 脚本日志
        self.script_log = QTextEdit()
        self.script_log.setReadOnly(True)
        self.script_log.setMaximumHeight(150)
        script_layout.addWidget(QLabel('执行日志:'))
        script_layout.addWidget(self.script_log)
        
        script_group.setLayout(script_layout)
        right_panel.addWidget(script_group)
        
        main_layout.addLayout(right_panel, 1)
        
        self.setLayout(main_layout)
    
    def refresh_ports(self):
        self.port_combo.clear()
        ports = serial.tools.list_ports.comports()
        for port in ports:
            self.port_combo.addItem(port.device)
    
    def toggle_connection(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            self.connect_btn.setText('连接')
            self.status_bar.showMessage('已断开')
        else:
            port = self.port_combo.currentText()
            if not port:
                self.status_bar.showMessage('请选择串口')
                return
            
            try:
                self.serial_port = serial.Serial(
                    port=port,
                    baudrate=115200,
                    timeout=0.1
                )
                self.connect_btn.setText('断开')
                self.status_bar.showMessage(f'已连接到 {port}')
            except Exception as e:
                self.status_bar.showMessage(f'连接失败: {str(e)}')
    
    def on_slider_changed(self, finger_num, value):
        self.finger_labels[finger_num - 1].setText(f'手指{finger_num}: {value}°')
        self.send_finger_command(finger_num, value)
    
    def send_finger_command(self, finger_num, angle):
        if not self.serial_port or not self.serial_port.is_open:
            return
        
        command = f"FINGER{finger_num}:{angle}\n"
        
        try:
            self.serial_port.write(command.encode('utf-8'))
            self.status_bar.showMessage(f'发送指令: {command.strip()}')
        except Exception as e:
            self.status_bar.showMessage(f'发送失败: {str(e)}')
    
    def apply_preset(self, angles):
        """应用预设动作"""
        for i, angle in enumerate(angles):
            self.finger_sliders[i].setValue(angle)
    
    def load_script(self):
        """加载脚本文件"""
        file_path, _ = QFileDialog.getOpenFileName(
            self, '选择脚本文件', '', 'Text Files (*.txt);;All Files (*)'
        )
        if file_path:
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    self.script_editor.setText(f.read())
                self.script_log.append(f'已加载脚本: {file_path}')
            except Exception as e:
                self.script_log.append(f'加载失败: {str(e)}')
    
    def save_script(self):
        """保存脚本文件"""
        file_path, _ = QFileDialog.getSaveFileName(
            self, '保存脚本文件', '', 'Text Files (*.txt);;All Files (*)'
        )
        if file_path:
            try:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(self.script_editor.toPlainText())
                self.script_log.append(f'已保存脚本: {file_path}')
            except Exception as e:
                self.script_log.append(f'保存失败: {str(e)}')
    
    def run_script(self):
        """执行脚本"""
        if not self.serial_port or not self.serial_port.is_open:
            self.script_log.append('请先连接串口')
            return
        
        script_text = self.script_editor.toPlainText()
        commands = script_text.split('\n')
        
        self.script_log.append('开始执行脚本...')
        
        self.script_runner = ScriptRunner(self.serial_port, commands)
        self.script_runner.log_signal.connect(self.script_log.append)
        self.script_runner.finished_signal.connect(self.on_script_finished)
        self.script_runner.start()
    
    def stop_script(self):
        """停止脚本执行"""
        if self.script_runner and self.script_runner.isRunning():
            self.script_runner.stop()
            self.script_log.append('脚本已停止')
    
    def on_script_finished(self):
        """脚本执行完成"""
        self.script_log.append('脚本执行完成')
    
    def closeEvent(self, event):
        if self.script_runner and self.script_runner.isRunning():
            self.script_runner.stop()
            self.script_runner.wait()
        
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        event.accept()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = FingerController()
    window.show()
    sys.exit(app.exec_())
