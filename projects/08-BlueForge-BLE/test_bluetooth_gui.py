"""用模拟 BLE 设备验证真实 Tk 窗口、后台线程与完整操作流程。"""

import asyncio
from pathlib import Path
import tempfile
import threading
import time
import tkinter as tk
import unittest
from types import SimpleNamespace
from unittest.mock import AsyncMock, patch

import bluetooth_gui as gui


WRITE_UUID = "0000fff1-0000-1000-8000-00805f9b34fb"
NOTIFY_UUID = "0000fff2-0000-1000-8000-00805f9b34fb"


class FakeServices(list):
    def get_characteristic(self, key):
        for service in self:
            for char in service.characteristics:
                if key in (char.handle, char.uuid):
                    return char
        return None


class FakeClient:
    def __init__(self, device, *, disconnected_callback, **kwargs):
        self.callback = disconnected_callback
        self.is_connected = False
        self.packets = []
        self.receive = None
        self.services = FakeServices([SimpleNamespace(
            uuid="test-service", characteristics=[
                SimpleNamespace(uuid=WRITE_UUID, handle=1, properties=["write"], max_write_without_response_size=20),
                SimpleNamespace(uuid=NOTIFY_UUID, handle=2, properties=["notify"]),
            ])])

    async def connect(self):
        self.is_connected = True

    async def disconnect(self):
        self.is_connected = False
        self.callback(self)

    async def write_gatt_char(self, char, packet, response):
        self.packets.append(bytes(packet))
        if self.receive:
            self.receive(None, bytearray(b"OK"))

    async def start_notify(self, char, callback):
        self.receive = callback

    async def stop_notify(self, char):
        self.receive = None


class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root = tk.Tk()
        self.root.withdraw()
        self.callback_errors = []
        self.root.report_callback_exception = lambda *error: self.callback_errors.append(error)
        device = SimpleNamespace(name="Test BLE", address="AA:BB:CC:DD:EE:FF")
        self.scanner = SimpleNamespace(discover=AsyncMock(return_value={
            device.address: (device, SimpleNamespace(local_name=device.name, rssi=-42)),
        }))
        self.patches = [
            patch.object(gui, "load_bleak", return_value=(FakeClient, self.scanner)),
            patch.object(gui, "find_device", new=AsyncMock(return_value=device)),
            patch.object(gui, "resolve_characteristic", side_effect=lambda client, spec: client.services.get_characteristic(int(spec.split(":")[1]))),
            patch.object(gui.messagebox, "showerror"),
        ]
        for item in self.patches:
            item.start()
        self.app = gui.BluetoothApp(self.root)

    def pump_until(self, predicate, timeout=4):
        deadline = time.monotonic() + timeout
        while not predicate() and time.monotonic() < deadline:
            self.root.update()
            time.sleep(0.01)
        self.assertTrue(predicate(), "窗口或后台操作未在预期时间内完成")
        self.assertEqual(self.callback_errors, [])

    def tearDown(self):
        if not self.app.closed:
            self.app.close()
            self.pump_until(lambda: self.app.closed)
        for item in reversed(self.patches):
            item.stop()

    def connect_device(self):
        self.app.address.set("AA:BB:CC:DD:EE:FF")
        self.app.connect()
        self.pump_until(lambda: not self.app.busy)
        self.assertTrue(self.app.connected)

    def test_scan_connect_send_all_formats_receive_and_disconnect(self):
        app = self.app
        app.scan()
        self.pump_until(lambda: not app.busy)
        rows = app.device_tree.get_children()
        self.assertEqual(len(rows), 1)
        app.device_tree.selection_set(rows[0])
        app._select_device()
        self.assertEqual(app.address.get(), "AA:BB:CC:DD:EE:FF")
        self.connect_device()
        self.assertEqual(app.write_var.get(), "")
        self.assertEqual(len(app.write_choices), 1)
        client = app.worker.session.client
        app.write_var.set(next(iter(app.write_choices)))
        app.notify_var.set(next(iter(app.notify_choices)))
        app.subscribe()
        self.pump_until(lambda: not app.busy)
        self.assertIsNotNone(client.receive)
        app.text_input.insert("1.0", "你好，蓝牙！")
        app.size.set("3")
        app.interval.set("0")
        app.send()
        self.pump_until(lambda: not app.busy)
        self.assertEqual(b"".join(client.packets), "你好，蓝牙！".encode())
        self.assertIn("收到文本：OK", app.log_box.get("1.0", "end"))
        self.assertEqual(float(app.progress["value"]), 100)
        self.assertTrue(client.is_connected)
        client.packets.clear()
        app.source_tabs.select(1)
        app.hex_input.insert("1.0", "00 01 ff")
        app.send()
        self.pump_until(lambda: not app.busy)
        self.assertEqual(b"".join(client.packets), b"\x00\x01\xff")
        client.packets.clear()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "data.bin"
            path.write_bytes(bytes(range(47)))
            app.source_tabs.select(2)
            app.file_path.set(str(path))
            app.send()
            self.pump_until(lambda: not app.busy)
        self.assertEqual(b"".join(client.packets), bytes(range(47)))
        app.notify_var.set("不订阅")
        app.subscribe()
        self.pump_until(lambda: not app.busy)
        self.assertIsNone(client.receive)
        app.disconnect()
        self.pump_until(lambda: not app.busy)
        self.assertFalse(app.connected)
        self.assertFalse(client.is_connected)
        self.assertEqual(app.write_choices, {})

    def test_scan_failure_and_invalid_input_leave_window_usable(self):
        app = self.app
        self.scanner.discover.side_effect = RuntimeError("模拟蓝牙适配器不可用")
        app.scan()
        self.pump_until(lambda: not app.busy)
        self.assertIn("模拟蓝牙适配器不可用", app.log_box.get("1.0", "end"))
        self.assertEqual(str(app.scan_button["state"]), "normal")
        app.timeout.set("nan")
        app.scan()
        gui.messagebox.showerror.assert_called_once()
        self.assertFalse(app.busy)
        self.assertEqual(self.scanner.discover.await_count, 1)

    def test_remote_disconnect_clears_service_selection(self):
        self.connect_device()
        client = self.app.worker.session.client
        asyncio.run_coroutine_threadsafe(client.disconnect(), self.app.worker.loop).result(2)
        self.pump_until(lambda: not self.app.connected)
        self.assertEqual(self.app.write_choices, {})
        self.assertEqual(str(self.app.send_button["state"]), "disabled")

    def test_close_cancels_pending_scan_and_stops_worker(self):
        started, cancelled = threading.Event(), threading.Event()

        async def slow_scan(**kwargs):
            started.set()
            try:
                await asyncio.sleep(60)
            finally:
                cancelled.set()

        self.scanner.discover.side_effect = slow_scan
        self.app.scan()
        self.pump_until(started.is_set)
        self.app.close()
        self.pump_until(lambda: self.app.closed)
        self.assertTrue(cancelled.is_set())

    def test_close_disconnects_existing_connection(self):
        self.connect_device()
        client = self.app.worker.session.client
        self.app.close()
        self.pump_until(lambda: self.app.closed)
        self.assertFalse(client.is_connected)


if __name__ == "__main__":
    unittest.main()
