#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wifi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>
//Wifi
const char* ssid = "Phong 2";
const char* password = "0123456789@#";
const char* mqtt_server = "192.168.90.53";
const int mqtt_port = 1883;
const char* mqtt_topic_pub = "home/sensors/dht";
const char* mqtt_topic_sub = "home/actuators/led";


void performOTA(String firmware_url);
void checkForUpdates(String fw_version, String fw_url);

#define DHTTYPE DHT22 // Loại cảm biến là DHT22
#define DHTPIN 26    // Chân DATA của DHT11 nối với GPIO 26
DHT dht(DHTPIN, DHTTYPE); // Khởi tạo đối tượng DHT


// LED
//#define LED_PIN 2  // Chân GPIO2 để điều khiển LED

//WiFi & MQTT
WiFiClient espClient;
PubSubClient client(espClient);

#define CURRENT_FW_VERSION "1.0.0"

TaskHandle_t taskTeHu;
TaskHandle_t taskMQTT;
//Lưu dữ liệu cảm biến
float temperature = 0.0;
float humidity = 0.0;

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}
void publishFirmwareVersion(){
  String payload = "{\"fw_version\": \"" + String(CURRENT_FW_VERSION) + "\"}";
  client.publish("home/device/fw_version", payload.c_str());
  Serial.println("Published firmware version: " + String(CURRENT_FW_VERSION));
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32_Client")) {
      Serial.println("Connected!");
      publishFirmwareVersion();
      client.subscribe("home/actuators/led");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5s...");
      delay(5000);
    }
  }
}


// Xử lý khi nhận lệnh từ MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Received MQTT message: ");
  Serial.println(message);

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, message.c_str());

  if(!error){
    if(doc.containsKey("fw_version") && doc.containsKey("fw_url")){
      String newVersion = doc["fw_version"].as<String>();
      String newUrl = doc["fw_url"].as<String>();
      checkForUpdates(newVersion, newUrl);
    }
  }else{ 
    Serial.print("JSON Parse Error: ");
    Serial.println(error.c_str());
  }
  //Đây là phần mở rộng cho phiên bản 1.0.1, thêm tính năng bật/tắt LED
  // if (!error) { if(doc.containsKey("led")){
  //   bool ledState = doc["led"];
  //   digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  //   Serial.print("LED State: ");
  //   Serial.println(ledState ? "ON" : "OFF");
}
// Hàm kiểm tra và thực hiện OTA
void checkForUpdates(String fw_version, String fw_url){
  Serial.println("Checking for updates...");
  if(fw_version != CURRENT_FW_VERSION){ 
      Serial.println("New firmware available! Starting OTA...");
      performOTA(fw_url);
  }else{
      Serial.println("No update needed.");
  }
}
void testInternetConnection() {
  HTTPClient http;
  http.begin("http://www.google.com");
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.println("Internet is available!");
  } else {
    Serial.println("Internet connection failed!");
  }
  http.end();
}
// Hàm tải firmware từ URL và cập nhật OTA
void performOTA(String firmware_url) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  Serial.print("Free heap before OTA: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("Downloading firmware from: ");
  Serial.println(firmware_url);
  http.begin(client, firmware_url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  Serial.print("HTTP Response Code: ");
  Serial.println(httpCode);
  if(httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    return;
  }
  int totalSize = http.getSize();
  Serial.print("Firmware size: ");
  Serial.println(totalSize);
  if (totalSize <= 0) {
    Serial.println("Invalid firmware file!");
    return;
  }
  WiFiClient *stream = http.getStreamPtr();

  if(!Update.begin(totalSize)){
    Serial.println("Not enough space for OTA.");
    http.end();
    return;
  }
  size_t written = Update.writeStream(*stream);
  if(written == totalSize && Update.end()){
    Serial.println("Update successful, restarting...");
    ESP.restart();
  }else{
    Serial.println("Update failed.");
  }
  http.end();
}

//Hàm xử lý nhiệt độ, độ ẩm
void TaskTemperature_Humidity(void* pvParameters){
  while(1){
    /*
    int status = dht20.read(); //Đọc dữ liệu từ DHT20
    
    if(status == DHT20_OK){
      temperature = dht20.getTemperature();
      humidity = dht20.getHumidity();
      Serial.print("Temp: "); Serial.print(temperature); Serial.print(" *C");
      Serial.print(" Humidity: "); Serial.print(humidity); Serial.print(" %");
      // Gửi dữ liệu lên MQTT Broker
      String payload = "{\"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + "}";
      Serial.println("Publishing: " + payload);
      client.publish(mqtt_topic_pub, payload.c_str());
    }else{
      Serial.print("DHT20 Read Error: ");
      Serial.println(status);
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
  */
     //Phần này của DHT22
    UBaseType_t stack = uxTaskGetStackHighWaterMark(NULL);
    Serial.print("Free stack in TaskTemperature_Humidity: ");
    Serial.println(stack);
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if(!isnan(temperature) && !isnan(humidity)){
      Serial.print("Temp: "); Serial.print(temperature); Serial.print(" *C");
      Serial.print(" Humidity: "); Serial.print(humidity); Serial.print(" %");

      // Gửi dữ liệu lên MQTT Broker
      if (client.connected()) {
        String payload = "{\"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + "}";
        Serial.println("Publishing: " + payload);
        client.publish(mqtt_topic_pub, payload.c_str());
      }else{
        Serial.println("MQTT disconnected, skipping publish...");
      }
    }else{
      Serial.println("Failed to read from DHT sensor!");
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

//Hàm nhận dữ liệu từ MQTT
void MQTTSubscribeTask(void *pvParameters) {
  while (1) {
    if (!client.connected()) {
      reconnectMQTT();
    }
    client.loop();

    vTaskDelay(1000 / portTICK_PERIOD_MS);  
  }
}

void setup(){
  Serial.begin(115200);
  //pinMode(LED_PIN, OUTPUT);
  connectWiFi();
  testInternetConnection();
  client.setServer(mqtt_server, mqtt_port); //Trong thư viện PubSubClient
  client.setCallback(callback); 

  //Wire.begin(21,22); //SDA 21, SCL 22
  /*
  if(!dht20.begin()){
    Serial.println("Failed to find DHT20! Check wiring.");
    while(1) delay(10);
  }else{
    Serial.println("DHT20 sensor initialized!");
  }
  */
 
  dht.begin();
  xTaskCreatePinnedToCore(TaskTemperature_Humidity, "Temperature & Humidity", 8192, NULL, 2, &taskTeHu, 1);
  xTaskCreatePinnedToCore(MQTTSubscribeTask, "MQTT Subcribe Task", 8192, NULL, 2, &taskMQTT, 0);
}

void loop(){
}
