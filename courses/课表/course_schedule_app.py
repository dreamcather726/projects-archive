from __future__ import annotations

import sys
from dataclasses import dataclass
from datetime import date, timedelta
from html import escape

try:
    from PySide6.QtCore import QDate, QTime
    from PySide6.QtGui import QColor, QFont, QPalette
    from PySide6.QtWidgets import (
        QApplication,
        QDateEdit,
        QFrame,
        QGridLayout,
        QGroupBox,
        QHBoxLayout,
        QLabel,
        QMainWindow,
        QPushButton,
        QScrollArea,
        QSizePolicy,
        QVBoxLayout,
        QWidget,
    )
except ModuleNotFoundError:
    try:
        from PyQt6.QtCore import QDate, QTime
        from PyQt6.QtGui import QColor, QFont, QPalette
        from PyQt6.QtWidgets import (
            QApplication,
            QDateEdit,
            QFrame,
            QGridLayout,
            QGroupBox,
            QHBoxLayout,
            QLabel,
            QMainWindow,
            QPushButton,
            QScrollArea,
            QSizePolicy,
            QVBoxLayout,
            QWidget,
        )
    except ModuleNotFoundError:
        from PyQt5.QtCore import QDate, QTime
        from PyQt5.QtGui import QColor, QFont, QPalette
        from PyQt5.QtWidgets import (
            QApplication,
            QDateEdit,
            QFrame,
            QGridLayout,
            QGroupBox,
            QHBoxLayout,
            QLabel,
            QMainWindow,
            QPushButton,
            QScrollArea,
            QSizePolicy,
            QVBoxLayout,
            QWidget,
        )


TOTAL_LESSON_COUNT = 60
PUBLISHED_LESSON_TITLES = [
    "1-1 计算机的心脏与语言 V1.0",
    "1-2 电路世界的乐高积木 V1.0",
    "1-3 虚拟实验室-扩展篇 V1.0",
    "1-4 电子系统可靠性设计和PCB V1.0",
    "1-5 赋予硬件灵魂C/C++ V1.0",
    "1-6 Linux嵌入式开发 V1.0",
    "1-7 C语言进阶与硬件握手 V1.0",
    "1-8 电路仿真与单片机测试 V1.0",
    "2-1 具身智能机器人小脑构建 V1.0",
    "2-2 具身智能机器人小脑进阶",
    "2-3 具身智能机器人小脑高阶",
    "2-4 点亮你的AI视觉板从屏幕到声音",
    "2-5 构建网络专属应用&云端收音机",
    "2-6 连接云平台MQTT物联网通信实战",
    "2-7 智能农业监测系统",
    "2-8 ESP32实战实验汇总",
    "3-1 用深度解析嵌入式技术架构应",
    "3-2 嵌入式AI图像识别",
    "3-3 自然语言处理（NLP）",
    "3-4 嵌入式AI系统开发",
    "3-5 本地部署AI语音小助手 手册",
    "4-1 机器人 ROS2系统教学开发",
    "4-2 C++和Docker 使用指南",
    "4-3 基础功能开发",
    "4-4 OpenCV 高性能算法库",
    "4-5 外部资源的基本使用",
    "4-6 机器人ROS2系统:ROS2封装硬件",
    "4-7 机械臂仿真",
    "4-8 四足机器人仿真与强化学习",
    "4-9 双足机器人仿真实验与强化学习",
    "4-10 灵巧手仿真实验",
    "5.1 IsaacSim 5.0环境安装与测试",
    "5.2 如何使用IsaacSim的GUI组建一个机器人",
    "5.3 为简单机器人添加关节",
    "5-4 将摄像头和传感器添加到机器人",
    "5-5 Isaac Lab入门教学",
    "5-6 在 Isaac Sim 5中导入 SO100Arm 套件",
    "6-1 Isaac Lab创建一个新项目",
    "6-2 Isaac Lab的基本使用",
    "6-3 IsaacLab平台下实现TienKung-Lab",
    "6-4 基于 Isaac Lab 的四足机器人 Parkour 训练",
    "6-5 Isaac Lab机械臂仿真与ROS2部署",
    "6-6 基于 Isaac Lab 框架构建 Unitree 机器人仿真环境",
    "7-1 LeRobot环境搭建和基础验证",
    "7-2 LeRobot数据集系统深入理解",
    "7-3 Diffusion Policy 算法深度剖析",
    "7-4 四大策略算法对比学习",
    "7-5 训练流程与分布式训练机制",
    "7-6 数据处理管道与 Processor 系统",
    "7-7 仿真环境与 Sim2Real",
    "7-8 LeRobot项目完整使用指南",
    "8-1 环境搭建和基本测试",
    "8-2 ROS2 摄像头图像处理入门教程",
    "8-3 USB摄像头标定",
    "8-4 深度相机应用课程教学",
    "8-5 深度相机拓展应用",
    "8-6 通过ROS话题发布控制小车",
    "8-7 机器人建模 和 Gazebo仿真实现",
    "8-8 Gazebo SLAM建图以及NAV2导航",
]
PUBLISHED_LESSON_LINKS = [
    "https://wcn5nr4htkrb.feishu.cn/wiki/WN53wm3QSiASyzkVsnbceuMWnsS",
    "https://wcn5nr4htkrb.feishu.cn/wiki/OI7BwaZMPid1imkRNMncQQKKnfd",
    "https://wcn5nr4htkrb.feishu.cn/wiki/ZETCw0MTwi3cSakluWHcGmpHnTe",
    "https://wcn5nr4htkrb.feishu.cn/wiki/W3i1wx5JPig4PIkfytrcVJ7Onjg",
    "https://wcn5nr4htkrb.feishu.cn/wiki/Opj3wUWa6iEJHXkdHRScHlVunXg",
    "https://wcn5nr4htkrb.feishu.cn/wiki/WDcvwX0b9ij0phkg79CcYQUtnud",
    "https://wcn5nr4htkrb.feishu.cn/wiki/Up8nw6Viyi3guCkjpUecYq1CnLd",
    "https://wcn5nr4htkrb.feishu.cn/wiki/OoxAwcD7fifhfGk76yRcPxEEnUb",
    "https://wcn5nr4htkrb.feishu.cn/wiki/IzPCwjrZ5igvbmk0KMMcMJnInCh",
    "https://wcn5nr4htkrb.feishu.cn/wiki/MtMSw4wgti1R2Skb1idcwLhHnDg",
    "https://wcn5nr4htkrb.feishu.cn/wiki/We6ZwPQZai9fPskl92scEJmhn5b",
    "https://wcn5nr4htkrb.feishu.cn/wiki/AYiwwWUJaijkV3kNRbwctE3wnmc",
    "https://wcn5nr4htkrb.feishu.cn/wiki/TlT4wBCf7iyRU8kfMWcc8YaGnub",
    "https://wcn5nr4htkrb.feishu.cn/wiki/ZsNEw7jeLiXtA2khDokcVNvvnjh",
    "https://wcn5nr4htkrb.feishu.cn/wiki/JNB7wWazzi1LKIkYj5xcP8MMnwb",
    "https://wcn5nr4htkrb.feishu.cn/wiki/VhvEwndZriGz5akZQ6LcHRWgnth",
    "https://wcn5nr4htkrb.feishu.cn/wiki/BKJ9wTU6HiAC1XkPYFCcCUb6nIf",
    "https://wcn5nr4htkrb.feishu.cn/wiki/IW1Ewj89eijpuHk5MAvcvDUOnLe",
    "https://wcn5nr4htkrb.feishu.cn/wiki/Uv2ywLjhNivZMUkqa0VcF9NInPg",
    "https://wcn5nr4htkrb.feishu.cn/wiki/VqjOwgvJuiQaGrkiLOfc7OxXnLb",
    "https://wcn5nr4htkrb.feishu.cn/wiki/L5h4wk7pCiZK65kOrpUcxoXbn1c",
    "https://wcn5nr4htkrb.feishu.cn/wiki/QioAwzaz5irUMikpbvZcSNacnkf",
    "https://wcn5nr4htkrb.feishu.cn/wiki/Tkf6w9lZJid3Kwk2Qkics5bHnde",
    "https://wcn5nr4htkrb.feishu.cn/wiki/XfADw5CE8izaUKkaYw9cGaXrnmg",
    "https://wcn5nr4htkrb.feishu.cn/wiki/RrX7wqm2GiEfNkkuURncNrhenjf",
    "https://wcn5nr4htkrb.feishu.cn/wiki/F4a2w86HCiZQXikLlaGcRXOSnGh",
    "https://wcn5nr4htkrb.feishu.cn/wiki/DbKCwDljfiPbwYkC3G8cMVpznKb",
    "https://wcn5nr4htkrb.feishu.cn/wiki/VePnwcgxbihMdBkQHAVcMEP3nPh",
    "https://wcn5nr4htkrb.feishu.cn/wiki/MmXBwcubBiy51mkfM6PcdLR4nEc",
    "https://wcn5nr4htkrb.feishu.cn/wiki/VluZwAgyhiMOKWkNIG9cD1AUnKT",
    "https://wcn5nr4htkrb.feishu.cn/wiki/W4odwos9Bi8K1ak5vwxcQSXwnec",
    "https://wcn5nr4htkrb.feishu.cn/wiki/GmrHwClXmiKqdhkA7INcToTFnlc",
    "https://wcn5nr4htkrb.feishu.cn/wiki/X3lSweLALiv8oOkYELGc2wqTnUc",
    "https://wcn5nr4htkrb.feishu.cn/wiki/BrJowuWu0ixf86k8Jt9cc7IJncf",
    "https://wcn5nr4htkrb.feishu.cn/wiki/Fbi3wLlKIi1b0ykz9q4cfHZunvd",
    "https://wcn5nr4htkrb.feishu.cn/wiki/NW3GwqpVmis9hNkuB2Pc6O00nbd",
    "https://wcn5nr4htkrb.feishu.cn/wiki/KzvFwajCAiYGEHkFBEicdc2Bn0c",
    "https://wcn5nr4htkrb.feishu.cn/wiki/APokwlOZ6iiwYSkO12qciv91nkx",
    "https://wcn5nr4htkrb.feishu.cn/wiki/APokwlOZ6iiwYSkO12qciv91nkx",
    "https://wcn5nr4htkrb.feishu.cn/wiki/N39Gw7cfji5zaMk0FMPcKGZqnQf",
    "https://wcn5nr4htkrb.feishu.cn/wiki/T7EGwgDnjiXKemkxYIfciJQinAb",
    "https://wcn5nr4htkrb.feishu.cn/wiki/QhBowOIxqiUW80kQoOocWLXunhh",
    "https://wcn5nr4htkrb.feishu.cn/wiki/TkDCwzygqiL3czkYgVncPGTGneG",
    "https://wcn5nr4htkrb.feishu.cn/wiki/UlrCw42NCiVP5OkfTawcxowGnHe",
    "https://wcn5nr4htkrb.feishu.cn/wiki/ZD7cwtLCZi43pLkvY8IcHylknGD",
    "https://wcn5nr4htkrb.feishu.cn/wiki/ZSvuwq0Bdi4iibk74BgciBUTnVZ",
    "https://wcn5nr4htkrb.feishu.cn/wiki/EAG1wrqheimJNFkrTJdcgqxEnGf",
    "https://wcn5nr4htkrb.feishu.cn/wiki/LhBFwJy0ai5K2ckoJs1cuc88nnc",
    "https://wcn5nr4htkrb.feishu.cn/wiki/PEksw6rUYiJQ56kcW89cQmWMnrc",
    "https://wcn5nr4htkrb.feishu.cn/wiki/AdipwRbm8iWpVwkvoPscKRc1nEf",
    "https://wcn5nr4htkrb.feishu.cn/wiki/Z7NawBieyiBsNBkqmnWcZshxnrc",
    "https://wcn5nr4htkrb.feishu.cn/wiki/Upa6wiePri0BjLkePf1c2exVn8d",
    "https://wcn5nr4htkrb.feishu.cn/wiki/RDy1wtMP7iDUrvkzyYNcp99rnPh",
    "https://wcn5nr4htkrb.feishu.cn/wiki/M8FAwSpSgi7Q02kOsYScXjGrnkb",
    "https://wcn5nr4htkrb.feishu.cn/wiki/SRF0wXSOliVJFvkbRYpcUfOonpd",
    "https://wcn5nr4htkrb.feishu.cn/wiki/ASU9wtJlJiAMnTkZ7kQcFMWrnLh",
    "https://wcn5nr4htkrb.feishu.cn/wiki/TorEwsMzAi8erakDAAlcbBbKnVg",
    "https://wcn5nr4htkrb.feishu.cn/wiki/OceJwAiWQiagTykOHXDcm3h2nnh",
    "https://wcn5nr4htkrb.feishu.cn/wiki/XuQxwckBCiEyc5kJy7CcBo5dnRe",
]
PUBLISHED_LESSON_COUNT = len(PUBLISHED_LESSON_TITLES)
CLASS_WEEKDAYS = {
    1: "周一",
    3: "周三",
    5: "周五",
}
CLASS_TIME = QTime(19, 30)
PREVIEW_DAYS = 7
FRAME_NO_FRAME = getattr(getattr(QFrame, "Shape", QFrame), "NoFrame")
SIZE_POLICY_FIXED = getattr(getattr(QSizePolicy, "Policy", QSizePolicy), "Fixed")
HIGHLIGHT_ROLE = getattr(getattr(QPalette, "ColorRole", QPalette), "Highlight")


@dataclass(frozen=True)
class Lesson:
    index: int
    title: str
    link: str


def build_lessons() -> list[Lesson]:
    lessons = [
        Lesson(
            index=index,
            title=title,
            link=PUBLISHED_LESSON_LINKS[index - 1],
        )
        for index, title in enumerate(PUBLISHED_LESSON_TITLES, start=1)
    ]
    for index in range(len(lessons) + 1, TOTAL_LESSON_COUNT + 1):
        lessons.append(
            Lesson(
                index=index,
                title=f"Reserved lesson slot {index}",
                link="",
            )
        )
    return lessons


def get_next_class_date(start_date: date, search_date: date) -> date:
    cursor_date = max(start_date, search_date)
    while cursor_date.isoweekday() not in CLASS_WEEKDAYS:
        cursor_date += timedelta(days=1)
    return cursor_date


def get_next_monday(reference_date: date) -> date:
    days_until_monday = (8 - reference_date.isoweekday()) % 7
    if days_until_monday == 0:
        days_until_monday = 7
    return reference_date + timedelta(days=days_until_monday)


def count_lessons_through(target_date: date, start_date: date) -> int:
    if target_date < start_date:
        return 0
    lesson_count = 0
    cursor_date = start_date
    while cursor_date <= target_date:
        if cursor_date.isoweekday() in CLASS_WEEKDAYS:
            lesson_count += 1
        cursor_date += timedelta(days=1)
    return lesson_count


def format_date_label(class_date: date) -> str:
    weekday_name = CLASS_WEEKDAYS[class_date.isoweekday()]
    return f"{class_date:%Y-%m-%d} {weekday_name} 19:30"


def is_published_lesson(lesson: Lesson | None) -> bool:
    return lesson is not None and lesson.index <= PUBLISHED_LESSON_COUNT


def get_student_lesson_title(lesson: Lesson | None) -> str:
    if is_published_lesson(lesson):
        return lesson.title
    if lesson is None:
        return "后续课程安排待发布"
    return f"第 {lesson.index} 节课程内容待发布"


def build_resource_links_markup(
    link_url: str,
    link_text: str = "课程辅助文档链接",
    link_color: str | None = None,
) -> str:
    course_label = "课程辅助文档链接："
    if not link_url:
        return f"{course_label}{link_text}"
    escaped_url = escape(link_url, quote=True)
    escaped_text = escape(link_text)
    style_text = f' style="color: {escape(link_color, quote=True)};"' if link_color else ""
    link_markup = f'<a href="{escaped_url}"{style_text}>{escaped_text}</a>'
    return f"{course_label}{link_markup}"


def convert_qdate_to_date(qdate: QDate) -> date:
    if hasattr(qdate, "toPython"):
        return qdate.toPython()
    return qdate.toPyDate()


def run_application(app: QApplication) -> int:
    if hasattr(app, "exec"):
        return app.exec()
    return app.exec_()


class LessonCard(QFrame):
    def __init__(self, title: str, meta_text: str, link_url: str, accent_color: str) -> None:
        super().__init__()
        self.setObjectName("lessonCard")
        self.setStyleSheet(
            f"""
            QFrame#lessonCard {{
                background: #ffffff;
                border: 1px solid #dce7ef;
                border-left: 5px solid {accent_color};
                border-radius: 8px;
            }}
            """
        )

        title_label = QLabel(title)
        title_label.setWordWrap(True)
        title_label.setObjectName("cardTitle")

        meta_label = QLabel(meta_text)
        meta_label.setWordWrap(True)
        meta_label.setObjectName("cardMeta")

        link_label = QLabel(build_resource_links_markup(link_url))
        link_label.setObjectName("cardLink")
        link_label.setOpenExternalLinks(True)
        link_label.setWordWrap(True)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 12, 16, 14)
        layout.setSpacing(8)
        layout.addWidget(title_label)
        layout.addWidget(meta_label)
        if link_url:
            layout.addWidget(link_label)


class ScheduleWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.lessons = build_lessons()
        self.today = date.today()
        self.default_start_date = get_next_monday(self.today)
        self.setWindowTitle("教师课程排课助手")
        self.resize(1120, 780)

        self.start_date_edit = QDateEdit(
            QDate(
                self.default_start_date.year,
                self.default_start_date.month,
                self.default_start_date.day,
            )
        )
        self.start_date_edit.setCalendarPopup(True)
        self.start_date_edit.setDisplayFormat("yyyy-MM-dd")
        self.start_date_edit.dateChanged.connect(self.refresh_schedule)

        root = QWidget()
        root.setObjectName("root")
        self.setCentralWidget(root)

        self.today_title = QLabel()
        self.today_title.setObjectName("todayTitle")
        self.today_meta = QLabel()
        self.today_meta.setObjectName("todayMeta")
        self.today_hint = QLabel()
        self.today_hint.setObjectName("todayHint")
        self.today_hint.setWordWrap(True)
        self.today_link = QLabel()
        self.today_link.setObjectName("todayLink")
        self.today_link.setOpenExternalLinks(True)
        self.today_link.setWordWrap(True)

        self.next_week_layout = QVBoxLayout()
        self.lesson_grid = QGridLayout()
        self.lesson_grid.setSpacing(12)

        self.build_layout(root)
        self.apply_styles()
        self.refresh_schedule()

    def build_layout(self, root: QWidget) -> None:
        page_layout = QVBoxLayout(root)
        page_layout.setContentsMargins(28, 24, 28, 24)
        page_layout.setSpacing(18)

        header_layout = QHBoxLayout()
        title_box = QVBoxLayout()
        title_label = QLabel("IoT 全栈嵌入式远程项目实战课表")
        title_label.setObjectName("pageTitle")
        subtitle_label = QLabel("每周一、三、五晚 19:30 自动排课，查看今日课程与近期安排。")
        subtitle_label.setObjectName("pageSubtitle")
        title_box.addWidget(title_label)
        title_box.addWidget(subtitle_label)

        control_group = QGroupBox("排课起点")
        control_layout = QHBoxLayout(control_group)
        control_layout.setContentsMargins(14, 18, 14, 12)
        start_label = QLabel("第 1 节日期")
        reset_button = QPushButton("设为下周一")
        reset_button.clicked.connect(self.reset_start_date_to_next_monday)
        control_layout.addWidget(start_label)
        control_layout.addWidget(self.start_date_edit)
        control_layout.addWidget(reset_button)

        header_layout.addLayout(title_box, stretch=1)
        header_layout.addWidget(control_group)
        page_layout.addLayout(header_layout)

        top_layout = QHBoxLayout()
        top_layout.setSpacing(16)
        top_layout.addWidget(self.build_today_panel(), stretch=2)
        top_layout.addWidget(self.build_week_panel(), stretch=3)
        page_layout.addLayout(top_layout)

        lesson_group = QGroupBox("已发布课程内容")
        lesson_group_layout = QVBoxLayout(lesson_group)
        lesson_group_layout.setContentsMargins(14, 18, 14, 14)
        lesson_group_layout.setSpacing(10)

        scroll_area = QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setFrameShape(FRAME_NO_FRAME)
        scroll_content = QWidget()
        scroll_content.setLayout(self.lesson_grid)
        scroll_area.setWidget(scroll_content)
        lesson_group_layout.addWidget(scroll_area)
        page_layout.addWidget(lesson_group, stretch=1)

    def build_today_panel(self) -> QFrame:
        panel = QFrame()
        panel.setObjectName("todayPanel")
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(22, 20, 22, 22)
        layout.setSpacing(12)

        badge_label = QLabel("今日课程")
        badge_label.setObjectName("badge")
        badge_label.setSizePolicy(SIZE_POLICY_FIXED, SIZE_POLICY_FIXED)

        layout.addWidget(badge_label)
        layout.addWidget(self.today_title)
        layout.addWidget(self.today_meta)
        layout.addStretch(1)
        layout.addWidget(self.today_hint)
        layout.addWidget(self.today_link)
        return panel

    def build_week_panel(self) -> QGroupBox:
        panel = QGroupBox("未来一周上课安排")
        panel_layout = QVBoxLayout(panel)
        panel_layout.setContentsMargins(14, 18, 14, 14)
        panel_layout.setSpacing(10)
        panel_layout.addLayout(self.next_week_layout)
        return panel

    def reset_start_date_to_next_monday(self) -> None:
        next_monday = get_next_monday(date.today())
        self.today = date.today()
        self.default_start_date = next_monday
        self.start_date_edit.setDate(QDate(next_monday.year, next_monday.month, next_monday.day))

    def refresh_schedule(self) -> None:
        start_date = convert_qdate_to_date(self.start_date_edit.date())
        self.update_today_panel(start_date)
        self.update_next_week_panel(start_date)
        self.update_lesson_grid(start_date)

    def update_today_panel(self, start_date: date) -> None:
        today_lesson = self.get_lesson_for_date(self.today, start_date)
        if today_lesson is None:
            next_class_date = get_next_class_date(start_date, self.today)
            next_lesson_number = count_lessons_through(next_class_date, start_date)
            self.today_title.setText("今天没有安排课程")
            self.today_meta.setText(
                f"下一次上课：{format_date_label(next_class_date)}｜第 {next_lesson_number} 节"
            )
            next_lesson = self.get_lesson_by_number(next_lesson_number)
            self.today_hint.setText("可以预习一下下一节课的内容：")
            self.today_link.setText(
                build_resource_links_markup(next_lesson.link if next_lesson else "", link_color="#facc15")
            )
            return

        self.today_title.setText(get_student_lesson_title(today_lesson))
        self.today_meta.setText(f"今天 {format_date_label(self.today)}｜第 {today_lesson.index} 节")
        self.today_hint.setText("本节课内容：")
        self.today_link.setText(build_resource_links_markup(today_lesson.link, link_color="#facc15"))

    def update_next_week_panel(self, start_date: date) -> None:
        clear_layout(self.next_week_layout)
        cursor_date = self.today
        end_date = self.today + timedelta(days=PREVIEW_DAYS)
        has_class = False
        while cursor_date <= end_date:
            if cursor_date.isoweekday() in CLASS_WEEKDAYS and cursor_date >= start_date:
                lesson_number = count_lessons_through(cursor_date, start_date)
                lesson = self.get_lesson_by_number(lesson_number)
                if lesson is not None:
                    self.next_week_layout.addWidget(
                        LessonCard(
                            title=get_student_lesson_title(lesson),
                            meta_text=f"{format_date_label(cursor_date)}｜第 {lesson.index} 节",
                            link_url=lesson.link,
                            accent_color="#f59e0b",
                        )
                    )
                    has_class = True
            cursor_date += timedelta(days=1)
        if not has_class:
            empty_label = QLabel("未来一周没有可显示课程，请检查开课日期。")
            empty_label.setWordWrap(True)
            empty_label.setObjectName("emptyText")
            self.next_week_layout.addWidget(empty_label)
        self.next_week_layout.addStretch(1)

    def update_lesson_grid(self, start_date: date) -> None:
        clear_layout(self.lesson_grid)
        for lesson in self.lessons[:PUBLISHED_LESSON_COUNT]:
            class_date = self.get_class_date_by_lesson_number(lesson.index, start_date)
            card = LessonCard(
                title=lesson.title,
                meta_text=f"预计时间：{format_date_label(class_date)}｜第 {lesson.index} 节",
                link_url=lesson.link,
                accent_color="#f97316",
            )
            row = (lesson.index - 1) // 3
            column = (lesson.index - 1) % 3
            self.lesson_grid.addWidget(card, row, column)
        for column in range(3):
            self.lesson_grid.setColumnStretch(column, 1)

    def get_lesson_for_date(self, class_date: date, start_date: date) -> Lesson | None:
        if class_date.isoweekday() not in CLASS_WEEKDAYS or class_date < start_date:
            return None
        lesson_number = count_lessons_through(class_date, start_date)
        return self.get_lesson_by_number(lesson_number)

    def get_lesson_by_number(self, lesson_number: int) -> Lesson | None:
        if lesson_number < 1 or lesson_number > len(self.lessons):
            return None
        return self.lessons[lesson_number - 1]

    def get_class_date_by_lesson_number(self, lesson_number: int, start_date: date) -> date:
        cursor_date = start_date
        found_count = 0
        while found_count < lesson_number:
            if cursor_date.isoweekday() in CLASS_WEEKDAYS:
                found_count += 1
            if found_count == lesson_number:
                return cursor_date
            cursor_date += timedelta(days=1)
        return cursor_date

    def apply_styles(self) -> None:
        QApplication.instance().setFont(QFont("Microsoft YaHei", 10))
        self.setStyleSheet(
            """
            QWidget#root {
                background: #f4f7fb;
                color: #22313f;
            }
            QLabel#pageTitle {
                color: #16232e;
                font-size: 26px;
                font-weight: 700;
            }
            QLabel#pageSubtitle {
                color: #5f7182;
                font-size: 13px;
            }
            QGroupBox {
                background: #ffffff;
                border: 1px solid #dbe5ee;
                border-radius: 8px;
                margin-top: 10px;
                font-weight: 700;
                color: #22313f;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 14px;
                padding: 0 6px;
            }
            QFrame#todayPanel {
                background: #172554;
                border-radius: 8px;
            }
            QLabel#badge {
                background: #f6c85f;
                color: #21313a;
                border-radius: 8px;
                padding: 5px 10px;
                font-weight: 700;
            }
            QLabel#todayTitle {
                color: #ffffff;
                font-size: 23px;
                font-weight: 700;
            }
            QLabel#todayMeta {
                color: #cbe4eb;
                font-size: 13px;
            }
            QLabel#todayHint {
                color: #f5fbfd;
                background: rgba(255, 255, 255, 0.12);
                border-radius: 8px;
                padding: 10px 12px 0 12px;
                line-height: 150%;
            }
            QLabel#todayLink {
                color: #ffffff;
                background: rgba(255, 255, 255, 0.12);
                border-radius: 8px;
                padding: 0 12px 10px 12px;
                line-height: 150%;
            }
            QLabel#todayLink a {
                color: #facc15;
                text-decoration: underline;
                font-weight: 700;
            }
            QLabel#cardTitle {
                color: #172533;
                font-size: 14px;
                font-weight: 700;
            }
            QLabel#cardMeta {
                color: #5a6c7d;
                font-size: 12px;
            }
            QLabel#cardLink {
                color: #1d4ed8;
                font-size: 12px;
                font-weight: 700;
            }
            QLabel#emptyText {
                color: #5f7182;
                padding: 16px;
            }
            QPushButton {
                background: #ea580c;
                border: none;
                border-radius: 6px;
                color: #ffffff;
                font-weight: 700;
                padding: 7px 12px;
            }
            QPushButton:hover {
                background: #c2410c;
            }
            QDateEdit {
                background: #f8fbfc;
                border: 1px solid #cddbe6;
                border-radius: 6px;
                padding: 6px 10px;
                min-width: 118px;
            }
            QScrollArea {
                background: transparent;
            }
            """
        )


def clear_layout(layout: QVBoxLayout | QGridLayout) -> None:
    while layout.count():
        item = layout.takeAt(0)
        widget = item.widget()
        child_layout = item.layout()
        if widget is not None:
            widget.deleteLater()
        if child_layout is not None:
            clear_layout(child_layout)


def main() -> int:
    app = QApplication(sys.argv)
    palette = app.palette()
    palette.setColor(HIGHLIGHT_ROLE, QColor("#ea580c"))
    app.setPalette(palette)
    window = ScheduleWindow()
    window.show()
    return run_application(app)


if __name__ == "__main__":
    raise SystemExit(main())
