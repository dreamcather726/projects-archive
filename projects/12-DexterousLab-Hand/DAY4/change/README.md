# 手指控制上位机

## 功能说明
- 使用 PyQt5 图形界面控制 4 个手指
- 通过串口与 ESP32-S3 通信
- 每个手指可单独控制角度（90°~180°）

## 硬件连接
- ESP32-S3 → PCA9685: SDA = 14, SCL = 15
- 4 个手指，每个手指 2 个舵机

## 安装依赖
```bash
pip install -r requirements.txt
```

## 使用方法
1. 编译并上传 Arduino 代码到 ESP32-S3
2. 运行 Python 上位机：
   ```bash
   python finger_controller.py
   ```
3. 在界面中：
   - 选择正确的串口
   - 点击「连接」按钮
   - 拖动滑块调整每个手指的角度
   - 点击「发送」按钮控制对应手指

## 通信协议
- 指令格式：`FINGER1:角度`、`FINGER2:角度`、`FINGER3:角度`、`FINGER4:角度`
- 角度范围：90°~180°
- 例如：`FINGER1:120` 控制手指1到120度
