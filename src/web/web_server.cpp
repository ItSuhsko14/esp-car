#include "web_server.h"
#include "../camera/camera_manager.h"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Arduino.h>

AsyncWebServer server(80);

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

String getUptime() {
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    String uptime = "";
    if (days > 0) uptime += String(days) + "d ";
    if (hours > 0) uptime += String(hours) + "h ";
    if (minutes > 0) uptime += String(minutes) + "m ";
    uptime += String(seconds) + "s";
    
    return uptime;
}

String getHTML() {
    String cameraStatus = cameraIsInitialized() ? "Active ✓" : "Not initialized ✗";
    
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-CAM Control</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 900px;
            margin: 0 auto;
        }
        h1 {
            color: white;
            text-align: center;
            margin-bottom: 30px;
            font-size: 32px;
            text-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }
        .video-container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            padding: 20px;
            margin-bottom: 20px;
        }
        .video-wrapper {
            position: relative;
            width: 100%;
            padding-bottom: 75%;
            background: #000;
            border-radius: 10px;
            overflow: hidden;
        }
        .video-wrapper img {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            object-fit: contain;
        }
        .info-container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            padding: 30px;
        }
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
        }
        .status-item {
            background: #f7f7f7;
            border-radius: 10px;
            padding: 15px;
        }
        .status-label {
            color: #666;
            font-size: 12px;
            margin-bottom: 5px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        .status-value {
            color: #333;
            font-size: 18px;
            font-weight: bold;
        }
        .status-ok { color: #10b981; }
        .status-error { color: #ef4444; }
        @media (max-width: 768px) {
            h1 { font-size: 24px; }
            .info-grid { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚗 ESP32-CAM Control</h1>
        
        <div class="video-container">
            <div class="video-wrapper">
                <img src="/stream" alt="Camera Stream">
            </div>
        </div>
        
        <div class="info-container">
            <div class="info-grid">
                <div class="status-item">
                    <div class="status-label">Camera</div>
                    <div class="status-value )rawliteral" + String(cameraIsInitialized() ? "status-ok" : "status-error") + R"rawliteral(">)rawliteral" + cameraStatus + R"rawliteral(</div>
                </div>
                
                <div class="status-item">
                    <div class="status-label">WiFi</div>
                    <div class="status-value status-ok">)rawliteral" + String(WiFi.status() == WL_CONNECTED ? "Connected ✓" : "Disconnected ✗") + R"rawliteral(</div>
                </div>
                
                <div class="status-item">
                    <div class="status-label">IP Address</div>
                    <div class="status-value">)rawliteral" + WiFi.localIP().toString() + R"rawliteral(</div>
                </div>
                
                <div class="status-item">
                    <div class="status-label">Signal</div>
                    <div class="status-value">)rawliteral" + String(WiFi.RSSI()) + R"rawliteral( dBm</div>
                </div>
                
                <div class="status-item">
                    <div class="status-label">Uptime</div>
                    <div class="status-value">)rawliteral" + getUptime() + R"rawliteral(</div>
                </div>
                
                <div class="status-item">
                    <div class="status-label">Device</div>
                    <div class="status-value">ESP32-CAM</div>
                </div>
            </div>
        </div>
    </div>
</body>
</html>
)rawliteral";
    
    return html;
}

// Простий MJPEG stream handler
void streamHandler(AsyncWebServerRequest *request) {
    if (!cameraIsInitialized()) {
        request->send(503, "text/plain", "Camera not initialized");
        return;
    }
    
    Serial.println("Stream requested");
    
    AsyncWebServerResponse *response = request->beginChunkedResponse(_STREAM_CONTENT_TYPE, 
        [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        
        // Захоплюємо кадр
        camera_fb_t * fb = cameraCapture();
        if (!fb) {
            Serial.println("Camera capture failed in stream");
            return 0;
        }
        
        Serial.printf("Frame captured: %d bytes\n", fb->len);
        
        // Формуємо MJPEG частину
        char part_buf[64];
        size_t hlen = snprintf(part_buf, 64, _STREAM_PART, fb->len);
        size_t blen = strlen(_STREAM_BOUNDARY);
        
        // Перевіряємо чи вміститься
        size_t total = blen + hlen + fb->len;
        if (total > maxLen) {
            Serial.printf("Buffer too small: need %d, have %d\n", total, maxLen);
            cameraReturn(fb);
            return 0;
        }
        
        size_t len = 0;
        
        // Додаємо boundary
        memcpy(buffer + len, _STREAM_BOUNDARY, blen);
        len += blen;
        
        // Додаємо заголовок
        memcpy(buffer + len, part_buf, hlen);
        len += hlen;
        
        // Додаємо JPEG дані
        memcpy(buffer + len, fb->buf, fb->len);
        len += fb->len;
        
        // Звільняємо кадр
        cameraReturn(fb);
        
        // Невелика затримка для стабільності (~20 FPS)
        delay(50);
        
        return len;
    });
    
    request->send(response);
    Serial.println("Stream response sent");
}

void webServerInit() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", getHTML());
    });
    
    server.on("/stream", HTTP_GET, streamHandler);
    
    server.begin();
    Serial.println("Web server started on port 80");
    Serial.print("Open browser at: http://");
    Serial.println(WiFi.localIP());
}