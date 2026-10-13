import shutil
import subprocess

try:
    from PySide6.QtCore import QProcess, Qt, QTimer
    from PySide6.QtWidgets import QFrame, QLabel, QPushButton, QVBoxLayout, QWidget
except ImportError:
    from PyQt5.QtCore import QProcess, Qt, QTimer
    from PyQt5.QtWidgets import QFrame, QLabel, QPushButton, QVBoxLayout, QWidget


class RvizPanel(QFrame):
    """Start RViz2 and reparent its X11 window into the center cockpit area."""

    def __init__(self, app_state=None):
        super().__init__()
        self.app_state = app_state
        self.rviz_process = None
        self.rviz_window_id = None
        if self.app_state is not None:
            self.app_state.set_node_state("rviz2", False)
        self.poll_count = 0

        self.layout = QVBoxLayout(self)
        self.layout.setContentsMargins(12, 10, 12, 12)
        self.layout.setSpacing(8)

        title = QLabel("RViz2 导航地图")
        title.setObjectName("sectionTitle")
        self.layout.addWidget(title)

        self.start_button = QPushButton("启动并嵌入 RViz2")
        self.start_button.clicked.connect(self.start_rviz)
        self.layout.addWidget(self.start_button)

        self.rviz_host = QFrame()
        self.rviz_host.setObjectName("rvizHost")
        self.rviz_host.setMinimumHeight(620)
        self.rviz_host.setAttribute(Qt.WA_NativeWindow, True)
        self.rviz_host.setAttribute(Qt.WA_DontCreateNativeAncestors, False)
        host_layout = QVBoxLayout(self.rviz_host)
        host_layout.setContentsMargins(18, 18, 18, 18)
        self.message_label = QLabel(
            "这里显示真正的 RViz2。\n\n"
            "点击上方按钮后会启动 rviz2，并尝试把 RViz2 窗口嵌入到这个中间区域。\n"
            "需要 Ubuntu 使用 X11，并安装 xdotool。"
        )
        self.message_label.setObjectName("rvizPlaceholder")
        self.message_label.setAlignment(Qt.AlignCenter)
        host_layout.addWidget(self.message_label, 1)
        self.layout.addWidget(self.rviz_host, 1)

        self.window_poll_timer = QTimer(self)
        self.window_poll_timer.timeout.connect(self.try_embed_rviz_window)

    def start_rviz(self):
        if self.rviz_process is not None and self.rviz_process.state() != QProcess.NotRunning:
            self.sync_embedded_window_geometry()
            self.message_label.setText("RViz2 已经启动，正在保持在中间区域。")
            return

        if shutil.which("xdotool") is None:
            self.message_label.setText(
                "无法嵌入 RViz2：系统没有安装 xdotool。\n\n"
                "请执行：sudo apt install xdotool\n"
                "并确认登录会话是 Ubuntu on Xorg。"
            )
            return

        self.rviz_process = QProcess(self)
        self.rviz_process.setProgram("rviz2")
        self.rviz_process.setArguments([])
        self.rviz_process.start()

        if not self.rviz_process.waitForStarted(3000):
            self.message_label.setText(
                "没有成功启动 rviz2。\n"
                "请确认已经 source ROS2 环境，例如：source /opt/ros/humble/setup.bash"
            )
            return

        self.start_button.setText("RViz2 嵌入中")
        if self.app_state is not None:
            self.app_state.set_node_state("rviz2", True)
        self.poll_count = 0
        self.message_label.setText("RViz2 正在启动，等待窗口出现...")
        self.window_poll_timer.start(700)

    def try_embed_rviz_window(self):
        self.poll_count += 1
        window_id = self.find_rviz_window_id()
        if window_id is not None:
            self.window_poll_timer.stop()
            self.reparent_rviz_window(window_id)
            return

        if self.poll_count >= 16:
            self.window_poll_timer.stop()
            self.message_label.setText(
                "RViz2 已启动，但没有找到可嵌入的 X11 窗口。\n\n"
                "请确认：\n"
                "1. 当前登录会话是 Ubuntu on Xorg，不是 Wayland；\n"
                "2. 已安装 xdotool；\n"
                "3. 不是在没有图形权限的容器里运行。"
            )

    def find_rviz_window_id(self):
        if self.rviz_process is None:
            return None

        process_id = int(self.rviz_process.processId())
        try:
            output = subprocess.check_output(
                ["xdotool", "search", "--pid", str(process_id), "--onlyvisible"],
                stderr=subprocess.DEVNULL,
                timeout=1.0,
                text=True,
            )
        except (subprocess.SubprocessError, OSError):
            return None

        window_ids = [line.strip() for line in output.splitlines() if line.strip().isdigit()]
        if not window_ids:
            return None
        return int(window_ids[-1])

    def reparent_rviz_window(self, window_id):
        self.rviz_window_id = int(window_id)
        host_window_id = int(self.rviz_host.winId())
        self.message_label.hide()
        self.run_xdotool(["windowreparent", str(self.rviz_window_id), str(host_window_id)])
        self.run_xdotool(["windowmap", str(self.rviz_window_id)])
        self.run_xdotool(["windowraise", str(self.rviz_window_id)])
        self.sync_embedded_window_geometry()
        self.start_button.setText("RViz2 已在中间")
        if self.app_state is not None:
            self.app_state.set_node_state("rviz2", True)

    def sync_embedded_window_geometry(self):
        if self.rviz_window_id is None:
            return
        width = max(300, self.rviz_host.width())
        height = max(220, self.rviz_host.height())
        self.run_xdotool(["windowmove", str(self.rviz_window_id), "0", "0"])
        self.run_xdotool(["windowsize", str(self.rviz_window_id), str(width), str(height)])
        self.run_xdotool(["windowraise", str(self.rviz_window_id)])

    def run_xdotool(self, arguments):
        try:
            subprocess.run(["xdotool"] + arguments, check=False, timeout=1.0)
        except (subprocess.SubprocessError, OSError):
            return

    def resizeEvent(self, event):
        super().resizeEvent(event)
        QTimer.singleShot(0, self.sync_embedded_window_geometry)

    def stop_rviz(self):
        if self.rviz_process is None:
            return
        if self.rviz_process.state() != QProcess.NotRunning:
            self.rviz_process.terminate()
            if not self.rviz_process.waitForFinished(1500):
                self.rviz_process.kill()
        self.rviz_process = None
        self.rviz_window_id = None
        if self.app_state is not None:
            self.app_state.set_node_state("rviz2", False)