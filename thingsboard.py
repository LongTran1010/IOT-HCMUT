print("Hello Core IOT")
import paho.mqtt.client as mqttclient
import time
import json

# Cấu hình MQTT Local (Giao tiếp với ESP32)
LOCAL_MQTT_HOST = "192.168.90.53"
LOCAL_MQTT_PORT = 1883
LOCAL_MQTT_TOPIC_SUB = "home/sensors/dht"
LOCAL_MQTT_TOPIC_PUB = "home/actuators/led"

#MQTT Core IOT
BROKER_ADDRESS = "app.coreiot.io"
ACCESS_TOKEN = "Longteo@123"
ACCESS_USERNAME = "longthangtran456"
COREIOT_SUB_TOPIC = "v1/devices/me/rpc/request/+" #topic nhận lệnh từ core iot

# MQTT Client để giao tiếp với ESP32
esp_client = mqttclient.Client()
esp_client.connect(LOCAL_MQTT_HOST, LOCAL_MQTT_PORT, 60)

# Kết nối MQTT với core iot
tb_client = mqttclient.Client("IOT_DEVICE_2")
tb_client.username_pw_set(ACCESS_USERNAME, ACCESS_TOKEN)
def connected(client, userdata, flags, rc):
    if rc == 0:
        print("Connected successfully!!")
        client.subscribe(COREIOT_SUB_TOPIC)
    else:
        print("Connection failed, rc =", rc)

def subscribed(client, userdata, mid, granted_qos):
    print("Subscribed...")

# Hàm xử lý khi nhận lệnh từ CoreIOT
def on_tb_message(client, userdata, message):
    try:
        data = json.loads(message.payload.decode("utf-8"))
        print(f"Received command from ThingsBoard: {data}")

        if 'method' in data and data['method'] == "setValueLED":
            led_state = data['params']
            esp_payload = json.dumps({"led": led_state})

            # Gửi lệnh LED xuống ESP32
            esp_client.publish(LOCAL_MQTT_TOPIC_PUB, esp_payload)
            print(f"Sent to ESP32: {esp_payload}")

    except json.JSONDecodeError:
        print("Error: Received invalid JSON from Core IOT")
    except Exception as e:
        print("Error processing message:", e)

#Hàm xử lý khi nhận dữ liệu từ ESP32
def recv_message(client, userdata, message):
    try:
        json_str = message.payload.decode("utf-8")
        data = json.loads(json_str)
        print(f"Received from ESP32: {data}")
        if "temperature" in data and "humidity" in data:
            telemetry = {
                "temperature": data["temperature"],
                "humidity": data["humidity"]
            }
            tb_client.publish("v1/devices/me/telemetry", json.dumps(telemetry))
            print(f"Sent to CoreIOT: {telemetry}")
        else:
            print("Invalid data received from ESP32:", data)
    except json.JSONDecodeError:
        print("Error: Received invalid JSON from ESP32")
    except Exception as e:
        print("Error processing message:", e)
    """
    print("Received from ESP32: ", message.payload.decode("utf-8"))
    temp_data = {'value': True}
    try:
        jsonobj = json.loads(message.payload)
        if jsonobj['method'] == "setValue":
            temp_data['value'] = jsonobj['params']
            tb_client.publish('v1/devices/me/attributes', json.dumps(temp_data), 1)
    except:
        pass
    """
esp_client.subscribe(LOCAL_MQTT_TOPIC_SUB)
esp_client.on_message = recv_message

tb_client.on_connect = connected
tb_client.on_subscribe = subscribed
tb_client.on_message = on_tb_message
tb_client.connect(BROKER_ADDRESS, 1883, 60)
while True:
    try:
        if not tb_client.is_connected():
            print("Reconnecting to CoreIOT...")
            tb_client.connect(BROKER_ADDRESS, 1883, 60)

        tb_client.loop()
        esp_client.loop()
        time.sleep(1)

    except Exception as e:
        print("Error in main loop:", e)
        time.sleep(5)