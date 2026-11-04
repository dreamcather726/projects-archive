"""扫描 BLE 设备，或连接指定设备查看 GATT 服务。Python 3.10+。"""

import argparse
import asyncio
import math
import sys


def positive_seconds(value):
    value = float(value)
    if not math.isfinite(value) or value <= 0:
        raise argparse.ArgumentTypeError("时间必须是大于 0 的有限数字")
    return value


def load_bleak():
    try:
        from bleak import BleakClient, BleakScanner
    except ImportError as exc:
        raise RuntimeError(
            "缺少依赖，请先运行：python -m pip install -r requirements.txt"
        ) from exc
    return BleakClient, BleakScanner


def bluetooth_error_hint(exc):
    """根据 Bleak 2+ 的具体原因给出提示，兼容旧版或未安装依赖的环境。"""
    try:
        from bleak.exc import BleakBluetoothNotAvailableError
    except ImportError:
        return None
    if not isinstance(exc, BleakBluetoothNotAvailableError):
        return None
    reason = exc.reason.name
    hints = {
        "NO_BLUETOOTH": (
            "Windows 未检测到蓝牙适配器，扫描尚未开始。\n"
            "请按 Win+X 打开设备管理器，检查是否有蓝牙适配器及其驱动。\n"
            "如果使用 USB 蓝牙适配器，请重新插入或更换 USB 口，并安装厂商驱动；\n"
            "如果电脑没有蓝牙硬件，需要添加支持 BLE 的蓝牙适配器。\n"
            "蓝牙 5.0 设备本身不能代替电脑端的蓝牙适配器。"
        ),
        "NO_BLE_CENTRAL_ROLE": (
            "当前蓝牙适配器不支持 BLE 中心角色。请使用支持 BLE 的适配器，"
            "并检查是否安装了兼容的驱动。"
        ),
        "POWERED_OFF": (
            "蓝牙适配器已关闭。请在 Windows 设置中打开蓝牙，并检查飞行模式。"
        ),
        "DENIED_BY_USER": "蓝牙访问权限被拒绝。请检查系统中的应用蓝牙访问权限。",
        "DENIED_BY_SYSTEM": "系统策略禁止访问蓝牙。请检查系统策略或联系管理员。",
        "DENIED_BY_UNKNOWN": "蓝牙访问被拒绝。请检查系统权限和蓝牙设置。",
    }
    return hints.get(reason, "蓝牙当前不可用。请检查蓝牙硬件、驱动、开关及系统权限。")


async def find_device(address, timeout):
    _, scanner = load_bleak()
    device = await scanner.find_device_by_address(address, timeout=timeout)
    if device is None:
        raise RuntimeError(
            f"未发现设备 {address}。请确认蓝牙已开启，设备正在广播且未被其他程序占用。"
        )
    return device


def print_services(client):
    for service in client.services:
        print(f"\n服务：{service.uuid}  {service.description}")
        for char in service.characteristics:
            properties = ", ".join(char.properties)
            writable = {"write", "write-without-response"}.intersection(char.properties)
            tag = " [可写]" if writable else ""
            print(f"  特征值：{char.uuid}  handle={char.handle}{tag}")
            print(f"    属性：{properties}")


async def discover(args):
    client_class, scanner = load_bleak()
    if args.address:
        device = await find_device(args.address, args.timeout)
        async with client_class(device, timeout=args.timeout, pair=args.pair) as client:
            print(f"已连接：{device.name or '未命名设备'}  {device.address}")
            print_services(client)
        return

    print(f"正在扫描 BLE 设备（{args.timeout:g} 秒）……")
    found = await scanner.discover(timeout=args.timeout, return_adv=True)
    devices = sorted(found.values(), key=lambda item: item[1].rssi, reverse=True)
    count = 0
    for device, advertisement in devices:
        name = advertisement.local_name or device.name or "未命名设备"
        if args.name and args.name.casefold() not in name.casefold():
            continue
        count += 1
        print(f"\n{count}. 名称：{name}")
        print(f"   地址：{device.address}")
        print(f"   信号：{advertisement.rssi} dBm")
        if advertisement.service_uuids:
            print(f"   广播服务：{', '.join(advertisement.service_uuids)}")
    if not count:
        print("未发现符合条件的设备。请确认设备支持 BLE 且正在广播，可尝试延长扫描时间。")
    else:
        print(f"\n共发现 {count} 个设备。用 --address 地址 连接并查看可写特征值。")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=positive_seconds, default=10, help="扫描/连接超时秒数，默认 10")
    parser.add_argument("--name", help="扫描结果按名称包含关系过滤")
    parser.add_argument("--address", help="连接该设备，列出完整服务和特征值，不发送数据")
    parser.add_argument("--pair", action="store_true", help="连接时请求配对，可增加 --timeout 留出确认时间")
    args = parser.parse_args()
    if args.pair and not args.address:
        parser.error("--pair 需要同时指定 --address")
    try:
        asyncio.run(discover(args))
    except KeyboardInterrupt:
        print("\n已取消。", file=sys.stderr)
        return 130
    except Exception as exc:
        hint = bluetooth_error_hint(exc)
        print(f"发现设备失败：{hint or f'{type(exc).__name__}: {exc}'}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
