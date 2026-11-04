"""通过 BLE GATT 特征值发送文本、十六进制数据或文件。"""

import argparse
import asyncio
import math
from pathlib import Path
import sys

from bluetooth_discover import bluetooth_error_hint, find_device, load_bleak, positive_seconds
from xiaozhi_ble import CHAR_UUID, XiaozhiChannel, encode_text


def nonnegative_seconds(value):
    value = float(value)
    if not math.isfinite(value) or value < 0:
        raise argparse.ArgumentTypeError("时间必须是大于等于 0 的有限数字")
    return value


def chunk_size(value):
    value = int(value)
    if not 1 <= value <= 512:
        raise argparse.ArgumentTypeError("分包大小必须在 1 到 512 字节之间")
    return value


def load_payload(args):
    if args.text is not None:
        data = args.text.encode(args.encoding)
    elif args.hex is not None:
        data = bytes.fromhex(args.hex)
    else:
        data = Path(args.file).read_bytes()
    if not data:
        raise ValueError("发送内容不能为空")
    return data


def resolve_characteristic(client, specifier):
    # 同一 UUID 可能出现多次；这种情况下可使用发现程序显示的 handle。
    from bleak.uuids import normalize_uuid_str

    if specifier.lower().startswith("handle:"):
        key = int(specifier.split(":", 1)[1], 0)
    else:
        key = normalize_uuid_str(specifier)
    char = client.services.get_characteristic(key)
    if char is None:
        raise ValueError(f"找不到特征值 {specifier}，请先运行发现程序查看服务")
    return char


def choose_response(properties, mode):
    if mode == "auto":
        response = "write" in properties
    else:
        response = mode == "yes"
    required = "write" if response else "write-without-response"
    if required not in properties:
        raise ValueError(f"目标特征值不支持 {required}，请检查 --char 和 --response")
    return response


async def write_payload(client, char, data, response, size, interval, progress=None):
    limit = 512 if response else char.max_write_without_response_size
    if size > limit:
        raise ValueError(f"分包大小 {size} 超过当前写入上限 {limit}，请调小 --chunk-size")
    sent = 0
    for offset in range(0, len(data), size):
        packet = data[offset : offset + size]
        await client.write_gatt_char(char, packet, response=response)
        sent += len(packet)
        if progress is None:
            print(f"已提交：{sent}/{len(data)} 字节")
        else:
            progress(sent, len(data))
        if interval and sent < len(data):
            await asyncio.sleep(interval)


async def send(args, data):
    client_class, _ = load_bleak()
    device = await find_device(args.address, args.timeout)
    async with client_class(device, timeout=args.timeout, pair=args.pair) as client:
        print(f"已连接：{device.name or '未命名设备'}  {device.address}")
        if getattr(args, "xiaozhi_text", None) is not None:
            char = resolve_characteristic(client, CHAR_UUID)
            if not {"read", "write", "notify"}.issubset(char.properties):
                raise ValueError("小智控制特征值必须支持 Read、Write、Notify")
            channel = XiaozhiChannel(client, char, print)
            await client.start_notify(char, channel.received)
            try:
                await channel.send_text(args.xiaozhi_text)
                # Keep the connection and show the actual cloud response state.
                deadline = asyncio.get_running_loop().time() + 65
                while asyncio.get_running_loop().time() < deadline:
                    await asyncio.sleep(1)
                    status = await channel.command(b"status")
                    phase = status.get("text_status")
                    print(f"小智状态：{status.get('state')}，文字提问：{phase}")
                    if phase == "done":
                        print("小智已收到云端语音回答。")
                        return
                    if phase in ("network_error", "send_failed", "timeout", "interrupted", "state_not_ready"):
                        raise RuntimeError(f"文字提问未完成：{phase}")
                raise RuntimeError("等待语音回答超时，请检查小智联网和云端服务")
            finally:
                if client.is_connected:
                    await client.stop_notify(char)
        char = resolve_characteristic(client, args.char)
        response = choose_response(char.properties, args.response)
        print(f"写入特征值：{char.uuid}  handle={char.handle}  response={response}")
        notify_char = None
        if args.notify:
            notify_char = resolve_characteristic(client, args.notify)
            if not {"notify", "indicate"}.intersection(notify_char.properties):
                raise ValueError("--notify 指定的特征值不支持通知或指示")

            def on_receive(sender, received):
                print(f"收到通知 HEX：{received.hex(' ')}")
                print(f"收到通知文本：{received.decode(args.encoding, errors='replace')}")

            await client.start_notify(notify_char, on_receive)
        try:
            await write_payload(
                client, char, data, response, args.chunk_size, args.interval
            )
            if notify_char is not None and args.wait:
                print(f"等待设备通知 {args.wait:g} 秒……")
                await asyncio.sleep(args.wait)
        finally:
            if notify_char is not None and client.is_connected:
                await client.stop_notify(notify_char)
        print("写入操作完成。设备是否已处理数据，请以设备协议及其回复为准。")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--address", required=True, help="发现程序显示的设备地址")
    parser.add_argument("--char", help="接收数据的可写特征值 UUID，或 handle:数字；小智文字模式自动填写")
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--text", help="发送文本（不自动添加换行）")
    source.add_argument("--hex", help='发送十六进制字节，例如 "01 02 FF"')
    source.add_argument("--file", help="发送文件原始字节")
    source.add_argument("--xiaozhi-text", help="通过蓝牙向小智提问，自动分包和等待回执；需新版小智固件")
    parser.add_argument("--encoding", default="utf-8", help="文本编码，默认 utf-8")
    parser.add_argument("--response", choices=("auto", "yes", "no"), default="auto", help="写入是否要求 GATT 响应；auto 优先有响应写入")
    parser.add_argument("--chunk-size", type=chunk_size, default=20, help="每次写入字节数，默认 20")
    parser.add_argument("--interval", type=nonnegative_seconds, default=0.05, help="分包间隔秒数，默认 0.05")
    parser.add_argument("--notify", help="订阅回复特征值 UUID，或 handle:数字")
    parser.add_argument("--wait", type=nonnegative_seconds, default=2, help="发送后等待通知秒数，默认 2，仅配合 --notify")
    parser.add_argument("--timeout", type=positive_seconds, default=15, help="扫描/连接超时秒数，默认 15")
    parser.add_argument("--pair", action="store_true", help="连接时请求配对")
    args = parser.parse_args()
    try:
        # 在连接前校验输入，避免无效数据触发蓝牙连接。
        "".encode(args.encoding)
        if args.xiaozhi_text is not None:
            data = encode_text(args.xiaozhi_text)
        else:
            if not args.char:
                parser.error("普通发送模式需要 --char")
            data = load_payload(args)
        asyncio.run(send(args, data))
    except KeyboardInterrupt:
        print("\n已取消，部分数据可能已经发送。", file=sys.stderr)
        return 130
    except Exception as exc:
        hint = bluetooth_error_hint(exc)
        print(f"发送失败：{hint or f'{type(exc).__name__}: {exc}'}", file=sys.stderr)
        if hint is None:
            print("若失败发生在写入期间，之前的分包可能已发送，请核实设备状态后再重试。", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
