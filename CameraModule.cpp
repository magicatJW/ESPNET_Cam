// ------------------------------------------------------
// CameraModule.cpp
// 負責 ESP32-CAM 相機初始化
// ------------------------------------------------------

#include "CameraModule.h"
#include "esp_camera.h"

// ---------- AI Thinker ESP32-CAM 腳位定義 ----------
#define Pwdn_Gpio_Num      32
#define Reset_Gpio_Num     -1
#define Xclk_Gpio_Num       0
#define Siod_Gpio_Num      26
#define Sioc_Gpio_Num      27

#define Y9_Gpio_Num        35
#define Y8_Gpio_Num        34
#define Y7_Gpio_Num        39
#define Y6_Gpio_Num        36
#define Y5_Gpio_Num        21
#define Y4_Gpio_Num        19
#define Y3_Gpio_Num        18
#define Y2_Gpio_Num         5
#define Vsync_Gpio_Num     25
#define Href_Gpio_Num      23
#define Pclk_Gpio_Num      22

// 相機初始化函式
bool Camera_Init() {
  camera_config_t Config_Value;

  Config_Value.ledc_channel = LEDC_CHANNEL_0;
  Config_Value.ledc_timer   = LEDC_TIMER_0;

  Config_Value.pin_d0       = Y2_Gpio_Num;
  Config_Value.pin_d1       = Y3_Gpio_Num;
  Config_Value.pin_d2       = Y4_Gpio_Num;
  Config_Value.pin_d3       = Y5_Gpio_Num;
  Config_Value.pin_d4       = Y6_Gpio_Num;
  Config_Value.pin_d5       = Y7_Gpio_Num;
  Config_Value.pin_d6       = Y8_Gpio_Num;
  Config_Value.pin_d7       = Y9_Gpio_Num;

  Config_Value.pin_xclk     = Xclk_Gpio_Num;
  Config_Value.pin_pclk     = Pclk_Gpio_Num;
  Config_Value.pin_vsync    = Vsync_Gpio_Num;
  Config_Value.pin_href     = Href_Gpio_Num;

  Config_Value.pin_sccb_sda = Siod_Gpio_Num;
  Config_Value.pin_sccb_scl = Sioc_Gpio_Num;

  Config_Value.pin_pwdn     = Pwdn_Gpio_Num;
  Config_Value.pin_reset    = Reset_Gpio_Num;

  Config_Value.xclk_freq_hz = 20000000;
  Config_Value.pixel_format = PIXFORMAT_JPEG;

  // 有 PSRAM 的話開高一點
  if (psramFound()) {
    Config_Value.frame_size   = FRAMESIZE_VGA;   // 640x480
    Config_Value.jpeg_quality = 12;              // 0~63，越小越好
    Config_Value.fb_count     = 2;
  } else {
    Config_Value.frame_size   = FRAMESIZE_QVGA;  // 320x240
    Config_Value.jpeg_quality = 15;
    Config_Value.fb_count     = 1;
  }

  esp_err_t Err_Value = esp_camera_init(&Config_Value);

  if (Err_Value != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", Err_Value);
    return false;
  }

  return true;
}

