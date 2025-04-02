print("Hello Core IOT")
import paho.mqtt.client as mqttclient
import time
import json
import requests

# Cấu hình MQTT Local (Giao tiếp với ESP32)
LOCAL_MQTT_HOST = "192.168.90.53"
LOCAL_MQTT_PORT = 1883
LOCAL_MQTT_TOPIC_SUB = "home/sensors/dht"
LOCAL_MQTT_TOPIC_PUB = "home/actuators/led"
LOCAL_FW_VERSION_TOPIC = "home/device/fw_version"

#MQTT Core IOT
BROKER_ADDRESS = "app.coreiot.io"
ACCESS_TOKEN = "Longteo@123"
ACCESS_USERNAME = "longthangtran456"
COREIOT_SUB_TOPIC = "v1/devices/me/rpc/request/+" #topic nhận lệnh từ core iot
current_fw_version = "1.0.0"
FIRMWARE_JSON_URL = "https://drive.google.com/uc?export=download&id=1El0QW66C0PpML7bTCTsdEnb6GwaV_g_R"
CHECK_INTERVAL = 600
last_check_time = 0
def get_latest_firmware():
    try:
        response = requests.get(FIRMWARE_JSON_URL, timeout=10)
        response.raise_for_status()

        data = response.json()
        return data.get("fw_version"), data.get("fw_url")

    except requests.RequestException as e:
        print(f"Error fetching firmware JSON: {e}")
        return None, None
    except json.JSONDecodeError:
        print("Error decoding JSON response")
        return None, None

def send_ota_update():
    latest_fw_version, latest_fw_url = get_latest_firmware()
    if not latest_fw_version or not latest_fw_url:
        return

    if latest_fw_version and latest_fw_url and current_fw_version != latest_fw_version:
        ota_payload = json.dumps({"fw_version": latest_fw_version, "fw_url": latest_fw_url})
        print(f"Sending OTA update: {ota_payload}")
        esp_client.publish(LOCAL_MQTT_TOPIC_PUB, ota_payload)

def on_fw_version_message(client, userdata, message):
    global current_fw_version
    try:
        data = json.loads(message.payload.decode("utf-8"))
        if "fw_version" in data:
            current_fw_version = data["fw_version"]
            print(f"ESP32 firmware version: {current_fw_version}")
            send_ota_update()
    except json.JSONDecodeError:
        print("Error: Received invalid JSON from ESP32")

def connected(client, userdata, flags, rc):
    if rc == 0:
        print("Connected successfully!!")
        tb_client.subscribe(COREIOT_SUB_TOPIC)
    else:
        print("Connection failed, rc =", rc)

def subscribed(client, userdata, mid, granted_qos):
    print("Subscribed...")

# Hàm xử lý khi nhận lệnh từ CoreIOT
def on_tb_message(client, userdata, message):
    try:
        data = json.loads(message.payload.decode("utf-8"))
        print(f"Received RPC from Core IOT: {data}")

        #request_id = message.topic.split('/')[-1] #Lấy request ID từ topic

        if 'method' in data and data['method'] == "setValueLED":
            led_state = data['params']
            esp_payload = json.dumps({"led": led_state})

            # Gửi lệnh LED xuống ESP32
            esp_client.publish(LOCAL_MQTT_TOPIC_PUB, esp_payload)
            print(f"Sent to ESP32: {esp_payload}")

            #Phản hồi lại CoreIOT
            #response_topic = "v1/devices/me/rpc/response/{request_id}"
            #response_payload = json.dumps({"status": "success", "led": led_state})
            #client.publish(response_topic, response_payload)
            #print(f"Sent response to CoreIOT: {response_payload}")

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

esp_client = mqttclient.Client()
esp_client.connect(LOCAL_MQTT_HOST, LOCAL_MQTT_PORT, 60)

esp_client.subscribe(LOCAL_MQTT_TOPIC_SUB)
esp_client.subscribe(LOCAL_FW_VERSION_TOPIC)
esp_client.on_message = recv_message
esp_client.message_callback_add(LOCAL_FW_VERSION_TOPIC, on_fw_version_message)

tb_client = mqttclient.Client("IOT_DEVICE_2")
tb_client.username_pw_set(ACCESS_USERNAME, ACCESS_TOKEN)
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
        if time.time() - last_check_time > CHECK_INTERVAL:
            print("Checking for firmware update...")
            send_ota_update()
            last_check_time = time.time()
        time.sleep(1)

    except Exception as e:
        print("Error in main loop:", e)
        time.sleep(5)