#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wifi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


//Config
const char* ssid = "Iphone 2";
const char* password = "12345789";
const char* coreiot_server = "app.coreiot.io";
const char* access_token = "longthangtran456";

//WiFi & MQTT
WiFiClient espClient;
PubSubClient client(espClient);
//Khai báo chân và loại cảm biến DHT
#define DHTTYPE DHT22 // Loại cảm biến là DHT22
#define DHTPIN 26    // Chân DATA của DHT22 nối với GPIO 26
DHT dht(DHTPIN, DHTTYPE); // Khởi tạo đối tượng DHT
//LED
#define LED_PIN 2  // Chân GPIO2 để điều khiển LED
//Lưu dữ liệu cảm biến
float temperature = 0.0;
float humidity = 0.0;

void connectWiFi(){
  Serial.println("Starting WiFi connection...");

  WiFi.begin(ssid, password);
  int attempt = 0;
  while(WiFi.status() != WL_CONNECTED && attempt < 20){  // Thử kết nối 20 lần (~10 giây)
      delay(500);
      Serial.print(".");
      attempt++;
  }
  if(WiFi.status() == WL_CONNECTED){
      Serial.println("\nWiFi connected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
  }else{
      Serial.println("\nWiFi Connection Failed!");
      ESP.restart();
  }
}

void reconnectMQTT(){
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32_Client", access_token, NULL)) {
        Serial.println("Connected to MQTT Broker");
        client.subscribe("v1/devices/me/rpc/request/+");
    } else {
        Serial.print("Failed, rc=");
        Serial.print(client.state());
        Serial.println(" Retrying in 5s...");
        delay(5000);
    }
  }
}


// Xử lý khi nhận lệnh từ MQTT
void RPCHandler(char* topic, byte* payload, unsigned int length){
  payload[length] = '\0';
  String message = String((char*)payload);
  Serial.print("Received RPC Request: ");
  Serial.println(message);

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message.c_str());

  if(!error){
    String method = doc["method"].as<String>();
    if(method == "setValueLED"){
      bool ledState = doc["params"];
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      Serial.print("LED state: ");
      Serial.println(ledState ? "ON" : "OFF");
    }
    String topicStr = String(topic);
    int lastSlash = topicStr.lastIndexOf('/');
    String requestId = topicStr.substring(lastSlash + 1);
    String responseTopic = "v1/devices/me/rpc/response/" + requestId;

    String responsePayload = "{\"status\":\"success\"}";
    client.publish(responseTopic.c_str(), responsePayload.c_str());
    Serial.println("Sent response to CoreIOT: " + responsePayload);
  }else{
    Serial.print("JSON Parse Error: ");
    Serial.println(error.c_str());
  }
}

//Hàm xử lý nhiệt độ, độ ẩm
void TaskTemperature_Humidity(void *pvParameters) {
  while (1) {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

      if (!isnan(temperature) && !isnan(humidity)) {
        Serial.print("Temp: "); Serial.print(temperature); Serial.print(" *C");
        Serial.print(" Humidity: "); Serial.print(humidity); Serial.print(" %");

        String payload = "{\"temperature\":" + String(temperature) + ", \"humidity\":" + String(humidity) + "}";
        Serial.println("Publishing: " + payload);
        client.publish("v1/devices/me/telemetry", payload.c_str());
      } else {
          Serial.println("DHT Sensor Error!");
      }
      vTaskDelay(10000 / portTICK_PERIOD_MS);  // Gửi mỗi 10 giây
  }
}

void TaskMQTT(void *pvParameters) {
  while (1) {
      if(!client.connected()){
        reconnectMQTT();
      }
      client.loop();
      vTaskDelay(500 / portTICK_PERIOD_MS);  // Kiểm tra MQTT mỗi 500ms
  }
}

void setup(){
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  connectWiFi(); 
  client.setServer(coreiot_server, 1883); //Trong thư viện PubSubClient
  client.setCallback(RPCHandler); 

  dht.begin();

  xTaskCreate(TaskMQTT, "MQTTHandler", 2048, NULL, 1, NULL);
  xTaskCreate(TaskTemperature_Humidity, "Temperature & Humidity", 2048, NULL, 2, NULL);
}
void loop() {
}

