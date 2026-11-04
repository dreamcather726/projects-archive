import paho.mqtt.client as mqtt
import json
import time
import random
import threading

# 设备配置信息
# 这些配置用于连接华为云IoT平台
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

# 全局配置
REPORT_INTERVAL = 10  # 上报间隔（秒）

# 回调函数：连接成功
class DeviceClient:
    def __init__(self, config):
        self.config = config
        self.device_id = config["device_id"]
        self.device_name = config["device_name"]
        self.client = None
        self.is_connected = False
    
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.is_connected = True
            print(f"✅ [{self.device_name}] 连接华为云成功")
        else:
            print(f"❌ [{self.device_name}] 连接失败，错误码: {rc}")
    
    def on_disconnect(self, client, userdata, rc):
        self.is_connected = False
        print(f"⚠️  [{self.device_name}] 与华为云断开连接")
    
    def on_publish(self, client, userdata, mid):
        print(f"📤 [{self.device_name}] 数据上报成功，消息ID: {mid}")
    
    def connect(self):
        # 创建MQTT客户端
        self.client = mqtt.Client(client_id=self.config["clientId"])
        self.client.username_pw_set(self.config["username"], self.config["password"])
        self.client.tls_set()  # 启用TLS加密
        
        # 设置回调函数
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_publish = self.on_publish
        
        try:
            self.client.connect(self.config["hostname"], self.config["port"], keepalive=60)
            self.client.loop_start()  # 启动后台循环
            return True
        except Exception as e:
            print(f"❌ [{self.device_name}] 连接异常: {str(e)}")
            return False
    
    def disconnect(self):
        if self.client:
            self.client.loop_stop()
            self.client.disconnect()
    
    def generate_data(self):
        """生成模拟的传感器数据"""
        # 生成合理范围内的随机数据
        temp = round(random.uniform(20, 35), 1)  # 温度20-35℃
        hum = round(random.uniform(40, 90), 1)   # 湿度40-90%
        light = random.randint(0, 1000)          # 光照0-1000
        soil_hum = random.randint(200, 800)      # 土壤湿度200-800
        weather = random.choice([True, False])   # 天气状态 True/False
        pump = random.choice([True, False])      # 水泵状态 True/False
        fan = random.choice([True, False])       # 风扇状态 True/False
        auto_mode = random.choice([True, False]) # 自动模式 True/False
        
        return {
            "Temp": temp,
            "Hum": hum,
            "Light": light,
            "Soil_hum": soil_hum,
            "Weather": weather,
            "Pump_Status": pump,
            "Fan_Status": fan,
            "Auto_Mode": auto_mode
        }
    
    def report_data(self):
        """上报数据到华为云IoT平台"""
        if not self.is_connected:
            print(f"⏳ [{self.device_name}] 未连接，尝试重新连接...")
            if not self.connect():
                return False
        
        try:
            # 生成模拟数据
            data = self.generate_data()
            
            # 构建华为云IoT平台要求的上报格式
            # 使用DataGet服务进行上报
            payload = {
                "services": [
                    {
                        "service_id": "DataGet",
                        "properties": data,
                        "event_time": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
                    }
                ]
            }
            
            # 构建发布主题
            topic = f"$oc/devices/{self.device_id}/sys/properties/report"
            
            # 发布消息
            result = self.client.publish(
                topic=topic,
                payload=json.dumps(payload),
                qos=1
            )
            
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                print(f"📊 [{self.device_name}] 上报数据: {json.dumps(data)}")
                return True
            else:
                print(f"❌ [{self.device_name}] 发布失败: {result.rc}")
                return False
                
        except Exception as e:
            print(f"❌ [{self.device_name}] 上报异常: {str(e)}")
            return False
    
    def start_report_loop(self):
        """启动周期性上报循环"""
        def report_task():
            while True:
                self.report_data()
                time.sleep(REPORT_INTERVAL)
        
        thread = threading.Thread(target=report_task, daemon=True)
        thread.start()
        print(f"⏱️  [{self.device_name}] 数据上报线程已启动（每{REPORT_INTERVAL}秒上报一次）")
        return thread

def main():
    print("🚀 华为云IoT平台数据上报程序启动")
    print(f"📅 上报间隔: {REPORT_INTERVAL}秒")
    print("=" * 50)
    
    clients = []
    threads = []
    
    # 为每个设备创建客户端并启动上报
    for config in DEVICE_CONFIGS:
        client = DeviceClient(config)
        clients.append(client)
        
        # 连接并启动上报
        if client.connect():
            thread = client.start_report_loop()
            threads.append(thread)
        time.sleep(1)  # 间隔1秒，避免同时连接
    
    print("=" * 50)
    print("✅ 所有设备初始化完成，程序正在运行...")
    print("🔄 按 Ctrl+C 停止程序")
    
    try:
        # 主线程保持运行
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n🛑 正在停止程序...")
        
        # 断开所有连接
        for client in clients:
            client.disconnect()
        
        print("✅ 所有连接已断开，程序已终止")

if __name__ == "__main__":
    # 检查paho-mqtt模块
    try:
        import paho.mqtt.client
    except ImportError:
        print("❌ 未找到paho-mqtt模块，请先安装：pip install paho-mqtt")
        exit(1)
    
    main()

# 使用说明：
# 1. 确保已安装 paho-mqtt: pip install paho-mqtt
# 2. 直接运行本文件：python text.py
# 3. 程序会自动连接华为云IoT平台并定期上报模拟数据