#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ui_module.py（界面模块）

模块职责：
1) 仅负责创建 PyQt5 窗口、布局、所有控件
2) 不写任何业务逻辑：不绑定信号、不操作串口/脚本、不做文件读写
3) 仅暴露控件对象，供 main.py 做信号绑定、供 interaction_module.py 做界面更新

对外接口清单：
- FingerControllerUI：主窗口 QWidget，包含全部控件属性（供外部使用）
  - port_combo / refresh_btn / connect_btn / status_bar
  - finger_sliders / finger_labels
  - preset_buttons（dict[str, QPushButton]）
  - script_editor / load_btn / save_btn / run_btn / stop_btn / script_log
"""

from __future__ import annotations

from typing import Dict, List

from PyQt5.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QLabel,
    QComboBox,
    QPushButton,
    QSlider,
    QStatusBar,
    QTextEdit,
    QGroupBox,
    QGridLayout,
)
from PyQt5.QtCore import Qt


class FingerControllerUI(QWidget):
    """
    手指控制器界面（纯 UI）
    整个窗口只创建控件，不做任何业务绑定和操作
    """

    def __init__(self):
        super().__init__()
        self.port_combo: QComboBox
        self.refresh_btn: QPushButton
        self.connect_btn: QPushButton
        self.status_bar: QStatusBar

        self.finger_sliders: List[QSlider] = []
        self.finger_labels: List[QLabel] = []

        self.preset_buttons: Dict[str, QPushButton] = {}

        self.script_editor: QTextEdit
        self.load_btn: QPushButton
        self.save_btn: QPushButton
        self.run_btn: QPushButton
        self.stop_btn: QPushButton
        self.script_log: QTextEdit

        self._setup_ui()

    def _setup_ui(self) -> None:
        """创建并摆放所有 UI 控件（不绑定信号、不做业务逻辑）"""
        self.setWindowTitle("我的手指控制器")
        self.setGeometry(100, 100, 800, 600)

        main_layout = QHBoxLayout()

        left_panel = QVBoxLayout()

        port_group = QGroupBox("串口设置")
        port_layout = QHBoxLayout()
        port_label = QLabel("串口:")
        self.port_combo = QComboBox()

        self.refresh_btn = QPushButton("刷新")
        self.connect_btn = QPushButton("连接")

        port_layout.addWidget(port_label)
        port_layout.addWidget(self.port_combo)
        port_layout.addWidget(self.refresh_btn)
        port_layout.addWidget(self.connect_btn)
        port_group.setLayout(port_layout)
        left_panel.addWidget(port_group)

        finger_group = QGroupBox("手指控制")
        finger_layout = QVBoxLayout()

        for i in range(1, 5):
            finger_hbox = QHBoxLayout()
            finger_label = QLabel(f"手指{i}: 90°")
            self.finger_labels.append(finger_label)

            slider = QSlider(Qt.Horizontal)
            slider.setMinimum(90)
            slider.setMaximum(180)
            slider.setValue(90)
            self.finger_sliders.append(slider)

            finger_hbox.addWidget(finger_label)
            finger_hbox.addWidget(slider)
            finger_layout.addLayout(finger_hbox)

        finger_group.setLayout(finger_layout)
        left_panel.addWidget(finger_group)

        preset_group = QGroupBox("预设动作")
        preset_layout = QGridLayout()

        preset_names = ["张开", "握拳", "OK手势", "指示"]
        for idx, name in enumerate(preset_names):
            btn = QPushButton(name)
            self.preset_buttons[name] = btn
            preset_layout.addWidget(btn, idx // 2, idx % 2)

        preset_group.setLayout(preset_layout)
        left_panel.addWidget(preset_group)

        self.status_bar = QStatusBar()
        self.status_bar.showMessage("未连接")
        left_panel.addWidget(self.status_bar)

        main_layout.addLayout(left_panel, 1)

        right_panel = QVBoxLayout()

        script_group = QGroupBox("脚本控制")
        script_layout = QVBoxLayout()

        self.script_editor = QTextEdit()
        self.script_editor.setPlaceholderText(
            "# 脚本格式示例:\n"
            "# FINGER1:180  # 手指1弯曲\n"
            "# FINGER2:90   # 手指2复位\n"
            "# DELAY:500    # 等待500毫秒\n\n"
            "FINGER1:90\n"
            "FINGER2:90\n"
            "FINGER3:90\n"
            "FINGER4:90\n"
        )
        script_layout.addWidget(self.script_editor)

        script_btn_layout = QHBoxLayout()
        self.load_btn = QPushButton("加载脚本")
        self.save_btn = QPushButton("保存脚本")
        self.run_btn = QPushButton("执行脚本")
        self.stop_btn = QPushButton("停止")

        script_btn_layout.addWidget(self.load_btn)
        script_btn_layout.addWidget(self.save_btn)
        script_btn_layout.addWidget(self.run_btn)
        script_btn_layout.addWidget(self.stop_btn)
        script_layout.addLayout(script_btn_layout)

        self.script_log = QTextEdit()
        self.script_log.setReadOnly(True)
        self.script_log.setMaximumHeight(150)
        script_layout.addWidget(QLabel("执行日志:"))
        script_layout.addWidget(self.script_log)

        script_group.setLayout(script_layout)
        right_panel.addWidget(script_group)

        main_layout.addLayout(right_panel, 1)
        self.setLayout(main_layout)


if __name__ == "__main__":
    """
    独立测试脚本：
    1) 界面正常显示
    2) 核心控件属性存在（用于 main / interaction 调用）
    """

    import sys
    from PyQt5.QtWidgets import QApplication

    app = QApplication(sys.argv)
    ui = FingerControllerUI()

    assert ui.port_combo is not None
    assert ui.refresh_btn is not None
    assert ui.connect_btn is not None
    assert ui.status_bar is not None
    assert len(ui.finger_sliders) == 4
    assert len(ui.finger_labels) == 4
    assert set(ui.preset_buttons.keys()) == {"张开", "握拳", "OK手势", "指示"}
    assert ui.script_editor is not None
    assert ui.load_btn is not None
    assert ui.save_btn is not None
    assert ui.run_btn is not None
    assert ui.stop_btn is not None
    assert ui.script_log is not None

    ui.show()
    sys.exit(app.exec_())
