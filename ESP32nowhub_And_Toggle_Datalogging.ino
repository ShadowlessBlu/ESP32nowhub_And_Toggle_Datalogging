#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "LittleFS.h"

#define PH_UP_PIN 25
#define PH_DOWN_PIN 26
#define STIRRER_PIN 2
#define PUMP_PIN 14
#define LIGHT_PIN 27
#define LOGGING_LED 22
#define LOGGING_BUTTON 0

const char* ssid = "Galaxy A529DB7";
const char* password = "shadowless12";

// Variables
bool loggingEnabled = false;  // Start with logging disabled
const char* dataPath = "/data.txt";
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 300;
unsigned long lastLogTime = 0;
const unsigned long logInterval = 500; // Log every 5 seconds
unsigned long epochTime = 0;
String dataMessage;

// Sensor Data Structure
typedef struct {
    float temperature, humidity, rainfall, soilMoisture, lightIntensity, waterLevel, pH;
    bool stirrerState, pumpState;
    char nodeName[20];
} SensorData;

String logEntry;

SensorData receivedData;

void appendFile(fs::FS &fs, const char *path, const char *message) {
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    file.print(message);
    file.close();
}

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    memcpy(&receivedData, incomingData, sizeof(receivedData));

    Serial.printf("Received data from %s: Temp: %.2f°C, Humidity: %.2f%%, Soil Moisture: %d, Rain: %d\n",
                  receivedData.nodeName, receivedData.temperature, receivedData.humidity,
                  receivedData.soilMoisture, receivedData.rainfall);

    if (loggingEnabled) {
        String logEntry = String(loggingEnabled) + ", " + String(millis()) + ", " + String(receivedData.nodeName) + ", " +
                          String(receivedData.temperature) + ", " + String(receivedData.humidity) + ", " +
                          String(receivedData.soilMoisture) + ", " + String(receivedData.rainfall) + ", " +
                          String(receivedData.lightIntensity) + ", " + String(receivedData.waterLevel) + ", " +
                          String(receivedData.pH) + ", " + String(receivedData.pumpState) + ", " +
                          String(receivedData.stirrerState) + "\r\n";
        appendFile(SD, dataPath, logEntry.c_str());
        Serial.println("Logged: " + logEntry);
        HubLogging();
    }else if(!loggingEnabled) { 
      String logEntry = String(loggingEnabled) + ", " + String(millis()) + ", " + String(receivedData.nodeName) + ", " +
                          String(receivedData.temperature) + ", " + String(receivedData.humidity) + ", " +
                          String(receivedData.soilMoisture) + ", " + String(receivedData.rainfall) + ", " +
                          String(receivedData.lightIntensity) + ", " + String(receivedData.waterLevel) + ", " +
                          String(receivedData.pH) + ", " + String(receivedData.pumpState) + ", " +
                          String(receivedData.stirrerState) + "\r\n";

      Serial.println("Not Logged: " + logEntry);
    }

    processSensorData();
}

void processSensorData() {
    digitalWrite(PUMP_PIN, receivedData.soilMoisture < 300 ? HIGH : LOW);
    digitalWrite(LIGHT_PIN, receivedData.temperature > 30 ? HIGH : LOW);
}

void toggleLogging() {
    if (millis() - lastDebounceTime > debounceDelay) {
        loggingEnabled = !loggingEnabled;
        digitalWrite(LOGGING_LED, loggingEnabled ? HIGH : LOW);
        Serial.println(loggingEnabled ? "Logging ENABLED" : "Logging DISABLED");
        lastDebounceTime = millis();
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    esp_now_register_recv_cb(OnDataRecv);

    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(PH_UP_PIN, OUTPUT);
    pinMode(PH_DOWN_PIN, OUTPUT);
    pinMode(STIRRER_PIN, OUTPUT);
    pinMode(LOGGING_LED, OUTPUT);
    pinMode(LOGGING_BUTTON, INPUT_PULLUP);

    if (!SD.begin(5)) {
        Serial.println("SD Card Mount Failed");
    }

    digitalWrite(LOGGING_LED, LOW);
    Serial.println("ESP32 Hub Ready...");
}

void loop() {
    if (digitalRead(LOGGING_BUTTON) == LOW) {
        toggleLogging();
        delay(50);
    }
    if (loggingEnabled) {
        HubLogging();
    }
    else if(!loggingEnabled) { 
        lastLogTime = millis();
        epochTime = millis(); // Timestamp

        // Generate Dummy Sensor Data
        float dht = random(200, 300) / 10.0;
        float rain = random(0, 100);
        float soil = random(200, 800) / 10.0;
        float ldr = random(0, 1023);
        float ph = random(50, 90) / 10.0;
        float water_level = random(0, 100);
        float ph_up = random(0, 2);   // Fixed Boolean Randomization
        float ph_down = random(0, 2);
        float stirrer = random(0, 2);
        float pump = random(0, 2);
        float light_intensity = random(0, 1023);

        // Create Data String
        dataMessage = String(epochTime) + ", " + String("ESP_HUB") + "," + String(dht) + "," + String(rain) + "," +
                      String(soil) + "," + String(ldr) + "," + String(ph) + "," +
                      String(water_level) + "," + String(ph_up) + "," + 
                      String(ph_down) + "," + String(stirrer) + "," + 
                      String(pump) + "," + String(light_intensity) + "\r\n";

        Serial.println("HUB_Data Not Logged: " + dataMessage);
        delay(3000);
    }
    
    // if (loggingEnabled && millis() - lastLogTime >= logInterval) {
    //     lastLogTime = millis();
    //     epochTime = millis(); // Timestamp

    //     // Generate Dummy Sensor Data
    //     float dht = random(200, 300) / 10.0;
    //     float rain = random(0, 100);
    //     float soil = random(200, 800) / 10.0;
    //     float ldr = random(0, 1023);
    //     float ph = random(50, 90) / 10.0;
    //     float water_level = random(0, 100);
    //     float ph_up = random(0, 2);   // Fixed Boolean Randomization
    //     float ph_down = random(0, 2);
    //     float stirrer = random(0, 2);
    //     float pump = random(0, 2);
    //     float light_intensity = random(0, 1023);

    //     // Create Data String
    //     dataMessage = String(epochTime) + ", " + String("ESP_HUB") + "," + String(dht) + "," + String(rain) + "," +
    //                   String(soil) + "," + String(ldr) + "," + String(ph) + "," +
    //                   String(water_level) + "," + String(ph_up) + "," + 
    //                   String(ph_down) + "," + String(stirrer) + "," + 
    //                   String(pump) + "," + String(light_intensity) + "\r\n";

    //     // Save Data
    //     appendFile(SD, dataPath, dataMessage.c_str());

    //     Serial.println("Data Logged: " + dataMessage);
    // }
}


void HubLogging() {
    lastLogTime = millis();
        epochTime = millis(); // Timestamp

        // Generate Dummy Sensor Data
        float dht = random(200, 300) / 10.0;
        float rain = random(0, 100);
        float soil = random(200, 800) / 10.0;
        float ldr = random(0, 1023);
        float ph = random(50, 90) / 10.0;
        float water_level = random(0, 100);
        float ph_up = random(0, 2);   // Fixed Boolean Randomization
        float ph_down = random(0, 2);
        float stirrer = random(0, 2);
        float pump = random(0, 2);
        float light_intensity = random(0, 1023);

        // Create Data String
        dataMessage = String(epochTime) + ", " + String("ESP_HUB") + "," + String(dht) + "," + String(rain) + "," +
                      String(soil) + "," + String(ldr) + "," + String(ph) + "," +
                      String(water_level) + "," + String(ph_up) + "," + 
                      String(ph_down) + "," + String(stirrer) + "," + 
                      String(pump) + "," + String(light_intensity) + "\r\n";

        // Save Data
        appendFile(SD, dataPath, dataMessage.c_str());

        Serial.println("HUB_Data Logged: " + dataMessage);
        delay(3000);
}