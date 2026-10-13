import json
import socket
import sys
import time
import tkinter as tk
from urllib.parse import urlparse


DEFAULT_IP = "192.168.0.187"
UDP_PORT = 8888
SERVO_COUNT = 5
HEARTBEAT_MS = 500
UDP_TIMEOUT_SEC = 0.25
RESET_POSE = (90, 0, 180, 130, 90)

BG = "#0b1018"
PANEL = "#111a27"
PANEL_2 = "#162231"
LINE = "#25384d"
TEXT = "#d7e7f7"
MUTED = "#7f95aa"
CYAN = "#21d4fd"
GREEN = "#31f6a8"
YELLOW = "#ffd166"
RED = "#ff4d6d"
PACK_KEYS = {"side", "fill", "expand", "padx", "pady", "anchor", "ipadx", "ipady"}


def normalize_host(value):
    if "://" not in value:
        return value
    return urlparse(value).hostname or value


class RobotPanel(tk.Tk):
    def __init__(self, esp32_ip):
        super().__init__()
        self.esp32_ip = esp32_ip
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(UDP_TIMEOUT_SEC)
        self.last_ok_at = 0.0
        self.online = False

        self.joint_vars = [tk.IntVar(value=value) for value in RESET_POSE]
        self.current_vars = [tk.StringVar(value="---") for _ in range(SERVO_COUNT)]
        self.target_vars = [tk.StringVar(value=f"{value:03d}") for value in RESET_POSE]
        self.status_var = tk.StringVar(value="LINK INIT")
        self.mode_var = tk.StringVar(value="WAIT")
        self.stop_var = tk.StringVar(value="--")
        self.heartbeat_var = tk.StringVar(value="-- ms")

        self.title("JXB Robot Arm Control")
        self.geometry("1100x680")
        self.minsize(980, 600)
        self.configure(bg=BG)

        self._build_ui()
        self.protocol("WM_DELETE_WINDOW", self._close)
        self.after(100, self._heartbeat)

    def _label(self, parent, text=None, var=None, bg=PANEL, fg=TEXT, font=("Consolas", 10), **options):
        pack_options = {key: options.pop(key) for key in list(options) if key in PACK_KEYS}
        label = tk.Label(parent, text=text, textvariable=var, bg=bg, fg=fg, font=font, **options)
        label.pack(**pack_options)
        return label

    def _panel(self, parent, bg=PANEL, frame_padx=16, frame_pady=12, **pack_options):
        if "pack_padx" in pack_options:
            pack_options["padx"] = pack_options.pop("pack_padx")
        if "pack_pady" in pack_options:
            pack_options["pady"] = pack_options.pop("pack_pady")

        frame = tk.Frame(
            parent,
            bg=bg,
            padx=frame_padx,
            pady=frame_pady,
            highlightbackground=LINE,
            highlightthickness=1,
        )
        frame.pack(**pack_options)
        return frame

    def _build_ui(self):
        self._build_header()
        self._build_status_bar()
        self._build_joint_panel()
        self._build_actions()

    def _build_header(self):
        header = tk.Frame(self, bg=BG, padx=22, pady=18)
        header.pack(fill="x")

        left = tk.Frame(header, bg=BG)
        left.pack(side="left")
        self._label(left, "JXB ROBOT ARM", bg=BG, font=("Segoe UI", 19, "bold"), anchor="w")
        self._label(left, "WIRELESS SERVO CONTROL TERMINAL", bg=BG, fg=CYAN, anchor="w", pady=(4, 0))

        right = tk.Frame(header, bg=BG)
        right.pack(side="right")
        self._label(right, f"{self.esp32_ip}:{UDP_PORT}", bg=BG, font=("Consolas", 11), anchor="e")
        self._label(right, "UDP HEARTBEAT 500 MS", bg=BG, fg=MUTED, font=("Consolas", 9), anchor="e", pady=(4, 0))

    def _build_status_bar(self):
        bar = self._panel(self, fill="x", pack_padx=22)

        self.link_dot = tk.Canvas(bar, width=14, height=14, bg=PANEL, highlightthickness=0)
        self.link_dot.pack(side="left")
        self.link_dot_id = self.link_dot.create_oval(2, 2, 12, 12, fill=YELLOW, outline="")

        self._label(bar, var=self.status_var, font=("Consolas", 11, "bold"), side="left", padx=(10, 22))
        for name, var in (("MODE", self.mode_var), ("STOP", self.stop_var), ("HB", self.heartbeat_var)):
            self._metric(bar, name, var)

    def _metric(self, parent, name, value_var):
        box = tk.Frame(parent, bg=PANEL)
        box.pack(side="left", padx=14)
        self._label(box, name, fg=MUTED, font=("Consolas", 8), anchor="w")
        self._label(box, var=value_var, fg=GREEN, font=("Consolas", 11, "bold"), anchor="w")

    def _build_joint_panel(self):
        panel = self._panel(self, frame_padx=18, frame_pady=18, fill="both", expand=True, pack_padx=22, pack_pady=18)
        self._label(panel, "JOINT TARGETS", fg=CYAN, font=("Consolas", 11, "bold"), anchor="w", pady=(0, 10))

        for idx, var in enumerate(self.joint_vars):
            self._joint_row(panel, idx, var)

    def _joint_row(self, parent, idx, var):
        row = tk.Frame(parent, bg=PANEL)
        row.pack(fill="x", pady=8)

        self._label(row, f"J{idx + 1}", width=4, font=("Consolas", 12, "bold"), side="left")
        scale = tk.Scale(
            row,
            from_=0,
            to=180,
            orient="horizontal",
            variable=var,
            command=lambda value, i=idx: self._on_slider(i, value),
            showvalue=False,
            resolution=1,
            bg=PANEL,
            fg=TEXT,
            troughcolor=PANEL_2,
            activebackground=CYAN,
            highlightthickness=0,
            bd=0,
            length=620,
        )
        scale.pack(side="left", fill="x", expand=True, padx=(10, 18))
        scale.bind("<ButtonRelease-1>", lambda _event: self.send_movej())

        self._readout(row, "TARGET", self.target_vars[idx], CYAN)
        self._readout(row, "CURRENT", self.current_vars[idx], GREEN)

    def _readout(self, parent, title, value_var, color):
        box = self._panel(parent, bg=PANEL_2, frame_padx=10, frame_pady=5, side="left", pack_padx=5)
        self._label(box, title, bg=PANEL_2, fg=MUTED, font=("Consolas", 7), anchor="w")
        self._label(box, var=value_var, bg=PANEL_2, fg=color, font=("Consolas", 13, "bold"), width=4, anchor="e")

    def _build_actions(self):
        actions = tk.Frame(self, bg=BG, padx=22)
        actions.pack(fill="x", pady=(0, 18))

        buttons = [
            ("\u53d1\u9001", self.send_movej, CYAN, "left", (0, 10)),
            ("\u590d\u4f4d", self.reset_pose, GREEN, "left", 10),
            ("\u8fd0\u884c", lambda: self._send_text("run"), YELLOW, "left", 10),
            ("\u505c\u6b62", lambda: self._send_text("stop"), RED, "left", 10),
            ("PING", lambda: self._send_text("ping"), MUTED, "right", 0),
        ]
        for text, command, color, side, padx in buttons:
            self._button(actions, text, command, color).pack(side=side, padx=padx)

    def _button(self, parent, text, command, color):
        return tk.Button(
            parent,
            text=text,
            command=command,
            bg=PANEL,
            fg=color,
            activebackground=PANEL_2,
            activeforeground=color,
            bd=0,
            highlightthickness=1,
            highlightbackground=color,
            padx=24,
            pady=9,
            font=("Segoe UI", 10, "bold"),
            cursor="hand2",
        )

    def _on_slider(self, index, value):
        angle = int(round(float(value)))
        self.joint_vars[index].set(angle)
        self.target_vars[index].set(f"{angle:03d}")

    def _send_text(self, message):
        # All commands use the same simple UDP request/reply protocol.
        try:
            self.sock.sendto(message.encode("utf-8"), (self.esp32_ip, UDP_PORT))
            data, _addr = self.sock.recvfrom(1024)
            text = data.decode("utf-8", errors="replace")
            self.last_ok_at = time.time()
            self._set_online(text)
            return text
        except OSError as exc:
            self._set_offline(str(exc))
            return None

    def send_movej(self):
        values = [var.get() for var in self.joint_vars]
        return self._send_text("movej:" + ",".join(str(value) for value in values))

    def reset_pose(self):
        for idx, value in enumerate(RESET_POSE):
            self.joint_vars[idx].set(value)
            self.target_vars[idx].set(f"{value:03d}")
        return self.send_movej()

    def _heartbeat(self):
        # Heartbeat keeps the ESP32 watchdog alive and refreshes UI status.
        reply = self._send_text("heartbeat")
        if reply:
            self._apply_status(reply)
        self.after(HEARTBEAT_MS, self._heartbeat)

    def _apply_status(self, reply):
        try:
            status = json.loads(reply)
        except json.JSONDecodeError:
            return

        for idx, value in enumerate(status.get("current", [])[:SERVO_COUNT]):
            self.current_vars[idx].set(f"{int(value):03d}")
        for idx, value in enumerate(status.get("target", [])[:SERVO_COUNT]):
            self.target_vars[idx].set(f"{int(value):03d}")

        stopped = bool(status.get("stop", False))
        self.stop_var.set("TRUE" if stopped else "FALSE")
        self.mode_var.set("STOP" if stopped else "READY")
        self.heartbeat_var.set(f"{int(status.get('lastUdpMs', 0)):03d} ms")

    def _set_online(self, detail):
        self.online = True
        self.status_var.set("ONLINE")
        self.link_dot.itemconfig(self.link_dot_id, fill=GREEN)
        if detail in ("pong", "movej ok", "running", "stopped"):
            self.mode_var.set(detail.upper())

    def _set_offline(self, _detail):
        self.online = False
        age = time.time() - self.last_ok_at if self.last_ok_at else 0
        self.status_var.set(f"OFFLINE {age:.1f}s")
        self.mode_var.set("NO LINK")
        self.heartbeat_var.set("-- ms")
        self.link_dot.itemconfig(self.link_dot_id, fill=RED)

    def _close(self):
        # Stop the robot before closing the control panel.
        try:
            self._send_text("stop")
        finally:
            self.sock.close()
            self.destroy()


def main():
    esp32_ip = normalize_host(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_IP
    RobotPanel(esp32_ip).mainloop()


if __name__ == "__main__":
    main()
