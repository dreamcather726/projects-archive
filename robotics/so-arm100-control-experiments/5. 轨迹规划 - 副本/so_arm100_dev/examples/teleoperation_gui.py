# -*- coding: utf-8 -*-
"""Simple Tkinter front end for SO-ARM100 teleoperation."""

import sys
import threading
from pathlib import Path
import tkinter as tk
from tkinter import messagebox, ttk


PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJECT_ROOT))

from utils.port_finder import find_available_ports


def find_acm_ports():
    """Return only /dev/ttyACM* serial devices."""
    return sorted(
        port
        for port in find_available_ports()
        if Path(port).name.startswith("ttyACM")
    )


class TeleoperationWindow:
    def __init__(self, root):
        self.root = root
        self.root.title("SO-ARM100 Teleoperation")
        self.root.geometry("620x360")
        self.root.resizable(False, False)

        self.teleop = None
        self.worker = None
        self.stop_requested = threading.Event()
        self.leader_port = tk.StringVar()
        self.follower_port = tk.StringVar()
        self.status = tk.StringVar(value="Select the leader and follower ports")

        self._build_window()
        self.refresh_ports()
        self.root.protocol("WM_DELETE_WINDOW", self.close_window)

    def _build_window(self):
        frame = ttk.Frame(self.root, padding=28)
        frame.pack(fill="both", expand=True)
        frame.columnconfigure(1, weight=1)

        ttk.Label(
            frame,
            text="SO-ARM100 Teleoperation",
            font=("Sans", 21, "bold"),
        ).grid(row=0, column=0, columnspan=3, pady=(0, 25))

        ttk.Label(frame, text="Leader port:", font=("Sans", 14)).grid(
            row=1, column=0, sticky="e", padx=(0, 12), pady=10
        )
        self.leader_box = ttk.Combobox(
            frame,
            textvariable=self.leader_port,
            state="readonly",
            font=("Sans", 13),
        )
        self.leader_box.grid(row=1, column=1, sticky="ew", pady=10)

        ttk.Label(frame, text="Follower port:", font=("Sans", 14)).grid(
            row=2, column=0, sticky="e", padx=(0, 12), pady=10
        )
        self.follower_box = ttk.Combobox(
            frame,
            textvariable=self.follower_port,
            state="readonly",
            font=("Sans", 13),
        )
        self.follower_box.grid(row=2, column=1, sticky="ew", pady=10)

        self.refresh_button = ttk.Button(
            frame,
            text="Refresh",
            command=self.refresh_ports,
        )
        self.refresh_button.grid(row=1, column=2, rowspan=2, padx=(12, 0))

        ttk.Label(frame, textvariable=self.status, font=("Sans", 12)).grid(
            row=3, column=0, columnspan=3, pady=18
        )

        button_frame = ttk.Frame(frame)
        button_frame.grid(row=4, column=0, columnspan=3)

        self.start_button = ttk.Button(
            button_frame,
            text="Start collection",
            command=self.start_teleoperation,
        )
        self.start_button.pack(side="left", padx=12, ipadx=16, ipady=8)

        self.stop_button = ttk.Button(
            button_frame,
            text="Stop collection",
            command=self.stop_teleoperation,
            state="disabled",
        )
        self.stop_button.pack(side="left", padx=12, ipadx=16, ipady=8)

    def refresh_ports(self):
        try:
            ports = find_acm_ports()
        except Exception as exc:
            messagebox.showerror("Port error", str(exc), parent=self.root)
            return

        self.leader_box["values"] = ports
        self.follower_box["values"] = ports

        if ports:
            self.leader_port.set(ports[0])
            self.follower_port.set(ports[1] if len(ports) > 1 else ports[0])
            self.status.set(f"Found {len(ports)} ttyACM port(s)")
        else:
            self.leader_port.set("")
            self.follower_port.set("")
            self.status.set("No /dev/ttyACM* ports found")

    def start_teleoperation(self):
        leader = self.leader_port.get()
        follower = self.follower_port.get()

        if not leader or not follower:
            messagebox.showwarning(
                "Select ports",
                "Select both the leader and follower ports.",
                parent=self.root,
            )
            return
        if leader == follower:
            messagebox.showwarning(
                "Invalid selection",
                "Leader and follower must use different ports.",
                parent=self.root,
            )
            return

        self.start_button.configure(state="disabled")
        self.stop_button.configure(state="normal")
        self.refresh_button.configure(state="disabled")
        self.status.set("Starting teleoperation...")
        self.stop_requested.clear()

        self.worker = threading.Thread(
            target=self._run_teleoperation,
            args=(leader, follower),
            daemon=True,
        )
        self.worker.start()

    def _run_teleoperation(self, leader, follower):
        try:
            from examples.teleoperation import TeleOperation

            self.teleop = TeleOperation(leader, follower, frequency=50.0)
            self.teleop.connect()
            if not self.stop_requested.is_set():
                self.root.after(0, self.status.set, "Teleoperation is running")
                self.teleop.run()
        except Exception as exc:
            if not self.stop_requested.is_set():
                self.root.after(0, self._show_error, str(exc))
        finally:
            if self.teleop is not None:
                try:
                    self.teleop.disconnect()
                except Exception:
                    pass
            self.teleop = None

    def stop_teleoperation(self):
        self.stop_requested.set()
        if self.teleop is not None:
            self.teleop.stop()

        self.start_button.configure(state="normal")
        self.stop_button.configure(state="disabled")
        self.refresh_button.configure(state="normal")
        self.status.set("Teleoperation stopped")
        messagebox.showinfo(
            "Collection successful",
            "Collection saved successfully.",
            parent=self.root,
        )

    def _show_error(self, message):
        self.start_button.configure(state="normal")
        self.stop_button.configure(state="disabled")
        self.refresh_button.configure(state="normal")
        self.status.set("Teleoperation failed")
        messagebox.showerror("Teleoperation error", message, parent=self.root)

    def close_window(self):
        self.stop_requested.set()
        if self.teleop is not None:
            self.teleop.stop()
        self.root.destroy()


def main():
    root = tk.Tk()
    TeleoperationWindow(root)
    root.mainloop()


if __name__ == "__main__":
    main()
