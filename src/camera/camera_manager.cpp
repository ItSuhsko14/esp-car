#include "camera_manager.h"
#include <Arduino.h>

// Camera sensor PIDs
#define OV2640_PID 0x2642
#define OV3660_PID 0x3660
#define OV5640_PID 0x5640

// AI-Thinker ESP32-CAM pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

static bool camera_initialized = false;

bool cameraInit() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_LATEST;
    
    // Налаштування якості та роздільності
    if(psramFound()){
        Serial.println("PSRAM found");
        config.frame_size = FRAMESIZE_VGA; // 640x480 - оптимально для streaming
        config.jpeg_quality = 12; // 10-15 оптимально (менше число = краща якість, більший розмір)
        config.fb_count = 2;
        config.fb_location = CAMERA_FB_IN_PSRAM;
    } else {
        Serial.println("PSRAM not found, using lower resolution");
        config.frame_size = FRAMESIZE_CIF; // 400x296
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }
    
    // Ініціалізація камери
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }
    
    // Отримуємо сенсор і виводимо інформацію
    sensor_t * s = esp_camera_sensor_get();
    if (s == NULL) {
        Serial.println("ERROR: Failed to get camera sensor");
        return false;
    }
    
    // Виводимо модель камери
    Serial.print("Camera sensor PID: 0x");
    Serial.println(s->id.PID, HEX);
    
    if (s->id.PID == OV3660_PID) {
        Serial.println("OV3660 camera detected");
    } else if (s->id.PID == OV2640_PID) {
        Serial.println("OV2640 camera detected");
    } else if (s->id.PID == OV5640_PID) {
        Serial.println("OV5640 camera detected");
    } else {
        Serial.println("Unknown camera model");
    }
    
    // Базові налаштування для всіх камер
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect)
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4
    s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_aec_value(s, 300);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
    
    // Спеціальні налаштування для OV3660
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);      // OV3660 часто потребує vflip
        s->set_brightness(s, 1); // Трохи світліше для OV3660
        s->set_saturation(s, -1); // Менша насиченість
    }
    
    camera_initialized = true;
    Serial.println("Camera initialized successfully");
    
    // Тестове захоплення кадру
    Serial.println("Testing camera capture...");
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("ERROR: Camera capture test failed");
        return false;
    }
    Serial.printf("Test capture OK: %dx%d, %d bytes\n", fb->width, fb->height, fb->len);
    esp_camera_fb_return(fb);
    
    return true;
}

camera_fb_t* cameraCapture() {
    if (!camera_initialized) {
        return NULL;
    }
    return esp_camera_fb_get();
}

void cameraReturn(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

bool cameraIsInitialized() {
    return camera_initialized;
}

