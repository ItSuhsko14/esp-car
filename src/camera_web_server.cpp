#include "esp_camera.h"
#include <WebServer.h>

WebServer server(80);

static const char* STREAM_CONTENT_TYPE =
  "multipart/x-mixed-replace;boundary=frame";
static const char* STREAM_BOUNDARY = "\r\n--frame\r\n";
static const char* STREAM_PART =
  "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

void handle_jpg_stream() {
  WiFiClient client = server.client();
  if (!client.connected()) return;

  camera_fb_t * fb = NULL;

  client.printf(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Connection: close\r\n\r\n",
    STREAM_CONTENT_TYPE
  );

  while (client.connected()) {
    fb = esp_camera_fb_get();
    if (!fb) break;

    client.print(STREAM_BOUNDARY);
    client.printf(STREAM_PART, fb->len);
    client.write(fb->buf, fb->len);

    esp_camera_fb_return(fb);
    fb = NULL;

    // Маленька пауза для стабільності
    delay(1);
  }
}

void handle_root() {
  server.send(
    200,
    "text/html",
    "<html><body>"
    "<h1>ESP32-CAM Stream</h1>"
    "<img src=\"/stream\"/>"
    "</body></html>"
  );
}

void startCameraServer() {
  server.on("/", handle_root);
  server.on("/stream", HTTP_GET, handle_jpg_stream);
  server.begin();
}