#include "stream_server.h"
#include "web_server.h"
#include "../camera/camera_manager.h"
#include <WebServer.h>
#include <WiFi.h>
#include <Arduino.h>

WebServer streamServer(81);

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// MJPEG Stream handler
void handleStream() {
    if (!cameraIsInitialized()) {
        streamServer.send(503, "text/plain", "Camera not initialized");
        return;
    }
    
    Serial.println("Stream started");
    
    WiFiClient client = streamServer.client();
    
    unsigned long frameCount = 0;
    unsigned long startTime = millis();
    
    // Відправляємо HTTP заголовки одним write для швидкості
    String headers = "HTTP/1.1 200 OK\r\n";
    headers += "Content-Type: ";
    headers += _STREAM_CONTENT_TYPE;
    headers += "\r\n";
    headers += "Access-Control-Allow-Origin: *\r\n";
    headers += "Cache-Control: no-cache, no-store, must-revalidate\r\n";
    headers += "Pragma: no-cache\r\n";
    headers += "\r\n";
    client.write((uint8_t*)headers.c_str(), headers.length());
    
    // Streaming loop - зупиняється при OTA
    while (client.connected() && !otaInProgress) {
        camera_fb_t * fb = cameraCapture();
        if (!fb) {
            Serial.println("Camera capture failed");
            delay(100);
            continue;
        }
        
        // Формуємо заголовок в буфері
        char part_buf[128];
        size_t hlen = snprintf(part_buf, sizeof(part_buf), "%s%s%u\r\n\r\n", 
                               _STREAM_BOUNDARY, 
                               "Content-Type: image/jpeg\r\nContent-Length: ", 
                               fb->len);
        
        // Відправляємо заголовок одним write
        client.write((uint8_t*)part_buf, hlen);
        
        // Відправляємо JPEG дані
        client.write(fb->buf, fb->len);
        
        cameraReturn(fb);
        
        frameCount++;
        
        // Виводимо FPS кожні 5 секунд
        if (frameCount % 100 == 0) {
            unsigned long elapsed = millis() - startTime;
            float fps = (frameCount * 1000.0) / elapsed;
            Serial.printf("Streaming FPS: %.1f\n", fps);
        }
    }
    
    unsigned long elapsed = millis() - startTime;
    float avgFps = (frameCount * 1000.0) / elapsed;
    
    if (otaInProgress) {
        Serial.println("Stream stopped for OTA update");
    } else {
        Serial.printf("Stream stopped. Average FPS: %.1f, Total frames: %lu\n", avgFps, frameCount);
    }
}

void streamServerInit() {
    streamServer.on("/stream", handleStream);
    streamServer.begin();
    Serial.println("Stream server started on port 81");
}

void streamServerHandle() {
    streamServer.handleClient();
}

