import glob
import getpass
import os
import sys
import threading
import time

import cv2
import mediapipe as mp
import numpy as np
import serial
import serial.tools.list_ports
from PyQt5.QtCore import Qt, QThread, QTimer, pyqtSignal
from PyQt5.QtGui import QImage, QPixmap
from PyQt5.QtWidgets import (
    QApplication,
    QComboBox,
    QFileDialog,
    QFrame,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QSlider,
    QSizePolicy,
    QStatusBar,
    QTabWidget,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)


mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles
mp_hands = mp.solutions.hands


def get_finger_bend(landmarks, tip_idx, mcp_idx):
    """计算单个手指弯曲程度。"""
    tip = np.array(
        [landmarks.landmark[tip_idx].x, landmarks.landmark[tip_idx].y]
    )
    mcp = np.array(
        [landmarks.landmark[mcp_idx].x, landmarks.landmark[mcp_idx].y]
    )
    return np.linalg.norm(tip - mcp)


def map_bend_to_angle(bend_value, finger_type):
    """将弯曲程度映射到 90~180 度。"""
    finger_thresholds = {
        "thumb": {"straight": 0.11, "bend": 0.08},
        "index": {"straight": 0.24, "bend": 0.09},
        "middle": {"straight": 0.25, "bend": 0.10},
        "ring": {"straight": 0.24, "bend": 0.10},
        "pinky": {"straight": 0.17, "bend": 0.08},
    }

    thresholds = finger_thresholds.get(
        finger_type, {"straight": 0.11, "bend": 0.10}
    )
    straight_threshold = thresholds["straight"]
    bend_threshold = thresholds["bend"]

    if bend_value >= straight_threshold:
        return 90
    if bend_value <= bend_threshold:
        return 180

    return int(
        90
        + 90
        * (straight_threshold - bend_value)
        / (straight_threshold - bend_threshold)
    )


def discover_linux_serial_ports():
    """发现香橙派/Linux 上常见串口，并标记可能连接 ESP32 的端口。"""
    try:
        port_infos = list(serial.tools.list_ports.comports(include_links=True))
    except TypeError:
        port_infos = list(serial.tools.list_ports.comports())

    metadata = {}
    for port in port_infos:
        item = {
            "description": port.description or "串口设备",
            "hwid": port.hwid or "",
            "vid": port.vid,
            "pid": port.pid,
        }
        metadata[port.device] = item
        metadata[os.path.realpath(port.device)] = item

    patterns = [
        "/dev/serial/by-id/*",
        "/dev/ttyUSB*",
        "/dev/ttyACM*",
        "/dev/ttyCH341*",
        "/dev/ttyCH343*",
        "/dev/ttyGS*",
        "/dev/ttyAMA*",
        "/dev/ttyS*",
        "/dev/ttyAS*",
        "/dev/ttyFIQ*",
        "/dev/ttyXRUSB*",
    ]

    # by-id 最稳定，因此排在最前面；按真实设备路径去重。
    result = []
    seen_real_paths = set()
    candidate_paths = []

    for pattern in patterns:
        candidate_paths.extend(sorted(glob.glob(pattern)))
    candidate_paths.extend(sorted(port.device for port in port_infos))

    # ESP32 原生 USB、CP210x、CH340/CH343 和 FTDI 的常见 USB VID。
    esp32_related_vids = {0x303A, 0x10C4, 0x1A86, 0x0403}
    esp32_keywords = (
        "esp32",
        "espressif",
        "cp210",
        "ch340",
        "ch341",
        "ch343",
        "wch",
        "usb serial",
    )

    for device in candidate_paths:
        if not os.path.exists(device):
            continue
        real_path = os.path.realpath(device)
        if real_path in seen_real_paths:
            continue
        seen_real_paths.add(real_path)

        info = metadata.get(device) or metadata.get(real_path) or {}
        description = info.get("description", "Linux 串口设备")
        vid = info.get("vid")
        pid = info.get("pid")
        hwid = info.get("hwid", "")
        searchable = f"{device} {description} {hwid}".lower()
        is_esp32_candidate = vid in esp32_related_vids or any(
            keyword in searchable for keyword in esp32_keywords
        )
        if vid is not None and pid is not None:
            description = f"{description} [{vid:04X}:{pid:04X}]"
        can_access = os.access(device, os.R_OK | os.W_OK)
        result.append((device, description, can_access, is_esp32_candidate))

    def port_priority(item):
        device, _, can_access, is_candidate = item
        if "/serial/by-id/" in device:
            device_rank = 0
        elif "ttyACM" in device or "ttyUSB" in device or "ttyCH" in device:
            device_rank = 1
        else:
            device_rank = 2
        return (not is_candidate, not can_access, device_rank, device)

    return sorted(result, key=port_priority)


class ScriptRunner(QThread):
    log_signal = pyqtSignal(str)
    finished_signal = pyqtSignal()

    def __init__(self, serial_port, commands, serial_lock):
        super().__init__()
        self.serial_port = serial_port
        self.commands = commands
        self.serial_lock = serial_lock
        self.running = True

    def run(self):
        for cmd in self.commands:
            if not self.running:
                break

            cmd = cmd.strip()
            if not cmd or cmd.startswith("#"):
                continue

            if cmd.startswith("ALL:") or cmd.startswith("FINGER"):
                self.send_command(cmd)
            elif cmd.startswith("DELAY"):
                try:
                    delay_ms = int(cmd.split(":", 1)[1])
                    self.log_signal.emit(f"等待 {delay_ms}ms")
                    time.sleep(delay_ms / 1000)
                except (ValueError, IndexError):
                    self.log_signal.emit(f"无效的 DELAY 命令: {cmd}")
            else:
                self.log_signal.emit(f"未知命令: {cmd}")

        self.finished_signal.emit()

    def send_command(self, command):
        if self.serial_port and self.serial_port.is_open:
            try:
                with self.serial_lock:
                    self.serial_port.write((command + "\n").encode("utf-8"))
                self.log_signal.emit(f"发送: {command}")
                time.sleep(0.05)
            except Exception as exc:
                self.log_signal.emit(f"发送失败: {exc}")

    def stop(self):
        self.running = False


class HandTrackingThread(QThread):
    frame_signal = pyqtSignal(QImage)
    data_signal = pyqtSignal(dict)
    status_signal = pyqtSignal(str)

    def __init__(self, camera_index=0):
        super().__init__()
        self.camera_index = camera_index
        self.running = True

    def run(self):
        # 香橙派/Linux 优先使用 V4L2，失败时回退到默认后端。
        cap = cv2.VideoCapture(self.camera_index, cv2.CAP_V4L2)
        if not cap.isOpened():
            cap.release()
            cap = cv2.VideoCapture(self.camera_index)

        if not cap.isOpened():
            self.status_signal.emit(
                f"摄像头 /dev/video{self.camera_index} 打开失败，请检查设备编号和权限"
            )
            return

        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

        try:
            with mp_hands.Hands(
                model_complexity=0,
                min_detection_confidence=0.5,
                min_tracking_confidence=0.5,
            ) as hands:
                while self.running:
                    ret, frame = cap.read()
                    if not ret:
                        self.status_signal.emit("摄像头读取失败")
                        break

                    frame = cv2.flip(frame, 1)
                    frame.flags.writeable = False
                    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                    results = hands.process(rgb_frame)
                    frame.flags.writeable = True

                    finger_data = {}

                    if results.multi_hand_landmarks and results.multi_handedness:
                        for idx, handedness in enumerate(results.multi_handedness):
                            hand_type = handedness.classification[0].label
                            landmarks = results.multi_hand_landmarks[idx]

                            bends = {
                                "thumb": get_finger_bend(
                                    landmarks,
                                    mp_hands.HandLandmark.THUMB_TIP,
                                    mp_hands.HandLandmark.THUMB_MCP,
                                ),
                                "index": get_finger_bend(
                                    landmarks,
                                    mp_hands.HandLandmark.INDEX_FINGER_TIP,
                                    mp_hands.HandLandmark.INDEX_FINGER_MCP,
                                ),
                                "middle": get_finger_bend(
                                    landmarks,
                                    mp_hands.HandLandmark.MIDDLE_FINGER_TIP,
                                    mp_hands.HandLandmark.MIDDLE_FINGER_MCP,
                                ),
                                "ring": get_finger_bend(
                                    landmarks,
                                    mp_hands.HandLandmark.RING_FINGER_TIP,
                                    mp_hands.HandLandmark.RING_FINGER_MCP,
                                ),
                                "pinky": get_finger_bend(
                                    landmarks,
                                    mp_hands.HandLandmark.PINKY_TIP,
                                    mp_hands.HandLandmark.PINKY_MCP,
                                ),
                            }

                            finger_data[hand_type] = {
                                name: {
                                    "bend": bend,
                                    "angle": map_bend_to_angle(bend, name),
                                }
                                for name, bend in bends.items()
                            }

                            mp_drawing.draw_landmarks(
                                frame,
                                landmarks,
                                mp_hands.HAND_CONNECTIONS,
                                mp_drawing_styles.get_default_hand_landmarks_style(),
                                mp_drawing_styles.get_default_hand_connections_style(),
                            )

                    h, w, ch = frame.shape
                    qt_image = QImage(
                        frame.data,
                        w,
                        h,
                        ch * w,
                        QImage.Format_BGR888,
                    ).copy()
                    self.frame_signal.emit(qt_image)

                    if finger_data:
                        self.data_signal.emit(finger_data)
        finally:
            cap.release()

    def stop(self):
        self.running = False


class MainApplication(QWidget):
    def __init__(self):
        super().__init__()
        self.serial_port = None
        self.serial_lock = threading.Lock()
        self.script_runner = None
        self.hand_tracking_thread = None
        self.tracking_stop_timer = None
        self._port_signature = None
        self.last_finger_angles = {}
        self.init_ui()

    def init_ui(self):
        self.setWindowTitle("香橙派手部追踪与手指控制器")
        self.setGeometry(100, 50, 760, 600)
        self.setMinimumSize(700, 520)
        main_layout = QVBoxLayout()
        self.status_bar = QStatusBar()
        self.status_bar.showMessage("未连接串口")

        port_group = QGroupBox("串口设置（香橙派/Linux）")
        port_layout = QHBoxLayout()
        port_layout.addWidget(QLabel("串口:"))

        self.port_combo = QComboBox()
        self.port_combo.setEditable(True)
        self.port_combo.lineEdit().setPlaceholderText(
            "可手动输入，例如 /dev/ttyUSB0 或 /dev/ttyACM0"
        )
        port_layout.addWidget(self.port_combo, 1)

        refresh_btn = QPushButton("刷新")
        refresh_btn.clicked.connect(lambda: self.refresh_ports())
        port_layout.addWidget(refresh_btn)

        self.connect_btn = QPushButton("连接")
        self.connect_btn.clicked.connect(self.toggle_connection)
        port_layout.addWidget(self.connect_btn)

        port_group.setLayout(port_layout)
        main_layout.addWidget(port_group)

        finger_control_group = QGroupBox("手指控制")
        finger_control_layout = QVBoxLayout()
        self.finger_sliders = []
        self.finger_labels = []

        for i in range(1, 5):
            row = QHBoxLayout()
            label = QLabel(f"手指{i}: 90°")
            slider = QSlider(Qt.Horizontal)
            slider.setRange(90, 180)
            slider.setValue(90)
            slider.setEnabled(False)
            slider.valueChanged.connect(
                lambda value, idx=i: self.on_slider_changed(idx, value)
            )
            self.finger_labels.append(label)
            self.finger_sliders.append(slider)
            row.addWidget(label)
            row.addWidget(slider)
            finger_control_layout.addLayout(row)

        presets = [
            ("张开", [90, 90, 90, 90]),
            ("握拳", [180, 180, 180, 180]),
            ("OK手势", [180, 180, 90, 90]),
            ("指示", [90, 180, 180, 180]),
        ]
        preset_layout = QGridLayout()
        self.preset_buttons = []
        for idx, (name, angles) in enumerate(presets):
            button = QPushButton(name)
            button.setEnabled(False)
            button.clicked.connect(lambda _, values=angles: self.apply_preset(values))
            self.preset_buttons.append(button)
            preset_layout.addWidget(button, idx // 2, idx % 2)

        finger_control_layout.addLayout(preset_layout)
        finger_control_group.setLayout(finger_control_layout)

        hand_tracking_group = QGroupBox("手部追踪")
        hand_tracking_layout = QVBoxLayout()
        self.video_label = QLabel()
        self.video_label.setAlignment(Qt.AlignCenter)
        self.video_label.setFrameStyle(QFrame.Panel | QFrame.Sunken)
        self.video_label.setMinimumSize(480, 360)
        self.video_label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.video_label.setText("点击“开始追踪”打开摄像头")
        hand_tracking_layout.addWidget(self.video_label)

        hand_tracking_layout.addWidget(QLabel("手部数据:"))
        self.hand_data_label = QTextEdit()
        self.hand_data_label.setReadOnly(True)
        self.hand_data_label.setMaximumHeight(90)
        hand_tracking_layout.addWidget(self.hand_data_label)

        self.start_tracking_btn = QPushButton("开始追踪")
        self.start_tracking_btn.clicked.connect(self.toggle_hand_tracking)
        hand_tracking_layout.addWidget(self.start_tracking_btn)
        hand_tracking_group.setLayout(hand_tracking_layout)

        tabs = QTabWidget()
        control_tab = QWidget()
        control_layout = QHBoxLayout()
        control_layout.addWidget(finger_control_group, 1)
        control_layout.addWidget(hand_tracking_group, 1)
        control_tab.setLayout(control_layout)
        tabs.addTab(control_tab, "实时控制")

        script_control_group = QGroupBox("脚本控制")
        script_control_layout = QVBoxLayout()
        self.script_editor = QTextEdit()
        self.script_editor.setPlaceholderText(
            "# 脚本格式示例:\n"
            "# FINGER1:180\n"
            "# DELAY:500\n\n"
            "FINGER1:90\n"
            "FINGER2:90\n"
            "FINGER3:90\n"
            "FINGER4:90\n"
        )
        script_control_layout.addWidget(self.script_editor)

        script_buttons = QHBoxLayout()
        self.load_btn = QPushButton("加载脚本")
        self.load_btn.clicked.connect(self.load_script)
        self.save_btn = QPushButton("保存脚本")
        self.save_btn.clicked.connect(self.save_script)
        self.run_btn = QPushButton("执行脚本")
        self.run_btn.clicked.connect(self.run_script)
        self.run_btn.setEnabled(False)
        self.stop_btn = QPushButton("停止")
        self.stop_btn.clicked.connect(self.stop_script)
        self.stop_btn.setEnabled(False)

        for button in (self.load_btn, self.save_btn, self.run_btn, self.stop_btn):
            script_buttons.addWidget(button)
        script_control_layout.addLayout(script_buttons)
        script_control_group.setLayout(script_control_layout)

        script_tab = QWidget()
        script_tab_layout = QVBoxLayout()
        script_tab_layout.addWidget(script_control_group)
        script_tab.setLayout(script_tab_layout)
        tabs.addTab(script_tab, "脚本控制")

        main_layout.addWidget(tabs, 1)
        main_layout.addWidget(self.status_bar)

        self.setLayout(main_layout)
        self.refresh_ports()

        # USB 串口插拔后自动刷新，不必重启程序。
        self.port_refresh_timer = QTimer(self)
        self.port_refresh_timer.timeout.connect(self.auto_refresh_ports)
        self.port_refresh_timer.start(2000)

    def refresh_ports(self, detected_ports=None):
        """刷新 Linux 串口列表，并允许用户手动输入设备路径。"""
        previous_port = self.current_port()
        ports = detected_ports if detected_ports is not None else discover_linux_serial_ports()
        self._port_signature = tuple(ports)

        self.port_combo.clear()
        selected_index = -1
        candidate_count = 0
        for index, (device, description, can_access, is_candidate) in enumerate(ports):
            permission = "可读写" if can_access else "无权限"
            candidate = "ESP32候选" if is_candidate else "普通串口"
            self.port_combo.addItem(
                f"{device}  |  {description}  |  {candidate}  |  {permission}",
                device,
            )
            candidate_count += int(is_candidate)
            if device == previous_port:
                selected_index = index

        if selected_index >= 0:
            self.port_combo.setCurrentIndex(selected_index)
        elif previous_port:
            # 保留用户手动输入的设备路径。
            self.port_combo.setEditText(previous_port)

        if ports:
            self.status_bar.showMessage(
                f"发现 {len(ports)} 个串口设备，其中 {candidate_count} 个 ESP32 候选"
            )
        else:
            if not previous_port:
                self.port_combo.setCurrentText("")
            self.status_bar.showMessage(
                "系统未生成串口设备；请检查 USB 数据线、驱动和 /dev/ttyUSB0"
            )

    def auto_refresh_ports(self):
        """检测 USB 串口热插拔，仅在设备列表变化时刷新界面。"""
        if self.serial_port and self.serial_port.is_open:
            return
        if self.port_combo.view().isVisible():
            return
        ports = discover_linux_serial_ports()
        if tuple(ports) != self._port_signature:
            self.refresh_ports(ports)

    def current_port(self):
        data = self.port_combo.currentData()
        if data:
            return str(data).strip()

        text = self.port_combo.currentText().strip()
        if "|" in text:
            text = text.split("|", 1)[0].strip()
        return text

    def set_serial_controls_enabled(self, enabled):
        for slider in self.finger_sliders:
            slider.setEnabled(enabled)
        for button in self.preset_buttons:
            button.setEnabled(enabled)
        self.run_btn.setEnabled(enabled)
        self.stop_btn.setEnabled(enabled)

    def toggle_connection(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            self.serial_port = None
            self.connect_btn.setText("连接")
            self.status_bar.showMessage("串口已断开")
            self.set_serial_controls_enabled(False)
            return

        port = self.current_port()
        if not port:
            self.status_bar.showMessage(
                "未选择串口，可手动输入 /dev/ttyUSB0 或 /dev/ttyACM0"
            )
            return

        if not os.path.exists(port):
            self.status_bar.showMessage(f"设备不存在: {port}")
            return

        try:
            self.serial_port = serial.Serial(
                port=port,
                baudrate=115200,
                timeout=0.1,
                write_timeout=1,
            )
            time.sleep(0.3)
            self.serial_port.reset_input_buffer()
            self.connect_btn.setText("断开")
            self.status_bar.showMessage(f"已连接到 {port}，波特率 115200")
            self.set_serial_controls_enabled(True)
        except PermissionError:
            user = getpass.getuser()
            self.serial_port = None
            self.status_bar.showMessage(
                f"没有串口权限：执行 sudo usermod -aG dialout {user} 后重启"
            )
        except serial.SerialException as exc:
            self.serial_port = None
            message = str(exc)
            if "Permission denied" in message:
                user = getpass.getuser()
                message = (
                    f"没有串口权限：执行 sudo usermod -aG dialout {user} 后重启"
                )
            elif "busy" in message.lower() or "resource" in message.lower():
                message = f"串口被其他程序占用：{port}"
            self.status_bar.showMessage(f"连接失败：{message}")
        except Exception as exc:
            self.serial_port = None
            self.status_bar.showMessage(f"连接失败：{exc}")

    def toggle_hand_tracking(self):
        """摄像头追踪可独立运行；连接串口后才发送控制指令。"""
        if self.hand_tracking_thread and self.hand_tracking_thread.isRunning():
            self.hand_tracking_thread.stop()
            self.start_tracking_btn.setText("停止中...")
            self.start_tracking_btn.setEnabled(False)

            if self.tracking_stop_timer:
                self.tracking_stop_timer.stop()
            self.tracking_stop_timer = QTimer(self)
            self.tracking_stop_timer.timeout.connect(self.check_tracking_stopped)
            self.tracking_stop_timer.start(100)
            return

        self.hand_tracking_thread = HandTrackingThread(camera_index=0)
        self.hand_tracking_thread.frame_signal.connect(self.update_video_frame)
        self.hand_tracking_thread.data_signal.connect(self.update_hand_data)
        self.hand_tracking_thread.status_signal.connect(self.status_bar.showMessage)
        self.hand_tracking_thread.finished.connect(self.on_tracking_finished)
        self.hand_tracking_thread.start()
        self.start_tracking_btn.setText("停止追踪")
        if not self.serial_port or not self.serial_port.is_open:
            self.status_bar.showMessage("追踪已启动；串口未连接，仅显示手势数据")

    def check_tracking_stopped(self):
        if not self.hand_tracking_thread or not self.hand_tracking_thread.isRunning():
            self.tracking_stop_timer.stop()
            self.start_tracking_btn.setText("开始追踪")
            self.start_tracking_btn.setEnabled(True)

    def on_tracking_finished(self):
        self.start_tracking_btn.setText("开始追踪")
        self.start_tracking_btn.setEnabled(True)

    def update_video_frame(self, qt_image):
        pixmap = QPixmap.fromImage(qt_image)
        self.video_label.setPixmap(
            pixmap.scaled(
                self.video_label.size(),
                Qt.KeepAspectRatio,
                Qt.SmoothTransformation,
            )
        )

    def update_hand_data(self, finger_data):
        lines = []
        for hand_type, fingers in finger_data.items():
            lines.append(f"{hand_type}手:")
            for finger_name, data in fingers.items():
                if finger_name != "pinky":
                    lines.append(f"  {finger_name}: ({data['angle']}°)")
            lines.append("")
        self.hand_data_label.setText("\n".join(lines))
        self.send_hand_tracking_data(finger_data)

    def write_serial(self, data):
        if not self.serial_port or not self.serial_port.is_open:
            return False
        with self.serial_lock:
            self.serial_port.write(data)
        return True

    def send_hand_tracking_data(self, finger_data):
        if not self.serial_port or not self.serial_port.is_open:
            return

        finger_map = {"thumb": 4, "index": 1, "middle": 2, "ring": 3}
        finger_angles = {
            i: self.last_finger_angles.get(i, 90) for i in range(1, 5)
        }
        changed = False

        for fingers in finger_data.values():
            for finger_name, data in fingers.items():
                finger_num = finger_map.get(finger_name)
                if not finger_num:
                    continue
                angle = data["angle"]
                last_angle = self.last_finger_angles.get(finger_num, 90)
                if abs(angle - last_angle) > 10:
                    finger_angles[finger_num] = angle
                    self.last_finger_angles[finger_num] = angle
                    changed = True

        # 没有明显变化时不重复发送，避免串口被视频帧率刷满。
        if not changed:
            return

        command = (
            f"ALL:{finger_angles[1]},{finger_angles[2]},"
            f"{finger_angles[3]},{finger_angles[4]}\n"
        )
        try:
            self.write_serial(command.encode("utf-8"))
        except Exception as exc:
            self.status_bar.showMessage(f"发送失败: {exc}")

    def on_slider_changed(self, finger_num, value):
        self.finger_labels[finger_num - 1].setText(f"手指{finger_num}: {value}°")
        self.send_finger_command(finger_num, value)

    def send_finger_command(self, finger_num, angle):
        command = f"FINGER{finger_num}:{angle}\n"
        try:
            if self.write_serial(command.encode("utf-8")):
                self.status_bar.showMessage(f"发送指令: {command.strip()}")
        except Exception as exc:
            self.status_bar.showMessage(f"发送失败: {exc}")

    def apply_preset(self, angles):
        for index, angle in enumerate(angles):
            self.finger_sliders[index].setValue(angle)

    def load_script(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self, "选择脚本文件", "", "Text Files (*.txt);;All Files (*)"
        )
        if not file_path:
            return
        try:
            with open(file_path, "r", encoding="utf-8") as file:
                self.script_editor.setText(file.read())
            self.status_bar.showMessage(f"已加载脚本: {file_path}")
        except Exception as exc:
            self.status_bar.showMessage(f"加载失败: {exc}")

    def save_script(self):
        file_path, _ = QFileDialog.getSaveFileName(
            self, "保存脚本文件", "", "Text Files (*.txt);;All Files (*)"
        )
        if not file_path:
            return
        try:
            with open(file_path, "w", encoding="utf-8") as file:
                file.write(self.script_editor.toPlainText())
            self.status_bar.showMessage(f"已保存脚本: {file_path}")
        except Exception as exc:
            self.status_bar.showMessage(f"保存失败: {exc}")

    def run_script(self):
        if not self.serial_port or not self.serial_port.is_open:
            self.status_bar.showMessage("请先连接串口")
            return

        commands = self.script_editor.toPlainText().splitlines()
        self.script_runner = ScriptRunner(
            self.serial_port, commands, self.serial_lock
        )
        self.script_runner.finished_signal.connect(self.on_script_finished)
        self.script_runner.start()
        self.status_bar.showMessage("开始执行脚本...")

    def stop_script(self):
        if self.script_runner and self.script_runner.isRunning():
            self.script_runner.stop()
            self.status_bar.showMessage("正在停止脚本...")

    def on_script_finished(self):
        self.status_bar.showMessage("脚本执行完成")

    def keyPressEvent(self, event):
        """F11 切换全屏，Esc 退出全屏。"""
        if event.key() == Qt.Key_F11:
            if self.isFullScreen():
                self.showNormal()
            else:
                self.showFullScreen()
            return
        if event.key() == Qt.Key_Escape and self.isFullScreen():
            self.showNormal()
            return
        super().keyPressEvent(event)

    def closeEvent(self, event):
        if self.hand_tracking_thread and self.hand_tracking_thread.isRunning():
            self.hand_tracking_thread.stop()
            self.hand_tracking_thread.wait(3000)

        if self.script_runner and self.script_runner.isRunning():
            self.script_runner.stop()
            self.script_runner.wait(3000)

        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()

        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainApplication()
    window.showFullScreen()
    sys.exit(app.exec_())
