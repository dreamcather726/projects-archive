import sys
import time
import cv2
import numpy as np
import mediapipe as mp
import serial
import serial.tools.list_ports
from PyQt5.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QTabWidget,
    QLabel, QComboBox, QPushButton, QSlider, QStatusBar,
    QFileDialog, QTextEdit, QGroupBox, QGridLayout, QFrame, QLineEdit
)
from PyQt5.QtCore import Qt, QThread, pyqtSignal, QTimer
from PyQt5.QtGui import QImage, QPixmap

# MediaPipe 初始化
mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles
mp_hands = mp.solutions.hands

# ====================== 手部追踪功能 ======================

def get_finger_bend(landmarks, tip_idx, mcp_idx):
    """计算单个手指的弯曲程度（0~1 数值越小越弯）"""
    tip = np.array([landmarks.landmark[tip_idx].x, landmarks.landmark[tip_idx].y])
    mcp = np.array([landmarks.landmark[mcp_idx].x, landmarks.landmark[mcp_idx].y])
    dist = np.linalg.norm(tip - mcp)
    return dist

def map_bend_to_angle(bend_value, finger_type):
    """将弯曲度映射到90-180范围
    90: 全部伸直
    180: 全弯曲
    """
    # 为每个手指设置不同的伸直和弯曲阈值
    finger_thresholds = {
        'thumb': {'straight': 0.11, 'bend': 0.08},    # 拇指伸直和弯曲阈值
        'index': {'straight': 0.24, 'bend': 0.09},    # 食指伸直和弯曲阈值
        'middle': {'straight': 0.25, 'bend': 0.10},   # 中指伸直和弯曲阈值
        'ring': {'straight': 0.24, 'bend': 0.10},     # 无名指伸直和弯曲阈值
        'pinky': {'straight': 0.17, 'bend': 0.08}     # 小指伸直和弯曲阈值
    }
    
    # 获取对应手指的阈值
    thresholds = finger_thresholds.get(finger_type, {'straight': 0.11, 'bend': 0.10})
    straight_threshold = thresholds['straight']
    bend_threshold = thresholds['bend']
    
    if bend_value >= straight_threshold:
        return 90
    elif bend_value <= bend_threshold:
        return 180
    else:
        # 中间值线性映射
        return int(90 + (180 - 90) * (straight_threshold - bend_value) / (straight_threshold - bend_threshold))

# ====================== 脚本执行线程 ======================

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

# ====================== 手部追踪线程 ======================

class HandTrackingThread(QThread):
    """手部追踪线程"""
    frame_signal = pyqtSignal(QImage)
    data_signal = pyqtSignal(dict)
    #error_signal:PyQt5 的信号类，(str) 表示该信号只能传递字符串类型的参数
    #传入HandTrackingThread类的参数（视频流地址，如http://192.168.1.154:81/stream或本地摄像头0）；
    error_signal = pyqtSignal(str)
    
    def __init__(self, stream_url):
        super().__init__()
        self.running = True
        self.stream_url = stream_url#把外部传入的视频流地址保存到线程内部
    
    def run(self):
        # 线程run方法中读取该属性，用于打开视频流
        cap = cv2.VideoCapture(self.stream_url)
        ##########
        if not cap.isOpened():#线程内发射错误信号
            self.error_signal.emit(f'无法打开视频流: {self.stream_url}')
            return
        
        with mp_hands.Hands(
            model_complexity=0,
            min_detection_confidence=0.5,
            min_tracking_confidence=0.5) as hands:
            
            while self.running:
                ret, frame = cap.read()
                if not ret:
                    #使用self.stream_url打开视频流失败时，发射错误信号
                    self.error_signal.emit(f'读取视频帧失败: {self.stream_url}')
                    break
                
                frame = cv2.flip(frame, 1)
                frame.flags.writeable = False
                frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                results = hands.process(frame)
                frame.flags.writeable = True
                frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
                
                finger_data = {}
                
                if results.multi_hand_landmarks and results.multi_handedness:
                    for idx, handedness in enumerate(results.multi_handedness):
                        hand_type = handedness.classification[0].label  # Left / Right
                        landmarks = results.multi_hand_landmarks[idx]
                        
                        # 计算每个手指的弯曲度
                        thumb_bend   = get_finger_bend(landmarks, mp_hands.HandLandmark.THUMB_TIP, mp_hands.HandLandmark.THUMB_MCP)
                        index_bend   = get_finger_bend(landmarks, mp_hands.HandLandmark.INDEX_FINGER_TIP, mp_hands.HandLandmark.INDEX_FINGER_MCP)
                        middle_bend  = get_finger_bend(landmarks, mp_hands.HandLandmark.MIDDLE_FINGER_TIP, mp_hands.HandLandmark.MIDDLE_FINGER_MCP)
                        ring_bend    = get_finger_bend(landmarks, mp_hands.HandLandmark.RING_FINGER_TIP, mp_hands.HandLandmark.RING_FINGER_MCP)
                        pinky_bend   = get_finger_bend(landmarks, mp_hands.HandLandmark.PINKY_TIP, mp_hands.HandLandmark.PINKY_MCP)
                        
                        # 映射到角度
                        thumb_angle   = map_bend_to_angle(thumb_bend, 'thumb')
                        index_angle   = map_bend_to_angle(index_bend, 'index')
                        middle_angle  = map_bend_to_angle(middle_bend, 'middle')
                        ring_angle    = map_bend_to_angle(ring_bend, 'ring')
                        pinky_angle   = map_bend_to_angle(pinky_bend, 'pinky')
                        
                        # 存储数据
                        finger_data[hand_type] = {
                            'thumb': {'bend': thumb_bend, 'angle': thumb_angle},
                            'index': {'bend': index_bend, 'angle': index_angle},
                            'middle': {'bend': middle_bend, 'angle': middle_angle},
                            'ring': {'bend': ring_bend, 'angle': ring_angle},
                            'pinky': {'bend': pinky_bend, 'angle': pinky_angle}
                        }
                        
                        # 绘制手部
                        mp_drawing.draw_landmarks(
                            frame,
                            landmarks,
                            mp_hands.HAND_CONNECTIONS,
                            mp_drawing_styles.get_default_hand_landmarks_style(),
                            mp_drawing_styles.get_default_hand_connections_style())
                
                # 转换为QImage
                h, w, ch = frame.shape
                bytes_per_line = ch * w
                qt_image = QImage(frame.data, w, h, bytes_per_line, QImage.Format_BGR888)
                self.frame_signal.emit(qt_image)
                
                # 发送手指数据
                if finger_data:
                    self.data_signal.emit(finger_data)
        
        cap.release()
    
    def stop(self):
        self.running = False

# ====================== 主应用 ======================

class MainApplication(QWidget):
    def __init__(self):
        super().__init__()
        self.serial_port = None
        self.script_runner = None
        self.hand_tracking_thread = None
        # 存储上一次的手指角度，用于检测变化
        self.last_finger_angles = {}
        self.init_ui()
    
    def init_ui(self):
        self.setWindowTitle('手部追踪与手指控制器')
        self.setGeometry(100, 100, 1000, 900)
        
        main_layout = QVBoxLayout()
        
        # 串口设置（移到最上面）
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
        main_layout.addWidget(port_group)
        
        # 手指控制部分
        finger_control_group = QGroupBox('手指控制')
        finger_control_layout = QVBoxLayout()
        
        # 手指控制
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
            slider.setEnabled(False)  # 初始禁用
            self.finger_sliders.append(slider)
            
            finger_hbox.addWidget(finger_label)
            finger_hbox.addWidget(slider)
            finger_layout.addLayout(finger_hbox)
        
        finger_control_layout.addLayout(finger_layout)
        
        # 预设动作按钮
        preset_layout = QGridLayout()
        
        self.preset_buttons = []
        presets = [
            ('张开', [90, 90, 90, 90]),
            ('握拳', [180, 180, 180, 180]),
            ('OK手势', [180, 180, 90, 90]),
            ('指示', [90, 180, 180, 180]),
        ]
        
        for idx, (name, angles) in enumerate(presets):
            btn = QPushButton(name)
            btn.clicked.connect(lambda _, a=angles: self.apply_preset(a))
            btn.setEnabled(False)  # 初始禁用
            self.preset_buttons.append(btn)
            preset_layout.addWidget(btn, idx // 2, idx % 2)
        
        finger_control_layout.addLayout(preset_layout)
        
        # 状态栏
        self.status_bar = QStatusBar()
        self.status_bar.showMessage('未连接')
        finger_control_layout.addWidget(self.status_bar)
        
        finger_control_group.setLayout(finger_control_layout)
        main_layout.addWidget(finger_control_group)
        
        # 手部追踪部分
        hand_tracking_group = QGroupBox('手部追踪')
        hand_tracking_layout = QVBoxLayout()


##########
#核心作用是在 UI 界面的「手部追踪」模块中创建并配置「视频流地址输入框」，
#让用户可以手动输入 / 修改要追踪的视频流地址（如局域网摄像头、本地摄像头地址）
        stream_layout = QHBoxLayout()
        stream_layout.addWidget(QLabel('视频流地址:'))
        self.stream_url_input = QLineEdit()
        self.stream_url_input.setText('http://192.168.1.154:81/stream')
        stream_layout.addWidget(self.stream_url_input)
        hand_tracking_layout.addLayout(stream_layout)
##########        
        # 视频显示区域（减小尺寸）
        self.video_label = QLabel()
        self.video_label.setAlignment(Qt.AlignCenter)
        self.video_label.setFrameStyle(QFrame.Panel | QFrame.Sunken)
        # 设置固定的视频显示大小（缩小）
        fixed_width = 320
        fixed_height = 240
        self.video_label.setFixedSize(fixed_width, fixed_height)
        hand_tracking_layout.addWidget(self.video_label)
        
        # 手部数据显示
        self.hand_data_label = QTextEdit()
        self.hand_data_label.setReadOnly(True)
        self.hand_data_label.setMaximumHeight(120)
        hand_tracking_layout.addWidget(QLabel('手部数据:'))
        hand_tracking_layout.addWidget(self.hand_data_label)
        
        # 控制按钮
        btn_layout = QHBoxLayout()
        self.start_tracking_btn = QPushButton('开始追踪')
        self.start_tracking_btn.clicked.connect(self.toggle_hand_tracking)
        btn_layout.addWidget(self.start_tracking_btn)
        hand_tracking_layout.addLayout(btn_layout)
        
        hand_tracking_group.setLayout(hand_tracking_layout)
        main_layout.addWidget(hand_tracking_group)
        
        # 脚本控制部分
        script_control_group = QGroupBox('脚本控制')
        script_control_layout = QVBoxLayout()
        
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
        script_control_layout.addWidget(self.script_editor)
        
        # 脚本按钮
        script_btn_layout = QHBoxLayout()
        
        self.load_btn = QPushButton('加载脚本')
        self.load_btn.clicked.connect(self.load_script)
        
        self.save_btn = QPushButton('保存脚本')
        self.save_btn.clicked.connect(self.save_script)
        
        self.run_btn = QPushButton('执行脚本')
        self.run_btn.clicked.connect(self.run_script)
        self.run_btn.setEnabled(False)  # 初始禁用
        
        self.stop_btn = QPushButton('停止')
        self.stop_btn.clicked.connect(self.stop_script)
        self.stop_btn.setEnabled(False)  # 初始禁用
        
        script_btn_layout.addWidget(self.load_btn)
        script_btn_layout.addWidget(self.save_btn)
        script_btn_layout.addWidget(self.run_btn)
        script_btn_layout.addWidget(self.stop_btn)
        
        script_control_layout.addLayout(script_btn_layout)
        
        # 移除执行日志
        
        script_control_group.setLayout(script_control_layout)
        main_layout.addWidget(script_control_group)
        
        self.setLayout(main_layout)
    

    
    def toggle_hand_tracking(self):
        """切换手部追踪状态"""
        if not self.serial_port or not self.serial_port.is_open:
            self.hand_data_label.setText('请先连接串口')
            return
        
        if self.hand_tracking_thread and self.hand_tracking_thread.isRunning():
            # 优化线程停止逻辑，避免卡死
            self.hand_tracking_thread.stop()
            self.start_tracking_btn.setText('停止中...')
            self.start_tracking_btn.setEnabled(False)
            
            # 使用QTimer来定期检查线程状态，避免阻塞主线程
            def check_thread_status():
                if not self.hand_tracking_thread or not self.hand_tracking_thread.isRunning():
                    self.start_tracking_btn.setText('开始追踪')
                    self.start_tracking_btn.setEnabled(True)
                    return False  # 停止定时器
                return True  # 继续检查
            
            # 每100毫秒检查一次线程状态
            from PyQt5.QtCore import QTimer
            timer = QTimer(self)
            timer.timeout.connect(check_thread_status)
            timer.start(100)
        else:
            ##################
            #启动手部追踪前的「视频流地址合法性校验」，确保用户输入了有效的视频流地址后再创建追踪线程
            stream_url = self.stream_url_input.text().strip()
            if not stream_url:
                self.hand_data_label.setText('请输入视频流地址')
                return
            ##################

            #主线程中创建线程实例时传入视频流地址
            self.hand_tracking_thread = HandTrackingThread(stream_url)

            self.hand_tracking_thread.frame_signal.connect(self.update_video_frame)
            self.hand_tracking_thread.data_signal.connect(self.update_hand_data)
            self.hand_tracking_thread.error_signal.connect(self.on_hand_tracking_error)# 主线程绑定信号
            self.hand_tracking_thread.start()
            self.start_tracking_btn.setText('停止追踪')


#########################
    def on_hand_tracking_error(self, message):
        self.hand_data_label.setText(message)#主线程处理错误
#########################

    def update_video_frame(self, qt_image):
        """更新视频帧"""
        # 使用与init_ui中设置的相同的视频大小
        fixed_width = 320
        fixed_height = 240
        
        # 缩放图像到固定大小
        pixmap = QPixmap.fromImage(qt_image)
        self.video_label.setPixmap(pixmap.scaled(fixed_width, fixed_height, Qt.KeepAspectRatio))
    
    def update_hand_data(self, finger_data):
        """更新手部数据显示并发送串口数据"""
        data_text = ''
        for hand_type, fingers in finger_data.items():
            data_text += f"{hand_type}手:\n"
            # 只处理前四个手指（去掉小拇指）
            for finger_name, data in fingers.items():
                if finger_name == 'pinky':
                    continue
                data_text += f"  {finger_name}:({data['angle']}°)\n"
            data_text += "\n"
        self.hand_data_label.setText(data_text)
        
        # 发送串口数据（只发送前四个手指，且角度变化大于10°）
        self.send_hand_tracking_data(finger_data)
    
    def send_hand_tracking_data(self, finger_data):
        """发送手部追踪数据到串口"""
        if not self.serial_port or not self.serial_port.is_open:
            return
        
        # 手指名称到编号的映射（根据要求调整）
        finger_map = {
            'thumb': 4,      # 大拇指对应手指4
            'index': 1,      # 食指对应手指1
            'middle': 2,     # 中指对应手指2
            'ring': 3        # 无名指对应手指3
        }
        
        # 收集所有手指的角度，使用上一次的角度值作为基础
        finger_angles = {}
        # 初始化手指角度为上一次的值，默认为90
        for i in range(1, 5):
            finger_angles[i] = self.last_finger_angles.get(i, 90)
        
        for hand_type, fingers in finger_data.items():
            for finger_name, data in fingers.items():
                # 跳过小拇指
                if finger_name == 'pinky':
                    continue
                
                finger_num = finger_map.get(finger_name)
                if not finger_num:
                    continue
                
                angle = data['angle']
                # 检查角度变化是否大于10°
                last_angle = self.last_finger_angles.get(finger_num, 90)
                if abs(angle - last_angle) > 10:
                    # 更新角度
                    finger_angles[finger_num] = angle
                    # 更新上次角度
                    self.last_finger_angles[finger_num] = angle
        
        # 构建ALL指令
        all_command = f"ALL:{finger_angles[1]},{finger_angles[2]},{finger_angles[3]},{finger_angles[4]}\n"
        try:
            self.serial_port.write(all_command.encode('utf-8'))
        except Exception as e:
            self.status_bar.showMessage(f'发送失败: {str(e)}')
    
    def refresh_ports(self):
        """刷新串口列表"""
        self.port_combo.clear()
        ports = serial.tools.list_ports.comports()
        for port in ports:
            self.port_combo.addItem(port.device)
    
    def toggle_connection(self):
        """切换串口连接状态"""
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            self.connect_btn.setText('连接')
            self.status_bar.showMessage('已断开')
            # 禁用手指控制和预设动作按钮
            for slider in self.finger_sliders:
                slider.setEnabled(False)
            for btn in self.preset_buttons:
                btn.setEnabled(False)
            # 禁用脚本执行按钮
            self.run_btn.setEnabled(False)
            self.stop_btn.setEnabled(False)
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
                # 启用手指控制和预设动作按钮
                for slider in self.finger_sliders:
                    slider.setEnabled(True)
                for btn in self.preset_buttons:
                    btn.setEnabled(True)
                # 启用脚本执行按钮
                self.run_btn.setEnabled(True)
                self.stop_btn.setEnabled(True)
            except Exception as e:
                self.status_bar.showMessage(f'连接失败: {str(e)}')
    
    def on_slider_changed(self, finger_num, value):
        """滑块值变化处理"""
        self.finger_labels[finger_num - 1].setText(f'手指{finger_num}: {value}°')
        self.send_finger_command(finger_num, value)
    
    def send_finger_command(self, finger_num, angle):
        """发送手指控制命令"""
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
                self.status_bar.showMessage(f'已加载脚本: {file_path}')
            except Exception as e:
                self.status_bar.showMessage(f'加载失败: {str(e)}')
    
    def save_script(self):
        """保存脚本文件"""
        file_path, _ = QFileDialog.getSaveFileName(
            self, '保存脚本文件', '', 'Text Files (*.txt);;All Files (*)'
        )
        if file_path:
            try:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(self.script_editor.toPlainText())
                self.status_bar.showMessage(f'已保存脚本: {file_path}')
            except Exception as e:
                self.status_bar.showMessage(f'保存失败: {str(e)}')
    
    def run_script(self):
        """执行脚本"""
        if not self.serial_port or not self.serial_port.is_open:
            self.status_bar.showMessage('请先连接串口')
            return
        
        script_text = self.script_editor.toPlainText()
        commands = script_text.split('\n')
        
        self.status_bar.showMessage('开始执行脚本...')
        
        self.script_runner = ScriptRunner(self.serial_port, commands)
        # 移除日志信号连接
        self.script_runner.finished_signal.connect(self.on_script_finished)
        self.script_runner.start()
    
    def stop_script(self):
        """停止脚本执行"""
        if self.script_runner and self.script_runner.isRunning():
            self.script_runner.stop()
            self.status_bar.showMessage('脚本已停止')
    
    def on_script_finished(self):
        """脚本执行完成"""
        self.status_bar.showMessage('脚本执行完成')
    
    def closeEvent(self, event):
        """关闭事件处理"""
        # 停止手部追踪线程
        if self.hand_tracking_thread and self.hand_tracking_thread.isRunning():
            self.hand_tracking_thread.stop()
            self.hand_tracking_thread.wait()
        
        # 停止脚本执行线程
        if self.script_runner and self.script_runner.isRunning():
            self.script_runner.stop()
            self.script_runner.wait()
        
        # 关闭串口
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        
        event.accept()

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MainApplication()
    window.show()
    sys.exit(app.exec_())
