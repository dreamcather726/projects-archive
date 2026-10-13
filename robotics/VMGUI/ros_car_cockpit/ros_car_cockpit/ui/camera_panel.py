try:
    from PySide6.QtCore import Qt
    from PySide6.QtGui import QImage, QPixmap
    from PySide6.QtWidgets import QLabel
except ImportError:
    from PyQt5.QtCore import Qt
    from PyQt5.QtGui import QImage, QPixmap
    from PyQt5.QtWidgets import QLabel


class CameraPanel(QLabel):
    def __init__(self):
        super().__init__("摄像头未开启")
        self.setAlignment(Qt.AlignCenter)
        self.setMinimumSize(640, 360)

    def show_image_message(self, message):
        if message.encoding != "rgb8":
            self.setText(f"暂不支持图像格式：{message.encoding}")
            return
        image = QImage(
            bytes(message.data),
            message.width,
            message.height,
            message.step,
            QImage.Format_RGB888,
        )
        pixmap = QPixmap.fromImage(image.copy())
        self.setPixmap(pixmap.scaled(self.size(), Qt.KeepAspectRatio, Qt.SmoothTransformation))
