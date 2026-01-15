#include "web_server.h"
#include "../camera/camera_manager.h"
#include "../motor/motor_manager.h"
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <WiFi.h>
#include <Arduino.h>

AsyncWebServer server(80);
volatile bool otaInProgress = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>RC Car</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        
        html, body {
            width: 100%;
            height: 100%;
            overflow: hidden;
            background: #0a0a0a;
            touch-action: none;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            user-select: none;
        }
        
        .fullscreen {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        #stream {
            max-width: 100%;
            max-height: 100%;
            width: 100%;
            height: 100%;
            object-fit: contain;
        }
        
        .controls {
            position: fixed;
            bottom: 0;
            left: 0;
            width: 100%;
            pointer-events: none;
            display: flex;
            justify-content: space-between;
            padding: 20px 30px 30px 30px;
        }
        
        .control-group {
            display: flex;
            flex-direction: row;
            gap: 15px;
            pointer-events: auto;
        }
        
        .btn {
            width: 90px;
            height: 90px;
            border-radius: 50%;
            border: 3px solid rgba(255, 255, 255, 0.6);
            background: rgba(0, 0, 0, 0.4);
            color: white;
            font-size: 36px;
            display: flex;
            align-items: center;
            justify-content: center;
            cursor: pointer;
            transition: all 0.1s ease;
            backdrop-filter: blur(4px);
            -webkit-backdrop-filter: blur(4px);
        }
        
        .btn:active, .btn.active {
            background: rgba(59, 130, 246, 0.8);
            border-color: #3b82f6;
            transform: scale(0.95);
            box-shadow: 0 0 30px rgba(59, 130, 246, 0.6);
        }
        
        .btn-forward { background: rgba(34, 197, 94, 0.3); }
        .btn-forward:active, .btn-forward.active { background: rgba(34, 197, 94, 0.8); border-color: #22c55e; box-shadow: 0 0 30px rgba(34, 197, 94, 0.6); }
        
        .btn-backward { background: rgba(239, 68, 68, 0.3); }
        .btn-backward:active, .btn-backward.active { background: rgba(239, 68, 68, 0.8); border-color: #ef4444; box-shadow: 0 0 30px rgba(239, 68, 68, 0.6); }
        
        .status-indicator {
            position: fixed;
            top: 10px;
            left: 50%;
            transform: translateX(-50%);
            background: rgba(0, 0, 0, 0.6);
            color: white;
            padding: 8px 16px;
            border-radius: 20px;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            font-size: 12px;
            backdrop-filter: blur(4px);
            -webkit-backdrop-filter: blur(4px);
            opacity: 0;
            transition: opacity 0.3s;
        }
        
        .status-indicator.visible {
            opacity: 1;
        }
        
        .settings-btn {
            position: fixed;
            top: 15px;
            right: 15px;
            width: 44px;
            height: 44px;
            border-radius: 50%;
            border: 2px solid rgba(255, 255, 255, 0.4);
            background: rgba(0, 0, 0, 0.4);
            color: white;
            font-size: 20px;
            display: flex;
            align-items: center;
            justify-content: center;
            cursor: pointer;
            backdrop-filter: blur(4px);
            -webkit-backdrop-filter: blur(4px);
            text-decoration: none;
            z-index: 100;
        }
        
        .settings-btn:active {
            background: rgba(255, 255, 255, 0.2);
        }
        
        @media (orientation: portrait) {
            .btn {
                width: 70px;
                height: 70px;
                font-size: 28px;
            }
            .controls {
                padding: 15px 20px 20px 20px;
            }
            .control-group {
                gap: 10px;
            }
        }
    </style>
</head>
<body>
    <div class="fullscreen">
        <img id="stream" src="" alt="Stream">
    </div>
    
    <div class="controls">
        <div class="control-group">
            <button class="btn btn-forward" id="btnForward">▲</button>
            <button class="btn btn-backward" id="btnBackward">▼</button>
        </div>
        <div class="control-group">
            <button class="btn" id="btnLeft">◄</button>
            <button class="btn" id="btnRight">►</button>
        </div>
    </div>
    
    <div class="status-indicator" id="status"></div>
    
    <a href="/update" class="settings-btn" title="Firmware Update">⚙</a>
    
    <script>
        const streamImg = document.getElementById('stream');
        const status = document.getElementById('status');
        
        // Stream setup
        streamImg.src = 'http://' + window.location.hostname + ':81/stream';
        streamImg.onerror = function() {
            setTimeout(() => {
                streamImg.src = 'http://' + window.location.hostname + ':81/stream?' + Date.now();
            }, 1000);
        };
        
        // Control functions
        function sendCommand(action) {
            fetch('/control?action=' + action, { method: 'POST' })
                .then(r => {
                    if (r.ok) showStatus(action.toUpperCase());
                })
                .catch(e => showStatus('ERROR'));
        }
        
        function showStatus(text) {
            status.textContent = text;
            status.classList.add('visible');
            clearTimeout(status.timer);
            status.timer = setTimeout(() => status.classList.remove('visible'), 500);
        }
        
        // Button handlers
        function setupButton(id, action) {
            const btn = document.getElementById(id);
            
            const start = (e) => {
                e.preventDefault();
                btn.classList.add('active');
                sendCommand(action);
            };
            
            const end = (e) => {
                e.preventDefault();
                btn.classList.remove('active');
                sendCommand('stop');
            };
            
            // Touch events
            btn.addEventListener('touchstart', start, { passive: false });
            btn.addEventListener('touchend', end, { passive: false });
            btn.addEventListener('touchcancel', end, { passive: false });
            
            // Mouse events (for desktop testing)
            btn.addEventListener('mousedown', start);
            btn.addEventListener('mouseup', end);
            btn.addEventListener('mouseleave', (e) => {
                if (btn.classList.contains('active')) {
                    end(e);
                }
            });
        }
        
        setupButton('btnForward', 'forward');
        setupButton('btnBackward', 'backward');
        setupButton('btnLeft', 'left');
        setupButton('btnRight', 'right');
        
        // Prevent default touch behaviors
        document.addEventListener('touchmove', e => e.preventDefault(), { passive: false });
        
        // Fullscreen on first touch (optional, improves mobile UX)
        document.body.addEventListener('click', () => {
            if (document.documentElement.requestFullscreen && !document.fullscreenElement) {
                document.documentElement.requestFullscreen().catch(() => {});
            }
        }, { once: true });
    </script>
</body>
</html>
)rawliteral";

const char update_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Firmware Update</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: rgba(255,255,255,0.1);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 40px;
            max-width: 400px;
            width: 100%;
            text-align: center;
        }
        h1 {
            color: white;
            margin-bottom: 10px;
            font-size: 24px;
        }
        p {
            color: rgba(255,255,255,0.7);
            margin-bottom: 30px;
            font-size: 14px;
        }
        .upload-area {
            border: 2px dashed rgba(255,255,255,0.3);
            border-radius: 15px;
            padding: 30px;
            margin-bottom: 20px;
            transition: all 0.3s;
        }
        .upload-area:hover {
            border-color: #3b82f6;
            background: rgba(59,130,246,0.1);
        }
        input[type="file"] {
            display: none;
        }
        label {
            color: white;
            cursor: pointer;
            display: block;
        }
        label span {
            display: block;
            font-size: 40px;
            margin-bottom: 10px;
        }
        button {
            background: #3b82f6;
            color: white;
            border: none;
            padding: 15px 40px;
            border-radius: 10px;
            font-size: 16px;
            cursor: pointer;
            width: 100%;
            transition: background 0.3s;
        }
        button:hover {
            background: #2563eb;
        }
        button:disabled {
            background: #666;
            cursor: not-allowed;
        }
        #progress {
            margin-top: 20px;
            display: none;
        }
        #progress-bar {
            width: 100%;
            height: 10px;
            background: rgba(255,255,255,0.2);
            border-radius: 5px;
            overflow: hidden;
        }
        #progress-fill {
            height: 100%;
            background: #22c55e;
            width: 0%;
            transition: width 0.3s;
        }
        #progress-text {
            color: white;
            margin-top: 10px;
            font-size: 14px;
        }
        .back-link {
            display: inline-block;
            margin-top: 20px;
            color: rgba(255,255,255,0.6);
            text-decoration: none;
        }
        .back-link:hover {
            color: white;
        }
        #filename {
            color: #22c55e;
            margin-top: 10px;
            font-size: 12px;
            word-break: break-all;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Firmware Update</h1>
        <p>Select firmware.bin file to upload</p>
        <form id="upload-form" method="POST" action="/update" enctype="multipart/form-data">
            <div class="upload-area">
                <label for="file">
                    <span>📁</span>
                    Choose file or drag here
                </label>
                <input type="file" id="file" name="update" accept=".bin">
                <div id="filename"></div>
            </div>
            <button type="submit" id="submit-btn" disabled>Upload Firmware</button>
        </form>
        <div id="progress">
            <div id="progress-bar"><div id="progress-fill"></div></div>
            <div id="progress-text">Uploading... 0%</div>
        </div>
        <a href="/" class="back-link">← Back to Control</a>
    </div>
    <script>
        const fileInput = document.getElementById('file');
        const filename = document.getElementById('filename');
        const submitBtn = document.getElementById('submit-btn');
        const form = document.getElementById('upload-form');
        const progress = document.getElementById('progress');
        const progressFill = document.getElementById('progress-fill');
        const progressText = document.getElementById('progress-text');
        
        fileInput.addEventListener('change', function() {
            if (this.files.length > 0) {
                filename.textContent = this.files[0].name;
                submitBtn.disabled = false;
            }
        });
        
        form.addEventListener('submit', function(e) {
            e.preventDefault();
            const formData = new FormData(form);
            const xhr = new XMLHttpRequest();
            
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    progressFill.style.width = percent + '%';
                    if (percent < 100) {
                        progressText.textContent = 'Uploading... ' + percent + '%';
                    }
                }
            });
            
            xhr.upload.addEventListener('load', function() {
                // Upload завершено (100%) - показуємо успіх одразу
                progressText.textContent = 'Success! Rebooting...';
                progressFill.style.background = '#22c55e';
                setTimeout(() => { window.location.href = '/'; }, 5000);
            });
            
            xhr.addEventListener('error', function() {
                // Помилка мережі (може бути через reboot - це нормально)
                if (progressFill.style.width === '100%') {
                    progressText.textContent = 'Success! Rebooting...';
                    progressFill.style.background = '#22c55e';
                    setTimeout(() => { window.location.href = '/'; }, 5000);
                } else {
                    progressText.textContent = 'Upload failed!';
                    progressFill.style.background = '#ef4444';
                }
            });
            
            xhr.open('POST', '/update');
            progress.style.display = 'block';
            submitBtn.disabled = true;
            xhr.send(formData);
        });
    </script>
</body>
</html>
)rawliteral";

void webServerInit() {
    // Main page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });
    
    // Motor control API
    server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("action")) {
            String action = request->getParam("action")->value();
            
            if (action == "forward") {
                motorForward();
                Serial.println("CMD: Forward");
            } else if (action == "backward") {
                motorBackward();
                Serial.println("CMD: Backward");
            } else if (action == "left") {
                motorLeft();
                Serial.println("CMD: Left");
            } else if (action == "right") {
                motorRight();
                Serial.println("CMD: Right");
            } else if (action == "stop") {
                motorStop();
                Serial.println("CMD: Stop");
            }
            
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Missing action parameter");
        }
    });
    
    // OTA Update page
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", update_html);
    });
    
    // OTA Update handler
    server.on("/update", HTTP_POST, 
        [](AsyncWebServerRequest *request){
            bool success = !Update.hasError();
            AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", 
                success ? "OK" : "FAIL");
            response->addHeader("Connection", "close");
            request->send(response);
            if (success) {
                Serial.println("OTA: Rebooting...");
                delay(1500);  // Більше часу для відправки відповіді браузеру
                ESP.restart();
            } else {
                otaInProgress = false;
                Serial.println("OTA: Failed, resuming normal operation");
            }
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
            if (!index) {
                // Початок OTA - зупиняємо streaming
                otaInProgress = true;
                Serial.println("OTA: Starting update, stopping stream...");
                delay(100);  // Дати час streaming зупинитись
                
                // Перевірка вільного місця
                size_t freeSpace = ESP.getFreeSketchSpace();
                Serial.printf("OTA: Free space: %u bytes\n", freeSpace);
                Serial.printf("OTA: Receiving: %s\n", filename.c_str());
                
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    otaInProgress = false;
                    Update.printError(Serial);
                    return;
                }
            }
            
            if (Update.hasError()) {
                otaInProgress = false;
                return;
            }
            
            if (Update.write(data, len) != len) {
                otaInProgress = false;
                Update.printError(Serial);
                return;
            }
            
            if (final) {
                if (Update.end(true)) {
                    Serial.printf("OTA: Success! Total: %u bytes\n", index + len);
                } else {
                    otaInProgress = false;
                    Update.printError(Serial);
                }
            }
        }
    );
    
    server.begin();
    Serial.println("Web server started on port 80");
    Serial.println("OTA Update available at /update");
    Serial.print("Open browser at: http://");
    Serial.println(WiFi.localIP());
}
