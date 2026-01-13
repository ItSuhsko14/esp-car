#include "web_server.h"
#include "../camera/camera_manager.h"
#include "../motor/motor_manager.h"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Arduino.h>

AsyncWebServer server(80);

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
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            pointer-events: none;
            display: flex;
            justify-content: space-between;
            padding: 20px;
        }
        
        .control-group {
            display: flex;
            flex-direction: column;
            justify-content: space-between;
            height: 100%;
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
        
        @media (orientation: portrait) {
            .btn {
                width: 70px;
                height: 70px;
                font-size: 28px;
            }
            .controls {
                padding: 15px;
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
    
    server.begin();
    Serial.println("Web server started on port 80");
    Serial.print("Open browser at: http://");
    Serial.println(WiFi.localIP());
}
