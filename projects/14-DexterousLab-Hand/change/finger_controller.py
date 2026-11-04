#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
手指控制上位机
使用串口控制ESP32-S3上的4个手指
支持自定义预设动作脚本
"""

# 导入系统模块，用于程序退出、命令行参数等
import sys
# 导入时间模块，用于延时操作
import time
# 导入串口通信库
import serial
# 导入串口工具，用于扫描电脑上的串口端口
import serial.tools.list_ports

# 导入PyQt5所有需要的UI组件
# QApplication：应用程序对象
# QWidget：基础窗口控件
# QVBoxLayout/QHBoxLayout：垂直/水平布局管理器
# QLabel：文本标签
# QComboBox：下拉选择框
# QPushButton：按钮
# QSlider：滑动条
# QStatusBar：状态栏
# QFileDialog：文件对话框（打开/保存）
# QTextEdit：文本编辑框
# QGroupBox：分组框（带标题的容器）
# QGridLayout：网格布局
from PyQt5.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, 
    QLabel, QComboBox, QPushButton, QSlider, QStatusBar,
    QFileDialog, QTextEdit, QGroupBox, QGridLayout
)
# Qt：存放常量（如水平方向）
# QThread：线程类，用于后台执行任务
# pyqtSignal：信号，用于线程和主线程通信
from PyQt5.QtCore import Qt, QThread, pyqtSignal


class ScriptRunner(QThread):
    """
    脚本执行线程类
    作用：在后台执行脚本指令，不卡住UI界面
    继承自QThread，重写run()方法实现线程逻辑
    """
    # 定义信号：用于向主线程发送日志信息
    log_signal = pyqtSignal(str)
    # 定义信号：脚本执行完成时发送
    finished_signal = pyqtSignal()
    
    def __init__(self, serial_port, commands):
        """
        构造函数
        :param serial_port: 串口对象，用于发送指令
        :param commands: 脚本指令列表
        """
        super().__init__()  # 调用父类构造函数
        self.serial_port = serial_port  # 保存串口对象
        self.commands = commands        # 保存要执行的指令
        self.running = True             # 线程运行标志位，控制线程启停
    
    def run(self):
        """
        线程入口函数
        自动被调用，遍历执行所有脚本指令
        """
        # 遍历每一条指令
        for cmd in self.commands:
            # 如果停止标志为False，退出循环
            if not self.running:
                break
            
            # 去除指令首尾空白字符
            cmd = cmd.strip()
            # 如果是空行或者#开头的注释行，跳过不执行
            if not cmd or cmd.startswith('#'):
                continue
            
            # 解析并执行不同类型的指令
            if cmd.startswith('ALL:'):
                # 控制所有手指的指令
                self.send_command(cmd)
            elif cmd.startswith('FINGER'):
                # 控制单个手指的指令
                self.send_command(cmd)
            elif cmd.startswith('DELAY'):
                # 延时指令
                try:
                    # 分割指令，取出延时毫秒数
                    delay_ms = int(cmd.split(':')[1])
                    # 发送日志信号
                    self.log_signal.emit(f'等待 {delay_ms}ms')
                    # 执行延时（sleep单位是秒，所以除以1000）
                    time.sleep(delay_ms / 1000)
                except:
                    # 指令格式错误时输出日志
                    self.log_signal.emit(f'无效的DELAY命令: {cmd}')
            else:
                # 不认识的指令
                self.log_signal.emit(f'未知命令: {cmd}')
        
        # 所有指令执行完毕，发送完成信号
        self.finished_signal.emit()
    
    def send_command(self, command):
        """
        发送串口指令函数
        :param command: 要发送的指令字符串
        """
        # 判断串口是否已打开
        if self.serial_port and self.serial_port.is_open:
            try:
                # 指令末尾加换行符，作为结束标志
                full_cmd = command + '\n'
                # 通过串口发送UTF-8编码的指令
                self.serial_port.write(full_cmd.encode('utf-8'))
                # 发送日志
                self.log_signal.emit(f'发送: {command}')
                # 短暂延时，保证硬件能处理指令
                time.sleep(0.05)
            except Exception as e:
                # 发送失败时输出错误信息
                self.log_signal.emit(f'发送失败: {str(e)}')
    
    def stop(self):
        """停止线程执行"""
        self.running = False


class FingerController(QWidget):
    """
    手指控制器主窗口类
    整个GUI界面和逻辑都在这里
    """
    def __init__(self):
        super().__init__()
        self.serial_port = None      # 串口对象，初始为空
        self.script_runner = None    # 脚本执行线程，初始为空
        self.init_ui()              # 初始化界面
    
    def init_ui(self):
        """初始化所有UI界面、布局、控件"""
        self.setWindowTitle('我的手指控制器')  # 窗口标题
        self.setGeometry(100, 100, 800, 600)   # 窗口位置和大小（x,y,宽,高）
        
        # 主布局：水平布局，左边控制面板 + 右边脚本面板
        main_layout = QHBoxLayout()
        
        # ===================== 左侧控制面板 =====================
        left_panel = QVBoxLayout()
        
        # 串口设置分组
        port_group = QGroupBox('串口设置')
        port_layout = QHBoxLayout()
        port_label = QLabel('串口:')
        self.port_combo = QComboBox()    # 串口下拉选择框
        self.refresh_ports()             # 初始化时刷新串口列表
        
        refresh_btn = QPushButton('刷新')
        refresh_btn.clicked.connect(self.refresh_ports)  # 点击刷新串口
        
        connect_btn = QPushButton('连接')
        connect_btn.clicked.connect(self.toggle_connection)  # 点击连接/断开
        self.connect_btn = connect_btn
        
        # 把控件加入布局
        port_layout.addWidget(port_label)
        port_layout.addWidget(self.port_combo)
        port_layout.addWidget(refresh_btn)
        port_layout.addWidget(connect_btn)
        port_group.setLayout(port_layout)
        left_panel.addWidget(port_group)
        
        # 手指控制分组
        finger_group = QGroupBox('手指控制')
        finger_layout = QVBoxLayout()
        
        self.finger_sliders = []   # 保存4个手指的滑动条
        self.finger_labels = []    # 保存4个手指的角度显示标签
        
        # 循环创建4个手指的控制条
        for i in range(1, 5):
            finger_hbox = QHBoxLayout()
            finger_label = QLabel(f'手指{i}: 90°')
            self.finger_labels.append(finger_label)
            
            # 创建水平滑动条
            slider = QSlider(Qt.Horizontal)
            slider.setMinimum(90)     # 最小值90度
            slider.setMaximum(180)    # 最大值180度
            slider.setValue(90)       # 默认值90度
            # 滑动条值变化时触发函数
            slider.valueChanged.connect(lambda value, idx=i: self.on_slider_changed(idx, value))
            self.finger_sliders.append(slider)
            
            finger_hbox.addWidget(finger_label)
            finger_hbox.addWidget(slider)
            finger_layout.addLayout(finger_hbox)
        
        finger_group.setLayout(finger_layout)
        left_panel.addWidget(finger_group)
        
        # 预设动作按钮分组
        preset_group = QGroupBox('预设动作')
        preset_layout = QGridLayout()
        
        # 预设动作：名称 + 4个手指的角度
        presets = [
            ('张开', [90, 90, 90, 90]),
            ('握拳', [180, 180, 180, 180]),
            ('OK手势', [180, 180, 90, 90]),
            ('指示', [90, 180, 180, 180]),
        ]
        
        # 循环创建预设按钮
        for idx, (name, angles) in enumerate(presets):
            btn = QPushButton(name)
            # 点击按钮应用对应角度
            btn.clicked.connect(lambda _, a=angles: self.apply_preset(a))
            # 网格布局：两行两列
            preset_layout.addWidget(btn, idx // 2, idx % 2)
        
        preset_group.setLayout(preset_layout)
        left_panel.addWidget(preset_group)
        
        # 状态栏：显示提示信息
        self.status_bar = QStatusBar()
        self.status_bar.showMessage('未连接')
        left_panel.addWidget(self.status_bar)
        
        # 把左侧面板加入主布局
        main_layout.addLayout(left_panel, 1)
        
        # ===================== 右侧脚本面板 =====================
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
        
        # 脚本操作按钮
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
        
        # 脚本执行日志（只读）
        self.script_log = QTextEdit()
        self.script_log.setReadOnly(True)
        self.script_log.setMaximumHeight(150)
        script_layout.addWidget(QLabel('执行日志:'))
        script_layout.addWidget(self.script_log)
        
        script_group.setLayout(script_layout)
        right_panel.addWidget(script_group)
        
        # 右侧加入主布局
        main_layout.addLayout(right_panel, 1)
        
        # 设置窗口主布局
        self.setLayout(main_layout)
    
    def refresh_ports(self):
        """刷新可用串口列表"""
        self.port_combo.clear()  # 清空下拉框
        ports = serial.tools.list_ports.comports()  # 获取所有串口
        for port in ports:
            self.port_combo.addItem(port.device)  # 添加到下拉框
    
    def toggle_connection(self):
        """连接/断开串口（按钮点击触发）"""
        # 如果串口已打开，则关闭
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            self.connect_btn.setText('连接')
            self.status_bar.showMessage('已断开')
        else:
            # 否则尝试连接
            port = self.port_combo.currentText()
            if not port:
                self.status_bar.showMessage('请选择串口')
                return
            
            try:
                # 创建串口对象，波特率115200
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
        """
        滑动条数值改变时调用
        :param finger_num: 手指编号 1-4
        :param value: 角度值
        """
        # 更新标签显示
        self.finger_labels[finger_num - 1].setText(f'手指{finger_num}: {value}°')
        # 发送串口指令
        self.send_finger_command(finger_num, value)
    
    def send_finger_command(self, finger_num, angle):
        """发送单个手指控制指令"""
        if not self.serial_port or not self.serial_port.is_open:
            return
        
        # 拼接指令格式：FINGER1:90
        command = f"FINGER{finger_num}:{angle}\n"
        
        try:
            self.serial_port.write(command.encode('utf-8'))
            self.status_bar.showMessage(f'发送指令: {command.strip()}')
        except Exception as e:
            self.status_bar.showMessage(f'发送失败: {str(e)}')
    
    def apply_preset(self, angles):
        """应用预设动作，批量设置4个手指角度"""
        for i, angle in enumerate(angles):
            self.finger_sliders[i].setValue(angle)
    
    def load_script(self):
        """加载脚本文件到编辑区"""
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
        """保存编辑区脚本到文件"""
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
        
        # 获取编辑区所有文本，按行分割成指令列表
        script_text = self.script_editor.toPlainText()
        commands = script_text.split('\n')
        
        self.script_log.append('开始执行脚本...')
        
        # 创建线程并启动
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
        """脚本执行完成回调"""
        self.script_log.append('脚本执行完成')
    
    def closeEvent(self, event):
        """
        窗口关闭事件
        关闭前自动停止线程、关闭串口
        """
        # 如果线程在运行，先停止
        if self.script_runner and self.script_runner.isRunning():
            self.script_runner.stop()
            self.script_runner.wait()
        
        # 如果串口打开，关闭
        if self.serial_port and self.serial_port.is_open():
            self.serial_port.close()
        event.accept()


# 程序入口
if __name__ == '__main__':
    app = QApplication(sys.argv)   # 创建Qt应用
    window = FingerController()    # 创建主窗口
    window.show()                  # 显示窗口
    sys.exit(app.exec_())          # 运行消息循环