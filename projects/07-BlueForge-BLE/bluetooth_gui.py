"""中文 BLE 调试窗口：python bluetooth_gui.py（Python 3.10+）。"""

import asyncio
from concurrent.futures import CancelledError
from datetime import datetime
import queue
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext, ttk
from types import SimpleNamespace
import json

from xiaozhi_ble import CHAR_UUID, SERVICE_UUID, XiaozhiChannel, encode_text

from bluetooth_discover import bluetooth_error_hint, find_device, load_bleak, positive_seconds
from bluetooth_send import (
    choose_response, chunk_size, load_payload, nonnegative_seconds,
    resolve_characteristic, write_payload,
)


class BluetoothSession:
    """仅在后台 asyncio 线程使用；事件通过队列交给主线程。"""

    def __init__(self, emit):
        self.emit = emit
        self.client = None
        self.notify_char = None
        self.encoding = "utf-8"
        self.xiaozhi_channel = None

    async def scan(self, timeout, name):
        _, scanner = load_bleak()
        found = await scanner.discover(timeout=timeout, return_adv=True)
        rows = []
        for device, adv in sorted(found.values(), key=lambda item: item[1].rssi, reverse=True):
            label = adv.local_name or device.name or "未命名设备"
            if not name or name.casefold() in label.casefold():
                rows.append((label, device.address, adv.rssi))
        return rows

    async def connect(self, address, timeout, pair):
        client_class, _ = load_bleak()
        device = await find_device(address, timeout)
        client = client_class(device, timeout=timeout, pair=pair,
                              disconnected_callback=self._disconnected)
        self.client = client
        try:
            await client.connect()
            services = []
            for service in client.services:
                for char in service.characteristics:
                    services.append((service.uuid, char.uuid, char.handle, tuple(char.properties)))
            if not client.is_connected:
                raise RuntimeError("设备在读取服务时已断开，请重新连接")
            return services
        except BaseException:
            await asyncio.wait_for(self.disconnect(), timeout=8)
            raise

    def _disconnected(self, client):
        if client is self.client:
            self.client = None
            self.notify_char = None
            self.xiaozhi_channel = None
            self.emit("disconnected", None)

    async def disconnect(self):
        client = self.client
        if client is not None:
            try:
                await client.disconnect()
            finally:
                if not client.is_connected and self.client is client:
                    self.client = None
                    self.notify_char = None
                    self.xiaozhi_channel = None

    def connected_client(self):
        if self.client is None or not self.client.is_connected:
            raise RuntimeError("设备未连接，请先连接设备")
        return self.client

    async def subscribe(self, specifier, encoding):
        client = self.connected_client()
        self.encoding = encoding
        char = resolve_characteristic(client, specifier) if specifier else None
        if char is not None and not {"notify", "indicate"}.intersection(char.properties):
            raise ValueError("所选特征值不支持通知")
        if char is self.notify_char:
            return
        if self.notify_char is not None:
            await client.stop_notify(self.notify_char)
            self.notify_char = None
        if char is not None:
            await client.start_notify(char, self._received)
            self.notify_char = char

    def _received(self, sender, data):
        if self.xiaozhi_channel is not None:
            self.xiaozhi_channel.received(sender, data)
            return
        self.emit("log", f"收到 HEX：{data.hex(' ')}\n收到文本：{data.decode(self.encoding, errors='replace')}")

    async def _xiaozhi(self):
        client = self.connected_client()
        char = resolve_characteristic(client, CHAR_UUID)
        if not {"read", "write", "notify"}.issubset(char.properties):
            raise ValueError("小智控制特征值属性不完整，请检查固件")
        if self.xiaozhi_channel is None:
            self.xiaozhi_channel = XiaozhiChannel(client, char, lambda text: self.emit("log", text))
        try:
            await self.subscribe(CHAR_UUID, "utf-8")
        except BaseException:
            self.xiaozhi_channel = None
            raise
        return self.xiaozhi_channel

    async def xiaozhi_text(self, text):
        channel = await self._xiaozhi()
        self.emit("log", f"我：{text}")
        return await channel.send_text(text)

    async def xiaozhi_command(self, text):
        channel = await self._xiaozhi()
        reply = await channel.command(text.encode("ascii"))
        self.emit("log", "小智回复：" + json.dumps(reply, ensure_ascii=False))
        return reply

    async def send(self, options):
        self.xiaozhi_channel = None
        # 文件读取也在后台执行，避免大文件阻塞界面。
        data = await asyncio.to_thread(load_payload, options)
        client = self.connected_client()
        char = resolve_characteristic(client, options.char)
        response = choose_response(char.properties, options.response)
        await self.subscribe(options.notify, options.encoding)
        self.emit("log", f"开始发送 {len(data)} 字节，handle={char.handle}，有响应写入={response}")
        await write_payload(client, char, data, response, options.chunk_size, options.interval,
                            progress=lambda sent, total: self.emit("progress", (sent, total)))
        return len(data)


class AsyncWorker:
    def __init__(self, events):
        self.events = events
        self.loop = asyncio.new_event_loop()
        self.session = BluetoothSession(lambda kind, data: events.put((kind, data)))
        self.thread = threading.Thread(target=self._run, name="BLE worker", daemon=True)
        self.thread.start()

    def _run(self):
        asyncio.set_event_loop(self.loop)
        try:
            self.loop.run_forever()
        finally:
            self.loop.run_until_complete(self.loop.shutdown_asyncgens())
            self.loop.run_until_complete(self.loop.shutdown_default_executor())
            self.loop.close()

    def submit(self, operation, *args):
        future = asyncio.run_coroutine_threadsafe(getattr(self.session, operation)(*args), self.loop)

        def completed(result):
            try:
                self.events.put(("done", (operation, result.result(), None)))
            except CancelledError:
                self.events.put(("done", (operation, None, "操作已取消，部分数据可能已发送。")))
            except Exception as exc:
                self.events.put(("done", (operation, None, bluetooth_error_hint(exc) or str(exc))))

        future.add_done_callback(completed)

    def close(self):
        async def shutdown():
            tasks = [task for task in asyncio.all_tasks() if task is not asyncio.current_task()]
            for task in tasks:
                task.cancel()
            await asyncio.gather(*tasks, return_exceptions=True)
            try:
                await asyncio.wait_for(self.session.disconnect(), timeout=8)
            except Exception as exc:
                self.events.put(("log", f"关闭连接：{exc}"))
            finally:
                self.loop.call_soon(self.loop.stop)

        asyncio.run_coroutine_threadsafe(shutdown(), self.loop)


class BluetoothApp:
    def __init__(self, root):
        self.root = root
        self.events = queue.Queue()
        self.worker = AsyncWorker(self.events)
        self.busy = False
        self.connected = False
        self.xiaozhi_available = False
        self.closing = False
        self.closed = False
        self.write_choices = {}
        self.notify_choices = {}
        self.controls = []
        root.title("蓝牙 BLE 调试工具")
        root.geometry("1080x860")
        root.minsize(900, 760)
        style = ttk.Style(root)
        if "vista" in style.theme_names():
            style.theme_use("vista")
        style.configure("TButton", padding=(10, 4))
        self._build()
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(80, self._poll)

    def _entry(self, parent, label, value, column, width=12):
        ttk.Label(parent, text=label).grid(row=0, column=column, padx=(8, 4), sticky="w")
        variable = tk.StringVar(value=value)
        entry = ttk.Entry(parent, textvariable=variable, width=width)
        entry.grid(row=0, column=column + 1, sticky="ew", pady=4)
        self.controls.append((entry, "normal"))
        return variable

    def _build(self):
        body = ttk.Frame(self.root, padding=12)
        body.pack(fill="both", expand=True)
        body.columnconfigure(0, weight=1)
        body.rowconfigure(1, weight=1)
        body.rowconfigure(5, weight=1)
        ttk.Label(body, text="蓝牙 BLE 调试工具", font=("Microsoft YaHei UI", 16, "bold")).grid(sticky="w")

        devices = ttk.LabelFrame(body, text="1. 扫描并连接设备", padding=8)
        devices.grid(row=1, sticky="nsew", pady=(8, 6))
        devices.columnconfigure(0, weight=1)
        devices.rowconfigure(1, weight=1)
        toolbar = ttk.Frame(devices)
        toolbar.grid(sticky="ew")
        self.name = self._entry(toolbar, "名称筛选", "", 0, 18)
        self.timeout = self._entry(toolbar, "超时 / 秒", "10", 2, 6)
        self.pair = tk.BooleanVar()
        pair_button = ttk.Checkbutton(toolbar, text="连接时配对", variable=self.pair)
        pair_button.grid(row=0, column=4, padx=8)
        self.controls.append((pair_button, "normal"))
        self.scan_button = ttk.Button(toolbar, text="扫描设备", command=self.scan)
        self.scan_button.grid(row=0, column=5)
        self.device_tree = ttk.Treeview(devices, columns=("name", "address", "rssi"), show="headings", height=4)
        for key, title, width in (("name", "设备名称", 260), ("address", "设备地址", 260), ("rssi", "信号 / dBm", 110)):
            self.device_tree.heading(key, text=title)
            self.device_tree.column(key, width=width)
        self.device_tree.grid(row=1, sticky="nsew", pady=6)
        scroll = ttk.Scrollbar(devices, orient="vertical", command=self.device_tree.yview)
        scroll.grid(row=1, column=1, sticky="ns")
        self.device_tree.configure(yscrollcommand=scroll.set)
        self.device_tree.bind("<<TreeviewSelect>>", self._select_device)
        self.device_tree.bind("<Double-1>", lambda event: self.connect() if not self.busy and not self.connected else None)
        connection = ttk.Frame(devices)
        connection.grid(row=2, sticky="ew")
        connection.columnconfigure(1, weight=1)
        self.address = self._entry(connection, "设备地址", "", 0, 28)
        self.connect_button = ttk.Button(connection, text="连接 / 读取服务", command=self.connect)
        self.connect_button.grid(row=0, column=2, padx=8)
        self.disconnect_button = ttk.Button(connection, text="断开", command=self.disconnect)
        self.disconnect_button.grid(row=0, column=3)

        services = ttk.LabelFrame(body, text="2. 选择 GATT 特征值（请根据设备协议选择用途）", padding=8)
        services.grid(row=2, sticky="ew", pady=6)
        services.columnconfigure(1, weight=1)
        ttk.Label(services, text="写入特征值").grid(row=0, column=0, padx=(0, 8))
        self.write_var = tk.StringVar()
        self.write_combo = ttk.Combobox(services, textvariable=self.write_var, state="readonly")
        self.write_combo.grid(row=0, column=1, columnspan=2, sticky="ew", pady=3)
        ttk.Label(services, text="回复特征值").grid(row=1, column=0, padx=(0, 8))
        self.notify_var = tk.StringVar(value="不订阅")
        self.notify_combo = ttk.Combobox(services, textvariable=self.notify_var, values=("不订阅",), state="readonly")
        self.notify_combo.grid(row=1, column=1, sticky="ew", pady=3)
        self.subscribe_button = ttk.Button(services, text="应用订阅", command=self.subscribe)
        self.subscribe_button.grid(row=1, column=2, padx=(8, 0))

        send_frame = ttk.LabelFrame(body, text="3. 发送数据", padding=8)
        send_frame.grid(row=3, sticky="ew", pady=6)
        self.source_tabs = ttk.Notebook(send_frame)
        self.source_tabs.pack(fill="x")
        text_tab, hex_tab, file_tab = (ttk.Frame(self.source_tabs, padding=5) for _ in range(3))
        for frame, label in ((text_tab, "文本"), (hex_tab, "HEX"), (file_tab, "文件")):
            self.source_tabs.add(frame, text=label)
        self.text_input = scrolledtext.ScrolledText(text_tab, height=3, wrap="word", font=("Microsoft YaHei UI", 10))
        self.text_input.pack(fill="x")
        self.hex_input = scrolledtext.ScrolledText(hex_tab, height=3, wrap="word", font=("Consolas", 11))
        self.hex_input.pack(fill="x")
        ttk.Label(file_tab, text="发送文件原始字节；接收端需要支持对应协议。").pack(anchor="w", pady=4)
        file_row = ttk.Frame(file_tab)
        file_row.pack(fill="x", pady=6)
        self.file_path = tk.StringVar()
        ttk.Entry(file_row, textvariable=self.file_path).pack(side="left", fill="x", expand=True)
        ttk.Button(file_row, text="选择文件…", command=self._browse).pack(side="left", padx=(8, 0))
        settings = ttk.Frame(send_frame)
        settings.pack(fill="x", pady=(6, 0))
        self.encoding = self._entry(settings, "编码", "utf-8", 0, 10)
        self.size = self._entry(settings, "分包 / 字节", "20", 2, 5)
        self.interval = self._entry(settings, "间隔 / 秒", "0.05", 4, 6)
        ttk.Label(settings, text="写入响应").grid(row=0, column=6, padx=(8, 4))
        self.response = tk.StringVar(value="auto")
        response_combo = ttk.Combobox(settings, textvariable=self.response, values=("auto", "yes", "no"), width=6, state="readonly")
        response_combo.grid(row=0, column=7)
        self.controls.append((response_combo, "readonly"))
        action = ttk.Frame(send_frame)
        action.pack(fill="x", pady=(8, 0))
        self.send_button = ttk.Button(action, text="发送数据", command=self.send)
        self.send_button.pack(side="left")
        chat_action = ttk.Frame(send_frame)
        chat_action.pack(fill="x", pady=(8, 0))
        self.text_chat_button = ttk.Button(chat_action, text="发送文字，让小智回答", command=self.send_xiaozhi_text)
        self.text_chat_button.pack(side="left")
        self.xiaozhi_buttons = []
        for title, command in (("查询小智状态", "status"), ("唤醒 / 切换", "chat toggle"), ("打断回答", "chat abort")):
            button = ttk.Button(chat_action, text=title, command=lambda cmd=command: self.send_xiaozhi_command(cmd))
            button.pack(side="left", padx=(8, 0))
            self.xiaozhi_buttons.append(button)
        self.progress = ttk.Progressbar(action, maximum=100)
        self.progress.pack(side="left", fill="x", expand=True, padx=10)
        self.progress_text = tk.StringVar(value="0 字节")
        ttk.Label(action, textvariable=self.progress_text, width=24).pack(side="left")
        ttk.Label(body, text="打字聊天：在文本框输入问题，点击“发送文字，让小智回答”；小智需联网并烧录支持文字提问的固件。").grid(row=4, sticky="w", pady=3)

        logs = ttk.LabelFrame(body, text="运行记录 / 接收数据", padding=8)
        logs.grid(row=5, sticky="nsew", pady=6)
        self.log_box = scrolledtext.ScrolledText(logs, height=7, state="disabled", wrap="word", font=("Microsoft YaHei UI", 9))
        self.log_box.pack(fill="both", expand=True)
        footer = ttk.Frame(body)
        footer.grid(row=6, sticky="ew")
        self.status = tk.StringVar(value="未连接 · 请先扫描设备，或输入地址连接")
        ttk.Label(footer, textvariable=self.status).pack(side="left")
        ttk.Button(footer, text="清空记录", command=self._clear_log).pack(side="right")
        self._update_controls()

    def _browse(self):
        path = filedialog.askopenfilename(parent=self.root, title="选择要发送的文件")
        if path:
            self.file_path.set(path)

    def _select_device(self, event=None):
        selected = self.device_tree.selection()
        if selected and not self.busy and not self.connected:
            self.address.set(self.device_tree.item(selected[0], "values")[1])

    def _log(self, text):
        self.log_box.configure(state="normal")
        self.log_box.insert("end", f"[{datetime.now():%H:%M:%S}] {text}\n")
        # 限制长期订阅时的日志内存占用。
        if int(self.log_box.index("end-1c").split(".")[0]) > 2500:
            self.log_box.delete("1.0", "501.0")
        self.log_box.see("end")
        self.log_box.configure(state="disabled")

    def _clear_log(self):
        self.log_box.configure(state="normal")
        self.log_box.delete("1.0", "end")
        self.log_box.configure(state="disabled")

    def _update_controls(self):
        idle = not self.busy and not self.closing
        for button in (self.text_chat_button, *self.xiaozhi_buttons):
            button.configure(state="normal" if idle and self.connected and self.xiaozhi_available else "disabled")
        for widget, state in self.controls:
            widget.configure(state=state if idle else "disabled")
        for widget, enabled in ((self.scan_button, idle and not self.connected),
                                (self.connect_button, idle and not self.connected),
                                (self.disconnect_button, idle and self.connected),
                                (self.send_button, idle and self.connected and bool(self.write_choices)),
                                (self.subscribe_button, idle and self.connected)):
            widget.configure(state="normal" if enabled else "disabled")
        self.write_combo.configure(state="readonly" if idle and self.connected else "disabled")
        self.notify_combo.configure(state="readonly" if idle and self.connected else "disabled")

    def _start(self, operation, label, *args):
        if self.busy or self.closing:
            return
        self.busy = True
        self.status.set(label)
        self._log(label)
        self._update_controls()
        self.worker.submit(operation, *args)

    def _validated(self, callback):
        try:
            callback()
        except Exception as exc:
            # argparse 的参数校验异常也统一显示在窗口中。
            messagebox.showerror("请检查输入", str(exc), parent=self.root)

    def scan(self):
        self._validated(lambda: self._start("scan", "正在扫描设备…", positive_seconds(self.timeout.get()), self.name.get().strip()))

    def connect(self):
        def start():
            if not self.address.get().strip():
                raise ValueError("请先选择设备或输入设备地址")
            self._start("connect", "正在连接并读取服务…", self.address.get().strip(), positive_seconds(self.timeout.get()), self.pair.get())
        self._validated(start)

    def disconnect(self):
        self._start("disconnect", "正在断开连接…")

    def subscribe(self):
        def start():
            "".encode(self.encoding.get())
            self._start("subscribe", "正在更新回复订阅…", self.notify_choices.get(self.notify_var.get()), self.encoding.get())
        self._validated(start)

    def send(self):
        def start():
            mode = self.source_tabs.index("current")
            encoding = self.encoding.get().strip()
            "".encode(encoding)
            options = SimpleNamespace(
                text=self.text_input.get("1.0", "end-1c") if mode == 0 else None,
                hex=self.hex_input.get("1.0", "end-1c") if mode == 1 else None,
                file=self.file_path.get() if mode == 2 else None,
                encoding=encoding, char=self.write_choices.get(self.write_var.get()),
                notify=self.notify_choices.get(self.notify_var.get()), response=self.response.get(),
                chunk_size=chunk_size(self.size.get()), interval=nonnegative_seconds(self.interval.get()),
            )
            if not options.char:
                raise ValueError("请选择用于接收数据的可写特征值")
            if mode != 2:
                load_payload(options)
            elif not options.file:
                raise ValueError("请先选择文件")
            self.progress["value"] = 0
            self.progress_text.set("0 字节")
            self._start("send", "正在发送数据…", options)
        self._validated(start)

    def send_xiaozhi_text(self):
        def start():
            text = self.text_input.get("1.0", "end-1c")
            encode_text(text)
            self._start("xiaozhi_text", "正在发送文字给小智…", text)
        self._validated(start)

    def send_xiaozhi_command(self, command):
        self._start("xiaozhi_command", "正在控制小智…", command)

    def _set_services(self, services):
        self.write_choices.clear()
        self.notify_choices.clear()
        self.xiaozhi_available = False
        xiaozhi_label = None
        for service, uuid, handle, properties in services:
            label = f"handle:{handle} | {uuid} | {', '.join(properties)}"
            self._log(f"服务 {service}\n  {label}")
            if {"write", "write-without-response"}.intersection(properties):
                self.write_choices[label] = f"handle:{handle}"
            if {"notify", "indicate"}.intersection(properties):
                self.notify_choices[label] = f"handle:{handle}"
            if service.lower() == SERVICE_UUID and uuid.lower() == CHAR_UUID and {"read", "write", "notify"}.issubset(properties):
                self.xiaozhi_available = True
                xiaozhi_label = label
        self.write_combo["values"] = list(self.write_choices)
        self.notify_combo["values"] = ["不订阅", *self.notify_choices]
        # 不能仅凭可写属性猜测业务用途，由用户明确选择。
        self.write_var.set("")
        self.notify_var.set("不订阅")
        if xiaozhi_label is not None:
            self.write_var.set(xiaozhi_label)
            self.notify_var.set(xiaozhi_label)
            self._log("已识别小智控制服务。输入问题后点击“发送文字，让小智回答”；发送时自动订阅回执。")
        if not self.write_choices:
            self._log("此设备没有可写特征值，无法发送数据。")

    def _reset_connection(self):
        self.connected = False
        self.xiaozhi_available = False
        self.write_choices.clear()
        self.notify_choices.clear()
        self.write_combo["values"] = ()
        self.notify_combo["values"] = ("不订阅",)
        self.write_var.set("")
        self.notify_var.set("不订阅")

    def _poll(self):
        # 每次处理有限数量，持续通知或密集进度事件不会阻塞 Tk 消息循环。
        for _ in range(200):
            try:
                kind, data = self.events.get_nowait()
            except queue.Empty:
                break
            if kind == "log":
                self._log(data)
            elif kind == "progress":
                sent, total = data
                self.progress["value"] = sent * 100 / total
                self.progress_text.set(f"{sent} / {total} 字节")
            elif kind == "disconnected":
                self._reset_connection()
                self.status.set("设备已断开")
                self._log("设备已断开连接。")
            elif kind == "done":
                operation, result, error = data
                self.busy = False
                if error:
                    self.status.set("操作失败 · 请查看运行记录")
                    self._log(f"操作失败：{error}")
                    if operation == "send":
                        self._log("之前的分包可能已经发出；请核实设备状态后再重试。")
                elif operation == "scan":
                    self.device_tree.delete(*self.device_tree.get_children())
                    for row in result:
                        self.device_tree.insert("", "end", values=row)
                    self.status.set(f"扫描完成 · 发现 {len(result)} 个设备")
                    self._log(self.status.get() if result else "未发现符合条件的设备，请检查设备是否正在 BLE 广播。")
                elif operation == "connect":
                    self.connected = True
                    self._set_services(result)
                    self.status.set(f"已连接 {self.address.get()} · 请选择特征值")
                    self._log(self.status.get())
                elif operation == "disconnect":
                    self._reset_connection()
                    self.status.set("已断开连接")
                    self._log(self.status.get())
                elif operation == "subscribe":
                    self.status.set("回复订阅已更新")
                    self._log(self.status.get())
                elif operation == "send":
                    self.status.set(f"写入完成 · {result} 字节")
                    self._log(f"已提交 {result} 字节。设备是否处理成功，请以设备回复为准。")
                elif operation == "xiaozhi_text":
                    self.status.set("小智已受理文字 · 等待语音回答，可查询小智状态")
                    self._log("文字已提交。请听小智的喇叭；云端连接失败或超时可点击“查询小智状态”。")
                elif operation == "xiaozhi_command":
                    self.status.set("小智控制完成 · 请查看设备回复")
        self._update_controls()
        if self.closing and not self.worker.thread.is_alive():
            self.closed = True
            self.root.destroy()
            return
        self.root.after(80, self._poll)

    def close(self):
        if not self.closing:
            self.closing = True
            self.status.set("正在停止操作并断开连接…")
            self._update_controls()
            self.worker.close()


def main():
    root = tk.Tk()
    BluetoothApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
