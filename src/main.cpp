#include <Arduino.h>
#include <WiFi.h>
#include "wifi/wifi_manager.h"
#include "camera/camera_manager.h"
#include "motor/motor_manager.h"
#include "web/web_server.h"
#include "web/stream_server.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== ESP32-CAM Starting ===");
    
    Serial.println("Initializing Camera...");
    if (cameraInit()) {
        Serial.println("Camera initialized successfully!");
    } else {
        Serial.println("ERROR: Camera initialization failed!");
    }
    
    Serial.println("Initializing WiFi...");
    wifiInit();
    
    Serial.println("Initializing Motors...");
    motorInit();

    Serial.println("Starting Web Server...");
    webServerInit();
    
    Serial.println("Starting Stream Server...");
    streamServerInit();
    
    Serial.println("=== Setup Complete ===");
    Serial.println("System is running...\n");
}

void loop() {
    // Обробка streaming сервера
    streamServerHandle();
    
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 10000) {
        lastCheck = millis();
        
        Serial.print("Status: ");
        if (wifiIsConnected()) {
            Serial.print("WiFi OK, IP: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("WiFi DISCONNECTED!");
        }
    }
    
    delay(10);  // Зменшено для швидшої обробки streaming
}