import paho.mqtt.client as mqtt
import json
import time
import random
import threading

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
intervaltime = 5 # 5秒获取一次设备影子数据
# 存储设备影子数据的字典
device_shadow_data = {}
# [{'id': 1, 'temp': 105, 'hum': 60.0, 'light': 50, 'weather': True, 'soil_hum': 400, 'pump': True, 'fan': False}, 
# {'id': 2, 'temp': 205, 'hum': 9230.0, 'light': 50, 'weather': True, 'soil_hum': 400, 'pump': True, 'fan': False}]
device_data = [{'id': 1, 'temp': 0, 'hum': 0.0, 'light': 0, 'weather': 0, 'soil_hum': 0, 'pump': 0, 'fan': 0, 'auto_mode': 1}, 
{'id': 2, 'temp': 0, 'hum': 0.0, 'light': 0, 'weather': 0, 'soil_hum': 0, 'pump': 0, 'fan': 292, 'auto_mode': 1}]  # 初始化两个设备的数据字典
# 回调函数：连接成功后订阅响应Topic
def on_connect(client, userdata, flags, rc):
    device_config = userdata
    device_id = device_config["device_id"]
    # 订阅“设备影子响应”Topic
    request_id = f"req_{int(time.time() * 1000)}_{random.randint(1000, 9999)}"  # 生成唯一请求ID
    subscribe_topic = f"$oc/devices/{device_id}/sys/shadow/get/response/request_id={request_id}"
    client.subscribe(subscribe_topic, qos=1)
    # 订阅“设备影子响应”Topic
    publish_topic = f"$oc/devices/{device_id}/sys/shadow/get/request_id={request_id}"
    payload = json.dumps({
        "object_device_id": device_id,
        "request_id": request_id
    })
    client.publish(publish_topic, payload, qos=1)

# 回调函数：收到影子数据时解析并存储
def on_message(client, userdata, msg):
    device_config = userdata
    device_name = device_config["device_name"]
    payload = json.loads(msg.payload.decode())
    # 收到设备影子数据，减少打印输出
    pass
    
    # 提取核心影子数据（属性、配置等）
    if "shadow" in payload:
        shadow_data = payload["shadow"]
        # 存储到全局字典（可选，用于后续业务逻辑）
        device_shadow_data[device_name] = shadow_data
        
        # 查找service_id为'DataGet'的shadow对象
        data_get_shadow = None
        for shadow in shadow_data:
            if shadow.get("service_id") == "DataGet" and "reported" in shadow and "properties" in shadow["reported"]:
                data_get_shadow = shadow
                break
        
        # 打印设备属性数据
        if data_get_shadow:
            props = data_get_shadow["reported"]["properties"]
            # 根据设备名称确定对应的device_data索引
            if device_name == "device_1":
                device_index = 0
            elif device_name == "device_2":
                device_index = 1
            else:
                print(f"⚠️  未知设备名称: {device_name}")
                return
            
            # 更新设备数据字典
            device_data[device_index]['temp'] = props.get('Temp', 0)
            device_data[device_index]['hum'] = props.get('Hum', 0.0)
            device_data[device_index]['light'] = props.get('Light', 0)
            device_data[device_index]['weather'] = props.get('Weather', 0)
            device_data[device_index]['soil_hum'] = props.get('Soil_hum', 0)
            device_data[device_index]['pump'] = props.get('Pump_Status', 0)
            device_data[device_index]['fan'] = props.get('Fan_Status', 0)
            device_data[device_index]['auto_mode'] = props.get('Auto_Mode', 0)
            print(f"\n✅ [{device_name}] 设备ID已更新:{device_data[device_index]['id']}")
           
            # 数据已更新，减少打印输出
# 定期获取设备影子数据的函数
def periodically_get_shadow(client, config, interval=intervaltime):
    """定期发送获取设备影子的请求"""
    while True:
        try:
            # 生成唯一请求ID
            request_id = f"req_{int(time.time() * 1000)}_{random.randint(1000, 9999)}"
            # 订阅响应Topic
            subscribe_topic = f"$oc/devices/{config['device_id']}/sys/shadow/get/response/request_id={request_id}"
            client.subscribe(subscribe_topic, qos=1)
            
            # 发布获取影子请求
            publish_topic = f"$oc/devices/{config['device_id']}/sys/shadow/get/request_id={request_id}"
            payload = json.dumps({
                "object_device_id": config["device_id"],
                "request_id": request_id
            })
            client.publish(publish_topic, payload, qos=1)
            # 定期获取影子数据，减少打印输出
            
            # 等待指定的时间间隔
            time.sleep(interval)
        except Exception as e:
            # 保留错误信息以便调试
            # print(f"获取数据失败：{str(e)}")
            time.sleep(interval)  # 即使出错也等待一段时间再重试

def main():
    # 为每个设备创建并启动MQTT客户端
    clients = []
    threads = []
    
    for config in DEVICE_CONFIGS:
        client = mqtt.Client(client_id=config["clientId"])
        client.username_pw_set(config["username"], config["password"])
        client.tls_set()  # 启用TLS（MQTTS）
        client.on_connect = on_connect
        client.on_message = on_message
        client.user_data_set(config)  # 传递设备配置到回调
        
        try:
            client.connect(config["hostname"], config["port"], keepalive=60)
            client.loop_start()  # 后台线程运行MQTT循环
            clients.append(client)
            # print(f"✅ [{config['device_name']}] MQTT客户端初始化成功")
            
            # 创建并启动定期获取数据的线程（每intervaltime秒获取一次）
            thread = threading.Thread(target=periodically_get_shadow, args=(client, config, intervaltime))
            thread.daemon = True  # 设置为守护线程，主程序结束时自动终止
            thread.start()
            threads.append(thread)
            # print(f"⏱️  [{config['device_name']}] 定期获取数据线程已启动（每{intervaltime}秒）")
            
        except Exception as e:
                # 保留关键错误信息
                print(f"连接失败：{str(e)}")
    
    print("\n 程序已启动，将持续获取设备数据并实时更新")
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n正在关闭程序...")
        for client in clients:
            client.loop_stop()
            client.disconnect()
        print("✅ 程序已终止，所有连接已断开")

# 程序入口
if __name__ == "__main__":
    main()