from platform import node
import sys
import os
from datetime import datetime, timezone, timedelta
import serial
import json
import time
import subprocess
import re
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QWidget
from PyQt5.QtGui import QPixmap, QFont, QPainter, QBrush, QConicalGradient, QPen, QColor
from PyQt5.QtCore import QThread, QObject, pyqtSignal, Qt, QTimer, QRectF
import PyQt5.uic as uic
import paho.mqtt.client as mqtt
import ssl


### 语音输入串口参数
VOICE_PORT = 'COM2'    # 连接语音模块的串口
LORA_PORT = 'COM16'      # 连接LoRa模块的串口
VOICE_BAUD_RATE = 115200  # 语音模块的波特率
LORA_BAUD_RATE = 115200  # LoRa模块的波特率
VOICE_TIMEOUT  = 1 
LORA_TIMEOUT  = 1 

DISPLAY_MODE4K=False ##是否显示4K图片
asktime=5000  # 询问设备数据的时间间隔，默认5秒



OPEN = 0x01  # 打开指令
CLOSE = 0x00  # 关闭指令

DEVICE_1 = 0x01  # 门设备ID
DEVICE_2 = 0x02  # 灯设备ID

PUMP_PIN=0x01  # 水泵指令
FAN_PIN=0x02   # 风扇指令


##图片路径
IMG_PATH = r"PIC"
BACKGROUND_IMAGE = os.path.join(IMG_PATH, "background.jpg")
# --- 全局串口对象 ---
# 在程序启动时初始化，供全局调用
voice_ser = None  # 语音输入串口实例
lora_ser = None     # LoRa串口实例
send_lora_Times=3 # 发送LoRa指令的次数，默认3次
is_sending = False  # 全局变量，用于标识当前是否正在发送数据
# {"id":1,"t":1,"h":1,"l":50,"w":1,"s":1,"p":1,"f":1}
# {"id":2,"t":2,"h":2,"l":50,"w":2,"s":0,"p":0,"f":0}
# 创建一个长度为8的数组，用于保存设备数据
device_data = [{'id': 1, 'temp': 0, 'hum': 0.0, 'light': 0, 'weather': 0, 'soil_hum': 0, 'pump': 0, 'fan': 0}, 
{'id': 2, 'temp': 0, 'hum': 0.0, 'light': 0, 'weather': 0, 'soil_hum': 0, 'pump': 0, 'fan': 0}]  # 初始化两个设备的数据字典

# 设备信息配置 - 华为云IoT平台
# 你的设备配置
DEVICE_CONFIGS = [
    {
        "device_name": "device_1",
        "device_id": "68ee03050a9ab42ae5847f9e_test",
        "username": "68ee03050a9ab42ae5847f9e_test",
        "password": "REDACTED_CONFIGURE_LOCALLY",
        "clientId": "68ee03050a9ab42ae5847f9e_test_0_0_2025102308",
        "hostname": "6d5aef22d8.st1.iotda-device.cn-east-3.myhuaweicloud.com",
        "port": 8883,
        "protocol": "MQTTS"
    },
    {
        "device_name": "device_2",
        "device_id": "68ee03050a9ab42ae5847f9e_text2",
        "username": "68ee03050a9ab42ae5847f9e_text2",
        "password": "REDACTED_CONFIGURE_LOCALLY",
        "clientId": "68ee03050a9ab42ae5847f9e_text2_0_0_2025102308",
        "hostname": "6d5aef22d8.st1.iotda-device.cn-east-3.myhuaweicloud.com",
        "port": 8883,
        "protocol": "MQTTS"
    }
]

# 默认使用第一个设备的配置
CONFIG = DEVICE_CONFIGS[0]

# MQTT相关全局变量
mqtt_clients = {}
connected = {}
message_sent = {}
service_id = "DataGet"

# MQTT回调函数
def on_connect(client, userdata, flags, rc):
    """连接成功回调函数"""
    client_id = userdata.get('clientId', 'unknown')
    device_name = userdata.get('device_name', 'unknown')
    connected[client_id] = True
    if rc == 0:
        print(f"✅ [{device_name}] 成功连接到华为云IoT平台!")
    else:
        print(f"❌ [{device_name}] 连接失败，错误代码: {rc}")

def on_disconnect(client, userdata, rc):
    """断开连接回调函数"""
    client_id = userdata.get('clientId', 'unknown')
    device_name = userdata.get('device_name', 'unknown')
    connected[client_id] = False
    print(f"⚠️  [{device_name}] 与华为云IoT平台断开连接，错误代码: {rc}")
    if rc != 0:
        print(f"[{device_name}] 意外断开连接，尝试重新连接...")
        client.reconnect()

def on_publish(client, userdata, mid):
    """消息发布成功回调函数"""
    client_id = userdata.get('clientId', 'unknown')
    device_name = userdata.get('device_name', 'unknown')
    message_sent[client_id] = True
    print(f"✅ [{device_name}] 消息发布成功")

def get_pub_topic(client_id):
    """获取发布主题"""
    return "$oc/devices/{}/sys/properties/report".format(client_id)

def build_message(nodeId, Temp, Hum, Light, Weather, Soil_hum, Pump_Status, Fan_Status):
    """构建符合华为云IoT平台要求的JSON消息格式"""
    from datetime import datetime
    # 获取当前时间，格式化为ISO 8601格式
    now = datetime.now().strftime("%Y%m%dT%H%M%SZ")
    
    # 构建华为云要求的消息格式
    message = {
        "services": [
            {
                "service_id": service_id,
                "properties": {
                    "nodeId": nodeId,  # 设备ID
                    "Temp": Temp,      # 温度值
                    "Hum": Hum,        # 湿度值
                    "Light": Light,    # 光照强度
                    "Soil_hum": Soil_hum,  # 土壤湿度
                    "Weather": bool(Weather),  # 天气状况
                    "Pump_Status": bool(Pump_Status),  # 水泵状态
                    "Fan_Status": bool(Fan_Status)   # 风扇状态
                },
                "event_time": now  # 当前时间
            }
        ]
    }
    return json.dumps(message)

def connect_to_huaweicloud(device_config=None):
    """连接到华为云IoT平台"""
    # 如果没有提供设备配置，使用默认配置
    if device_config is None:
        device_config = CONFIG
    
    device_name = device_config.get('device_name', 'default_device')
    print(f"[{device_name}] 开始连接华为云IoT平台...")
    
    # 创建MQTT客户端
    client = mqtt.Client(client_id=device_config["clientId"])
    
    # 设置用户数据，用于在回调函数中识别设备
    client.user_data_set(device_config)
    
    # 设置回调函数
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_publish = on_publish
    
    # 设置用户名和密码
    client.username_pw_set(device_config["username"], device_config["password"])
    
    # 配置TLS连接（因为使用MQTTS）
    client.tls_set(
        ca_certs=None,  # 使用系统默认CA证书
        certfile=None,  # 客户端证书
        keyfile=None,   # 客户端密钥
        cert_reqs=ssl.CERT_REQUIRED,
        tls_version=ssl.PROTOCOL_TLS,
        ciphers=None
    )
    
    # 连接到服务器
    try:
        client.connect(device_config["hostname"], device_config["port"], keepalive=60)
        return client
    except Exception as e:
        print(f"❌ [{device_name}] 连接失败: {str(e)}")
        return None

def send_data(client, nodeId, Temp, Hum, Light, Weather, Soil_hum, Pump_Status, Fan_Status):
    """发送数据到华为云IoT平台"""
    if not client:
        print("❌ 客户端未初始化，无法发送数据")
        return False
    
    # 获取设备配置信息
    device_config = client._userdata
    device_name = device_config.get('device_name', 'unknown')
    client_id = device_config.get('clientId', 'unknown')
    
    # 检查连接状态
    if client_id not in connected or not connected[client_id]:
        print(f"❌ [{device_name}] 客户端未连接，无法发送数据")
        return False
    
    # 构建消息
    message = build_message(nodeId, Temp, Hum, Light, Weather, Soil_hum, Pump_Status, Fan_Status)
    
    # 获取发布主题
    pub_topic = get_pub_topic(client_id)
    
    # 发布消息
    result = client.publish(
        topic=pub_topic,
        payload=message,
        qos=1  # 使用QoS 1确保消息至少送达一次
    )
    
    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        # print(f"📤 [{device_name}] 正在发送消息到主题: {pub_topic}")
        return True
    else:
        print(f"❌ [{device_name}] 消息发布失败: {result.rc}")
        return False

# ================== LoRa指令发送 ==================
def send_lora_command(device_id, node_id, action):
    """
    构建并发送LoRa指令。
    指令格式: [帧头, 设备ID, 节点ID, 动作, 校验和]
    """
    global lora_ser, is_sending
    
    # 检查当前是否正在发送数据，如果是则等待
    while is_sending:
        print("[信息] 当前有数据正在发送，等待发送完成...")
        time.sleep(0.1)
    
    # 设置发送状态为正在发送
    is_sending = True
    
    try:
        # 检查LoRa串口是否已初始化并打开
        if not lora_ser or not lora_ser.is_open:
            print("[错误] LoRa串口未初始化或未打开")
            return
        
        # --- 构建指令字节串 ---
        header = 0x55  # 固定的帧头
        # 计算位
        checksum =0xFF
        # 将所有部分打包成一个bytes对象
        command = bytes([header, device_id, node_id, action, checksum])
        print(f"发送指令: {command}")
        # 为了提高无线通信的可靠性，同一指令连续发送3次
        for attempt in range(send_lora_Times):
            lora_ser.write(command)
            # 为了方便调试，将发送的字节指令以十六进制格式打印出来
            hex_string = ' '.join(f'{b:02x}' for b in command)
            print(f"-> LoRa发送 (第{attempt+1}次): {hex_string}")
            time.sleep(0.3)  # 每次发送后短暂延时
        
        # 3次发送完成后，再延时一小段时间，确保LoRa模块有时间处理和切换收发状态
        time.sleep(0.2)
    except Exception as e:
        print(f"[严重错误] LoRa发送失败: {e}")
    finally:
        # 无论是否发生异常，都设置发送状态为未发送
        is_sending = False

def send_inquiry_frame(device_id=0x01):
    """
    发送问询帧到LoRa模块
    
    Args:
        device_id (int): 目标设备ID，默认为0x01
    """
    global lora_ser, is_sending
    
    # 检查当前是否正在发送数据，如果是则等待
    while is_sending:
        print("[信息] 当前有数据正在发送，等待发送完成...")
        time.sleep(0.5)
    
    # 设置发送状态为正在发送
    is_sending = True
    
    try:
        # 检查LoRa串口是否已初始化并打开
        if not lora_ser or not lora_ser.is_open:
            print("[错误] LoRa串口未初始化或未打开，无法发送问询帧")
            return
        
        # 构建问询帧，格式: AA 设备号 FF
        inquiry_frame = bytes([0xAA, device_id, 0xFF])
        
        # 发送问询帧
        lora_ser.write(inquiry_frame)
        hex_string = ' '.join(f'{b:02x}' for b in inquiry_frame)
        print(f"-> 发送问询帧: {hex_string} (设备ID: {device_id})")
        time.sleep(1)  # 等待发送完成
    except Exception as e:
        print(f"[错误] 发送问询帧失败: {e}")
    finally:
        # 无论是否发生异常，都设置发送状态为未发送
        is_sending = False

##处理串口识别到的数据
class SerialWorker(QObject):
        """用于在后台线程中处理串口通信"""
        # 定义信号，用于向主窗口发送数据
        data_received = pyqtSignal(str)   # 发送设备数据给主线程
        data_voice_data = pyqtSignal(int,int,int)  # 发送语音指令数据给主线程
        
        def handle_Voice_data(self, data):
            """
            处理从语言识别串口接收到的数据，数据格式为十六进制格式(55 AA 01 01 00 FF)
            
            Args:
                data (str): 接收到的串口数据
            """
            try:
                # 检查是否有内容
                if not data.strip():
                    return
                
                # 尝试清理数据并提取十六进制值
                # 移除可能的空格、换行符等非十六进制字符
                clean_data = data.strip().replace(' ', '')
                
                # 检查数据是否为有效的十六进制格式
                if len(clean_data) % 2 != 0 or not re.match(r'^[0-9a-fA-F]+$', clean_data):
                    print(f"[处理] 不是有效的十六进制格式: {data}")
                    return
                
                # 将十六进制字符串转换为字节列表
                hex_bytes = []
                for i in range(0, len(clean_data), 2):
                    hex_bytes.append(int(clean_data[i:i+2], 16))
                
                # 检查是否符合指令格式 (55 01 01 00 FF)
                # 指令格式: [帧头1, 设备ID, 指令1, 指令2, 校验和]
                if len(hex_bytes) >= 5 and hex_bytes[0] == 0x55:
                    device_id = hex_bytes[1] if len(hex_bytes) > 2 else 0
                    node = hex_bytes[2] if len(hex_bytes) > 3 else 0
                    command = hex_bytes[3] if len(hex_bytes) > 4 else 0
                    self.data_voice_data.emit(device_id,node,command)
                else:
                    print(f"[处理] 不是有效的语音指令帧格式")
                    print(f"  接收到的字节: {['0x{:02X}'.format(b) for b in hex_bytes]}")
                    
            except Exception as e:
                print(f"[错误] 处理语音指令数据时发生异常: {e}")

        def handle_json_data(self, data: str):
            """处理十六进制格式的串口数据，解析为JSON并更新全局设备数据数组"""
            if not data.strip():
                return  # 空数据直接返回
            raw_hex = data.strip()

            try:
                # 1. 将十六进制字符串转换为字节（去除空格后直接转换）
                hex_str = raw_hex.replace(' ', '')  # 去掉空格，如"7b 22 64"→"7b2264"
                byte_data = bytes.fromhex(hex_str)  # 转换为字节串

                # 2. 字节串解码为UTF-8字符串（这一步会得到真正的JSON文本）
                json_str = byte_data.decode('utf-8')
                # print(f"-> 解码后的JSON: {json_str}")

                # 3. 解析JSON数据
                json_data = json.loads(json_str)

                # 4. 提取字段并更新全局设备数据数组
                device_id = json_data.get('id', '未知')
                
                # 确保device_id是整数，用于数组索引
                try:
                    index = int(device_id) - 1  # 假设设备ID从1开始，转换为0-based索引
                    # 检查索引是否在有效范围内
                    if 0 <= index < len(device_data):
                        # 创建设备数据字典
                        device_info = {
                            'id': device_id,
                            'temp': json_data.get('t', 'N/A'),      # 温度
                            'hum': json_data.get('h', 'N/A'),      # 湿度
                            'light': json_data.get('l', 'N/A'),      # 光照
                            'weather': json_data.get('w', 'N/A'),      # 天气
                            'soil_hum': json_data.get('s', 'N/A'),      # 土壤湿度
                            'pump': json_data.get('p', 'N/A'),      # 水泵状态
                            'fan': json_data.get('f', 'N/A'),      # 风扇状态
                        }
                        # 更新全局设备数据数组
                        device_data[index] = device_info
                        
                        
                        # 格式化输出字符串
                        result = self.format_device_data(device_info)
                        print(result)
                        # 发送格式化的数据到主窗口
                        self.data_received.emit(result)
                    else:
                        print(f"[错误] 设备ID {device_id} 超出数组范围")
                        # 即使索引无效，也尝试发送数据以便显示
                        result = self.format_device_data({
                            'id': device_id,
                            'temp': json_data.get('t', 'N/A'),
                            'hum': json_data.get('h', 'N/A'),
                            'light': json_data.get('l', 'N/A'),
                            'weather': json_data.get('w', 'N/A'),
                            'soil_hum': json_data.get('s', 'N/A'),
                            'pump': json_data.get('p', 'N/A'),
                            'fan': json_data.get('f', 'N/A')
                        })
                        self.data_received.emit(result)
                except ValueError:
                    print(f"[错误] 无效的设备ID: {device_id}")
                    # 处理非整数设备ID的情况
                    result = self.format_device_data({
                        'id': device_id,
                        'temp': json_data.get('t', 'N/A'),
                        'hum': json_data.get('h', 'N/A'),
                        'light': json_data.get('l', 'N/A'),
                        'weather': json_data.get('w', 'N/A'),
                        'soil_hum': json_data.get('s', 'N/A'),
                        'pump': json_data.get('p', 'N/A'),
                        'fan': json_data.get('f', 'N/A')
                    })
                    self.data_received.emit(result)

            except Exception as e:
                print(f"[解析失败] {e}")
        
        def format_device_data(self, device_info):
            """格式化设备数据为字符串"""
            device_id = device_info.get('id', '未知')
            temp = device_info.get('temp', 'N/A')
            humidity = device_info.get('hum', 'N/A')
            light = device_info.get('light', 'N/A')
            weather = device_info.get('weather', 'N/A')
            soil_humidity = device_info.get('soil_hum', 'N/A')
            pump_status = device_info.get('pump', 'N/A')
            fan_status = device_info.get('fan', 'N/A')
            
            return f"设备{device_id}: 温度{temp}°C, 湿度{humidity}%, 光照{light}, 天气{weather}, 土壤湿度{soil_humidity}, 水泵状态{'开' if pump_status == 1 else '关' if pump_status == 0 else pump_status}, 风扇状态{'开' if fan_status == 1 else '关' if fan_status == 0 else fan_status}"

        def __init__(self, port, baud_rate, timeout):
            super().__init__()
            self.port = port
            self._is_running = True
            self.baud_rate = baud_rate
            self.timeout = timeout
            self.ser = None
        def stop(self):
            """设置停止标志位"""
            self._is_running = False
            print("已向后台线程发送停止信号...")
        def run(self):
            """在后台线程中打开串口"""
            print("后台串口监听线程已启动。")
            while self._is_running:
                try:
                    ##监听语言识别的串口
                    if voice_ser and voice_ser.is_open:
                        # 直接读取原始字节数据，不进行UTF-8解码
                        raw_data = voice_ser.readline()
                        if raw_data:
                            # 将原始字节数据转换为十六进制字符串格式
                            hex_data = ' '.join(f'{b:02x}' for b in raw_data)
                            print(f"<- 串口接收 (原始字节): {hex_data}")
                            # 直接传递原始数据的十六进制表示给处理函数
                            self.handle_Voice_data(hex_data)
                    # 短暂休眠以减少CPU占用
                    time.sleep(0.01)
                except serial.SerialException as e:
                    print(f"[严重错误] 后台线程发生未知错误: {e}")
                    # 发生错误时短暂休眠，避免刷屏
                    time.sleep(2)
                except Exception as e:
                    print(f"[错误] 处理串口数据时发生异常: {e}")
                    time.sleep(0.1)
                #监听lora指令的串口
                try:
                    ##监听控制指令的串口
                    if lora_ser and lora_ser.is_open:
                        # 初始化接收缓冲区（如果不存在）
                        if not hasattr(self, 'json_buffer'):
                            self.json_buffer = b''
                            self.is_collecting_json = False
                        
                        # 读取数据
                        raw_data = lora_ser.read(1)  # 一次读取一个字节
                        if raw_data:
                            # 检查是否是JSON开始符
                            if raw_data == b'{':
                                self.is_collecting_json = True
                                self.json_buffer = b'{'  # 重置缓冲区，开始收集
                            # 如果正在收集JSON数据
                            elif self.is_collecting_json:
                                self.json_buffer += raw_data
                                # 检查是否是JSON结束符
                                if raw_data == b'}':
                                    self.is_collecting_json = False
                                    # 将完整的JSON数据转换为十六进制字符串格式
                                    hex_data = ' '.join(f'{b:02x}' for b in self.json_buffer)
                                    # 传递完整的JSON数据给处理函数
                                    self.handle_json_data(hex_data)
                                    # print(f"接收到完整JSON: {self.json_buffer.decode('utf-8', errors='ignore')}")
                                    # 清空缓冲区
                                    self.json_buffer = b''
                except Exception as e:
                    print(f"[错误] 处理控制指令数据时发生异常: {e}")
                    # 发生错误时重置缓冲区状态
                    self.json_buffer = b''
                    self.is_collecting_json = False
                    time.sleep(0.1)     

##主线程窗口
class MainWindow(QMainWindow):
    ##初始化主线程窗口，处理和更新ui显示
    def __init__(self):
        super().__init__()        
        uic.loadUi('main.ui', self)##加载UI文件

        self.set_background_image()##设置背景图片        
        self.init_time_display()##初始化时间显示        
        self.init_inquiry_timer()##初始化问询定时器
        self.init_labels()##初始化所有标签显示
        # 初始化MQTT连接
        self.init_mqtt_clients()
    def init_time_display(self):
        """初始化时间显示"""
        # 设置标签样式
        self.clock.setFont(QFont("SimHei", 25, QFont.Bold))
        self.clock.setStyleSheet("color: cyan;")
        self.clock.setAlignment(Qt.AlignCenter)
        self.clock.raise_()
        
        # 初始化计时器，每秒更新一次时间
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_time)
        self.timer.start(asktime)  # 5000毫秒 = 5秒
        
        # 立即更新一次时间
        self.update_time()        
    def update_time(self):
        """更新时间显示"""
        # 设置时区为UTC+8（北京时间）
        beijing_tz = timezone(timedelta(hours=8))
        now = datetime.now(beijing_tz)
        
        # 格式化星期，以便中文显示
        weekdays = ["星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日"]
        weekday_str = weekdays[now.weekday()]

        # 格式化日期和时间字符串，包含年月日、星期和时分秒
        time_str = f"{now.year}年{now.month}月{now.day}日 {weekday_str} {now.strftime('%H:%M:%S')}"
        
        self.clock.setText(time_str)
        self.clock.adjustSize() # 自动调整标签大小以适应内容               
    def keyPressEvent(self, event):
        """处理键盘事件"""
        if event.key() == Qt.Key_Escape:
            # ESC键：退出全屏显示（无论当前是否在全屏模式）
            if self.isFullScreen():
                self.showNormal()
                print("已退出全屏显示")
        elif event.key() == Qt.Key_Q:
            # Q键：退出程序
            print("用户按Q键退出程序")
            self.close()
        else:
            super().keyPressEvent(event)   
    def mouseDoubleClickEvent(self, event):
        """处理鼠标双击事件，切换全屏模式"""
        if self.isFullScreen():
            self.showNormal()            
        else:
            self.showFullScreen()    
    def init_label(self, label, initial_value, color="white"):
        """初始化标签显示"""
        # 设置文字内容
        label.setText(str(initial_value))
        
        # 设置文字样式
        label.setFont(QFont("SimHei", 15, QFont.Bold))  # 字体、大小、加粗
        label.setStyleSheet(f"color: {color};")  # 文字颜色   
        
        # 设置文字对齐方式
        label.setAlignment(Qt.AlignCenter)        
        # 确保文字显示在背景上方
        label.raise_()
    def init_labels(self):
        """初始化所有标签显示"""
        self.init_label(self.temp1, "88.8℃")
        self.init_label(self.hum1, "88.8%")
        self.init_label(self.light1, "88级")
        self.init_label(self.soilhum1, "88.8%")
        self.init_label(self.temp2, "88.8℃")
        self.init_label(self.hum2, "88.8%")
        self.init_label(self.light2, "88级")
        self.init_label(self.soilhum2, "88.8%")
    def set_background_image(self):
        # 保持原有的背景图片显示逻辑不变
        image_path = BACKGROUND_IMAGE           
        try:
            pixmap = QPixmap(image_path)
            self.background_label = QLabel(self)
            if DISPLAY_MODE4K:
                self.image_width = 3840  # 3840，1920
                self.image_height = 2160  # 2160，1080
            else:
                self.image_width = 1920  # 3840，1920
                self.image_height = 1080  # 2160，1080
            self.resize(self.image_width, self.image_height)
            
            scaled_pixmap = pixmap.scaled(
                self.image_width, 
                self.image_height, 
                Qt.KeepAspectRatio, 
                Qt.SmoothTransformation
            )            
            self.background_label.setGeometry(0, 0, self.image_width, self.image_height)
            self.background_label.setPixmap(scaled_pixmap)
            self.background_label.setAlignment(Qt.AlignCenter)
            self.background_label.lower()
        except Exception as e:
            print(f"设置背景图片出错: {e}")    
    

    def init_mqtt_clients(self):
        """初始化MQTT客户端连接"""
        global mqtt_clients, connected, message_sent
        
        # 为每个设备创建并连接客户端
        for device_config in DEVICE_CONFIGS:
            device_name = device_config['device_name']
            client = connect_to_huaweicloud(device_config)
            if client:
                mqtt_clients[device_name] = client
                client.loop_start()  # 启动客户端循环
                # 初始化连接状态
                connected[device_config['clientId']] = False
                message_sent[device_config['clientId']] = False
        
        print(f"已初始化 {len(mqtt_clients)} 个MQTT客户端")
    
    def init_inquiry_timer(self):
        """初始化问询定时器"""
        # 创建定时器用于定时问询设备状态
        self.inquiry_timer = QTimer(self)
        self.inquiry_timer.timeout.connect(self.send_inquiry)
        self.inquiry_timer.start(asktime)  # 设置问询间隔
        
        # 创建定时器用于按顺序问询不同设备
        self.device_timer = QTimer(self)
        self.device_timer.timeout.connect(self.send_next_inquiry)
        
        # 初始化设备索引，用于轮询
        self.current_device_index = 0
        print(f"问询定时器已启动，每{asktime/1000}秒发送一次问询帧")
    
    def send_inquiry(self):
        """发送问询帧到所有设备，使用非阻塞方式"""
        # 使用计数器跟踪当前要发送的设备ID
        self.current_device_index = 0
        # 启动设备发送定时器
        self.device_timer = QTimer(self)
        self.device_timer.timeout.connect(self.send_next_inquiry)
        self.device_timer.start(100)  # 立即开始发送第一个设备
    
    def send_next_inquiry(self):
        """发送下一个设备的问询帧"""
        # 检查是否还有设备需要发送
        if self.current_device_index < len(device_data):
            device_id = self.current_device_index + 1  # 设备ID从1开始
            send_inquiry_frame(device_id=device_id)
            print(f"发送问询帧到设备 {device_id}")
            # 增加索引
            self.current_device_index += 1
            # 设置下一次发送的延时为1秒
            self.device_timer.setInterval(asktime)
        else:
            # 所有设备都已发送，停止定时器
            self.device_timer.stop()
            print("完成所有设备问询帧发送")
        
    def update_display(self, data):
        """接收并处理SerialWorker发送的数据,同时从全局device_data数组中获取完整数据进行显示"""
        print(f"数据数组: {device_data}")
        
        # 将设备数据发送到华为云IoT平台
        self.send_to_huaweicloud()
    
    def send_to_huaweicloud(self):
        """将设备数据发送到华为云IoT平台"""
        global mqtt_clients, connected
        
        # 为每个设备发送对应的数据
        for i, device in enumerate(device_data):
            # 确保设备索引在有效范围内
            if i < len(DEVICE_CONFIGS) and i < len(mqtt_clients):
                device_config = DEVICE_CONFIGS[i]
                device_name = device_config['device_name']
                client = mqtt_clients.get(device_name)
                
                if client and connected.get(device_config['clientId'], False):
                    # 发送数据
                    send_data(
                        client=client,
                        nodeId=str(device['id']),
                        Temp=device['temp'],
                        Hum=device['hum'],
                        Light=device['light'],
                        Weather=device['weather'],
                        Soil_hum=device['soil_hum'],
                        Pump_Status=device['pump'],
                        Fan_Status=device['fan']
                    )
                    print(f"[{device_name}] 数据已发送: {device}")
                elif not client:
                    print(f"❌ 设备 {device_name} 的MQTT客户端未初始化")
                elif not connected.get(device_config['clientId'], False):
                    print(f"❌ 设备 {device_name} 未连接到华为云")
        
    
    def update_voice_display(self, device_id,node,command):
        """接收并处理SerialWorker发送的语音指令数据"""        
        print(f"主线程语音指令数据: 设备ID: 0x{device_id:02X}, 节点: 0x{node:02X}, 指令: 0x{command:02X}")
        
           

# ================== 主程序入口 ==================
def main():
    """主函数"""
    global voice_ser, lora_ser
    
    # 初始化变量，避免finally块中访问未定义变量
    serial_worker = None
    worker_thread = None
    
    try:
        # 创建Qt应用程序实例
        app = QApplication(sys.argv)
        app.setApplicationName("校园沙盘控制系统")
        
        # 初始化串口
        print("正在初始化串口...")
        # 1. 尝试打开语音识别模块串口
        try:
            voice_ser = serial.Serial(VOICE_PORT, VOICE_BAUD_RATE, timeout=VOICE_TIMEOUT)
            print(f"成功打开语音识别串口 {VOICE_PORT}")
            
        except serial.SerialException as e:
            print(f"[警告] 无法打开语音识别串口 {VOICE_PORT}: {e}")
            # 仅记录警告，不中断程序
        
        # 2. 尝试打开LoRa模块串口
        try:
            lora_ser = serial.Serial(LORA_PORT, LORA_BAUD_RATE, timeout=LORA_TIMEOUT)
            print(f"成功打开LoRa串口 {LORA_PORT}")
        except serial.SerialException as e:
            print(f"[警告] 无法打开LoRa串口 {LORA_PORT}: {e}")
            # 仅记录警告，不中断程序
        
        # 3. 创建并启动SerialWorker线程        
        serial_worker = SerialWorker(VOICE_PORT, VOICE_BAUD_RATE, VOICE_TIMEOUT)
        worker_thread = QThread()
        
        # 创建主窗口
        main_window = MainWindow()
        
        # 将worker移动到线程中
        serial_worker.moveToThread(worker_thread)
        
        # 连接信号和槽
        worker_thread.started.connect(serial_worker.run)
        # 连接SerialWorker的数据信号到MainWindow的显示槽
        serial_worker.data_received.connect(main_window.update_display)
        serial_worker.data_voice_data.connect(main_window.update_voice_display)
        
        # 启动线程，这将触发started信号并调用serial_worker.run()
        worker_thread.start()
        main_window.show()
        print("串口监听已开始，请按Ctrl+C停止程序...")
        print("MQTT客户端已初始化，正在连接华为云IoT平台...")
        # 运行应用程序主循环
        sys.exit(app.exec_())   
    except Exception as e:
        print(f"[严重错误] 程序发生异常: {e}")
    finally:
        # 停止工作线程
        if serial_worker is not None:
            try:
                serial_worker.stop()
            except Exception as e:
                print(f"停止工作线程时出错: {e}")
        
        if worker_thread is not None and worker_thread.isRunning():
            try:
                worker_thread.quit()
                worker_thread.wait()
                print("后台线程已停止")
            except Exception as e:
                print(f"停止线程时出错: {e}")
        
        # 关闭串口
        if voice_ser and voice_ser.is_open:
            try:
                voice_ser.close()
                print(f"已关闭语音输入串口 {VOICE_PORT}")
            except Exception as e:
                print(f"关闭语音串口时出错: {e}")
                
        if lora_ser and lora_ser.is_open:
            try:
                lora_ser.close()
                print(f"已关闭LoRa串口 {LORA_PORT}")
            except Exception as e:
                print(f"关闭LoRa串口时出错: {e}")
        
        # 关闭所有MQTT客户端连接
        global mqtt_clients
        for device_name, client in mqtt_clients.items():
            try:
                client.loop_stop()
                client.disconnect()
                print(f"已断开 {device_name} 的MQTT连接")
            except Exception as e:
                print(f"关闭 {device_name} 的MQTT连接时出错: {e}")
        
        print("程序已安全退出")


if __name__ == '__main__':
    main()