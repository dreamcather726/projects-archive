"""小智 BLE 控制及文字提问；每帧等待业务回执，禁止自动重发。"""

import asyncio
import json

SERVICE_UUID = "787a0000-0020-787a-7769-626c00000001"
CHAR_UUID = "787a0000-0020-787a-7769-626c00000002"
MAX_TEXT_BYTES = 1024


def encode_text(text):
    if not isinstance(text, str) or not text.strip() or "\0" in text:
        raise ValueError("请输入非空文字，文字中不能包含 NUL 字符")
    data = text.encode("utf-8")
    if len(data) > MAX_TEXT_BYTES:
        raise ValueError("文字最多 1024 个 UTF-8 字节，约 341 个汉字")
    return data


class XiaozhiChannel:
    def __init__(self, client, characteristic, emit=lambda text: None):
        self.client = client
        self.char = characteristic
        self.emit = emit
        self.buffer = bytearray()
        self.replies = asyncio.Queue(maxsize=8)

    def received(self, sender, data):
        self.buffer.extend(data)
        if len(self.buffer) > 2048:
            self.buffer.clear()
            self.emit("设备回复过长，已丢弃")
            return
        while b"\n" in self.buffer:
            line, _, rest = self.buffer.partition(b"\n")
            self.buffer = bytearray(rest)
            try:
                reply = json.loads(line.decode("utf-8"))
                if not isinstance(reply, dict):
                    raise ValueError("回复不是 JSON 对象")
            except (ValueError, UnicodeError):
                self.emit("收到非控制协议回复，请检查烧录的是否为小智主固件")
                continue
            if "event" in reply:
                continue
            if not self.replies.full():
                self.replies.put_nowait(reply)

    async def command(self, packet):
        if not 1 <= len(packet) <= 20:
            raise ValueError("小智控制命令必须在一次写入内发送，最多 20 字节")
        self.buffer.clear()
        while not self.replies.empty():
            self.replies.get_nowait()
        await self.client.write_gatt_char(self.char, packet, response=True)
        try:
            reply = await asyncio.wait_for(self.replies.get(), timeout=12)
        except asyncio.TimeoutError as exc:
            raise RuntimeError("未收到小智回执；请检查固件和连接。为避免重复提问，没有自动重发。") from exc
        if reply.get("ok") is not True:
            error = reply.get("error", "unknown")
            hints = {
                "invalid_command": "固件不支持此命令，请先烧录支持蓝牙文字提问的新固件",
                "state_not_ready": "小智当前忙碌或尚未完成联网激活；请等待空闲后重试",
                "invalid_text": "文字不完整或编码无效",
                "invalid_transfer": "文字传输不完整，请重新发送",
            }
            raise RuntimeError(hints.get(error, f"小智返回错误：{error}"))
        return reply

    async def send_text(self, text):
        data = encode_text(text)
        status = await self.command(b"status")
        if status.get("text_input") is not True:
            raise RuntimeError("当前固件只支持控制命令，请先烧录支持蓝牙文字提问的新固件")
        if status.get("state") != "idle":
            raise RuntimeError("请先等待小智空闲，再发送文字；需要时可用打断回答按钮")
        if status.get("wifi") is not True:
            raise RuntimeError("小智尚未连接 Wi-Fi，请先完成联网")
        await self.command(f"text begin {len(data)}".encode("ascii"))
        for offset in range(0, len(data), 15):
            reply = await self.command(b"text+" + data[offset:offset + 15])
            if reply.get("received") != min(offset + 15, len(data)):
                raise RuntimeError("小智接收字节数不一致，已停止提交")
        reply = await self.command(b"text send")
        if reply.get("accepted") is not True:
            raise RuntimeError("小智没有确认接收提问")
        self.emit("小智已受理文字，正在连接云端并等待语音回答……")
        return len(data)
