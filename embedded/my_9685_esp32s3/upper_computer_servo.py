import queue
import threading
import time
import json
from pathlib import Path
import math
from datetime import datetime
import tkinter as tk
from tkinter import ttk, messagebox, filedialog

from motion_script import (
    MotionSegmenter,
    MotionScript,
    ScriptStep,
    load_script_file,
    round_0p1,
    save_script_file,
    validate_angles,
)


try:
    import serial
    from serial.tools import list_ports
except Exception:
    serial = None
    list_ports = None


SERVOS = [
    ("R_f_1", 0),
    ("R_f_2", 1),
    ("R_f_3", 2),
    ("L_f_1", 4),
    ("L_f_2", 5),
    ("L_f_3", 6),
    ("R_b_1", 8),
    ("R_b_2", 9),
    ("R_b_3", 10),
    ("L_b_1", 12),
    ("L_b_2", 13),
    ("L_b_3", 14),
]

FRONT_LEG_CHANNELS = [0, 1, 2, 4, 5, 6]
BACK_LEG_CHANNELS = [8, 9, 10, 12, 13, 14]

CALIBRATION_FILE = Path(__file__).with_name("servo_zero.json")
PI = 3.1415
L1 = 120.0
L2 = 120.0
DEFAULT_START_X = -25.0
DEFAULT_END_X = 10.0
DEFAULT_START_Y = 145.0
DEFAULT_H = 105.0
SMOOTH_TICK_S = 0.02
SMOOTH_MAX_STEP_DEG = 2.0


class SerialWorker:
    def __init__(self, on_line):
        self._ser = None
        self._thread = None
        self._stop = threading.Event()
        self._on_line = on_line

    @property
    def is_connected(self):
        return self._ser is not None

    def connect(self, port, baud):
        if serial is None:
            raise RuntimeError("缺少依赖：pyserial。请先执行：pip install pyserial")
        self._ser = serial.Serial(port=port, baudrate=baud, timeout=0.1, write_timeout=0.2)
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def disconnect(self):
        self._stop.set()
        if self._thread:
            self._thread.join(timeout=0.5)
        self._thread = None
        if self._ser:
            try:
                self._ser.close()
            except Exception:
                pass
        self._ser = None

    def send_line(self, line):
        if not self._ser:
            return
        data = (line.strip() + "\n").encode("utf-8", errors="ignore")
        self._ser.write(data)

    def _run(self):
        buf = b""
        while not self._stop.is_set():
            try:
                chunk = self._ser.read(256)
                if not chunk:
                    continue
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    try:
                        s = line.decode("utf-8", errors="ignore").strip()
                    except Exception:
                        s = ""
                    if s:
                        self._on_line(s)
            except Exception as e:
                self._on_line(f"[串口异常] {e}")
                break
        try:
            if self._ser:
                self._ser.close()
        except Exception:
            pass


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("PCA9685 舵机上位机（串口）")
        self.geometry("920x640")

        self._rx_queue = queue.Queue()
        self._worker = SerialWorker(self._rx_queue.put)
        self._send_after_id = {}
        self._suspend_send = False
        self._pc_walk_thread = None
        self._pc_walk_stop = None
        self._smooth_thread = None
        self._smooth_stop = None
        self._target_raw = {}
        self._current_raw = {}
        self._last_sent_int = {}
        self._smooth_max_step_deg = float(SMOOTH_MAX_STEP_DEG)

        self._record_thread = None
        self._record_stop = None
        self._segmenter = None
        self._record_start_ts = None
        self._record_file_path = None

        self._loaded_script = None
        self._play_thread = None
        self._play_stop = None
        self._play_pause = None
        self._play_single_step = False

        self.walk_dir_var = tk.StringVar(value="forward")

        self._channels = [ch for _name, ch in SERVOS]
        self._zero_deg = {int(ch): 90.0 for ch in self._channels}
        self._has_calibration = False
        self._mode_var = tk.StringVar(value="calibrate")
        self._load_calibration()

        self.enable_front_var = tk.BooleanVar(value=True)
        self.enable_back_var = tk.BooleanVar(value=True)

        self._build_ui()
        style = ttk.Style(self)
        style.configure("Value.TLabel")
        style.configure("Warn.TLabel", foreground="red")
        self._init_smoother_state()
        if self._mode_var.get() == "run":
            self._suspend_send = True
            try:
                for ch in self._channels:
                    self.slider_vars[ch].set(0.0)
            finally:
                self._suspend_send = False
        self._refresh_ports()
        self.after(50, self._poll_rx)

        if serial is None:
            messagebox.showwarning("提示", "未检测到 pyserial：请先执行 pip install pyserial")

        self._apply_mode_ui()
        self._ensure_smoother_running()

    def _init_smoother_state(self):
        for ch in self._channels:
            ch = int(ch)
            self._target_raw[ch] = 90.0
            self._current_raw[ch] = 90.0
            self._last_sent_int[ch] = None

    def _ensure_smoother_running(self):
        if self._smooth_thread and self._smooth_thread.is_alive():
            return
        stop = threading.Event()
        self._smooth_stop = stop

        def run():
            while not stop.is_set():
                if self._worker.is_connected and not self._suspend_send:
                    for ch in self._channels:
                        ch = int(ch)
                        tgt = float(self._target_raw.get(ch, 90.0))
                        cur = float(self._current_raw.get(ch, 90.0))
                        delta = tgt - cur
                        if abs(delta) < 1e-6:
                            continue
                        step = max(-self._smooth_max_step_deg, min(self._smooth_max_step_deg, delta))
                        new_cur = cur + step
                        if new_cur < 0.0:
                            new_cur = 0.0
                        elif new_cur > 180.0:
                            new_cur = 180.0
                        self._current_raw[ch] = new_cur

                        out_q = int(round(new_cur * 10.0))
                        if self._last_sent_int.get(ch) != out_q:
                            self._last_sent_int[ch] = out_q
                            self._worker.send_line(f"CH{ch}:{(out_q / 10.0):.1f}")
                time.sleep(SMOOTH_TICK_S)

        self._smooth_thread = threading.Thread(target=run, daemon=True)
        self._smooth_thread.start()

    def _load_calibration(self):
        try:
            if not CALIBRATION_FILE.exists():
                return
            data = json.loads(CALIBRATION_FILE.read_text(encoding="utf-8"))
            zero = data.get("zero_deg", {})
            for ch in self._channels:
                key = str(int(ch))
                if key in zero:
                    self._zero_deg[int(ch)] = float(zero[key])
            self._has_calibration = True
            self._mode_var.set("run")
        except Exception:
            self._has_calibration = False

    def _save_calibration(self):
        data = {
            "version": 1,
            "zero_deg": {str(int(ch)): float(self._zero_deg[int(ch)]) for ch in self._channels},
        }
        CALIBRATION_FILE.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
        self._has_calibration = True

    def _clear_calibration(self):
        self._zero_deg = {int(ch): 90.0 for ch in self._channels}
        self._has_calibration = False
        try:
            if CALIBRATION_FILE.exists():
                CALIBRATION_FILE.unlink()
        except Exception:
            pass

    def _to_raw_deg(self, ch, ui_value):
        if self._mode_var.get() == "calibrate":
            return float(ui_value)
        zero = float(self._zero_deg.get(int(ch), 90.0))
        delta = float(ui_value)
        return zero + delta

    def _trim_deg(self, ch):
        return float(self._zero_deg.get(int(ch), 90.0)) - 90.0

    def _safe_acos(self, x):
        if x < -1.0:
            x = -1.0
        elif x > 1.0:
            x = 1.0
        return math.acos(x)

    def _r_x_y_to_angle_1(self, x, y):
        angle_2 = self._safe_acos((x * x + y * y - L1 * L1 - L2 * L2) / (2.0 * L1 * L2))
        judge_expression = 4.0 * (x * (L1 + L2 * math.cos(angle_2))) ** 2 - 4.0 * (x * x + y * y) * (
            (x ** 2) - (L2 * math.sin(angle_2)) ** 2
        )
        if judge_expression < 0.0:
            judge_expression = 0.0
        angle_1 = self._safe_acos(((x * (L1 + L2 * math.cos(angle_2))) + 0.5 * math.sqrt(judge_expression)) / (x * x + y * y))
        return angle_1 + PI / 6.0

    def _r_x_y_to_angle_2(self, x, y):
        angle_2 = self._safe_acos((x * x + y * y - L1 * L1 - L2 * L2) / (2.0 * L1 * L2))
        return angle_2 - PI / 6.0

    def _l_x_y_to_angle_1(self, x, y):
        return PI - self._r_x_y_to_angle_1(x, y)

    def _l_x_y_to_angle_2(self, x, y):
        return PI - self._r_x_y_to_angle_2(x, y)

    def _make_path(self, step_points, start_x, end_x, start_y, h):
        px = [0.0] * step_points
        py = [0.0] * step_points
        half = step_points // 2
        j = 0
        dx = (end_x - start_x) / float(half)
        x = start_x
        for _ in range(half):
            px[j] = x
            py[j] = start_y
            j += 1
            x += dx

        j = half
        x = end_x
        for _ in range(step_points - half):
            px[j] = x
            mid = (start_x + end_x) / 2.0
            py[j] = 4.0 * (start_y - h) / ((end_x - start_x) ** 2) * ((x - mid) ** 2) + h
            j += 1
            x -= dx

        return px, py

    def _build_ui(self):
        top = ttk.Frame(self, padding=10)
        top.pack(side=tk.TOP, fill=tk.X)

        ttk.Label(top, text="串口").pack(side=tk.LEFT)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(top, textvariable=self.port_var, width=18, state="readonly")
        self.port_combo.pack(side=tk.LEFT, padx=(6, 10))
        ttk.Button(top, text="刷新", command=self._refresh_ports).pack(side=tk.LEFT)

        ttk.Label(top, text="波特率").pack(side=tk.LEFT, padx=(16, 0))
        self.baud_var = tk.IntVar(value=115200)
        self.baud_entry = ttk.Entry(top, textvariable=self.baud_var, width=10)
        self.baud_entry.pack(side=tk.LEFT, padx=(6, 10))

        self.conn_btn = ttk.Button(top, text="连接", command=self._toggle_connect)
        self.conn_btn.pack(side=tk.LEFT)

        ttk.Separator(self, orient=tk.HORIZONTAL).pack(side=tk.TOP, fill=tk.X)

        mid = ttk.Frame(self, padding=10)
        mid.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        left = ttk.Frame(mid)
        left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        right = ttk.Frame(mid)
        right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=False)

        guide = ttk.Frame(left)
        guide.pack(side=tk.TOP, fill=tk.X)
        self.guide_label = ttk.Label(guide, text="", justify="left")
        self.guide_label.pack(side=tk.LEFT, fill=tk.X, expand=True)

        actions = ttk.Frame(left)
        actions.pack(side=tk.TOP, fill=tk.X)

        self.mode_btn = ttk.Button(actions, text="进入校准", command=self._enter_calibration)
        self.mode_btn.pack(side=tk.LEFT)

        self.save_zero_btn = ttk.Button(actions, text="保存为零位", command=self._save_zero_from_current)
        self.save_zero_btn.pack(side=tk.LEFT, padx=(8, 0))

        self.home_btn = ttk.Button(actions, text="回零(校准)", command=self._go_home_calibrated)
        self.home_btn.pack(side=tk.LEFT, padx=(8, 0))

        self.reset_zero_btn = ttk.Button(actions, text="清除零位", command=self._reset_zero)
        self.reset_zero_btn.pack(side=tk.LEFT, padx=(8, 0))

        ttk.Separator(left, orient=tk.HORIZONTAL).pack(side=tk.TOP, fill=tk.X, pady=10)

        actions2 = ttk.Frame(left)
        actions2.pack(side=tk.TOP, fill=tk.X)
        ttk.Button(actions2, text="PING", command=lambda: self._send("PING")).pack(side=tk.LEFT, padx=(8, 0))

        self.auto_steps_var = tk.IntVar(value=20)
        ttk.Label(actions2, text="AUTO步数").pack(side=tk.LEFT, padx=(16, 0))
        ttk.Entry(actions2, textvariable=self.auto_steps_var, width=8).pack(side=tk.LEFT, padx=(6, 0))
        ttk.Button(actions2, text="开始AUTO", command=self._start_auto).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Button(actions2, text="PC停止", command=self._stop_pc_walk).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Label(actions2, text="前进方向").pack(side=tk.LEFT, padx=(16, 0))
        self.walk_dir_combo = ttk.Combobox(actions2, textvariable=self.walk_dir_var, width=10, state="readonly")
        self.walk_dir_combo["values"] = ["forward", "reverse"]
        self.walk_dir_combo.pack(side=tk.LEFT, padx=(6, 0))
        ttk.Checkbutton(actions2, text="前腿", variable=self.enable_front_var).pack(side=tk.LEFT, padx=(16, 0))
        ttk.Checkbutton(actions2, text="后腿", variable=self.enable_back_var).pack(side=tk.LEFT, padx=(8, 0))

        ttk.Separator(left, orient=tk.HORIZONTAL).pack(side=tk.TOP, fill=tk.X, pady=10)

        tools = ttk.Frame(left)
        tools.pack(side=tk.TOP, fill=tk.X)

        self.record_btn = ttk.Button(tools, text="开始录制", command=self._start_recording)
        self.record_btn.pack(side=tk.LEFT)
        self.record_stop_btn = ttk.Button(tools, text="停止录制", command=self._stop_recording)
        self.record_stop_btn.pack(side=tk.LEFT, padx=(8, 0))
        self.record_click_btn = ttk.Button(tools, text="插入点击", command=self._record_insert_click)
        self.record_click_btn.pack(side=tk.LEFT, padx=(8, 0))

        self.load_script_btn = ttk.Button(tools, text="加载脚本", command=self._load_script_dialog)
        self.load_script_btn.pack(side=tk.LEFT, padx=(16, 0))
        self.play_btn = ttk.Button(tools, text="回放", command=self._start_playback)
        self.play_btn.pack(side=tk.LEFT, padx=(8, 0))
        self.pause_btn = ttk.Button(tools, text="暂停", command=self._pause_playback)
        self.pause_btn.pack(side=tk.LEFT, padx=(8, 0))
        self.resume_btn = ttk.Button(tools, text="继续", command=self._resume_playback)
        self.resume_btn.pack(side=tk.LEFT, padx=(8, 0))
        self.step_btn = ttk.Button(tools, text="单步", command=self._step_playback)
        self.step_btn.pack(side=tk.LEFT, padx=(8, 0))

        ttk.Label(tools, text="速度").pack(side=tk.LEFT, padx=(16, 0))
        self.speed_var = tk.DoubleVar(value=1.0)
        ttk.Scale(tools, from_=0.25, to=2.0, orient=tk.HORIZONTAL, variable=self.speed_var).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(6, 0))

        info = ttk.Frame(left)
        info.pack(side=tk.TOP, fill=tk.X, pady=(6, 0))
        self.status_var = tk.StringVar(value="就绪")
        ttk.Label(info, textvariable=self.status_var).pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.script_var = tk.StringVar(value="未加载脚本")
        ttk.Label(info, textvariable=self.script_var, width=28, anchor="e").pack(side=tk.RIGHT)

        sliders = ttk.Frame(left)
        sliders.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        self.slider_vars = {}
        self.value_labels = {}
        self.scales = {}
        for idx, (name, ch) in enumerate(SERVOS):
            r = idx // 2
            c = (idx % 2) * 3

            ttk.Label(sliders, text=f"{name} (CH{ch})").grid(row=r, column=c, sticky="w", padx=(0, 8), pady=6)
            v = tk.DoubleVar(value=90.0)
            self.slider_vars[ch] = v

            s = ttk.Scale(sliders, from_=0, to=180, orient=tk.HORIZONTAL, variable=v)
            s.grid(row=r, column=c + 1, sticky="ew", padx=(0, 8), pady=6)
            s.bind("<ButtonRelease-1>", lambda e, ch=ch: self._send_servo_deg(ch))
            s.bind("<B1-Motion>", lambda e, ch=ch: self._schedule_send_servo_deg(ch))
            self.scales[ch] = s

            val_lbl = ttk.Label(sliders, width=14, anchor="e", style="Value.TLabel")
            val_lbl.grid(row=r, column=c + 2, sticky="e", pady=6)
            self.value_labels[ch] = val_lbl
            v.trace_add("write", lambda *_args, ch=ch: self._update_value_label(ch))
            self._update_value_label(ch)

        sliders.columnconfigure(1, weight=1)
        sliders.columnconfigure(4, weight=1)

        ttk.Label(right, text="串口日志").pack(side=tk.TOP, anchor="w")
        self.log = tk.Text(right, width=40, height=25)
        self.log.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        cmd_bar = ttk.Frame(right)
        cmd_bar.pack(side=tk.TOP, fill=tk.X, pady=(10, 0))
        self.cmd_var = tk.StringVar()
        ttk.Entry(cmd_bar, textvariable=self.cmd_var).pack(side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Button(cmd_bar, text="发送", command=self._send_custom).pack(side=tk.LEFT, padx=(8, 0))

    def _refresh_ports(self):
        ports = []
        if list_ports is not None:
            ports = [p.device for p in list_ports.comports()]
        self.port_combo["values"] = ports
        if ports and not self.port_var.get():
            self.port_var.set(ports[0])

    def _toggle_connect(self):
        if self._worker.is_connected:
            self._worker.disconnect()
            self._append_log("已断开")
            self.conn_btn.config(text="连接")
            return

        port = self.port_var.get().strip()
        baud = int(self.baud_var.get())
        if not port:
            messagebox.showerror("错误", "请选择串口")
            return
        try:
            self._worker.connect(port, baud)
            self._append_log(f"已连接 {port} @ {baud}")
            self.conn_btn.config(text="断开")
            time.sleep(0.2)
            self._send("PING")
            if self._mode_var.get() == "calibrate":
                self._send("MANUAL")
        except Exception as e:
            messagebox.showerror("连接失败", str(e))

    def _append_log(self, s):
        self.log.insert(tk.END, s + "\n")
        self.log.see(tk.END)

    def _poll_rx(self):
        while True:
            try:
                s = self._rx_queue.get_nowait()
            except queue.Empty:
                break
            self._append_log(s)
        self.after(50, self._poll_rx)

    def _send(self, line):
        if self._suspend_send:
            return
        if not self._worker.is_connected:
            self._append_log("[未连接] " + line)
            return
        self._worker.send_line(line)
        self._append_log(">> " + line)

    def _send_custom(self):
        line = self.cmd_var.get().strip()
        if not line:
            return
        self._send(line)

    def _send_servo_deg(self, ch):
        ui_value = float(self.slider_vars[ch].get())
        raw = self._to_raw_deg(ch, ui_value)
        raw = max(0.0, min(180.0, raw))
        self._set_target_raw(int(ch), raw)

    def _schedule_send_servo_deg(self, ch):
        prev = self._send_after_id.get(ch)
        if prev:
            try:
                self.after_cancel(prev)
            except Exception:
                pass
        self._send_after_id[ch] = self.after(80, lambda: self._send_servo_deg(ch))

    def _set_target_raw(self, ch, raw):
        ch = int(ch)
        if ch not in self._enabled_channels():
            return
        self._target_raw[ch] = float(raw)

    def _enabled_channels(self):
        enabled = set()
        if self.enable_front_var.get():
            enabled |= set(FRONT_LEG_CHANNELS)
        if self.enable_back_var.get():
            enabled |= set(BACK_LEG_CHANNELS)
        if not enabled:
            enabled = set(FRONT_LEG_CHANNELS) | set(BACK_LEG_CHANNELS)
        return enabled

    def _start_auto(self):
        steps = int(self.auto_steps_var.get())
        if self._mode_var.get() == "run" and self._has_calibration:
            self._start_pc_walk(steps)
        else:
            self._send(f"AUTO {steps}")

    def _start_pc_walk(self, steps):
        if not self._worker.is_connected:
            self._append_log("[未连接] 无法开始上位机前进")
            return
        if self._pc_walk_thread and self._pc_walk_thread.is_alive():
            self._append_log("[提示] 上位机前进正在运行")
            return

        stop = threading.Event()
        self._pc_walk_stop = stop

        def run():
            try:
                self._worker.send_line("MANUAL")

                hip_channels = [0, 4, 8, 12]
                for ch in hip_channels:
                    raw = 90.0 + self._trim_deg(ch)
                    raw = max(0.0, min(180.0, raw))
                    self._set_target_raw(ch, raw)

                step_points = 40
                if self.walk_dir_var.get() == "reverse":
                    start_x, end_x = DEFAULT_START_X, DEFAULT_END_X
                else:
                    start_x, end_x = DEFAULT_END_X, DEFAULT_START_X
                px, py = self._make_path(step_points, start_x, end_x, DEFAULT_START_Y, DEFAULT_H)
                half = step_points // 2

                def send_joint(ch, deg):
                    raw = float(deg) + self._trim_deg(ch)
                    raw = max(0.0, min(180.0, raw))
                    self._set_target_raw(ch, raw)

                for _ in range(max(1, int(steps))):
                    for k in range(step_points):
                        if stop.is_set():
                            raise RuntimeError("STOP")

                        idx0 = k
                        idx1 = (k + half) % step_points

                        x0, y0 = px[idx0], py[idx0]
                        x1, y1 = px[idx1], py[idx1]

                        rf_a1 = self._r_x_y_to_angle_1(x0, y0)
                        rf_a2 = self._r_x_y_to_angle_2(x0, y0) - (PI / 2.0 - rf_a1)
                        rb_a1 = self._r_x_y_to_angle_1(x1, y1)
                        rb_a2 = self._r_x_y_to_angle_2(x1, y1) - (PI / 2.0 - rb_a1)

                        lf_a1 = self._l_x_y_to_angle_1(x1, y1)
                        lf_a2 = self._l_x_y_to_angle_2(x1, y1) - (PI / 2.0 - lf_a1)
                        lb_a1 = self._l_x_y_to_angle_1(x0, y0)
                        lb_a2 = self._l_x_y_to_angle_2(x0, y0) - (PI / 2.0 - lb_a1)

                        send_joint(1, rf_a1 * 180.0 / PI)
                        send_joint(2, rf_a2 * 180.0 / PI)
                        send_joint(9, rb_a1 * 180.0 / PI)
                        send_joint(10, rb_a2 * 180.0 / PI)
                        send_joint(5, lf_a1 * 180.0 / PI)
                        send_joint(6, lf_a2 * 180.0 / PI)
                        send_joint(13, lb_a1 * 180.0 / PI)
                        send_joint(14, lb_a2 * 180.0 / PI)

                        time.sleep(0.02)

                self.after(0, lambda: self._append_log("[完成] 上位机前进结束"))
            except RuntimeError:
                self.after(0, lambda: self._append_log("[停止] 上位机前进已停止"))
            except Exception as e:
                self.after(0, lambda: self._append_log(f"[错误] 上位机前进异常: {e}"))

        self._pc_walk_thread = threading.Thread(target=run, daemon=True)
        self._pc_walk_thread.start()
        self._append_log("[开始] 上位机前进(使用零位补偿)")

    def _stop_pc_walk(self):
        if self._pc_walk_stop:
            self._pc_walk_stop.set()

    def _set_status(self, s):
        self.status_var.set(str(s))

    def _snapshot_raw_angles(self):
        out = {}
        for ch in self._channels:
            ch = int(ch)
            out[ch] = round_0p1(float(self._current_raw.get(ch, 90.0)))
        return out

    def _snapshot_raw_angles_for(self, channels):
        out = {}
        for ch in channels:
            ch = int(ch)
            out[ch] = round_0p1(float(self._current_raw.get(ch, 90.0)))
        return out

    def _clear_highlights(self):
        for _name, ch in SERVOS:
            self.value_labels[ch].configure(style="Value.TLabel")

    def _highlight_channels(self, channels):
        for ch in channels:
            for _name, c in SERVOS:
                if int(c) == int(ch):
                    self.value_labels[c].configure(style="Warn.TLabel")

    def _start_recording(self):
        if self._record_thread and self._record_thread.is_alive():
            return
        if not self._worker.is_connected:
            messagebox.showerror("错误", "请先连接串口再开始录制")
            return

        self._stop_pc_walk()
        self._send("MANUAL")
        self._clear_highlights()

        self._segmenter = MotionSegmenter(max_steps=15)
        enabled = sorted(self._enabled_channels())
        self._record_start_ts = time.perf_counter()
        stop = threading.Event()
        self._record_stop = stop

        def run():
            try:
                self.after(0, lambda: self._set_status("录制中…"))
                sample_period = 1.0 / 20.0
                next_t = time.perf_counter()
                while not stop.is_set():
                    if not self._worker.is_connected:
                        raise RuntimeError("SERIAL_LOST")
                    now = time.perf_counter()
                    if now < next_t:
                        time.sleep(max(0.0, next_t - now))
                        continue
                    next_t += sample_period
                    t_rel = now - self._record_start_ts
                    angles = self._snapshot_raw_angles_for(enabled)
                    validated, errors = validate_angles(angles, required_channels=enabled)
                    if errors:
                        raise RuntimeError("ANGLE_OOR")
                    self._segmenter.add_sample(t_rel, validated)
                final_angles = self._snapshot_raw_angles_for(enabled)
                self._segmenter.flush_final(time.perf_counter() - self._record_start_ts, final_angles)
            except RuntimeError as e:
                code = str(e)
                if code == "SERIAL_LOST":
                    self.after(0, lambda: messagebox.showerror("录制中断", "串口连接丢失，已停止录制"))
                elif code == "ANGLE_OOR":
                    self.after(0, lambda: messagebox.showerror("录制中断", "检测到角度超限，已停止录制"))
                self.after(0, lambda: self._set_status("录制已停止"))
                stop.set()
            except Exception as e:
                self.after(0, lambda: messagebox.showerror("录制异常", str(e)))
                self.after(0, lambda: self._set_status("录制已停止"))
                stop.set()

        self._record_thread = threading.Thread(target=run, daemon=True)
        self._record_thread.start()

    def _record_insert_click(self):
        if not self._segmenter:
            return
        self._segmenter.set_pending_event("click")
        self._append_log("[录制] 插入点击事件")

    def _stop_recording(self):
        if not self._record_stop:
            return
        self._record_stop.set()
        self._set_status("录制停止中…")
        time.sleep(0.05)

        if not self._segmenter:
            self._set_status("就绪")
            return

        steps = self._segmenter.steps
        if not steps:
            messagebox.showinfo("提示", "未检测到有效动作步骤")
            self._set_status("就绪")
            return

        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        path = Path(__file__).with_name(f"motion_{ts}.json")
        enabled = sorted(self._enabled_channels())
        script = MotionScript(
            version=1,
            channels=[int(c) for c in enabled],
            steps=steps,
            meta={
                "created_at": ts,
                "sample_hz": 20,
                "zero_deg": {str(int(ch)): float(self._zero_deg.get(int(ch), 90.0)) for ch in enabled},
            },
        )
        save_script_file(script, path)
        self._record_file_path = path
        self._loaded_script = script
        self.script_var.set(path.name)
        self._append_log(f"[录制] 已保存脚本: {path}")
        self._set_status("录制完成")

    def _load_script_dialog(self):
        p = filedialog.askopenfilename(
            title="选择动作脚本",
            filetypes=[("Motion Script", "*.json"), ("All Files", "*.*")],
        )
        if not p:
            return
        try:
            script = load_script_file(Path(p))
        except Exception as e:
            messagebox.showerror("加载失败", str(e))
            return
        self._loaded_script = script
        self.script_var.set(Path(p).name)
        self._append_log(f"[脚本] 已加载: {p}")

    def _pause_playback(self):
        if self._play_pause:
            self._play_pause.set()
            self._set_status("已暂停")

    def _resume_playback(self):
        if self._play_pause:
            self._play_pause.clear()
            self._set_status("回放中…")

    def _step_playback(self):
        if not self._play_pause:
            return
        self._play_single_step = True
        self._play_pause.clear()
        self._set_status("单步执行…")

    def _start_playback(self):
        if self._play_thread and self._play_thread.is_alive():
            return
        if not self._loaded_script:
            messagebox.showerror("错误", "请先加载脚本")
            return
        if not self._worker.is_connected:
            messagebox.showerror("错误", "请先连接串口再回放")
            return

        self._stop_pc_walk()
        self._send("MANUAL")
        self._clear_highlights()

        stop = threading.Event()
        pause = threading.Event()
        self._play_stop = stop
        self._play_pause = pause
        self._play_single_step = False
        enabled = sorted(self._enabled_channels())

        def trigger_click(step: ScriptStep):
            self.after(0, lambda: self._append_log(f"[事件] click @ step {step.index}"))
            try:
                ser = getattr(self._worker, "_ser", None)
                if ser:
                    ser.setDTR(False)
                    time.sleep(0.05)
                    ser.setDTR(True)
            except Exception:
                pass

        def run():
            old_smooth = float(self._smooth_max_step_deg)
            try:
                self.after(0, lambda: self._set_status("回放中…"))
                for step in self._loaded_script.steps:
                    if stop.is_set():
                        break
                    while pause.is_set() and not stop.is_set():
                        time.sleep(0.02)

                    start_angles = self._snapshot_raw_angles_for(enabled)
                    target_angles, errors = validate_angles(step.angles, required_channels=enabled)
                    if errors:
                        raise RuntimeError("E_ANGLE_OOR")
                    target_angles = {int(ch): float(v) for ch, v in target_angles.items() if int(ch) in set(enabled)}

                    duration_s = 0.5 / max(0.25, float(self.speed_var.get()))
                    ticks = max(1, int(round(duration_s / SMOOTH_TICK_S)))
                    max_delta = 0.0
                    for ch in enabled:
                        ch = int(ch)
                        max_delta = max(max_delta, abs(float(target_angles.get(ch, 90.0)) - float(start_angles.get(ch, 90.0))))
                    self._smooth_max_step_deg = max(0.1, max_delta / float(ticks))

                    for ch, v in target_angles.items():
                        self._set_target_raw(int(ch), float(v))

                    t0 = time.perf_counter()
                    while time.perf_counter() - t0 < 1.0:
                        if stop.is_set():
                            break
                        cur = self._snapshot_raw_angles_for(enabled)
                        max_err = 0.0
                        for ch in enabled:
                            ch = int(ch)
                            max_err = max(max_err, abs(float(cur.get(ch, 90.0)) - float(target_angles.get(ch, 90.0))))
                        if max_err <= 2.0:
                            break
                        time.sleep(0.02)
                    else:
                        raise RuntimeError("E_TIMEOUT")

                    cur = self._snapshot_raw_angles_for(enabled)
                    bad = []
                    for ch in enabled:
                        ch = int(ch)
                        if abs(float(cur.get(ch, 90.0)) - float(target_angles.get(ch, 90.0))) > 1.0:
                            bad.append(ch)
                    if bad:
                        self.after(0, lambda bad=bad, idx=step.index: self._append_log(f"[校验] step {idx} 偏差>±1°: {bad}"))
                        self.after(0, lambda bad=bad: self._highlight_channels(bad))

                    if step.event == "click":
                        trigger_click(step)

                    if self._play_single_step:
                        self._play_single_step = False
                        pause.set()
                        self.after(0, lambda: self._set_status("已暂停"))

                self.after(0, lambda: self._set_status("回放完成"))
            except RuntimeError as e:
                code = str(e)
                if code == "E_TIMEOUT":
                    self.after(0, lambda: messagebox.showerror("急停", "错误代码 E_TIMEOUT：1s 内未到达目标±2°"))
                    self.after(0, lambda: self._append_log("[急停] E_TIMEOUT"))
                elif code == "E_ANGLE_OOR":
                    self.after(0, lambda: messagebox.showerror("急停", "错误代码 E_ANGLE_OOR：角度超限或缺少通道"))
                    self.after(0, lambda: self._append_log("[急停] E_ANGLE_OOR"))
                else:
                    self.after(0, lambda: messagebox.showerror("急停", f"错误代码 {code}"))
                stop.set()
                self.after(0, lambda: self._set_status("回放已停止"))
            except Exception as e:
                self.after(0, lambda: messagebox.showerror("回放异常", str(e)))
                stop.set()
                self.after(0, lambda: self._set_status("回放已停止"))
            finally:
                self._smooth_max_step_deg = old_smooth

        self._play_thread = threading.Thread(target=run, daemon=True)
        self._play_thread.start()

    def _update_value_label(self, ch):
        v = float(self.slider_vars[ch].get())
        if self._mode_var.get() == "calibrate":
            self.value_labels[ch].config(text=f"{v:.0f}")
        else:
            raw = self._to_raw_deg(ch, v)
            raw = max(0.0, min(180.0, raw))
            if v >= 0:
                self.value_labels[ch].config(text=f"+{v:.0f} -> {raw:.0f}")
            else:
                self.value_labels[ch].config(text=f"{v:.0f} -> {raw:.0f}")

    def _apply_mode_ui(self):
        if self._mode_var.get() == "calibrate":
            self.guide_label.config(
                text="初始位置校准：\n"
                     "1) 先连接串口\n"
                     "2) 拖动滑条，让机械结构处于你认为的“零位/起始位置”\n"
                     "3) 点击“保存为零位”，之后“回零(校准)”会回到这个位置\n"
                     "提示：校准模式下滑条数值=下发到舵机的原始角度"
            )
            self.mode_btn.config(text="退出校准", command=self._exit_calibration)
            self.save_zero_btn.state(["!disabled"])
            self.home_btn.state(["disabled"])
            for ch in self._channels:
                self.scales[ch].configure(from_=0, to=180)
        else:
            self.guide_label.config(
                text="运行模式：\n"
                     "滑条数值是“相对零位”的增量角度（0=回到你保存的零位）。\n"
                     "例：零位=70°，滑条=+2° -> 下发 72°\n"
                     "右侧显示：增量 -> 实际下发角度\n"
                     "提示：开始AUTO会优先用上位机前进(带零位补偿)，未校准时才用下位机AUTO"
            )
            self.mode_btn.config(text="进入校准", command=self._enter_calibration)
            self.save_zero_btn.state(["disabled"])
            self.home_btn.state(["!disabled"])
            for ch in self._channels:
                self.scales[ch].configure(from_=-45, to=45)
        for _name, ch in SERVOS:
            self._update_value_label(ch)

    def _enter_calibration(self):
        self._mode_var.set("calibrate")
        self._apply_mode_ui()
        if self._has_calibration:
            self._suspend_send = True
            try:
                for ch in self._channels:
                    self.slider_vars[ch].set(float(self._zero_deg.get(int(ch), 90.0)))
            finally:
                self._suspend_send = False
        self._send("MANUAL")

    def _exit_calibration(self):
        if not self._has_calibration:
            messagebox.showinfo("提示", "尚未保存零位，请先点击“保存为零位”。")
            return
        self._mode_var.set("run")
        self._apply_mode_ui()
        self._go_home_calibrated()

    def _save_zero_from_current(self):
        if self._has_calibration:
            ok = messagebox.askyesno("确认", "已存在零位数据，是否覆盖更新？")
            if not ok:
                return
        for ch in self._channels:
            self._zero_deg[int(ch)] = float(self.slider_vars[ch].get())
        try:
            self._save_calibration()
        except Exception as e:
            messagebox.showerror("保存失败", str(e))
            return

        self._mode_var.set("run")
        self._apply_mode_ui()
        self._go_home_calibrated()
        messagebox.showinfo("完成", f"零位已保存：{CALIBRATION_FILE.name}\n后续控制将以该零位为基准。")

    def _go_home_calibrated(self):
        self._suspend_send = True
        try:
            for ch in self._channels:
                self.slider_vars[ch].set(0.0)
        finally:
            self._suspend_send = False
        for ch in self._channels:
            self._send_servo_deg(ch)

    def _reset_zero(self):
        ok = messagebox.askyesno("确认", "清除零位后将恢复默认(90°为零位)。是否继续？")
        if not ok:
            return
        self._clear_calibration()
        self._mode_var.set("calibrate")
        self._apply_mode_ui()
        self._suspend_send = True
        try:
            for ch in self._channels:
                self.slider_vars[ch].set(90.0)
        finally:
            self._suspend_send = False
        messagebox.showinfo("完成", "已清除零位数据，请重新校准并保存。")


if __name__ == "__main__":
    App().mainloop()
