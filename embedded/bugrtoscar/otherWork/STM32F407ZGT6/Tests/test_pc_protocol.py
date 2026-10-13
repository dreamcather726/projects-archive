import pathlib
from itertools import product
import queue
import sys
import unittest
from collections import deque
from unittest import mock


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / "PC_Tools"))

import jy61p_motor_test as protocol  # noqa: E402


class ProtocolTests(unittest.TestCase):
    def test_crc_reference_vector(self) -> None:
        self.assertEqual(protocol.crc16_modbus(b"123456789"), 0x4B37)

    def test_ping_frame_matches_documented_example(self) -> None:
        frame = protocol.build_frame(
            protocol.DEST_CONTROLLER, 1, protocol.COMMAND_PING
        )
        self.assertEqual(frame.hex(" ").upper(), "5A 07 01 01 01 90 C0")

    def test_maximum_payload_and_oversize_rejection(self) -> None:
        frame = protocol.build_frame(1, 2, 3, bytes(248))
        self.assertEqual(len(frame), 255)
        with self.assertRaises(ValueError):
            protocol.build_frame(1, 2, 3, bytes(249))

    def test_fragmented_frame_is_reassembled(self) -> None:
        worker = protocol.SerialWorker()
        receive_buffer = bytearray()
        frame = protocol.build_frame(protocol.DEST_HOST, 7, 0x81, b"\x00")

        receive_buffer.extend(frame[:3])
        worker._extract_frames(receive_buffer)
        self.assertTrue(worker.events.empty())

        receive_buffer.extend(frame[3:])
        worker._extract_frames(receive_buffer)
        self.assertEqual(worker.events.get_nowait(), ("frame", frame))
        self.assertEqual(receive_buffer, bytearray())

    def test_noise_and_bad_crc_recover_to_next_frame(self) -> None:
        worker = protocol.SerialWorker()
        good = protocol.build_frame(protocol.DEST_HOST, 8, 0x81, b"\x00")
        bad = bytearray(good)
        bad[-1] ^= 0xFF
        receive_buffer = bytearray(b"\x11\x22" + bad + good)

        worker._extract_frames(receive_buffer)
        events = []
        while True:
            try:
                events.append(worker.events.get_nowait())
            except queue.Empty:
                break

        message, error_count = worker.take_protocol_errors()
        self.assertGreater(error_count, 0)
        self.assertTrue(message)
        self.assertIn(("frame", good), events)
        self.assertEqual(receive_buffer, bytearray())

    def test_imu_text_pattern(self) -> None:
        line = (
            "ACC[g]=0.001,-0.002,0.998;GYRO[dps]=0.000,0.061,0.000;"
            "YPR[deg]=1.230,-0.450,0.100;TEMP[C]=26.510;VER=123"
        )
        self.assertIsNotNone(protocol.IMU_LINE_PATTERN.fullmatch(line))


IMU_LINE = (
    "ACC[g]=0.001,-0.002,0.998;GYRO[dps]=0.000,0.061,0.000;"
    "YPR[deg]=1.230,-0.450,0.100;TEMP[C]=26.510;VER=123"
)


class FakeVariable:
    def __init__(self, value=None):
        self.value = value

    def get(self):
        return self.value

    def set(self, value):
        self.value = value


class FakeLog:
    """模拟 Text 的行索引，验证日志批量写入和淘汰旧行。"""

    def __init__(self):
        self.lines = []
        self.insert_count = 0
        self.state = protocol.tk.DISABLED

    def configure(self, state):
        self.state = state

    def insert(self, index, text):
        self.insert_count += 1
        self.lines.extend(text.splitlines())

    def index(self, index):
        return f"{len(self.lines) + 1}.0"

    def delete(self, start, end):
        if end == protocol.tk.END:
            self.lines.clear()
        else:
            del self.lines[:int(end.split(".")[0]) - 1]

    def see(self, index):
        pass


def make_application():
    app = protocol.TestApplication.__new__(protocol.TestApplication)
    app.root = mock.Mock()
    app.worker = protocol.SerialWorker()
    app._pending_log_lines = deque(maxlen=protocol.GUI_LOG_MAX_LINES)
    app.log_text = FakeLog()
    app.pending_requests = {}
    app._self_test = protocol.ConnectionSelfTest()
    app._self_test_reported = True
    app._self_test_sequences = set()
    app.device_status_vars = {key: FakeVariable() for key in protocol.DEVICE_NAMES}
    app.device_status_labels = {key: mock.Mock() for key in protocol.DEVICE_NAMES}
    app.self_test_summary_var = FakeVariable()
    app.self_test_button = mock.Mock()
    app.emergency_stop_button = mock.Mock()
    app.motor_widgets = []
    app.connection_var = FakeVariable()
    app.connect_button = mock.Mock()
    app.port_combo = mock.Mock()
    app.show_imu_log_var = FakeVariable(False)
    keys = ("ax", "ay", "az", "gx", "gy", "gz", "yaw", "pitch", "roll", "temp", "version", "count", "yaw_lock")
    app.sensor_vars = {key: FakeVariable() for key in keys}
    app.sample_count = 0
    app._motion_sequence = None
    app._motion_requests = set()
    app._lease_supported = False
    app.continuous_motion_var = FakeVariable(False)
    return app


class MotionSafetyTests(unittest.TestCase):
    def test_compact_recovery_state_matches_previous_boolean_logic(self):
        """625 event sequences compare the refactor against the previous state rules."""
        for actions in product(("estop", "self_test", "running", "passed", "failed"), repeat=4):
            worker = protocol.SerialWorker()
            worker._serial = mock.Mock(is_open=True)
            blocked = requested = running = False
            for action in actions:
                if action == "estop":
                    worker.send_frame(protocol.COMMAND_EMERGENCY_STOP)
                    blocked, requested, running = True, False, False
                elif action == "self_test":
                    worker.send_frame(protocol.COMMAND_SELF_TEST)
                    requested, running = True, False
                elif action == "running":
                    worker.observe_firmware_safety(1)
                    if requested:
                        running = True
                elif action == "passed":
                    worker.observe_firmware_safety(2)
                    if not blocked or requested and running:
                        blocked, requested = False, False
                else:
                    worker.observe_firmware_safety(3)
                self.assertEqual(worker._motion_blocked, blocked, actions)
                self.assertEqual(worker._recovery_state != 0, requested, actions)
                if requested:
                    self.assertEqual(worker._recovery_state == 2, running, actions)

    def test_writer_sends_estop_before_other_pending_requests(self):
        worker = protocol.SerialWorker()
        port = mock.Mock(is_open=True)
        worker._serial = port
        worker.send_frame(protocol.COMMAND_READ, b"\x01\x0d")
        _, stop = worker.send_frame(protocol.COMMAND_EMERGENCY_STOP)
        worker.send_frame(protocol.COMMAND_STATUS)
        def write(frame):
            worker._stop_event.set()
            return len(frame)
        port.write.side_effect = write
        worker._write_loop()
        port.write.assert_called_once_with(stop)
        port.flush.assert_not_called()

    def test_stale_renew_is_dropped_instead_of_extending_motion(self):
        worker = protocol.SerialWorker()
        worker._serial = mock.Mock(is_open=True)
        _, frame = worker.send_frame(protocol.COMMAND_RENEW, b"\x17")
        worker._renew_expiry[frame] = 0
        # Empty queue after discarding the renewal ends this deterministic worker run.
        worker._stop_event.wait = mock.Mock(side_effect=lambda _: worker._stop_event.set())
        worker._write_loop()
        worker._serial.write.assert_not_called()

    def test_absolute_zero_is_a_motion_request(self):
        data = protocol.struct.pack(">BBHBIBB", 1, 0, 100, 10, 0, 1, 0)
        self.assertTrue(protocol.TestApplication._is_motion_request(protocol.COMMAND_POSITION, data))

    def test_estop_discards_queued_motion_and_requires_recovery(self):
        worker = protocol.SerialWorker()
        worker._serial = mock.Mock(is_open=True)
        worker.send_frame(protocol.COMMAND_POSITION, bytes(11))
        worker.send_frame(protocol.COMMAND_EMERGENCY_STOP)
        self.assertTrue(worker._tx_queue.empty())
        self.assertEqual(worker._urgent_frame[4], protocol.COMMAND_EMERGENCY_STOP)
        worker.observe_firmware_safety(2)  # A stale PASSED event must not unlock.
        self.assertTrue(worker._motion_blocked)
        worker.send_frame(protocol.COMMAND_SELF_TEST)
        worker.observe_firmware_safety(2)
        self.assertTrue(worker._motion_blocked)
        worker.observe_firmware_safety(1)
        worker.observe_firmware_safety(2)
        self.assertFalse(worker._motion_blocked)

    def test_heartbeat_renews_only_explicit_authorization(self):
        app = make_application()
        app.worker = mock.Mock(connected=True)
        app._motion_sequence = 23
        app._lease_supported = True
        app._send_heartbeat()
        self.assertEqual(app.worker.send_frame.call_args_list, [mock.call(protocol.COMMAND_PING)])
        app.worker.send_frame.reset_mock()
        app.continuous_motion_var.set(True)
        app._send_heartbeat()
        self.assertEqual(app.worker.send_frame.call_args_list, [
            mock.call(protocol.COMMAND_PING), mock.call(protocol.COMMAND_RENEW, b"\x17")])

    def test_chassis_stop_clears_queue_and_authorization(self):
        for vector, speed in (((0, 0, 0), "invalid"), ((None, None, None), "20"),
                              ((100, 0, 0), "0")):
            with self.subTest(vector=vector, speed=speed):
                app = make_application()
                app.worker._serial = mock.Mock(is_open=True)
                app._motion_sequence = 23
                app._motion_requests.add(24)
                app.pending_requests[24] = (protocol.COMMAND_POSITION, "POS")
                app._lease_supported = True
                app.continuous_motion_var.set(True)
                app.chassis_forward_var = FakeVariable("0")
                app.chassis_left_var = FakeVariable("0")
                app.chassis_ccw_var = FakeVariable("0")
                app.chassis_speed_var = FakeVariable(speed)
                app.chassis_acceleration_var = FakeVariable("invalid")
                app.worker.send_frame(protocol.COMMAND_POSITION, bytes(11))
                app.worker.send_frame(protocol.COMMAND_RENEW, b"\x17")

                with mock.patch.object(protocol.messagebox, "showwarning") as warning:
                    app._drive_chassis(*vector)
                warning.assert_not_called()
                self.assertIsNone(app._motion_sequence)
                self.assertFalse(app._motion_requests)
                self.assertFalse(app.continuous_motion_var.get())
                self.assertEqual(len(app.pending_requests), 1)
                self.assertEqual(next(iter(app.pending_requests.values()))[0], protocol.COMMAND_STOP)
                frames = list(app.worker._tx_queue.queue)
                self.assertEqual(len(frames), 1)
                self.assertEqual(frames[0][4:-2], bytes((protocol.COMMAND_STOP, 0, 0)))
                self.assertFalse(app.worker._renew_expiry)

                # A late ACK for a discarded motion must not restore authorization.
                app._handle_received_frame(protocol.build_frame(
                    0, 24, protocol.COMMAND_POSITION | 0x80, b"\x00"))
                self.assertIsNone(app._motion_sequence)

                app._send_heartbeat()
                self.assertEqual([frame[4] for frame in app.worker._tx_queue.queue],
                                 [protocol.COMMAND_STOP, protocol.COMMAND_PING])

    def test_failed_or_missing_renew_ack_revokes_authorization(self):
        for payload, collision in product((b"\x07", b"\x04", b"\xff", b"", b"\x00\x00"),
                                          (False, True)):
            with self.subTest(payload=payload, collision=collision):
                app = make_application()
                app.worker = mock.Mock(connected=True)
                app._motion_sequence = 23
                app._motion_requests.add(24)
                app._lease_supported = True
                app.continuous_motion_var.set(True)
                # An automatic renewal must not consume another request with the same SEQ.
                if collision:
                    app.pending_requests[9] = (protocol.COMMAND_POSITION, "POS")
                app._handle_received_frame(protocol.build_frame(
                    0, 9, protocol.COMMAND_RENEW | 0x80, payload))
                self.assertIsNone(app._motion_sequence)
                self.assertFalse(app._motion_requests)
                self.assertFalse(app.continuous_motion_var.get())
                self.assertEqual(9 in app.pending_requests, collision)
                self.assertIn("续租失败", "\n".join(app._pending_log_lines))
                self.assertIn("查询状态", app.self_test_summary_var.get())
                app._send_heartbeat()
                app.worker.send_frame.assert_called_once_with(protocol.COMMAND_PING)

    def test_nonzero_chassis_command_keeps_encoding_and_waits_for_ack(self):
        app = make_application()
        app.worker._serial = mock.Mock(is_open=True)
        app.continuous_motion_var.set(True)
        app.chassis_forward_var = FakeVariable()
        app.chassis_left_var = FakeVariable()
        app.chassis_ccw_var = FakeVariable()
        app.chassis_speed_var = FakeVariable("20")
        app.chassis_acceleration_var = FakeVariable("10")
        app._drive_chassis(100, -200, 300)
        frame = app.worker._tx_queue.get_nowait()
        self.assertEqual(frame[4], protocol.COMMAND_CHASSIS)
        self.assertEqual(frame[5:-2], protocol.struct.pack(">hhhHB", 100, -200, 300, 20, 10))
        self.assertIsNone(app._motion_sequence)
        self.assertTrue(app.continuous_motion_var.get())
        app._handle_received_frame(protocol.build_frame(
            0, frame[3], protocol.COMMAND_CHASSIS | 0x80, b"\x00"))
        self.assertEqual(app._motion_sequence, frame[3])

    def test_successful_renew_ack_is_quiet_and_keeps_authorization(self):
        app = make_application()
        app._motion_sequence = 23
        app.continuous_motion_var.set(True)
        app._handle_received_frame(protocol.build_frame(0, 9, protocol.COMMAND_RENEW | 0x80, b"\x00"))
        self.assertEqual(app._motion_sequence, 23)
        self.assertTrue(app.continuous_motion_var.get())
        self.assertFalse(app._pending_log_lines)

    def test_renew_ack_for_other_destination_does_not_revoke_authorization(self):
        app = make_application()
        app._motion_sequence = 23
        app.continuous_motion_var.set(True)
        app._handle_received_frame(protocol.build_frame(1, 9, protocol.COMMAND_RENEW | 0x80, b"\x07"))
        self.assertEqual(app._motion_sequence, 23)
        self.assertTrue(app.continuous_motion_var.get())

    def test_extended_safety_clears_authorization_on_fault(self):
        app = make_application()
        app._motion_sequence = 23
        app._handle_firmware_safety(bytes((3, 15, 1, 4, 0, 0)) + bytes(4) + bytes((1, 0, 0, 23)))
        self.assertTrue(app._lease_supported)
        self.assertIsNone(app._motion_sequence)
        self.assertEqual(app.emergency_stop_button.configure.call_args, mock.call(state="disabled"))

    def test_extended_status_is_accepted(self):
        app = make_application()
        app._handle_status_payload(bytes(43) + bytes((2, 15, 1, 0, 0, 0)) + bytes(8))
        self.assertTrue(app._lease_supported)
        self.assertEqual(app._firmware_safety[0], 2)

    def test_ack_authorizes_position_without_resending_it(self):
        app = make_application()
        app.pending_requests[23] = (protocol.COMMAND_POSITION, "POS")
        app._motion_requests.add(23)
        app._handle_received_frame(protocol.build_frame(0, 23, protocol.COMMAND_POSITION | 0x80, b"\x00"))
        self.assertEqual(app._motion_sequence, 23)
        self.assertFalse(app._motion_requests)
        app._clear_motion_authorization()
        self.assertIsNone(app._motion_sequence)


class SelfTestTests(unittest.TestCase):
    def test_online_devices_are_checked_sequentially_with_only_read_commands(self):
        test = protocol.ConnectionSelfTest()
        test.start(0)
        self.assertEqual(test.tick(0)[:2], (protocol.COMMAND_STATUS, b""))
        test.observe_controller(1)
        test.observe_imu()
        now = 1
        for address in protocol.SELF_TEST_MOTOR_ADDRESSES:
            probe = test.tick(now)
            self.assertEqual(probe[:2], (protocol.COMMAND_READ, bytes((address, 13))))
            test.observe_motor(bytes((address, 0x3A, 1, 0x6B)), now + 0.01)
            now += 0.1
        self.assertIsNone(test.tick(now))
        self.assertFalse(test.active)
        self.assertTrue(all(test.motor_online.values()))

    def test_missing_controller_has_finite_startup_wait(self):
        test = protocol.ConnectionSelfTest()
        test.start(10)
        self.assertEqual(test.tick(10)[0], protocol.COMMAND_STATUS)
        self.assertIsNone(test.tick(10.1))
        self.assertEqual(test.tick(10.5)[0], protocol.COMMAND_STATUS)
        self.assertIsNone(test.tick(16))
        self.assertFalse(test.active)
        self.assertFalse(test.controller_online)
        self.assertTrue(all(value is None for value in test.motor_online.values()))

    def test_missing_motor_is_retried_and_does_not_block_remaining_motors(self):
        test = protocol.ConnectionSelfTest()
        test.start(0)
        test.observe_controller(0)
        test.observe_imu()
        self.assertEqual(test.tick(0)[1], b"\x01\x0d")
        self.assertEqual(test.tick(0.5)[1], b"\x01\x0d")
        self.assertEqual(test.tick(1)[1], b"\x02\x0d")
        self.assertFalse(test.motor_online[1])
        for address, now in ((2, 1), (3, 1.1), (4, 1.2)):
            if address != 2:
                self.assertEqual(test.tick(now)[1][0], address)
            test.observe_motor(bytes((address, 0x3A, 0, 0x6B)), now)
        test.tick(1.3)
        self.assertFalse(test.active)
        self.assertTrue(all(test.motor_online[address] for address in (2, 3, 4)))

    def test_ack_or_unrelated_motor_reply_cannot_mark_motor_online(self):
        app = make_application()
        app._self_test.start(0)
        app._self_test.observe_controller(0)
        app._self_test.tick(0)
        app.pending_requests[7] = (protocol.COMMAND_READ, "自检")
        ack = protocol.build_frame(0, 7, protocol.COMMAND_READ | 0x80, b"\x00")
        app._handle_received_frame(ack)
        for payload in (b"\x02\x3a\x01\x6b", b"\x01\xf3\x02\x6b",
                        b"\x01\x3a\x01", b"\x01\x3a\x01\x00"):
            app._self_test.observe_motor(payload, 0.1)
        self.assertIsNone(app._self_test.motor_online[1])

    def test_old_valid_count_does_not_prove_imu_is_still_online(self):
        app = make_application()
        app._self_test.start(0)
        payload = protocol.struct.pack(">IBIIIIIIB", 9600, 1, 1000, 0, 173, 0, 0, 0, 0)
        with mock.patch.object(protocol.time, "monotonic", return_value=0):
            app._handle_status_payload(payload)
        self.assertTrue(app._self_test.controller_online)
        self.assertFalse(app._self_test.imu_online)
        app._handle_imu_text(IMU_LINE)
        self.assertTrue(app._self_test.imu_online)

    def test_absent_imu_finishes_after_data_timeout(self):
        test = protocol.ConnectionSelfTest()
        test.start(0)
        test.observe_controller(0)
        for address in protocol.SELF_TEST_MOTOR_ADDRESSES:
            now = address * 0.1
            test.tick(now)
            test.observe_motor(bytes((address, 0x3A, 0, 0x6B)), now)
        test.tick(0.6)
        self.assertTrue(test.active)
        test.tick(protocol.SELF_TEST_IMU_TIMEOUT_SECONDS)
        self.assertFalse(test.active)
        self.assertFalse(test.imu_online)

    def test_controls_wait_for_self_test_but_emergency_stop_stays_available(self):
        app = make_application()
        app.worker = mock.Mock(connected=True)
        control = mock.Mock()
        app.motor_widgets.append(control)
        app._send_command = mock.Mock()
        app._start_self_test()
        self.assertTrue(app._self_test.active)
        control.configure.assert_called_with(state="disabled")
        app.emergency_stop_button.configure.assert_called_with(state="normal")
        self.assertEqual(app._send_command.call_args.args[0], protocol.COMMAND_STATUS)
        app._emergency_stop()
        app._send_command.assert_called_with(protocol.COMMAND_EMERGENCY_STOP, b"", "ESTOP")
        app._update_connection_state(False)
        self.assertFalse(app._self_test.active)
        self.assertEqual(app.device_status_vars["imu"].get(), "IMU：未连接")

    def test_startup_timeout_reports_failed_devices_and_allows_recheck(self):
        app = make_application()
        app.worker = mock.Mock(connected=True)
        app._send_command = mock.Mock()
        with mock.patch.object(protocol.time, "monotonic", return_value=0):
            app._start_self_test()
        with mock.patch.object(protocol.time, "monotonic", return_value=6):
            app._advance_self_test()
        self.assertTrue(app._self_test_reported)
        self.assertIn("下位机", app.self_test_summary_var.get())
        self.assertIn("4号电机", app.self_test_summary_var.get())
        app.self_test_button.configure.assert_called_with(state="normal")


class ResponsivenessTests(unittest.TestCase):
    def test_firmware_self_test_uses_status_without_competing_motor_queries(self):
        test = protocol.ConnectionSelfTest()
        test.start(0)
        test.observe_firmware(1, 1, 0, 0.1)
        self.assertEqual(test.tick(0.1)[0], protocol.COMMAND_STATUS)
        self.assertTrue(test.motor_online[2])
        test.observe_firmware(2, 15, 1, 1)
        self.assertFalse(test.active)
        self.assertTrue(all(test.motor_online.values()))

    def test_extended_status_and_runtime_fault_keep_emergency_stop_available(self):
        app = make_application()
        app.worker = mock.Mock(connected=True)
        control = mock.Mock()
        app.motor_widgets.append(control)
        app._self_test.start(0)
        base = protocol.struct.pack(">IBIIIIIIB", 9600, 0, 3, 0, 0, 0, 0, 0, 0)
        counters = protocol.struct.pack(">BIII", 0, 0, 0, 0)
        app._handle_status_payload(base + counters + protocol.struct.pack(">BBBBBBI", 2, 15, 1, 0, 0, 0, 0))
        self.assertEqual(app._firmware_safety[0], 2)
        control.configure.assert_called_with(state="normal")
        event = protocol.build_frame(0, 1, protocol.EVENT_SELF_TEST,
            protocol.struct.pack(">BBBBBBI", 3, 14, 1, 2, 2, 1, 1))
        app._handle_received_frame(event)
        control.configure.assert_called_with(state="disabled")
        app.emergency_stop_button.configure.assert_called_with(state="normal")
        self.assertIn("故障锁定", app.self_test_summary_var.get())

    def test_recheck_requests_firmware_stop_and_self_test(self):
        app = make_application()
        app.worker = mock.Mock(connected=True)
        app._firmware_safety = (3, 14, 1, 2, 2, 0, 1)
        app._send_command = mock.Mock()
        app._start_self_test()
        self.assertEqual(app._send_command.call_args_list[0].args[0], protocol.COMMAND_SELF_TEST)
        self.assertEqual(app._send_command.call_args_list[1].args[0], protocol.COMMAND_STATUS)
        app._update_connection_state(False)
        self.assertIsNone(app._firmware_safety)

    def test_firmware_self_test_wait_is_bounded(self):
        test = protocol.ConnectionSelfTest()
        test.start(0)
        test.observe_firmware(1, 0, 0, 1)
        self.assertIsNone(test.tick(7))
        self.assertFalse(test.active)

    def test_chassis_result_is_not_reported_as_motor_position_complete(self):
        app = make_application()
        event = protocol.build_frame(0, 1, protocol.EVENT_CHASSIS_RESULT, bytes((23, 2, 0)))
        app._handle_received_frame(event)
        self.assertTrue(any("非到位确认" in line for line in app._pending_log_lines))

    def test_imu_burst_keeps_latest_sample_and_preserves_motor_reply_order(self):
        app = make_application()
        replies = []
        latest = None
        for index in range(10000):
            latest = protocol.build_frame(0, index % 256, protocol.EVENT_IMU_TEXT, IMU_LINE.encode())
            app.worker._extract_frames(bytearray(latest))
            if index % 1000 == 0:
                reply = protocol.build_frame(0, index // 1000, protocol.EVENT_MOTOR_REPLY, b"\x01")
                app.worker._extract_frames(bytearray(reply))
                replies.append(reply)
        self.assertEqual(app.worker.events.qsize(), len(replies))
        for reply in replies:
            self.assertEqual(app.worker.events.get_nowait(), ("frame", reply))
        app._poll_serial_events()
        self.assertEqual(app.sample_count, 10000)
        self.assertEqual(app.sensor_vars["yaw"].get(), "1.230")
        self.assertEqual(app.log_text.lines, [])
        self.assertIsNone(app.worker.take_latest_imu())

    def test_protocol_error_burst_is_summarized(self):
        worker = protocol.SerialWorker()
        for _ in range(10000):
            worker._extract_frames(bytearray(b"\x11\x22"))
        self.assertTrue(worker.events.empty())
        message, count = worker.take_protocol_errors()
        self.assertEqual(count, 10000)
        self.assertIn("11 22", message)
        self.assertEqual(worker.take_protocol_errors(), ("", 0))

    def test_full_reply_queue_resumes_in_order_and_can_stop(self):
        worker = protocol.SerialWorker()
        for index in range(protocol.SERIAL_EVENT_QUEUE_SIZE):
            worker.events.put_nowait(("sent", index))

        finished = protocol.threading.Event()

        def post_reply():
            worker._post_event("frame", protocol.build_frame(0, 1, protocol.EVENT_READY))
            finished.set()

        producer = protocol.threading.Thread(target=post_reply, daemon=True)
        producer.start()
        self.assertFalse(finished.wait(0.01))
        self.assertEqual(worker.events.get_nowait(), ("sent", 0))
        self.assertTrue(finished.wait(1))
        producer.join(timeout=1)
        for index in range(1, protocol.SERIAL_EVENT_QUEUE_SIZE):
            self.assertEqual(worker.events.get_nowait(), ("sent", index))
        self.assertEqual(worker.events.get_nowait()[0], "frame")

        for index in range(protocol.SERIAL_EVENT_QUEUE_SIZE):
            worker.events.put_nowait(("sent", index))
        finished.clear()
        producer = protocol.threading.Thread(target=post_reply, daemon=True)
        producer.start()
        worker._stop_event.set()
        self.assertTrue(finished.wait(1))
        producer.join(timeout=1)
        self.assertEqual(worker.events.qsize(), protocol.SERIAL_EVENT_QUEUE_SIZE)

    def test_poll_yields_with_events_still_pending(self):
        app = make_application()
        frame = protocol.build_frame(0, 0, protocol.EVENT_READY)
        for _ in range(100):
            app.worker._post_event("frame", frame)
        app._handle_received_frame = mock.Mock()
        with mock.patch.object(protocol.time, "perf_counter", return_value=0):
            app._poll_serial_events()
        self.assertEqual(app._handle_received_frame.call_count, protocol.GUI_MAX_EVENTS_PER_TICK)
        self.assertGreater(app.worker.events.qsize(), 0)
        app.root.after.assert_called_once_with(1, app._poll_serial_events)

    def test_poll_time_budget_yields_before_count_limit(self):
        app = make_application()
        frame = protocol.build_frame(0, 0, protocol.EVENT_READY)
        for _ in range(10):
            app.worker._post_event("frame", frame)
        app._handle_received_frame = mock.Mock()
        with mock.patch.object(protocol.time, "perf_counter", side_effect=[0, 0, 1]):
            app._poll_serial_events()
        self.assertEqual(app._handle_received_frame.call_count, 1)
        self.assertEqual(app.worker.events.qsize(), 9)

    def test_handler_exception_does_not_stop_future_refreshes(self):
        app = make_application()
        frame = protocol.build_frame(0, 0, protocol.EVENT_READY)
        app.worker._post_event("frame", frame)
        app._handle_received_frame = mock.Mock(side_effect=ValueError("bad data"))
        app._poll_serial_events()
        self.assertTrue(any("ValueError" in line for line in app.log_text.lines))
        app.root.after.assert_called_once()
        app._handle_received_frame.side_effect = None
        app.worker._post_event("frame", frame)
        app._poll_serial_events()
        self.assertEqual(app._handle_received_frame.call_count, 2)

    def test_logging_batches_and_retains_only_recent_lines(self):
        app = make_application()
        for batch in range(20):
            for index in range(100):
                app._append_log(str(batch * 100 + index))
            app._flush_log()
        self.assertEqual(app.log_text.insert_count, 20)
        self.assertEqual(len(app.log_text.lines), protocol.GUI_LOG_MAX_LINES)
        self.assertEqual(app.log_text.lines[0], "1500")
        self.assertEqual(app.log_text.lines[-1], "1999")
        self.assertEqual(app.log_text.state, protocol.tk.DISABLED)
        app._append_log("pending")
        app._clear_log()
        app._flush_log()
        self.assertEqual(app.log_text.lines, [])

    def test_imu_logging_requires_checkbox_and_is_once_per_refresh(self):
        app = make_application()
        frame = protocol.build_frame(0, 0, protocol.EVENT_IMU_TEXT, IMU_LINE.encode())
        app._handle_received_frame(frame)
        self.assertEqual(list(app._pending_log_lines), [])
        app.show_imu_log_var.set(True)
        app._handle_received_frame(frame, 10)
        self.assertEqual(len(app._pending_log_lines), 1)
        self.assertTrue(app._pending_log_lines[0].startswith("IMU "))
        self.assertEqual(app.sample_count, 11)

    def test_connection_starts_threads_and_clears_previous_samples(self):
        worker = protocol.SerialWorker()
        worker._post_event("frame", protocol.build_frame(0, 0, protocol.EVENT_IMU_TEXT, IMU_LINE.encode()))
        with mock.patch.object(protocol.serial, "Serial"), \
                mock.patch.object(protocol.threading, "Thread") as thread, \
                mock.patch.object(protocol.time, "sleep"):
            worker.connect("TEST")
        self.assertFalse(worker._stop_event.is_set())
        self.assertEqual(thread.return_value.start.call_count, 2)
        self.assertIsNone(worker.take_latest_imu())


if __name__ == "__main__":
    unittest.main()
