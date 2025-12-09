// ------------------------------------------------------
// WebServerModule.h
// 影像串流 WebServer 模組
// ------------------------------------------------------
#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <Arduino.h>

// 啟動 HTTP 伺服器：
//   /          → 首頁（依狀態顯示：初次設定頁 or 控制頁）
//   /stream    → MJPEG 串流
//   /capture   → 單張拍照
//   /set       → 影像參數設定
//   /saveConfig→ 儲存 AP 設定（SSID / 密碼）
void WebServer_Start();

#endif  // WEB_SERVER_MODULE_H

