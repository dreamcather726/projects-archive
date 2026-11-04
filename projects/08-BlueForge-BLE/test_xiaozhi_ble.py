import asyncio
import json
import time
import tkinter as tk
import unittest
from types import SimpleNamespace
from unittest.mock import AsyncMock, patch

import bluetooth_gui as gui
from xiaozhi_ble import CHAR_UUID, SERVICE_UUID, XiaozhiChannel, encode_text


class FakeClient:
    def __init__(self, device=None, *, disconnected_callback=lambda client: None, **kwargs):
        self.is_connected = False
        self.callback = disconnected_callback
        self.receive = None
        self.packets = []
        self.text = bytearray()
        self.expected = 0
        self.supported = True
        self.state = "idle"
        self.char = SimpleNamespace(uuid=CHAR_UUID, handle=15, properties=("read", "write", "notify"))
        self.services = [SimpleNamespace(uuid=SERVICE_UUID, characteristics=[self.char])]

    async def connect(self):
        self.is_connected = True

    async def disconnect(self):
        self.is_connected = False
        self.callback(self)

    async def start_notify(self, char, callback):
        self.receive = callback

    async def stop_notify(self, char):
        self.receive = None

    async def write_gatt_char(self, char, packet, response):
        assert response is True
        self.packets.append(bytes(packet))
        if packet == b"status":
            reply = {"ok": True, "state": self.state, "wifi": True, "text_input": self.supported}
        elif packet.startswith(b"text begin "):
            self.expected = int(packet[11:])
            self.text.clear()
            reply = {"ok": True, "received": 0}
        elif packet.startswith(b"text+"):
            self.text.extend(packet[5:])
            reply = {"ok": True, "received": len(self.text)}
        elif packet == b"text send":
            assert len(self.text) == self.expected
            self.text.decode("utf-8")
            reply = {"ok": True, "accepted": True}
        else:
            reply = {"ok": False, "error": "state_not_ready"}
        payload = (json.dumps(reply, ensure_ascii=False) + "\n").encode()
        for offset in range(0, len(payload), 7):
            self.receive(char, payload[offset:offset + 7])


class ChannelTests(unittest.IsolatedAsyncioTestCase):
    async def channel(self):
        client = FakeClient()
        channel = XiaozhiChannel(client, client.char)
        await client.start_notify(client.char, channel.received)
        return client, channel

    async def test_long_chinese_quotes_and_multiline_roundtrip(self):
        client, channel = await self.channel()
        text = '请解释一下“蓝牙”："你好"\\\n' * 12
        size = await channel.send_text(text)
        self.assertEqual(client.text, text.encode())
        self.assertEqual(size, len(text.encode()))
        self.assertTrue(all(len(packet) <= 20 for packet in client.packets))
        self.assertEqual(client.packets[-1], b"text send")

    async def test_old_firmware_and_busy_device_send_no_text(self):
        for supported, state, error in ((False, "idle", "固件"), (True, "speaking", "空闲")):
            client, channel = await self.channel()
            client.supported, client.state = supported, state
            with self.assertRaisesRegex(RuntimeError, error):
                await channel.send_text("你好")
            self.assertEqual(client.packets, [b"status"])

    async def test_fragmented_utf8_and_invalid_reply(self):
        client, channel = await self.channel()
        payload = '{"ok":true,"text":"你好"}\n'.encode()
        channel.received(None, b"not json\n")
        for byte in payload:
            channel.received(None, bytes([byte]))
        self.assertEqual((await channel.replies.get())["text"], "你好")

    async def test_invalid_text_is_rejected_before_any_write(self):
        client, channel = await self.channel()
        for text in ("", " \n", "a\0b", "你" * 342):
            with self.assertRaises(ValueError):
                await channel.send_text(text)
        self.assertEqual(client.packets, [])
        self.assertEqual(len(encode_text("a" * 1024)), 1024)


class XiaozhiGuiTests(unittest.TestCase):
    def test_real_window_connect_and_send_text_button(self):
        root = tk.Tk()
        root.withdraw()
        errors = []
        root.report_callback_exception = lambda *error: errors.append(error)
        device = SimpleNamespace(name="Xiaozhi-Control", address="B8:1F:3F:A5:EC:DE")
        with patch.object(gui, "load_bleak", return_value=(FakeClient, None)), patch.object(
            gui, "find_device", new=AsyncMock(return_value=device)
        ), patch.object(gui, "resolve_characteristic", side_effect=lambda client, spec: client.char):
            app = gui.BluetoothApp(root)

            def pump(predicate):
                deadline = time.monotonic() + 5
                while not predicate() and time.monotonic() < deadline:
                    root.update()
                    time.sleep(0.01)
                self.assertTrue(predicate())
                self.assertEqual(errors, [])

            try:
                app.address.set(device.address)
                app.connect()
                pump(lambda: not app.busy)
                self.assertTrue(app.xiaozhi_available)
                self.assertIn(CHAR_UUID, app.write_var.get())
                self.assertEqual(str(app.text_chat_button["state"]), "normal")
                text = '你好，介绍一下你自己。\n请用中文回答。'
                app.text_input.insert("1.0", text)
                app.text_chat_button.invoke()
                pump(lambda: not app.busy)
                self.assertEqual(app.worker.session.client.text, text.encode())
                self.assertIn("已受理文字", app.status.get())
                app.send_xiaozhi_command("status")
                pump(lambda: not app.busy)
                self.assertIn("text_input", app.log_box.get("1.0", "end"))
                # At the supported minimum width, all text controls remain within the window.
                root.geometry("1080x860")
                root.update_idletasks()
                self.assertLess(app.xiaozhi_buttons[-1].winfo_x() + app.xiaozhi_buttons[-1].winfo_width(), 1000)
            finally:
                app.close()
                pump(lambda: app.closed)


if __name__ == "__main__":
    unittest.main()
