# -*- coding: utf-8 -*-
"""
机械臂控制上位机 - 虚拟模拟运行版
在原版 QWERTYYUIOP.py 基础上:
  1. 添加虚拟串口模拟（无需真实 ESP32 即可测试）
  2. 添加 Canvas 2D 机械臂可视化（实时显示6轴姿态）
  3. 所有原始功能保留
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import threading
import time
import json
import math

# ============ 虚拟串口类 ============
class VirtualSerial:
    """模拟 serial.Serial，无需真实硬件"""
    def __init__(self, port="VIRTUAL", baudrate=115200, timeout=0.1):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.is_open = True
        self.in_waiting = 0
        self._log = []

    def write(self, data):
        cmd = data.decode('utf-8').strip()
        self._log.append(cmd)
        # print(f"[虚拟串口] 发送: {cmd}")

    def readline(self):
        return b"OK\n"

    def close(self):
        self.is_open = False

    def get_log(self):
        return self._log


class ArmController:
    def __init__(self, root):
        self.root = root
        self.root.title("\u673a\u68b0\u81c2\u63a7\u5236\u5668 - \u865a\u62df\u6a21\u62df\u8fd0\u884c")
        self.root.geometry("1050x700")
        self.root.resizable(False, False)
        self.root.configure(bg="#2b2b2b")

        # \u4e32\u53e3\u76f8\u5173
        self.serial_port = None
        self.baud_rate = 115200
        self.is_connected = False
        self.virtual_mode = False

        # \u8235\u673a\u89d2\u5ea6\u503c
        self.servo_angles = [90, 90, 90, 90, 90, 90]

        # \u811a\u672c\u5f55\u5236\u76f8\u5173
        self.is_recording = False
        self.recorded_actions = []
        self.record_start_time = 0
        self.last_action_time = 0

        # \u811a\u672c\u56de\u653e\u76f8\u5173
        self.is_playing = False
        self.current_script = []
        self.play_thread = None

        # \u521b\u5efa\u4e3b\u754c\u9762
        self.create_widgets()
        self.scan_ports()

        # \u542f\u52a8\u53ef\u89c6\u5316\u5237\u65b0
        self.update_arm_visual()

    def create_widgets(self):
        style = ttk.Style()
        style.theme_use('clam')
        style.configure("TLabelframe", background="#2b2b2b", foreground="#ffffff")
        style.configure("TLabelframe.Label", background="#2b2b2b", foreground="#00ccff")
        style.configure("TLabel", background="#2b2b2b", foreground="#ffffff")
        style.configure("TButton", background="#3c3f41", foreground="#ffffff")
        style.configure("TFrame", background="#2b2b2b")

        # \u5de6\u4fa7\u63a7\u5236\u533a
        left_frame = ttk.Frame(self.root, padding="10")
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=False)

        # \u53f3\u4fa7\u53ef\u89c6\u5316\u533a
        right_frame = ttk.Frame(self.root, padding="10")
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        # ===== \u53f3\u4fa7: \u673a\u68b0\u81c2\u53ef\u89c6\u5316 =====
        vis_frame = ttk.LabelFrame(right_frame, text="\u673a\u68b0\u81c2\u53ef\u89c6\u5316 (2D\u4fa7\u89c6\u56fe)", padding="5")
        vis_frame.pack(fill=tk.BOTH, expand=True)

        self.canvas = tk.Canvas(vis_frame, width=450, height=550, bg="#1a1a2e", highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True)

        # ===== \u5de6\u4fa7: \u4e32\u53e3\u63a7\u5236 =====
        port_frame = ttk.LabelFrame(left_frame, text="\u4e32\u53e3\u8bbe\u7f6e", padding="10")
        port_frame.pack(fill=tk.X, pady=5)

        ttk.Label(port_frame, text="\u4e32\u53e3\uff1a").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(port_frame, textvariable=self.port_var, width=12)
        self.port_combo.grid(row=0, column=1, sticky=tk.W, pady=5)

        ttk.Button(port_frame, text="\u626b\u63cf", command=self.scan_ports).grid(row=0, column=2, padx=3, pady=5)
        self.connect_btn = ttk.Button(port_frame, text="\u8fde\u63a5", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=3, padx=3, pady=5)

        # \u865a\u62df\u6a21\u5f0f\u6309\u94ae
        self.virtual_btn = ttk.Button(port_frame, text="\u865a\u62df\u6a21\u5f0f", command=self.start_virtual_mode)
        self.virtual_btn.grid(row=1, column=0, columnspan=2, padx=3, pady=5, sticky=tk.W)

        self.status_var = tk.StringVar(value="\u672a\u8fde\u63a5")
        status_label = ttk.Label(port_frame, textvariable=self.status_var)
        status_label.grid(row=1, column=2, columnspan=2, sticky=tk.W, pady=5)

        # ===== \u8235\u673a\u63a7\u5236 =====
        servo_frame = ttk.LabelFrame(left_frame, text="\u8235\u673a\u63a7\u5236", padding="10")
        servo_frame.pack(fill=tk.BOTH, expand=True, pady=5)

        self.sliders = []
        self.slider_vars = []
        servo_names = ["\u5e95\u5ea7\u65cb\u8f6c", "\u80a9\u90e8\u4ef0\u4fef", "\u80a0\u90e8\u5c48\u4f38", "\u8155\u90e8\u65cb\u8f6c", "\u8155\u90e8\u4ef0\u4fef", "\u722a\u5b50\u5f00\u5408"]

        for i in range(6):
            frame = ttk.Frame(servo_frame)
            frame.pack(fill=tk.X, pady=2)

            ttk.Label(frame, text=f"{servo_names[i]}\uff1a", width=10).pack(side=tk.LEFT, padx=2)

            var = tk.IntVar(value=self.servo_angles[i])
            self.slider_vars.append(var)

            slider = ttk.Scale(frame, from_=0, to=180, orient=tk.HORIZONTAL, length=200,
                             variable=var, command=lambda v, idx=i: self.on_slider_change(idx, v))
            slider.pack(side=tk.LEFT, padx=2)
            self.sliders.append(slider)

            spinbox = ttk.Spinbox(frame, from_=0, to=180, width=4, textvariable=var,
                                 command=lambda idx=i: self.on_spinbox_change(idx))
            spinbox.pack(side=tk.LEFT, padx=2)

        # \u63a7\u5236\u6309\u94ae
        button_frame = ttk.Frame(left_frame, padding="3")
        button_frame.pack(fill=tk.X, pady=3)

        ttk.Button(button_frame, text="\u590d\u4f4d\u6240\u6709", command=self.reset_all_servos).pack(side=tk.LEFT, padx=3)
        ttk.Button(button_frame, text="\u540c\u65f6\u63a7\u5236", command=self.control_all_servos).pack(side=tk.LEFT, padx=3)
        ttk.Button(button_frame, text="\u6f14\u793a\u52a8\u4f5c", command=self.demo_motion).pack(side=tk.LEFT, padx=3)

        # ===== \u811a\u672c\u5f55\u5236\u56de\u653e =====
        script_frame = ttk.LabelFrame(left_frame, text="\u811a\u672c\u5f55\u5236\u4e0e\u56de\u653e", padding="5")
        script_frame.pack(fill=tk.X, pady=5)

        btn_frame = ttk.Frame(script_frame)
        btn_frame.pack(fill=tk.X, pady=3)

        self.record_btn = ttk.Button(btn_frame, text="\u5f00\u59cb\u5f55\u5236", command=self.toggle_recording)
        self.record_btn.pack(side=tk.LEFT, padx=3)

        self.play_btn = ttk.Button(btn_frame, text="\u56de\u653e\u811a\u672c", command=self.toggle_playback)
        self.play_btn.pack(side=tk.LEFT, padx=3)

        ttk.Button(btn_frame, text="\u4fdd\u5b58", command=self.save_script).pack(side=tk.LEFT, padx=3)
        ttk.Button(btn_frame, text="\u52a0\u8f7d", command=self.load_script).pack(side=tk.LEFT, padx=3)
        ttk.Button(btn_frame, text="\u6e05\u7a7a", command=self.clear_recording).pack(side=tk.LEFT, padx=3)

        self.record_status = tk.StringVar(value="\u672a\u5f55\u5236")
        ttk.Label(btn_frame, textvariable=self.record_status).pack(side=tk.LEFT, padx=10)

        self.action_count_var = tk.StringVar(value="\u5df2\u5f55\u5236\uff1a0")
        ttk.Label(script_frame, textvariable=self.action_count_var).pack(anchor=tk.W)

        # \u56de\u653e\u901f\u5ea6
        speed_frame = ttk.Frame(script_frame)
        speed_frame.pack(fill=tk.X, pady=3)
        ttk.Label(speed_frame, text="\u56de\u653e\u901f\u5ea6\uff1a").pack(side=tk.LEFT, padx=3)
        self.speed_var = tk.DoubleVar(value=1.0)
        ttk.Scale(speed_frame, from_=0.1, to=3.0, orient=tk.HORIZONTAL,
                  length=150, variable=self.speed_var).pack(side=tk.LEFT, padx=3)
        self.speed_label = ttk.Label(speed_frame, text="1.0x")
        self.speed_label.pack(side=tk.LEFT, padx=3)

    # ============ \u53ef\u89c6\u5316 ============
    def update_arm_visual(self):
        """\u5b9e\u65f6\u7ed8\u5236\u673a\u68b0\u81c2 2D \u4fa7\u89c6\u56fe"""
        c = self.canvas
        c.delete("all")
        W = int(c.winfo_width()) or 450
        H = int(c.winfo_height()) or 550

        # \u80cc\u666f\u7f51\u683c
        for x in range(0, W, 30):
            c.create_line(x, 0, x, H, fill="#252540", width=1)
        for y in range(0, H, 30):
            c.create_line(0, y, W, y, fill="#252540", width=1)

        # \u5e95\u5ea7
        base_x, base_y = W // 2, H - 50
        c.create_rectangle(base_x - 60, base_y, base_x + 60, base_y + 30,
                          fill="#444466", outline="#00ccff", width=2)
        c.create_text(base_x, base_y + 15, text="\u5e95\u5ea7", fill="#00ccff", font=("", 9))

        # \u5e95\u5ea7\u65cb\u8f6c\u6307\u793a
        a0 = self.servo_angles[0]
        c.create_text(base_x, base_y + 40, text=f"\u65cb\u8f6c: {a0}\u00b0", fill="#888888", font=("", 8))

        # 根据舵机角度计算各关节位置
        # 舵机1=底座旋转, 舵机2=肩部仰俯, 舵机3=肘部屈伸, 舵机4=腕部旋转, 舵机5=腕部仰俯, 舵机6=爪子开合
        # 5段独立臂段 + 独立爪子
        arm_lengths = [100, 80, 40, 35, 30]  # 大臂、小臂、腕旋转段、腕仰俯段、爪子连接
        colors = ["#ff6b6b", "#ffd93d", "#ff9f43", "#6bcb77", "#4d96ff"]
        labels = ["\u5927\u81c2", "\u5c0f\u81c2", "\u8155\u65cb\u8f6c", "\u8155\u4ef0\u4fef", "\u722a\u5b50"]
        angle_indices = [1, 2, 3, 4, 5]  # 每个舵机独立控制一段

        # 底座旋转影响水平偏移
        base_offset_x = int((a0 - 90) * 0.5)

        x, y = base_x + base_offset_x, base_y
        joints = [(x, y)]
        cumulative_angle = 0

        for i in range(5):
            servo_angle = self.servo_angles[angle_indices[i]]

            if i == 2:
                # 舵机4=腕部旋转：用水平偏移表示旋转（2D侧视图中旋转体现为左右偏移）
                rot_offset = (servo_angle - 90) * 0.25
                nx = x + rot_offset
                ny = y - arm_lengths[i]  # 仍向上延伸
            else:
                # 其他舵机正常按角度计算
                cumulative_angle += (servo_angle - 90)
                total_rad = math.radians(-90 + cumulative_angle * 0.55)
                nx = x + arm_lengths[i] * math.cos(total_rad)
                ny = y + arm_lengths[i] * math.sin(total_rad)

            # 画臂段
            c.create_line(x, y, nx, ny, fill=colors[i], width=6, capstyle=tk.ROUND)
            # 关节圆
            c.create_oval(nx - 6, ny - 6, nx + 6, ny + 6, fill=colors[i], outline="#ffffff", width=2)
            # 标签
            c.create_text(nx + 15, ny - 10, text=f"{labels[i]}\n{servo_angle}\u00b0",
                        fill=colors[i], font=("", 8), anchor=tk.W)

            joints.append((nx, ny))
            x, y = nx, ny

        # 爪子开合可视化（舵机6独立控制）
        claw_angle = self.servo_angles[5]
        claw_open = (claw_angle - 90) * 0.25
        # 计算爪子方向（沿最后一段臂的延伸方向）
        if len(joints) >= 2:
            px, py = joints[-2]
            dx, dy = x - px, y - py
            length = math.sqrt(dx*dx + dy*dy) or 1
            dx, dy = dx/length, dy/length
            # 垂直于臂段方向的左右张开
            perp_x, perp_y = -dy, dx
            claw_len = 18
            # 左爪
            c.create_line(x, y,
                         x + dx * claw_len + perp_x * (6 + claw_open),
                         y + dy * claw_len + perp_y * (6 + claw_open),
                         fill="#4d96ff", width=4, capstyle=tk.ROUND)
            # 右爪
            c.create_line(x, y,
                         x + dx * claw_len - perp_x * (6 + claw_open),
                         y + dy * claw_len - perp_y * (6 + claw_open),
                         fill="#4d96ff", width=4, capstyle=tk.ROUND)

        # \u72b6\u6001\u6587\u5b57
        status = "\u865a\u62df\u6a21\u5f0f" if self.virtual_mode else ("\u5df2\u8fde\u63a5" if self.is_connected else "\u672a\u8fde\u63a5")
        color = "#00ff88" if (self.virtual_mode or self.is_connected) else "#ff4444"
        c.create_text(10, 15, text=f"\u72b6\u6001: {status}", fill=color, font=("", 10, "bold"), anchor=tk.W)

        if self.is_recording:
            c.create_oval(W - 25, 10, W - 10, 25, fill="red")
            c.create_text(W - 35, 18, text="REC", fill="red", font=("", 9, "bold"), anchor=tk.E)

        if self.is_playing:
            c.create_text(W - 15, 18, text="\u25b6", fill="#00ccff", font=("", 14), anchor=tk.E)

        # \u5b9a\u65f6\u5237\u65b0
        self.root.after(50, self.update_arm_visual)

    # ============ \u865a\u62df\u6a21\u5f0f ============
    def start_virtual_mode(self):
        """\u542f\u52a8\u865a\u62df\u4e32\u53e3\u6a21\u5f0f"""
        self.serial_port = VirtualSerial()
        self.is_connected = True
        self.virtual_mode = True
        self.connect_btn.config(text="\u65ad\u5f00")
        self.status_var.set("\u865a\u62df\u6a21\u5f0f\u2705")
        messagebox.showinfo("\u865a\u62df\u6a21\u5f0f", "\u5df2\u8fdb\u5165\u865a\u62df\u6a21\u62df\u6a21\u5f0f\uff01\n\u65e0\u9700\u771f\u5b9e\u786c\u4ef6\uff0c\u6240\u6709\u64cd\u4f5c\u5c06\u5728\u53ef\u89c6\u5316\u7a97\u53e3\u4e2d\u6a21\u62df\u663e\u793a\u3002\n\n\u8bf7\u62d6\u52a8\u6ed1\u5757\u63a7\u5236\u8235\u673a\uff01")

    # ============ \u6f14\u793a\u52a8\u4f5c ============
    def demo_motion(self):
        """\u6f14\u793a\u4e00\u7ec4\u9884\u8bbe\u52a8\u4f5c"""
        if not self.is_connected and not self.virtual_mode:
            messagebox.showerror("\u9519\u8bef", "\u8bf7\u5148\u8fde\u63a5\u4e32\u53e3\u6216\u542f\u7528\u865a\u62df\u6a21\u5f0f\uff01")
            return

        def run_demo():
            demo_sequence = [
                # [servo_index, target_angle, delay_ms]
                [1, 45, 300],
                [2, 60, 300],
                [1, 135, 400],
                [2, 120, 300],
                [3, 45, 300],
                [4, 120, 200],
                [5, 30, 400],
                [5, 150, 400],
                [5, 90, 300],
                [3, 90, 300],
                [2, 90, 300],
                [1, 90, 300],
                [0, 45, 400],
                [0, 135, 400],
                [0, 90, 300],
            ]

            for idx, target, delay in demo_sequence:
                if not self.is_connected:
                    break
                current = self.servo_angles[idx]
                step = 1 if target > current else -1
                for a in range(current, target + step, step * 3):
                    a = max(0, min(180, a))
                    self.servo_angles[idx] = a
                    self.root.after(0, lambda i=idx, angle=a: self.slider_vars[i].set(angle))
                    if self.serial_port:
                        try:
                            self.serial_port.write(f"SERVO{idx+1}:{a}\n".encode('utf-8'))
                        except:
                            pass
                    time.sleep(0.02)
                self.servo_angles[idx] = target
                self.root.after(0, lambda i=idx, angle=target: self.slider_vars[i].set(angle))
                time.sleep(delay / 1000.0)

            self.root.after(0, lambda: messagebox.showinfo("\u5b8c\u6210", "\u6f14\u793a\u52a8\u4f5c\u5b8c\u6210\uff01"))

        threading.Thread(target=run_demo, daemon=True).start()

    # ============ \u4ee5\u4e0b\u4e3a\u539f\u7248\u529f\u80fd\uff08\u7565\u4fee\u6539\u4ee5\u517c\u5bb9\u865a\u62df\u6a21\u5f0f\uff09 ============

    def scan_ports(self):
        try:
            import serial.tools.list_ports
            ports = serial.tools.list_ports.comports()
            port_list = [port.device for port in ports]
        except ImportError:
            port_list = []
        self.port_combo['values'] = port_list
        if port_list:
            self.port_var.set(port_list[0])

    def toggle_connection(self):
        if not self.is_connected:
            port = self.port_var.get()
            if not port:
                messagebox.showerror("\u9519\u8bef", "\u8bf7\u9009\u62e9\u4e32\u53e3\u6216\u4f7f\u7528\u865a\u62df\u6a21\u5f0f")
                return
            try:
                import serial
                self.serial_port = serial.Serial(port, self.baud_rate, timeout=0.1)
                self.is_connected = True
                self.virtual_mode = False
                self.connect_btn.config(text="\u65ad\u5f00")
                self.status_var.set("\u5df2\u8fde\u63a5")
                threading.Thread(target=self.receive_data, daemon=True).start()
                messagebox.showinfo("\u6210\u529f", f"\u5df2\u8fde\u63a5\u5230 {port}")
            except Exception as e:
                messagebox.showerror("\u9519\u8bef", f"\u8fde\u63a5\u5931\u8d25\uff1a{str(e)}")
        else:
            if self.serial_port and not self.virtual_mode:
                self.serial_port.close()
            self.serial_port = None
            self.is_connected = False
            self.virtual_mode = False
            self.connect_btn.config(text="\u8fde\u63a5")
            self.status_var.set("\u672a\u8fde\u63a5")

    def receive_data(self):
        while self.is_connected and not self.virtual_mode:
            try:
                if self.serial_port and hasattr(self.serial_port, 'in_waiting') and self.serial_port.in_waiting:
                    self.serial_port.readline()
                time.sleep(0.1)
            except:
                pass

    def on_slider_change(self, idx, value):
        angle = int(float(value))
        self.servo_angles[idx] = angle
        if self.is_recording:
            self.record_action("SERVO", idx + 1, angle)
        self.control_single_servo(idx + 1, angle)

    def on_spinbox_change(self, idx):
        angle = self.slider_vars[idx].get()
        self.servo_angles[idx] = angle
        self.sliders[idx].set(angle)
        if self.is_recording:
            self.record_action("SERVO", idx + 1, angle)
        self.control_single_servo(idx + 1, angle)

    def record_action(self, action_type, target, value):
        current_time = time.time()
        if self.last_action_time > 0:
            delay_ms = int((current_time - self.last_action_time) * 1000)
            if delay_ms > 50:
                self.recorded_actions.append({"type": "DELAY", "delay": delay_ms})
        action = {"type": action_type, "target": target, "value": value,
                  "timestamp": current_time - self.record_start_time}
        self.recorded_actions.append(action)
        self.action_count_var.set(f"\u5df2\u5f55\u5236\uff1a{len(self.recorded_actions)}")
        self.last_action_time = current_time

    def toggle_recording(self):
        if not self.is_recording:
            if not self.is_connected:
                messagebox.showerror("\u9519\u8bef", "\u8bf7\u5148\u8fde\u63a5\u4e32\u53e3\u6216\u542f\u7528\u865a\u62df\u6a21\u5f0f\uff01")
                return
            self.is_recording = True
            self.recorded_actions = []
            self.record_start_time = time.time()
            self.last_action_time = 0
            self.record_btn.config(text="\u505c\u6b62\u5f55\u5236")
            self.record_status.set("\u6b63\u5728\u5f55\u5236...")
        else:
            self.is_recording = False
            self.record_btn.config(text="\u5f00\u59cb\u5f55\u5236")
            self.record_status.set(f"\u5f55\u5236\u5b8c\u6210({len(self.recorded_actions)}\u6b65)")
            messagebox.showinfo("\u63d0\u793a", f"\u5f55\u5236\u5b8c\u6210\uff01\u5171 {len(self.recorded_actions)} \u4e2a\u52a8\u4f5c\u3002")

    def clear_recording(self):
        if self.is_recording:
            messagebox.showwarning("\u8b66\u544a", "\u6b63\u5728\u5f55\u5236\u4e2d\uff0c\u8bf7\u5148\u505c\u6b62\uff01")
            return
        self.recorded_actions = []
        self.action_count_var.set("\u5df2\u5f55\u5236\uff1a0")
        self.record_status.set("\u672a\u5f55\u5236")

    def save_script(self):
        if not self.recorded_actions:
            messagebox.showwarning("\u8b66\u544a", "\u6ca1\u6709\u53ef\u4fdd\u5b58\u7684\u5f55\u5236\u5185\u5bb9\uff01")
            return
        file_path = filedialog.asksaveasfilename(title="\u4fdd\u5b58\u811a\u672c", defaultextension=".json",
                                                  filetypes=[("JSON\u6587\u4ef6", "*.json")])
        if file_path:
            try:
                with open(file_path, 'w', encoding='utf-8') as f:
                    json.dump(self.recorded_actions, f, indent=2, ensure_ascii=False)
                messagebox.showinfo("\u6210\u529f", f"\u5df2\u4fdd\u5b58\u5230\uff1a{file_path}")
            except Exception as e:
                messagebox.showerror("\u9519\u8bef", f"\u4fdd\u5b58\u5931\u8d25\uff1a{str(e)}")

    def load_script(self):
        if self.is_playing:
            messagebox.showwarning("\u8b66\u544a", "\u6b63\u5728\u56de\u653e\u4e2d\uff01")
            return
        file_path = filedialog.askopenfilename(title="\u52a0\u8f7d\u811a\u672c",
                                                filetypes=[("JSON\u6587\u4ef6", "*.json")])
        if file_path:
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    self.current_script = json.load(f)
                self.action_count_var.set(f"\u5df2\u52a0\u8f7d\uff1a{len(self.current_script)}")
                messagebox.showinfo("\u6210\u529f", f"\u5df2\u52a0\u8f7d {len(self.current_script)} \u4e2a\u52a8\u4f5c")
            except Exception as e:
                messagebox.showerror("\u9519\u8bef", f"\u52a0\u8f7d\u5931\u8d25\uff1a{str(e)}")

    def toggle_playback(self):
        if not self.is_playing:
            if not self.is_connected:
                messagebox.showerror("\u9519\u8bef", "\u8bf7\u5148\u8fde\u63a5\u6216\u542f\u7528\u865a\u62df\u6a21\u5f0f\uff01")
                return
            script = self.current_script if self.current_script else self.recorded_actions
            if not script:
                messagebox.showwarning("\u8b66\u544a", "\u6ca1\u6709\u53ef\u56de\u653e\u7684\u811a\u672c\uff01")
                return
            self.is_playing = True
            self.play_btn.config(text="\u505c\u6b62\u56de\u653e")
            self.record_status.set("\u6b63\u5728\u56de\u653e...")
            self.play_thread = threading.Thread(target=self.playback_loop, args=(script,), daemon=True)
            self.play_thread.start()
        else:
            self.is_playing = False
            self.play_btn.config(text="\u56de\u653e\u811a\u672c")
            self.record_status.set("\u56de\u653e\u5df2\u505c\u6b62")

    def playback_loop(self, script):
        try:
            for action in script:
                if not self.is_playing:
                    break
                if action["type"] == "SERVO":
                    num = action["target"]
                    angle = action["value"]
                    if self.serial_port:
                        self.serial_port.write(f"SERVO{num}:{angle}\n".encode('utf-8'))
                    idx = num - 1
                    self.servo_angles[idx] = angle
                    self.root.after(0, lambda i=idx, a=angle: self.slider_vars[i].set(a))
                elif action["type"] == "DELAY":
                    time.sleep(action["delay"] / self.speed_var.get() / 1000.0)
            self.is_playing = False
            self.root.after(0, lambda: self.play_btn.config(text="\u56de\u653e\u811a\u672c"))
            self.root.after(0, lambda: self.record_status.set("\u56de\u653e\u5b8c\u6210"))
            self.root.after(0, lambda: messagebox.showinfo("\u5b8c\u6210", "\u811a\u672c\u56de\u653e\u5b8c\u6210\uff01"))
        except Exception as e:
            self.is_playing = False
            self.root.after(0, lambda: messagebox.showerror("\u9519\u8bef", f"\u56de\u653e\u5931\u8d25\uff1a{str(e)}"))

    def reset_all_servos(self):
        for i in range(6):
            self.servo_angles[i] = 90
            self.slider_vars[i].set(90)
        self.control_all_servos()

    def control_all_servos(self):
        if not self.is_connected:
            return
        angles = ','.join(map(str, self.servo_angles))
        if self.serial_port:
            try:
                self.serial_port.write(f"ALL:{angles}\n".encode('utf-8'))
            except:
                pass

    def control_single_servo(self, servo_num, angle):
        if not self.is_connected:
            return
        if self.serial_port:
            try:
                self.serial_port.write(f"SERVO{servo_num}:{angle}\n".encode('utf-8'))
            except:
                pass


if __name__ == "__main__":
    root = tk.Tk()
    app = ArmController(root)
    root.mainloop()
