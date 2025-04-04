#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi & MQTT Config
const char* ssid = "Phong 2";
const char* password = "0123456789@#";
const char* mqtt_server = "app.coreiot.io";
const char* access_token = "longthangtran4567"; //access token của device Schedule

WiFiClient espClient;
PubSubClient client(espClient);

// Cảm biến độ ẩm đất
//#define SOIL_MOISTURE_PIN 32 // Đọc giá trị độ ẩm đất
#define RELAY_SOIL_MOISTURE_PIN 19 // Điều khiển bơm
//const float soilthreshold_on = 45;
//const float soilthreshold_off = 55;
bool pumpState = false;
//bool manual_pumpcontrol = false;
//bool autoMode = false;

void connectWiFi() {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
}

void reconnectMQTT() {
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        if (client.connect("ESP32_Client", access_token, NULL)) {
            Serial.println("Connected to MQTT Broker");
            client.subscribe("v1/devices/me/rpc/request/+");
        } else {
            Serial.println("Failed, retrying...");
            delay(5000);
        }
    }
}

void RPCHandler(char* topic, byte* payload, unsigned int length) {
    String message = String((char*)payload);
    Serial.println("Received RPC: " + message);
    
    StaticJsonDocument<200> doc;
    if (deserializeJson(doc, message.c_str())) {
        Serial.println("JSON Error");
        return;
    }
    
    String method = doc["method"].as<String>();
    if (method == "setValuePUMP1") {
        pumpState = doc["params"]["params"];
        digitalWrite(RELAY_SOIL_MOISTURE_PIN, pumpState);
        Serial.println("Pump: " + String(pumpState));
    } else if (method == "setValuePUMP2") {
        pumpState = doc["params"]["params"];
        digitalWrite(RELAY_SOIL_MOISTURE_PIN, pumpState);
        Serial.println("Pump: " + String(pumpState));
    }
    // } else if (method == "autoPUMP") {
    //     autoMode = doc["params"];
    //     manual_pumpcontrol = false;
    //     Serial.println(autoMode ? "Auto Mode ENABLED" : "Auto Mode DISABLED");
    // }
    String responseTopic = String("v1/devices/me/rpc/response/") + topic[strlen(topic) - 1];
    StaticJsonDocument<100> responseDoc;
    responseDoc["status"] = "OK";
    responseDoc["pumpState"] = pumpState;
    char responseBuffer[100];
    serializeJson(responseDoc, responseBuffer);
    client.publish(responseTopic.c_str(), responseBuffer);
}

// void TaskSoilMoisture(void *pvParameters) {
//     static unsigned long lastpumpaction = 0;
//     while (1) {
//         int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
//         float soilMoisture_percent = map(soilMoisture, 0, 4095, 100, 0);
//         Serial.print("Soil Moisture: ");
//         Serial.print(soilMoisture_percent);
//         Serial.println(" %");
        
//         if (autoMode && !manual_pumpcontrol && millis() - lastpumpaction > 5000) {
//             if (!pumpState && soilMoisture_percent < soilthreshold_on) {
//                 digitalWrite(RELAY_SOIL_MOISTURE_PIN, HIGH);
//                 Serial.println("Pump ON (Auto Mode)");
//                 pumpState = true;
//                 lastpumpaction = millis();
//             } else if (pumpState && soilMoisture_percent > soilthreshold_off) {
//                 digitalWrite(RELAY_SOIL_MOISTURE_PIN, LOW);
//                 Serial.println("Pump OFF (Auto Mode)");
//                 pumpState = false;
//                 lastpumpaction = millis();
//             }
//         }
        
//         String payload = "{\"soil_moisture\":" + String(soilMoisture_percent) + "}";
//         client.publish("v1/devices/me/telemetry", payload.c_str());
//         vTaskDelay(10000 / portTICK_PERIOD_MS);
//     }
// }

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_SOIL_MOISTURE_PIN, OUTPUT);
    digitalWrite(RELAY_SOIL_MOISTURE_PIN, LOW);
    pumpState = false;
    connectWiFi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(RPCHandler);
    
    //xTaskCreate(TaskSoilMoisture, "SoilMoisture", 2048, NULL, 2, NULL);
}

void loop() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();
}