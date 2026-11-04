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

# 设备信息配置 - 使用用户提供的最新连接属性
DEVICE_CONFIGS = [
    {
        "device_name": "device_2",
        "device_id": "68ee03050a9ab42ae5847f9e_test",
        "username": "68ee03050a9ab42ae5847f9e_test",
        "password": "REDACTED_CONFIGURE_LOCALLY",
        "clientId": "68ee03050a9ab42ae5847f9e_test_0_0_2025102210",
        "hostname": "6d5aef22d8.st1.iotda-device.cn-east-3.myhuaweicloud.com",
        "port": 8883,
        "protocol": "MQTTS"
    },
    {
        "device_name": "device_1",
        "device_id": "68ee03050a9ab42ae5847f9e_text2",
        "username": "68ee03050a9ab42ae5847f9e_text2",
        "password": "REDACTED_CONFIGURE_LOCALLY",
        "clientId": "68ee03050a9ab42ae5847f9e_text2_0_0_2025102307",
        "hostname": "6d5aef22d8.st1.iotda-device.cn-east-3.myhuaweicloud.com",
        "port": 8883,
        "protocol": "MQTTS"
    }
]

# 默认使用第一个设备的配置
CONFIG = DEVICE_CONFIGS[0]

# 华为云IoT平台的发布主题（固定格式）
def get_pub_topic(client_id):
    return "$oc/devices/{}/sys/properties/report".format(client_id)

# 全局变量
connected = {}
message_sent = {}
service_id = "DataGet"
# 默认的设备数据
nodeId = "ON"  # 设备ID
Temp = 22.0  # 温度值 (°C)
Hum = 60.0  # 湿度值 (%)
Light = 20.0  # 光照强度 (Lux)
Weather = False  # 天气状况 (True表示晴，False表示雨)
Soil_hum = 60.0  # 土壤湿度 (%)
Pump_Status = True  # 水泵状态
Fan_Status = True  # 风扇状态

# 回调函数定义
def on_connect(client, userdata, flags, rc):
    """连接成功回调函数"""
    global connected
    client_id = userdata.get('clientId', 'unknown')
    device_name = userdata.get('device_name', 'unknown')
    connected[client_id] = True
    if rc == 0:
        print(f"✅ [{device_name}] 成功连接到华为云IoT平台!")
    else:
        print(f"❌ [{device_name}] 连接失败，错误代码: {rc}")


def on_disconnect(client, userdata, rc):
    """断开连接回调函数"""
    global connected
    client_id = userdata.get('clientId', 'unknown')
    device_name = userdata.get('device_name', 'unknown')
    connected[client_id] = False
    print(f"⚠️  [{device_name}] 与华为云IoT平台断开连接，错误代码: {rc}")
    if rc != 0:
        print(f"[{device_name}] 意外断开连接，尝试重新连接...")
        client.reconnect()


def on_publish(client, userdata, mid):
    """消息发布成功回调函数"""
    global message_sent
    client_id = userdata.get('clientId', 'unknown')
    device_name = userdata.get('device_name', 'unknown')
    message_sent[client_id] = True
    print(f"✅ [{device_name}] 消息发布成功，消息ID: {mid}")


def on_message(client, userdata, msg):
    """接收消息回调函数"""
    device_name = userdata.get('device_name', 'unknown')
    print(f"📩 [{device_name}] 收到消息: 主题={msg.topic}, 内容={msg.payload.decode()}")


def build_message(nodeId, Temp, Hum, Light, Weather, Soil_hum, Pump_Status, Fan_Status):
    """构建符合华为云IoT平台要求的JSON消息格式
    
    Args:
        nodeId: 设备ID
        Temp: 温度值 (0-100, °C)
        Hum: 湿度值 (0-100, %)
        Light: 光照强度值 (0-100, Lux)
        Weather: 天气状况 (boolean, 晴或雨)
        Soil_hum: 土壤湿度值 (0-100, %)
        Pump_Status: 水泵状态 (boolean)
        Fan_Status: 风扇状态 (boolean)
        
    Returns:
        str: 符合华为云IoT平台格式的JSON字符串
    """
    # 获取当前时间，格式化为ISO 8601格式
    now = datetime.now().strftime("%Y%m%dT%H%M%SZ")
    
    # 构建华为云要求的消息格式
    message = {
        "services": [
            {
                "service_id": service_id,
                "properties": {
                    "nodeId": nodeId,  # 设备ID (string)
                    "Temp": Temp,  # 温度值 (decimal, 0-100, °C)
                    "Hum": Hum,  # 湿度值 (decimal, 0-100, %)
                    "Light": Light,  # 光照强度 (decimal, 0-100, Lux)
                    "Soil_hum": Soil_hum,  # 土壤湿度 (decimal, 0-100, %)
                    "Weather": Weather,  # 天气状况 (boolean, 晴或雨, RW)
                    "Pump_Status": Pump_Status,  # 水泵状态 (boolean, RW)
                    "Fan_Status": Fan_Status  # 风扇状态 (boolean, RW)
                },
                "event_time": now  # 当前时间
            }
        ]
    }
    return json.dumps(message)


def connect_to_huaweicloud(device_config=None):
    """连接到华为云IoT平台
    
    Args:
        device_config: 设备配置信息，如果为None则使用默认配置
        
    Returns:
        mqtt.Client: MQTT客户端实例
    """
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
    client.on_message = on_message
    
    # 设置用户名和密码
    client.username_pw_set(device_config["username"], device_config["password"])
    
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
        client.connect(device_config["hostname"], device_config["port"], keepalive=60)
        return client
    except Exception as e:
        print(f"❌ [{device_name}] 连接失败: {str(e)}")
        return None


def send_data(client, nodeId, Temp, Hum, Light, Weather, Soil_hum, Pump_Status, Fan_Status):
    """发送数据到华为云IoT平台
    
    Args:
        client: MQTT客户端实例
        nodeId: 设备ID
        Temp: 温度值
        Hum: 湿度值
        Light: 光照强度
        Weather: 天气状况
        Soil_hum: 土壤湿度
        Pump_Status: 水泵状态
        Fan_Status: 风扇状态
        
    Returns:
        bool: 发送是否成功
    """
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
        print(f"📤 [{device_name}] 正在发送消息到主题: {pub_topic}")
        return True
    else:
        print(f"❌ [{device_name}] 消息发布失败: {result.rc}")
        return False


def main():
    """主函数"""
    print("="*60)
    print("        华为云IoT平台MQTT客户端")
    print("="*60)
    
    # 存储所有设备的客户端实例
    clients = {}
    
    # 为每个设备创建并连接客户端
    for device_config in DEVICE_CONFIGS:
        device_name = device_config['device_name']
        client = connect_to_huaweicloud(device_config)
        if client:
            clients[device_name] = client
            client.loop_start()
            # 初始化连接状态
            connected[device_config['clientId']] = False
    
    if not clients:
        print("所有设备连接失败，程序退出...")
        return
    
    # 等待所有设备连接成功
    max_wait_time = 5  # 最多等待5秒
    start_time = time.time()
    all_connected = False
    
    while not all_connected and (time.time() - start_time) < max_wait_time:
        all_connected = True
        for device_config in DEVICE_CONFIGS:
            client_id = device_config['clientId']
            if client_id not in connected or not connected[client_id]:
                all_connected = False
                break
        if not all_connected:
            time.sleep(0.5)
    
    print(f"连接状态检查完成，成功连接的设备数: {sum(1 for c in connected.values() if c)}")
    
    try:
        while any(connected.values()):
            # 获取当前时间戳
            current_time = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
            
            # 为每个连接的设备发送数据
            for device_name, client in clients.items():
                device_config = client._userdata
                client_id = device_config['clientId']
                
                if connected.get(client_id, False):
                    # 打印带时间戳的数据
                    print(f"[{current_time}] [{device_name}] 发送数据: nodeId={nodeId}, Temp={Temp}, Hum={Hum}, Light={Light}, Weather={Weather}, Soil_hum={Soil_hum}, Pump_Status={Pump_Status}, Fan_Status={Fan_Status}")
                    
                    # 发送数据
                    send_data(client, nodeId, Temp, Hum, Light, Weather, Soil_hum, Pump_Status, Fan_Status)
                    
                    # 重置该设备的消息发送标志
                    message_sent[client_id] = False
            
            # 等待消息发送完成（为每个设备等待）
            wait_time = 3
            for _ in range(wait_time):
                # 检查所有连接的设备是否都已发送消息
                all_messages_sent = True
                for client_id in connected:
                    if connected[client_id] and message_sent.get(client_id, False) is False:
                        all_messages_sent = False
                        break
                
                if all_messages_sent:
                    break
                time.sleep(1)
            
            # 间隔一段时间再发送下一组数据（例如5秒）
            time.sleep(5)
    
    except KeyboardInterrupt:
        print("\n用户中断程序...")
    
    # 延迟关闭，确保消息处理完成
    time.sleep(2)
    
    # 关闭所有连接
    print("🔒 正在断开所有设备连接...")
    for device_name, client in clients.items():
        print(f"[{device_name}] 断开连接...")
        client.loop_stop()
        client.disconnect()
    
    print("✅ 程序执行完毕!")

if __name__ == "__main__":
    main()