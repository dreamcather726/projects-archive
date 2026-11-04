# 视觉交互：YOLO26 摄像头深度估计

使用电脑内置 RGB 摄像头和官方 `yolo26n-depth.pt` 做单目深度估计。左侧显示原图，右侧显示深度热力图，支持点击读取距离、保存单帧结果、自动选择 NVIDIA GPU 或 CPU。

模型输出是**估计深度**，不是深度传感器测量值。预训练权重包含全局尺度校准，但更换相机或场景可能产生明显的米制误差。未使用尺子/测距仪进行实测前，不把显示值当作精确距离。这里的深度表示沿相机光轴的距离，不等于任意像素到相机的直线距离。

## 启动

双击项目中的 **`start_camera.cmd`**。首次运行自动创建 `.venv`、安装依赖并下载官方模型，之后直接启动摄像头。需要 Python 3.10 及以上的 64 位环境；本机使用 Python 3.13。首次安装和权重下载需要联网。

也可以在当前项目目录的 PowerShell 中运行：

```powershell
.\start_camera.cmd
```

安装脚本检测到 `nvidia-smi` 时安装 CUDA 12.8 版 PyTorch；运行程序根据 `torch.cuda.is_available()` 选择 GPU 或 CPU。使用 CPU 安装：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\setup_env.ps1 -Cpu
.\start_camera.cmd --device cpu
```

## 操作

| 操作 | 功能 |
| --- | --- |
| 点击任意一侧画面 | 查看该坐标周围 9×9 区域有效深度的中位数 |
| C | 恢复读取画面中心 |
| S | 保存原图、预览图、浮点深度数组和元数据 |
| Q / Esc / 关闭窗口 | 退出并释放摄像头 |

热力图固定使用 0.2～10 的模型估计米值范围：近处暖色、远处冷色，无效值为黑色。范围外的值只在着色时截断，原始数组和读数不截断。固定范围保证同一数值跨帧对应相同颜色；它不能消除模型本身的时间抖动。默认不镜像，`--mirror` 会同时镜像模型输入和显示。

按 S 后文件保存在 `outputs/时间戳/`：

- `rgb.png`：输入图像。
- `preview.png`：原图与热力图对照。
- `depth_estimated_m.npy`：与原图同尺寸的 float32 深度数组；`numpy.load()` 读取，非有限或非正数为无效值。
- `metadata.json`：模型名、设备、镜像状态、时间和颜色范围。

摄像头画面仅在本机推理，默认不保存图像；按 S 才保存快照。本项目没有上传图像的业务逻辑。GitHub 仅备份代码、文档和测试，不纳入 `.venv/`、`models/`、`outputs/` 或摄像头图像。

## 常用命令

```powershell
# 探测摄像头 0～4
.\start_camera.cmd --list-cameras

# 内置相机不是 0 时选择其他索引
.\start_camera.cmd --camera 1

# 摄像头后端兼容性问题
.\start_camera.cmd --backend msmf

# CPU 降低输入尺寸以提高速度；可能损失精度
.\start_camera.cmd --device cpu --imgsz 384

# 设置热力图范围和镜像
.\start_camera.cmd --near 0.2 --far 5 --mirror

# 无窗口处理 10 帧，只保存统计报告，不保存图像
.\start_camera.cmd --no-display --frames 10 --report outputs\runtime_check.json

# 测试深度采样、无效值和可视化映射
.\.venv\Scripts\python.exe -m unittest discover -s tests -v
```

默认 `imgsz=768` 与官方预训练分辨率一致。界面中的 inference FPS 只统计推理、后处理和深度拷贝耗时，不包含采集与窗口显示，不能等同于端到端帧率。程序逐帧运行，不承诺摄像头原生帧率。

## 故障处理

- 摄像头打不开：关闭会议软件/系统相机等占用程序，打开镜头遮挡开关，检查 Windows“隐私和安全性 → 相机 → 允许桌面应用访问相机”，再尝试其他相机索引或后端。
- 权重下载失败：检查 GitHub 网络连接；从官方发布页下载 `yolo26n-depth.pt` 放到 `models/`，或使用 `--model` 指定本地权重。
- 不支持 depth：使用本项目固定的 Ultralytics 版本；普通 `yolo26n.pt` 是检测模型，不能代替 `yolo26n-depth.pt`。
- 无法使用 GPU：检查 NVIDIA 驱动和 CUDA 版 PyTorch安装；也可以使用 `--device cpu`。
- 距离偏差大：先用已知距离在实际相机和场景下记录误差，再按官方 `model.calibrate()` 流程校准。单目估计会受到光照、遮挡、反光和场景变化影响。

## 文件

| 路径 | 用途 |
| --- | --- |
| `camera_depth.py` | 摄像头、深度模型、交互显示与统计报告 |
| `start_camera.cmd` | Windows 一键启动 |
| `setup_env.ps1` | 隔离环境与依赖安装 |
| `requirements.txt` | 依赖版本 |
| `tests/test_depth.py` | 深度数值及显示逻辑检查 |
| `VALIDATION.md` | 本次实际验证记录 |

本项目未新增根目录的 `.gitignore`，提交时使用明确的源文件清单，后续请勿直接 `git add .` 上传运行环境与相机数据。

## 官方依据

- [YOLO26 模型](https://docs.ultralytics.com/models/yolo26)
- [深度输出、推理、尺度校准及相机测距说明](https://docs.ultralytics.com/tasks/depth)
- [官方权重发布页](https://github.com/ultralytics/assets/releases)
- [PyTorch CUDA 12.8 包源](https://download.pytorch.org/whl/cu128/)
- [Ultralytics 许可](https://www.ultralytics.com/license)
