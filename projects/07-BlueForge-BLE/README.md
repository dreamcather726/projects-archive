# Python 蓝牙调试工具（GUI / 命令行）

程序按 **BLE（低功耗蓝牙）GATT** 实现，提供中文 GUI 和命令行两种操作方式，适用于 Windows 10/11 上支持 BLE 的蓝牙适配器与目标设备，使用 Python 3.10 或以上版本。

**蓝牙 5.0 是版本号，不代表设备一定提供 BLE 数据收发服务。** 目标设备必须支持 BLE 广播、连接，并提供接收数据的可写特征值。HC-05/HC-06 等经典蓝牙串口设备应通过配对后的 COM 口通信；这两个 BLE 程序不支持经典蓝牙 SPP、音频传输或系统的“蓝牙发送文件”（OBEX）。

## 1. 安装依赖

在本目录打开 PowerShell：

```powershell
python -m pip install -r requirements.txt
```

## 图形界面（推荐）

### 小智：电脑打字，设备用语音回答

先给小智烧录支持 BLE 文字提问的新版主固件（仅有 BLE 控制的旧版不支持）。
小智需连上 Wi-Fi，并在官方 xiaozhi.me 完成激活。

1. 启动 `python bluetooth_gui.py`，扫描并连接 `Xiaozhi-Control`；通常无需勾选配对。
2. 程序识别小智服务后自动选中正确特征值；不要选择 `00002b29…`。
3. 在 **文本** 页输入问题，点击 **发送文字，让小智回答**，听小智喇叭播放回答。
4. 可点击 **查询小智状态** 检查云端进度，或在回答期间点击 **打断回答**。
5. 等小智回到空闲再发送下一条文字。**发送数据** 按钮仍用于原始控制命令。

文字发送按钮自动订阅并拼接设备 JSON 回执，不需要手动配置分包。
单条最多 1024 个 UTF-8 字节（约 341 个汉字）；支持引号、换行、表情。
文字经 BLE 发给设备，设备通过 Wi-Fi 请求云端，语音在小智上播放。
设备回执“已受理”仅表示已提交任务；联网失败或超时可查询 `text_status`。

命令行提问（自动填写 UUID、分包、订阅回执，并查询语音回答状态）：

```powershell
python bluetooth_send.py --address "B8:1F:3F:A5:EC:DE" --xiaozhi-text "你好，请介绍一下你自己"
```

旧固件会明确提示需要升级。超时不自动重发，避免重复提问。
`text_status:done` 表示收到了云端 TTS 结束消息，喇叭实际播放仍需设备验收。

### 通用蓝牙调试

```powershell
python bluetooth_gui.py
```

也可以双击本目录下的 `启动界面.pyw`，需要 Windows 已将 `.pyw` 文件关联到 Python。GUI 使用 Python 自带的 Tkinter，无需额外安装界面库。如果双击无反应，用上述命令启动以查看错误；若提示缺少 `tkinter`，请安装带 Tcl/Tk 的 Python。

1. 打开电脑蓝牙和目标设备，点击 **扫描设备**。可设置名称筛选和扫描超时。
2. 在列表中选择设备，点击 **连接 / 读取服务**；也可直接输入设备地址。需要配对的设备勾选 **连接时配对**，并将超时增加到 60 秒。
3. 根据设备协议，在 **写入特征值** 下拉框中选择接收数据的特征值。列表显示 handle、UUID 和属性；程序不会自动猜测业务用途。
4. 选择 **文本**、**HEX** 或 **文件** 标签页，输入内容或选择文件，点击 **发送数据**。编码、分包大小、分包间隔和写入响应模式均可调整，含义见下文参数说明。
5. 需要设备回复时，选择 **回复特征值** 并点击 **应用订阅**；发送时也会应用当前选择。订阅后持续显示回复的 HEX 和文本。选择 **不订阅** 后点击 **应用订阅** 可停止接收。
6. 使用结束点击 **断开**，或直接关闭窗口；关闭会停止后台操作并断开连接。发送中关闭时，之前的分包可能已发出。

扫描、连接、文件读取和发送在后台执行；所有 BLE 操作复用同一个持续运行的 asyncio 事件循环，遵循 [Bleak 的事件循环要求](https://bleak.readthedocs.io/en/stable/troubleshooting.html#calling-asyncio-run-more-than-once)。运行记录显示连接状态、服务、发送进度和错误原因。连接保持到手动断开、设备断连或窗口关闭，支持连续发送。扫描前需要先断开当前连接。

下方命令行工具仍可单独使用。

## 2. 发现附近设备

打开电脑蓝牙，让目标设备进入 BLE 广播状态：

```powershell
python bluetooth_discover.py
python bluetooth_discover.py --timeout 15 --name ESP32
```

输出包括设备名称、地址、信号强度和广播中的服务 UUID。广播中的服务列表可能不完整。

## 3. 查看目标设备的可写特征值

把示例地址替换为扫描结果中的实际地址：

```powershell
python bluetooth_discover.py --address "AA:BB:CC:DD:EE:FF"
```

连接后会列出所有服务、特征值 UUID、handle 和属性。标记 `[可写]` 的特征值支持写入，但应根据设备说明书或固件确认哪一个用于接收业务数据。**服务 UUID 和特征值 UUID 不可混用，也不能只凭“可写”判断用途。**

设备要求配对时，可以加 `--pair --timeout 60`，并按系统提示完成配对。

## 4. 发送数据

以下地址、UUID 和 handle 都是占位示例，请替换为目标设备的实际值。

发送 UTF-8 文本（不自动添加换行）：

```powershell
python bluetooth_send.py --address "AA:BB:CC:DD:EE:FF" --char "实际可写特征值UUID" --text "你好，蓝牙！"
```

发送十六进制数据：

```powershell
python bluetooth_send.py --address "AA:BB:CC:DD:EE:FF" --char "实际可写特征值UUID" --hex "01 02 03 FF"
```

发送文件原始字节：

```powershell
python bluetooth_send.py --address "AA:BB:CC:DD:EE:FF" --char "实际可写特征值UUID" --file ".\data.bin"
```

发送并订阅设备回复：

```powershell
python bluetooth_send.py --address "AA:BB:CC:DD:EE:FF" --char "实际可写特征值UUID" --text "hello" --notify "实际通知特征值UUID" --wait 5
```

同一 UUID 对应多个特征值时，可用发现程序显示的 handle 明确选择，例如 `--char "handle:42"`。handle 可能随设备固件或服务变化，使用前应重新查看。

## 参数与协议说明

- `--chunk-size 20`：默认每次写入 20 字节；可按设备协议调整，程序会检查当前写入上限。
- `--interval 0.05`：分包间隔，单位秒。
- `--response auto`：根据属性选择，优先使用有响应写入；可指定 `yes` 或 `no`，所选模式必须受目标特征值支持。
- `--encoding utf-8`：发送文本与显示通知文本时的编码，可改为设备要求的编码。
- `--hex "68 65 6C 6C 6F 0D 0A"`：发送带 CRLF 的 hello。`--text` 中的字面 `\r\n` 不会被自动转换为换行。

分包会改变单次 GATT 写入的边界。目标设备必须能按约定拼接；如果每次写入就是一条完整命令，请按设备协议设置分包大小，避免拆开命令。发送文件仅表示写入文件字节，不会自动传输文件名、保存文件、校验完整性或实现断点续传，这些需要接收端协议支持。

有响应写入的 GATT 响应也不等于业务操作成功；无响应写入只表示提交写入。程序不会自动重试，避免重复执行命令；中途失败时，之前的分包可能已发出。

## 常见问题

- `No Bluetooth adapter found` / `NO_BLUETOOTH`：Windows 未提供可用的本机蓝牙适配器，扫描尚未开始。先按 Win+X 打开设备管理器，检查蓝牙适配器是否存在、被禁用或有驱动错误。USB 蓝牙适配器可尝试重新插接、更换 USB 口并安装厂商驱动；电脑没有蓝牙硬件时，需要添加支持 BLE 的适配器。仅有远端蓝牙 5.0 设备、Wi-Fi 网卡或正在运行的蓝牙服务，并不代表本机有可用的蓝牙硬件。增加扫描时间或修改目标地址不能解决这个错误。
- 适配器存在但蓝牙关闭：在 Windows 设置中打开蓝牙，并检查飞行模式。程序会针对适配器缺失、蓝牙关闭、权限被拒绝等原因分别提示。
- 扫描不到：确认设备支持 BLE、正在广播、距离足够近；关闭其他占用设备的手机应用或连接，尝试增大 `--timeout`。
- 连接或写入失败：确认地址和特征值正确，检查设备是否要求配对，以及数据格式、权限和写入模式。
- 手机、电脑、耳机不一定开放通用 BLE 数据接收接口；接收端需要提供对应 GATT 服务。
- 运行 `python bluetooth_discover.py --help` 和 `python bluetooth_send.py --help` 查看全部参数。

API 参考：[Bleak 扫描文档](https://bleak.readthedocs.io/en/stable/api/scanner.html)、[Bleak 连接和写入文档](https://bleak.readthedocs.io/en/stable/api/client.html)。
