// ------------------------------------------------------
// ESP32_AP_Cam.ino
// 開機：
//   1. 相機初始化
//   2. 從 NVS 讀 AP 設定（未設則用預設 ESPNET）
//   3. 啟動 AP
//   4. 啟動 WebServer（含：初次設定頁 / 控制頁 / 串流）
// ------------------------------------------------------

#include <Arduino.h>
#include "CameraModule.h"
#include "ApConfigModule.h"
#include "WebServerModule.h"
#include "DnsCaptiveModule.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("ESP32 AP CAM booting...");

  // 1. 相機初始化
  if (!Camera_Init()) {
    Serial.println("Camera init failed, rebooting...");
    delay(2000);
    ESP.restart();
  }

  // 2. 從 NVS 讀 AP 設定（是不是已經配過網）
  Ap_Config_Load_From_Nvs();

  // 3. 用目前設定的 SSID / 密碼啟動 AP
  if (!Ap_Start_Access_Point()) {
    Serial.println("AP start failed, rebooting...");
    delay(2000);
    ESP.restart();
  }

  // 4. 若「尚未完成初次設定」，啟動 Captive DNS
  //    目標：手機一連進來就會以為是需要登入的熱點 → 自動跳設定頁
  if (!Ap_Config_Is_Done()) {
    IPAddress Ap_Ip_Value = WiFi.softAPIP();
    Dns_Captive_Start(Ap_Ip_Value);
  }

  // 5. 啟動 WebServer
  //    WebServer 內部會依 Ap_Config_Is_Done()
  //    決定 "/" 回 AP 設定頁 或 CATNET-CAM 主頁
  WebServer_Start();

  Serial.println("System ready.");
}

void loop() {

  Dns_Captive_Process();

  delay(10);
}