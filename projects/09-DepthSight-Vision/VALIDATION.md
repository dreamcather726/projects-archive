# 验证记录

日期：2026-11-02。以下是本机实际运行结果，不是预期输出。

## 环境

- Windows，Python 3.13.9，项目独立 `.venv`。
- NVIDIA GeForce RTX 4060 Laptop GPU，显存 8 GB，驱动 591.74。
- Ultralytics 8.4.198，PyTorch 2.11.0+cu128，torchvision 0.26.0+cu128。
- OpenCV 4.14.0，NumPy 2.5.4。
- 内置摄像头索引 0，DirectShow 后端，实际画面 640×480。
- 官方 `yolo26n-depth.pt`，推理输入尺寸 768。

权重来源：`https://github.com/ultralytics/assets/releases/download/v8.4.0/yolo26n-depth.pt`。
本机权重 SHA256：`befed1b8561d8b2eaa66274b070cdfc9c44853bda6d6d62a04759f9383af74e9`。
CUDA 安装包下载后与 PyTorch 官方索引中的 SHA256 一致；安装后删除了本次临时 wheel 文件。

## 通过的检查

1. `python -m unittest discover -s tests -v`：5 项通过，涵盖深度中位数、无效值、边界取样、跨帧颜色一致性、图像尺寸和无窗口模式的有限帧约束。
2. `python -m pip check`：`No broken requirements found.`
3. `setup_env.ps1`：执行成功，正确识别已安装 CUDA 依赖，没有重复下载，报告 `CUDA available: True`。
4. Python 语法编译和 PowerShell 脚本语法解析通过。
5. 真实摄像头采集、模型推理、原图尺寸深度输出检查和正常退出均通过。

## 摄像头与模型实测

| 路径 | 帧数 | 设备 | 平均推理耗时 | 结果 |
| --- | --- | --- | --- | --- |
| `start_camera.cmd --frames 30 --report outputs\runtime_check.json`，带预览窗口 | 30 | CUDA 0 / RTX 4060 | 30.01 ms | 成功退出 |
| `camera_depth.py --no-display --frames 3 --device cpu --report outputs\runtime_cpu_check.json` | 3 | CPU | 226.87 ms | 成功退出 |

两次测试均使用 640×480 摄像头画面和 `imgsz=768`；每帧都检查深度图与原图尺寸一致，且存在有限的正深度值。测试结束后执行摄像头释放和窗口销毁。

这里的耗时包含模型调用、后处理及深度数组拷贝，不包含摄像头读取与窗口显示，也不包含首次导入、模型加载和预热；不能据此声称端到端达到 33 FPS。统计报告仅保存在本机 `outputs/`，不纳入 Git 备份。

另在初始 CPU 环境（PyTorch 2.15.0+cpu）中运行过带窗口的 300 帧测试，平均推理耗时 254.37 ms，成功退出。最终依赖环境以上表为准。

## 未验证的部分

- 没有用已知距离、测距仪或真实深度标签验证绝对距离误差；没有进行当前相机的尺度校准。报告中的 `accuracy_validated` 保持 `false`。
- Windows 窗口检查工具返回 `Computer Use native pipe is unavailable`，重试和重置后仍不可用。因此预览窗口创建及持续推理已通过程序运行验证，点击、按键和窗口画质没有完成自动化视觉复核。
- 未做长时间稳定性、其他电脑/摄像头或不同光照场景的验证。

## 版本备份范围

仅提交代码、依赖说明、安装/启动脚本、文档与测试。虚拟环境、模型文件、统计报告、缓存和摄像头图像不上传。未新增项目根目录 `.gitignore`。
