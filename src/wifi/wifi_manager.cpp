#include "wifi_manager.h"
#include <WiFi.h>
#include <Arduino.h>

static const char* ssid = "Pixel";
static const char* password = "naspiatero";

void wifiInit() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    Serial.println("\n=== WiFi Scan ===");
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.println(" dBm)");
    }
    Serial.println("=================\n");
    
    Serial.print("Connecting to: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect!");
        Serial.print("WiFi status: ");
        Serial.println(WiFi.status());
    }
}

bool wifiIsConnected() {
    return WiFi.status() == WL_CONNECTED;
}