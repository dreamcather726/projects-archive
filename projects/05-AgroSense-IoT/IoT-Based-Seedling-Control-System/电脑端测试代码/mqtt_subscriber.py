import paho.mqtt.client as mqtt
import ssl
import time
import json

# 设备配置 - 从text.py导入（如果需要单独运行，可以取消下面注释并使用现有的DEVICE_CONFIGS）
# DEVICE_CONFIGS = [
#     {
#         "device_name": "device_1",
#         "device_id": "68ee03050a9ab42ae5847f9e_test",
#         "username": "68ee03050a9ab42ae5847f9e_test",
#         "password": "REDACTED_CONFIGURE_LOCALLY",
#         "clientId": "68ee03050a9ab42ae5847f9e_test_0_0_2025102308",
#         "hostname": "6d5aef22d8.st1.iotda-device.cn-east-3.myhuaweicloud.com",
#         "port": 8883,
#         "protocol": "MQTTS"
#     },
#     {
#         "device_name": "device_2",
#         "device_id": "68ee03050a9ab42ae5847f9e_text2",
#         "username": "68ee03050a9ab42ae5847f9e_text2",
#         "password": "REDACTED_CONFIGURE_LOCALLY",
#         "clientId": "68ee03050a9ab42ae5847f9e_text2_0_0_2025102308",
#         "hostname": "6d5aef22d8.st1.iotda-device.cn-east-3.myhuaweicloud.com",
#         "port": 8883,
#         "protocol": "MQTTS"
#     }
# ]

# 从text.py导入设备配置
try:
    from text import DEVICE_CONFIGS
    print("成功导入设备配置")
except ImportError:
    print("无法导入text.py，使用默认配置")
    # 如果导入失败，使用默认配置
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
        }
    ]

class MQTTSubscriber:
    def __init__(self, device_config):
        self.device_config = device_config
        self.client = mqtt.Client(client_id=device_config["clientId"])
        # 设置回调函数
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        self.client.on_log = self.on_log
        
        # 设置用户名密码
        self.client.username_pw_set(
            username=device_config["username"],
            password=device_config["password"]
        )
        
        # 配置SSL（华为云IoT平台使用MQTTS协议）
        self.client.tls_set(
            ca_certs=None,  # 可以指定CA证书路径以增加安全性
            certfile=None,
            keyfile=None,
            cert_reqs=ssl.CERT_REQUIRED,
            tls_version=ssl.PROTOCOL_TLSv1_2,
            ciphers=None
        )
        # 允许自签名证书（生产环境应使用有效的证书）
        self.client.tls_insecure_set(True)
    
    def on_connect(self, client, userdata, flags, rc):
        """连接成功回调函数"""
        print(f"设备 {self.device_config['device_name']} 连接成功，返回码: {rc}")
        
        # 订阅华为云IoT平台的命令主题
        # 命令主题格式: $oc/devices/{device_id}/sys/commands/#
        command_topic = f"$oc/devices/{self.device_config['device_id']}/sys/commands/#"
        client.subscribe(command_topic, qos=1)
        print(f"已订阅命令主题: {command_topic}")
        
        # 订阅设备消息主题（根据实际需求调整）
        # 设备消息主题格式: $oc/devices/{device_id}/sys/messages/#
        message_topic = f"$oc/devices/{self.device_config['device_id']}/sys/messages/#"
        client.subscribe(message_topic, qos=1)
        print(f"已订阅消息主题: {message_topic}")
    
    def on_message(self, client, userdata, msg):
        """接收消息回调函数"""
        print(f"{time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())} 设备 {self.device_config['device_name']} 收到消息:")
        print(f"  主题: {msg.topic}")
        try:
            # 尝试解析JSON格式的消息
            payload = json.loads(msg.payload.decode('utf-8'))
            print(f"  消息内容: {json.dumps(payload, indent=2, ensure_ascii=False)}")
        except json.JSONDecodeError:
            # 如果不是JSON格式，直接打印
            print(f"  消息内容: {msg.payload.decode('utf-8')}")
    
    def on_disconnect(self, client, userdata, rc):
        """断开连接回调函数"""
        if rc != 0:
            print(f"设备 {self.device_config['device_name']} 意外断开连接，返回码: {rc}")
        else:
            print(f"设备 {self.device_config['device_name']} 正常断开连接")
    
    def on_log(self, client, userdata, level, buf):
        """日志回调函数"""
        # 可以根据需要调整日志级别
        if level == mqtt.MQTT_LOG_ERR:
            print(f"MQTT错误: {buf}")
        # uncomment the following line to see all MQTT logs
        # print(f"MQTT日志 ({level}): {buf}")
    
    def connect(self):
        """连接到华为云IoT平台"""
        try:
            print(f"正在连接设备 {self.device_config['device_name']} 到华为云IoT平台...")
            self.client.connect(
                host=self.device_config["hostname"],
                port=self.device_config["port"],
                keepalive=60
            )
            return True
        except Exception as e:
            print(f"设备 {self.device_config['device_name']} 连接失败: {str(e)}")
            return False
    
    def start(self):
        """开始MQTT客户端循环"""
        try:
            self.client.loop_start()  # 非阻塞模式
            print(f"设备 {self.device_config['device_name']} MQTT客户端已启动")
        except Exception as e:
            print(f"设备 {self.device_config['device_name']} 启动MQTT客户端失败: {str(e)}")
    
    def stop(self):
        """停止MQTT客户端"""
        try:
            self.client.loop_stop()
            self.client.disconnect()
            print(f"设备 {self.device_config['device_name']} MQTT客户端已停止")
        except Exception as e:
            print(f"设备 {self.device_config['device_name']} 停止MQTT客户端失败: {str(e)}")

# 主函数
def main():
    subscribers = []
    
    try:
        # 为每个设备创建MQTT订阅客户端
        for device_config in DEVICE_CONFIGS:
            subscriber = MQTTSubscriber(device_config)
            if subscriber.connect():
                subscriber.start()
                subscribers.append(subscriber)
            else:
                print(f"跳过设备 {device_config['device_name']}")
        
        if not subscribers:
            print("没有成功连接的设备，程序退出")
            return
        
        print("\n所有设备已启动，等待接收消息...")
        print("按Ctrl+C停止程序")
        
        # 保持程序运行
        while True:
            time.sleep(1)
    
    except KeyboardInterrupt:
        print("\n用户中断程序")
    except Exception as e:
        print(f"程序发生异常: {str(e)}")
    finally:
        # 清理资源
        print("正在停止所有MQTT客户端...")
        for subscriber in subscribers:
            subscriber.stop()
        print("程序已退出")

if __name__ == "__main__":
    main()