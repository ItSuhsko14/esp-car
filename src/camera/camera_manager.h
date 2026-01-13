#pragma once

#include <esp_camera.h>

bool cameraInit();
camera_fb_t* cameraCapture();
void cameraReturn(camera_fb_t* fb);
bool cameraIsInitialized();

