#include "wifi_manager.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <Arduino.h>

WiFiMulti wifiMulti;

// Налаштування WiFi мереж (SSID, Password)
// WiFiMulti автоматично вибере найсильнішу доступну
#define WIFI_NETWORK_1_SSID     "Pixel"
#define WIFI_NETWORK_1_PASS     "naspiatero"

#define WIFI_NETWORK_2_SSID     "netis_406223"
#define WIFI_NETWORK_2_PASS     "naspiatero"

#define WIFI_NETWORK_3_SSID     "Myroslav"
#define WIFI_NETWORK_3_PASS     "naspiatero"

void wifiInit() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    // Додаємо всі мережі
    wifiMulti.addAP(WIFI_NETWORK_1_SSID, WIFI_NETWORK_1_PASS);
    wifiMulti.addAP(WIFI_NETWORK_2_SSID, WIFI_NETWORK_2_PASS);
    wifiMulti.addAP(WIFI_NETWORK_3_SSID, WIFI_NETWORK_3_PASS);
    
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
    
    Serial.println("Connecting to best available network...");
    
    int attempts = 0;
    while (wifiMulti.run() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected!");
        Serial.print("Network: ");
        Serial.println(WiFi.SSID());
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    } else {
        Serial.println("\nFailed to connect to any network!");
        Serial.print("WiFi status: ");
        Serial.println(WiFi.status());
    }
}

bool wifiIsConnected() {
    return WiFi.status() == WL_CONNECTED;
}
