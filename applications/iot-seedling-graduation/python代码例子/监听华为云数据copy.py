import paho.mqtt.client as mqtt
import json
import time

# 你的设备配置（直接使用提供的DEVICE_CONFIGS）
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

# 回调函数：连接成功后订阅Topic
def on_connect(client, userdata, flags, rc):
    device_config = userdata
    device_name = device_config["device_name"]
    device_id = device_config["device_id"]
    # 订阅“设备属性上报响应”Topic
    topic = f"$oc/devices/{device_id}/sys/properties/report/response"
    client.subscribe(topic, qos=1)
    print(f" [{device_name}] 已订阅Topic: {topic}，等待接收数据...")

# 回调函数：收到消息时解析数据
def on_message(client, userdata, msg):
    device_config = userdata
    device_name = device_config["device_name"]
    payload = json.loads(msg.payload.decode())
    print(f" [{device_name}] 收到设备数据：")
    print(json.dumps(payload, indent=2, ensure_ascii=False))

# 为每个设备创建MQTT客户端并启动循环
clients = []
for config in DEVICE_CONFIGS:
    client = mqtt.Client(client_id=config["clientId"])
    client.username_pw_set(config["username"], config["password"])
    client.tls_set()  # 启用TLS（MQTTS）
    client.on_connect = on_connect
    client.on_message = on_message
    client.user_data_set(config)  # 传递设备配置到回调函数
    client.connect(config["hostname"], config["port"], keepalive=60)
    clients.append(client)
    client.loop_start()  # 启动客户端循环（后台线程）

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("程序终止，正在断开连接...")
    for client in clients:
        client.loop_stop()
        client.disconnect()