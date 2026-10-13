#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
华为云IoT平台MQTT客户端示例
用于发送数据到华为云IoT平台
"""

import paho.mqtt.client as mqtt
import ssl
import json
import time
from datetime import datetime
import random

# 设备信息配置 - 从用户提供的信息填充
CONFIG = {
    "username": "68ee03050a9ab42ae5847f9e_test",
    "password": "REDACTED_CONFIGURE_LOCALLY",
    "clientId": "68ee03050a9ab42ae5847f9e_test_0_0_2025101503",
    "hostname": "6d5aef22d8.st1.iotda-device.cn-east-3.myhuaweicloud.com",
    "port": 8883,
    "protocol": "MQTTS"
}

# 华为云IoT平台的发布主题（固定格式）
PUB_TOPIC = "$oc/devices/{}/sys/properties/report".format(CONFIG["clientId"])

# 全局变量
connected = False
message_sent = False
service_id = "DataGet"
SoidTemp=20
SoidHum=60
SoildPH=7.0
SoildEle=1000   
IndoorTemp=22
Indoorhum=60
Light = 20
WaterBox=True
Pump=True
Fan=True 
curtain = True

def text_data():
    """测试数据"""
    global SoidTemp,SoidHum,SoildPH,SoildEle,IndoorTemp,Indoorhum,Light,WaterBox,Pump,Fan,curtain
    SoidTemp=random.randint(20,30)
    SoidHum=random.randint(0,80)
    SoildPH=random.randint(1,14)
    SoildEle=random.randint(1000,20000)   
    IndoorTemp=random.randint(20,30)
    Indoorhum=random.randint(0,80)
    Light = random.randint(0,1)
    WaterBox=True
    Pump=True
    Fan=True 
    curtain = True

# 回调函数定义
def on_connect(client, userdata, flags, rc):
    """连接成功回调函数"""
    global connected
    connected = True
    if rc == 0:
        print("✅ 成功连接到华为云IoT平台!")
    else:
        print(f"❌ 连接失败，错误代码: {rc}")


def on_disconnect(client, userdata, rc):
    """断开连接回调函数"""
    global connected
    connected = False
    print(f"⚠️  与华为云IoT平台断开连接，错误代码: {rc}")
    if rc != 0:
        print("意外断开连接，尝试重新连接...")
        client.reconnect()


def on_publish(client, userdata, mid):
    """消息发布成功回调函数"""
    global message_sent
    message_sent = True
    print(f"✅ 消息发布成功，消息ID: {mid}")


def on_message(client, userdata, msg):
    """接收消息回调函数"""
    print(f"📩 收到消息: 主题={msg.topic}, 内容={msg.payload.decode()}")


def build_message(SoidTemp, SoidHum, SoildPH, SoildEle, IndoorTemp, Indoorhum, Light, WaterBox, Pump, Fan, curtain):
    """构建符合华为云IoT平台要求的JSON消息格式"""
    # 获取当前时间，格式化为ISO 8601格式
    now = datetime.now().strftime("%Y%m%dT%H%M%SZ")
    
    # 构建华为云要求的消息格式
    message = {
        "services": [
            {
                "service_id": service_id,
                "properties": {
                    "SoidTemp": SoidTemp,  # 土壤温度值
                    "SoidHum": SoidHum,  # 土壤湿度值
                    "SoildPH": SoildPH  ,  # 土壤PH值
                    "SoildEle": SoildEle,  # 土壤Ele值
                    "IndoorTemp": IndoorTemp,  # 室内温度值
                    "Indoorhum": Indoorhum,  # 室内湿度值
                    "Light": Light,  # 光照值（1表示有光照，0表示无光照）
                    "WaterBox": WaterBox,  # 水盒状态（1表示有水，0表示无水）
                    "Pump": Pump,  # 水泵状态（1表示运行，0表示停止）
                    "Fan": Fan,  # 风扇状态（1表示运行，0表示停止）
                    "curtain": curtain  # 窗帘状态（1表示打开，0表示关闭）
                },
                "event_time": now  # 当前时间
            }
        ]
    }
    
    return json.dumps(message)


def connect_to_huaweicloud():
    """连接到华为云IoT平台"""
    print("开始连接华为云IoT平台...")
    
    # 创建MQTT客户端
    client = mqtt.Client(client_id=CONFIG["clientId"])
    
    # 设置回调函数
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_publish = on_publish
    client.on_message = on_message
    
    # 设置用户名和密码
    client.username_pw_set(CONFIG["username"], CONFIG["password"])
    
    # 配置TLS连接（因为使用MQTTS）
    client.tls_set(
        ca_certs=None,  # 使用系统默认CA证书
        certfile=None,  # 客户端证书（如需要双向认证）
        keyfile=None,   # 客户端密钥（如需要双向认证）
        cert_reqs=ssl.CERT_REQUIRED,
        tls_version=ssl.PROTOCOL_TLS,
        ciphers=None
    )
    
    # 连接到服务器
    try:
        client.connect(CONFIG["hostname"], CONFIG["port"], keepalive=60)
        return client
    except Exception as e:
        print(f"❌ 连接失败: {str(e)}")
        return None


def send_data(client, SoidTemp, SoidHum, SoildPH, SoildEle, IndoorTemp, Indoorhum, Light, WaterBox, Pump, Fan, curtain):
    """发送数据到华为云IoT平台"""
    if not client or not connected:
        print("❌ 客户端未连接，无法发送数据")
        return False
    
    # 构建消息
    message = build_message(SoidTemp, SoidHum, SoildPH, SoildEle, IndoorTemp, Indoorhum, Light, WaterBox, Pump, Fan, curtain)  
    
    # 发布消息
    result = client.publish(
        topic=PUB_TOPIC,
        payload=message,
        qos=1  # 使用QoS 1确保消息至少送达一次
    )
    
    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"📤 正在发送消息到主题: {PUB_TOPIC}")
        return True
    else:
        print(f"❌ 消息发布失败: {result.rc}")
        return False


def main():
    """主函数"""
    print("="*60)
    print("        华为云IoT平台MQTT客户端")
    print("="*60)
    
    # 连接到华为云
    client = connect_to_huaweicloud()
    if not client:
        print("程序退出...")
        return
    
    # 启动客户端循环
    client.loop_start()
    
    # 等待连接成功
    max_wait_time = 5  # 最多等待5秒
    start_time = time.time()
    while not connected and (time.time() - start_time) < max_wait_time:
        time.sleep(1)
    
    if connected:
        # 发送测试数据
        print("开始发送测试数据...")
        
        # 发送数据，使用全局定义的所有参数值
        send_data(client, SoidTemp, SoidHum, SoildPH, SoildEle, IndoorTemp, Indoorhum, Light, WaterBox, Pump, Fan, curtain)
        
        # 重置消息发送标志
        global message_sent
        message_sent = False
        
        # 等待消息发送完成
        wait_time = 3
        while wait_time > 0 and not message_sent:
            time.sleep(1)
            wait_time -= 1
        
    # 延迟关闭，确保消息处理完成
    time.sleep(2)
    
    # 关闭连接
    print("🔒 正在断开连接...")
    client.loop_stop()
    client.disconnect()
    
    print("✅ 程序执行完毕!")

if __name__ == "__main__":
    main()