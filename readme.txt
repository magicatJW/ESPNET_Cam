# ESPNET_Cam

ESP32-CAM 專案，提供：

- 開機進入 AP 模式，SSID：`ESPNET`
- 手機連線後自動跳出設定頁（Captive Portal）
- 可輸入目標 WiFi SSID / 密碼，儲存後切換為 STA 模式
- 連上路由器後，可透過瀏覽器觀看即時影像串流

## 專案結構

- `ESPNET_Cam.ino`：主程式入口，負責初始化系統與主要流程控制
- `ApConfigModule.*`：AP 模式、WiFi 設定邏輯
- `DnsCaptiveModule.*`：Captive portal / DNS 轉向
- `HtmlApSetupModule.*`：初次設定頁面的 HTML
- `HtmlMainPageModule.*`：主控制頁面 HTML（串流、解析度切換等）
- `WebServerModule.*`：HTTP 伺服器與路由
- `CameraModule.*`：ESP32-CAM 初始化與影像串流控制

## 環境需求

- 開發板：ESP32-CAM
- 開發環境：Arduino IDE / PlatformIO
- 板子設定：ESP32 Wrover Module（PSRAM enabled）

## 使用方式（概略）

1. 燒錄本專案至 ESP32-CAM
2. 上電後搜尋 WiFi：`ESPNET`
3. 用手機連上後，瀏覽器自動跳出設定頁
4. 輸入目標 WiFi SSID / Password，送出
5. 重新上電，ESP32-CAM 自動連上設定好的 WiFi
6. 在同一網段用瀏覽器打開顯示的 IP，即可看到串流畫面
