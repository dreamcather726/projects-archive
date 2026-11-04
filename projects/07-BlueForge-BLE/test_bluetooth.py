"""无需蓝牙硬件的输入、扫描与分包测试：python -m unittest -v。"""

import argparse
import io
import unittest
from types import SimpleNamespace
from unittest.mock import AsyncMock, patch

import bluetooth_discover as discovery
import bluetooth_send as sender


class InputTests(unittest.TestCase):
    def test_unrelated_error_keeps_original_details(self):
        self.assertIsNone(discovery.bluetooth_error_hint(ValueError("invalid data")))

    def test_adapter_failure_and_power_off_have_different_hints(self):
        try:
            from bleak.exc import (
                BleakBluetoothNotAvailableError,
                BleakBluetoothNotAvailableReason,
            )
        except ImportError:
            self.skipTest("可选的 Bleak 2+ 异常类型未安装")
        missing = BleakBluetoothNotAvailableError(
            "No Bluetooth adapter found", BleakBluetoothNotAvailableReason.NO_BLUETOOTH
        )
        powered_off = BleakBluetoothNotAvailableError(
            "Bluetooth is off", BleakBluetoothNotAvailableReason.POWERED_OFF
        )
        self.assertIn("未检测到蓝牙适配器", discovery.bluetooth_error_hint(missing))
        self.assertIn("已关闭", discovery.bluetooth_error_hint(powered_off))
        output = io.StringIO()
        with patch("sys.argv", ["bluetooth_discover.py"]), patch(
            "sys.stderr", output
        ), patch.object(discovery, "discover", new=AsyncMock(side_effect=missing)):
            self.assertEqual(discovery.main(), 1)
        self.assertIn("设备管理器", output.getvalue())
        self.assertNotIn("Traceback", output.getvalue())

    def test_text_uses_requested_encoding(self):
        args = SimpleNamespace(text="你好", hex=None, file=None, encoding="utf-8")
        self.assertEqual(sender.load_payload(args), "你好".encode("utf-8"))

    def test_hex_preserves_binary_bytes(self):
        args = SimpleNamespace(text=None, hex="00 01 ff", file=None)
        self.assertEqual(sender.load_payload(args), b"\x00\x01\xff")
        args.hex = "not hex"
        with self.assertRaises(ValueError):
            sender.load_payload(args)

    def test_empty_payload_is_rejected(self):
        args = SimpleNamespace(text="", encoding="utf-8")
        with self.assertRaises(ValueError):
            sender.load_payload(args)

    def test_nonfinite_and_invalid_durations(self):
        for value in ("nan", "inf", "-1", "0"):
            with self.subTest(value=value):
                with self.assertRaises(argparse.ArgumentTypeError):
                    discovery.positive_seconds(value)
        self.assertEqual(sender.nonnegative_seconds("0"), 0)

    def test_response_mode_checks_properties(self):
        self.assertTrue(sender.choose_response(["write", "write-without-response"], "auto"))
        self.assertFalse(sender.choose_response(["write-without-response"], "auto"))
        with self.assertRaises(ValueError):
            sender.choose_response(["read"], "auto")
        with self.assertRaises(ValueError):
            sender.choose_response(["write-without-response"], "yes")


class BluetoothTests(unittest.IsolatedAsyncioTestCase):
    async def test_discovery_failure_does_not_return_invalid_device(self):
        scanner = SimpleNamespace(find_device_by_address=AsyncMock(return_value=None))
        with patch.object(discovery, "load_bleak", return_value=(None, scanner)):
            with self.assertRaisesRegex(RuntimeError, "未发现设备"):
                await discovery.find_device("AA:BB:CC:DD:EE:FF", 5)

    async def test_chunks_reconstruct_exact_original_payload(self):
        client = SimpleNamespace(write_gatt_char=AsyncMock())
        char = SimpleNamespace(max_write_without_response_size=20)
        data = bytes(range(47))
        with patch("builtins.print"):
            await sender.write_payload(client, char, data, False, 20, 0)
        calls = client.write_gatt_char.await_args_list
        self.assertEqual([len(call.args[1]) for call in calls], [20, 20, 7])
        self.assertEqual(b"".join(call.args[1] for call in calls), data)
        self.assertTrue(all(call.kwargs == {"response": False} for call in calls))

    async def test_excessive_packet_size_writes_nothing(self):
        client = SimpleNamespace(write_gatt_char=AsyncMock())
        char = SimpleNamespace(max_write_without_response_size=20)
        with self.assertRaises(ValueError):
            await sender.write_payload(client, char, b"hello", False, 21, 0)
        client.write_gatt_char.assert_not_awaited()

    async def test_write_failure_stops_without_retry(self):
        client = SimpleNamespace(
            write_gatt_char=AsyncMock(side_effect=[None, OSError("disconnected")])
        )
        char = SimpleNamespace(max_write_without_response_size=20)
        with patch("builtins.print"):
            with self.assertRaises(OSError):
                await sender.write_payload(client, char, bytes(60), True, 20, 0)
        self.assertEqual(client.write_gatt_char.await_count, 2)


if __name__ == "__main__":
    unittest.main()
