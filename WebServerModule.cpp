// ------------------------------------------------------
// WebServerModule.cpp
// CATNET-CAM + AP 初次設定 + Captive Portal
//
// 模式說明：
//
// 1. 尚未完成 AP 設定 (Ap_Config_Is_Done()==false)
//    - AP SSID = 預設 "ESPNET"
//    - DNS Captive 在主程式啟動 (DnsCaptiveModule)
//    - WebServer 提供：
//        GET /saveConfig  → 儲存 AP SSID / 密碼到 NVS
//        GET /*           → 顯示 AP 設定頁 (HtmlApSetupModule)
//
// 2. 已完成 AP 設定 (Ap_Config_Is_Done()==true)
//    - AP SSID = 使用者設定值
//    - WebServer 提供：
//        GET /            → 主頁 (HtmlMainPageModule)
//        GET /index.html  → 同 /
//        GET /stream      → MJPEG 串流
//        GET /set?res=..  → 修改解析度
//        GET /capture     → 拍照
//        GET /resetAp     → 清除 AP 設定，回預設 ESPNET (需手動重啟)
// ------------------------------------------------------

#include "WebServerModule.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "ApConfigModule.h"
#include "HtmlMainPageModule.h"
#include "HtmlApSetupModule.h"
#include <WiFi.h>
#include <Preferences.h>

// ------------------------------------------------------
// 串流用常數定義
// ------------------------------------------------------

#define Stream_Content_Type_Text_Value "multipart/x-mixed-replace;boundary=frame"
#define Stream_Boundary_Text_Value "\r\n--frame\r\n"
#define Stream_Part_Header_Format_Value "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n"

// STA 設定用 NVS key
#define Sta_Pref_Namespace_Text_Value      "esp_sta_cfg"
#define Sta_Pref_Key_Configured_Text_Value "sta_cfg_ok"
#define Sta_Pref_Key_Ssid_Text_Value       "sta_ssid"
#define Sta_Pref_Key_Pass_Text_Value       "sta_pass"

// ------------------------------------------------------
// 解析度對照表
// ------------------------------------------------------

typedef struct {
  const char *Name_Text_Value;
  framesize_t Frame_Size_Value;
} Resolution_Map_Item;

static const Resolution_Map_Item Resolution_Map_Table[] = {
  { "qqvga", FRAMESIZE_QQVGA },  // 160x120
  { "qvga", FRAMESIZE_QVGA },    // 320x240
  { "hvga", FRAMESIZE_HVGA },    // 480x320
  { "vga", FRAMESIZE_VGA },      // 640x480
  { "svga", FRAMESIZE_SVGA },    // 800x600
  { "xga", FRAMESIZE_XGA },      // 1024x768
  { "sxga", FRAMESIZE_SXGA },    // 1280x1024
  { "uxga", FRAMESIZE_UXGA }     // 1600x1200
};

#define Resolution_Count_Value (sizeof(Resolution_Map_Table) / sizeof(Resolution_Map_Table[0]))

// ------------------------------------------------------
// 工具函式：由名稱找到解析度索引
// ------------------------------------------------------

static int Find_FrameSize_Index(const char *Name_Text_Value) {
  for (int Index_Value = 0; Index_Value < (int)Resolution_Count_Value; Index_Value++) {
    if (strcmp(Name_Text_Value, Resolution_Map_Table[Index_Value].Name_Text_Value) == 0) {
      return Index_Value;
    }
  }
  return -1;
}

// ------------------------------------------------------
// Handler: MJPEG 串流 /stream
// ------------------------------------------------------

static esp_err_t Stream_Handler(httpd_req_t *Request_Ptr) {
  esp_err_t Result_Value = httpd_resp_set_type(Request_Ptr, Stream_Content_Type_Text_Value);
  if (Result_Value != ESP_OK) {
    return Result_Value;
  }

  camera_fb_t *Frame_Ptr = NULL;
  char Header_Buffer_Value[64];

  while (true) {
    Frame_Ptr = esp_camera_fb_get();
    if (!Frame_Ptr) {
      Serial.println("Camera capture failed");
      return ESP_FAIL;
    }

    // boundary
    Result_Value = httpd_resp_send_chunk(Request_Ptr,
                                         Stream_Boundary_Text_Value,
                                         strlen(Stream_Boundary_Text_Value));
    if (Result_Value != ESP_OK) {
      esp_camera_fb_return(Frame_Ptr);
      break;
    }

    // header
    size_t Header_Len_Value = snprintf(Header_Buffer_Value,
                                       sizeof(Header_Buffer_Value),
                                       Stream_Part_Header_Format_Value,
                                       Frame_Ptr->len);
    Result_Value = httpd_resp_send_chunk(Request_Ptr,
                                         Header_Buffer_Value,
                                         Header_Len_Value);
    if (Result_Value != ESP_OK) {
      esp_camera_fb_return(Frame_Ptr);
      break;
    }

    // payload
    Result_Value = httpd_resp_send_chunk(Request_Ptr,
                                         (const char *)Frame_Ptr->buf,
                                         Frame_Ptr->len);
    esp_camera_fb_return(Frame_Ptr);
    if (Result_Value != ESP_OK) {
      break;
    }
  }

  return Result_Value;
}

// ------------------------------------------------------
// Handler: 拍照 /capture
// ------------------------------------------------------

static esp_err_t Capture_Handler(httpd_req_t *Request_Ptr) {
  camera_fb_t *Frame_Ptr = esp_camera_fb_get();
  if (!Frame_Ptr) {
    httpd_resp_set_status(Request_Ptr, "500 Internal Server Error");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Capture failed");
    return ESP_OK;
  }

  httpd_resp_set_type(Request_Ptr, "image/jpeg");
  httpd_resp_set_hdr(Request_Ptr, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_send(Request_Ptr, (const char *)Frame_Ptr->buf, Frame_Ptr->len);

  esp_camera_fb_return(Frame_Ptr);
  return ESP_OK;
}

// ------------------------------------------------------
// Handler: 首頁 / 或任何路徑 (Index)
// 依 Ap_Config_Is_Done() 顯示不同 HTML
// ------------------------------------------------------

static esp_err_t Index_Handler(httpd_req_t *Request_Ptr) {
  httpd_resp_set_type(Request_Ptr, "text/html");

  if (!Ap_Config_Is_Done()) {
    // 尚未完成 AP 設定 → 顯示 AP 設定頁
    return httpd_resp_send(Request_Ptr,
                           Html_ApSetup_Page_Text_Value,
                           strlen(Html_ApSetup_Page_Text_Value));
  }

  // 已完成 AP 設定 → 顯示主控制頁
  return httpd_resp_send(Request_Ptr,
                         Html_Main_Page_Text_Value,
                         strlen(Html_Main_Page_Text_Value));
}

// ------------------------------------------------------
// Handler: /set?res=... 只處理解析度
// ------------------------------------------------------

static esp_err_t Set_Handler(httpd_req_t *Request_Ptr) {
  char Query_Buffer_Value[64];

  int Query_Len_Value = httpd_req_get_url_query_len(Request_Ptr) + 1;
  if (Query_Len_Value <= 1 || Query_Len_Value > (int)sizeof(Query_Buffer_Value)) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Bad query");
    return ESP_OK;
  }

  if (httpd_req_get_url_query_str(Request_Ptr,
                                  Query_Buffer_Value,
                                  Query_Len_Value)
      != ESP_OK) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "No query");
    return ESP_OK;
  }

  char Res_Buffer_Value[16];
  if (httpd_query_key_value(Query_Buffer_Value, "res",
                            Res_Buffer_Value, sizeof(Res_Buffer_Value))
      != ESP_OK) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Missing res");
    return ESP_OK;
  }

  // 轉成小寫，避免大小寫問題
  for (int i = 0; Res_Buffer_Value[i]; i++) {
    if (Res_Buffer_Value[i] >= 'A' && Res_Buffer_Value[i] <= 'Z') {
      Res_Buffer_Value[i] = Res_Buffer_Value[i] - 'A' + 'a';
    }
  }

  int Idx_Value = Find_FrameSize_Index(Res_Buffer_Value);
  if (Idx_Value < 0) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Invalid res");
    return ESP_OK;
  }

  sensor_t *Sensor_Ptr = esp_camera_sensor_get();
  if (!Sensor_Ptr) {
    httpd_resp_set_status(Request_Ptr, "500 Internal Server Error");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "No sensor");
    return ESP_OK;
  }

  Sensor_Ptr->set_framesize(Sensor_Ptr,
                            Resolution_Map_Table[Idx_Value].Frame_Size_Value);

  Serial.print("Resolution changed: ");
  Serial.println(Res_Buffer_Value);

  httpd_resp_set_status(Request_Ptr, "200 OK");
  httpd_resp_set_type(Request_Ptr, "text/plain");
  httpd_resp_sendstr(Request_Ptr, "OK");
  return ESP_OK;
}

// ------------------------------------------------------
// /saveConfig Handler：儲存 AP SSID / 密碼到 NVS
// 使用 GET /saveConfig?ssid=...&pass=...
// ------------------------------------------------------
static esp_err_t SaveConfig_Handler(httpd_req_t *Request_Ptr) {
  char Query_Buffer_Value[192];

  int Query_Len_Value = httpd_req_get_url_query_len(Request_Ptr) + 1;
  if (Query_Len_Value <= 1 || Query_Len_Value > (int)sizeof(Query_Buffer_Value)) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Bad query");
    return ESP_OK;
  }

  if (httpd_req_get_url_query_str(Request_Ptr,
                                  Query_Buffer_Value,
                                  Query_Len_Value)
      != ESP_OK) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "No query");
    return ESP_OK;
  }

  char Ssid_Buffer_Value[33];  // 32 + '\0'
  char Pass_Buffer_Value[65];  // 64 + '\0'

  if (httpd_query_key_value(Query_Buffer_Value, "ssid",
                            Ssid_Buffer_Value, sizeof(Ssid_Buffer_Value))
        != ESP_OK
      || httpd_query_key_value(Query_Buffer_Value, "pass",
                               Pass_Buffer_Value, sizeof(Pass_Buffer_Value))
           != ESP_OK) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Missing ssid or pass");
    return ESP_OK;
  }

  size_t Ssid_Len_Value = strlen(Ssid_Buffer_Value);
  size_t Pass_Len_Value = strlen(Pass_Buffer_Value);

  if (Ssid_Len_Value == 0 || Ssid_Len_Value > 32) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Invalid SSID length");
    return ESP_OK;
  }

  if (Pass_Len_Value < 8 || Pass_Len_Value > 64) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Invalid password length");
    return ESP_OK;
  }

  // 寫入 NVS
  bool Save_Result_Value = Ap_Save_Config_To_Nvs(Ssid_Buffer_Value,
                                                 Pass_Buffer_Value);
  if (!Save_Result_Value) {
    httpd_resp_set_status(Request_Ptr, "500 Internal Server Error");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Save failed");
    return ESP_OK;
  }

  Serial.print("AP config saved. New SSID: ");
  Serial.println(Ssid_Buffer_Value);

  // 取得目前 AP IP（一般是 192.168.4.1）
  IPAddress Ap_Ip_Value = WiFi.softAPIP();
  String Ap_Ip_Text_Value = Ap_Ip_Value.toString();

  // 組成功頁面 HTML，加入引導步驟與網址
  String Html_Text_Value;
  Html_Text_Value.reserve(800);

  Html_Text_Value = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  Html_Text_Value += "<title>AP Config Saved</title>";
  Html_Text_Value += "<style>";
  Html_Text_Value += "body{background:#111;color:#eee;font-family:Arial,Helvetica,sans-serif;text-align:center;}";
  Html_Text_Value += "h2{margin-top:20px;}";
  Html_Text_Value += "p,li{font-size:14px;}";
  Html_Text_Value += "a{color:#4da3ff;text-decoration:none;}";
  Html_Text_Value += "a:hover{text-decoration:underline;}";
  Html_Text_Value += ".box{margin-top:30px;display:inline-block;padding:20px 30px;border:1px solid #444;border-radius:6px;background:#181818;text-align:left;}";
  Html_Text_Value += "</style></head><body>";

  Html_Text_Value += "<h2>AP Config Saved</h2>";
  Html_Text_Value += "<div class='box'>";
  Html_Text_Value += "<p><b>New AP SSID:</b> ";
  Html_Text_Value += Ssid_Buffer_Value;
  Html_Text_Value += "</p>";

  Html_Text_Value += "<ol>";
  Html_Text_Value += "<li>Reboot ESP32.</li>";
  Html_Text_Value += "<li>Connect your phone/PC to WiFi SSID: <b>";
  Html_Text_Value += Ssid_Buffer_Value;
  Html_Text_Value += "</b>.</li>";
  Html_Text_Value += "<li>Open the streaming page:<br>";
  Html_Text_Value += "<a href='http://";
  Html_Text_Value += Ap_Ip_Text_Value;
  Html_Text_Value += "/'>http://";
  Html_Text_Value += Ap_Ip_Text_Value;
  Html_Text_Value += "/</a>";
  Html_Text_Value += "</li>";
  Html_Text_Value += "</ol>";

  Html_Text_Value += "<p style='margin-top:12px;font-size:12px;'>If the page does not open automatically, please enter the URL manually in your browser.</p>";
  Html_Text_Value += "</div>";

  Html_Text_Value += "</body></html>";

  httpd_resp_set_status(Request_Ptr, "200 OK");
  httpd_resp_set_type(Request_Ptr, "text/html");
  httpd_resp_sendstr(Request_Ptr, Html_Text_Value.c_str());

  return ESP_OK;
}

// ------------------------------------------------------
// /saveConnect Handler：儲存目標 WiFi (STA) SSID / 密碼
// 並嘗試連線，成功後在成功頁面顯示 STA IP
// GET /saveConnect?ssid=...&pass=...
// ------------------------------------------------------
static esp_err_t SaveConnect_Handler(httpd_req_t *Request_Ptr) {
  char Query_Buffer_Value[192];

  int Query_Len_Value = httpd_req_get_url_query_len(Request_Ptr) + 1;
  if (Query_Len_Value <= 1 || Query_Len_Value > (int)sizeof(Query_Buffer_Value)) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Bad query");
    return ESP_OK;
  }

  if (httpd_req_get_url_query_str(Request_Ptr,
                                  Query_Buffer_Value,
                                  Query_Len_Value) != ESP_OK) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "No query");
    return ESP_OK;
  }

  char Ssid_Buffer_Value[33];  // 32 + '\0'
  char Pass_Buffer_Value[65];  // 64 + '\0'

  if (httpd_query_key_value(Query_Buffer_Value, "ssid",
                            Ssid_Buffer_Value, sizeof(Ssid_Buffer_Value)) != ESP_OK ||
      httpd_query_key_value(Query_Buffer_Value, "pass",
                            Pass_Buffer_Value, sizeof(Pass_Buffer_Value)) != ESP_OK) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Missing ssid or pass");
    return ESP_OK;
  }

  size_t Ssid_Len_Value = strlen(Ssid_Buffer_Value);
  size_t Pass_Len_Value = strlen(Pass_Buffer_Value);

  if (Ssid_Len_Value == 0 || Ssid_Len_Value > 32) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Invalid SSID length");
    return ESP_OK;
  }

  if (Pass_Len_Value < 8 || Pass_Len_Value > 64) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Invalid password length");
    return ESP_OK;
  }

  // --------- 儲存 STA 設定到 NVS ---------
  Preferences Sta_Pref_Object;
  Sta_Pref_Object.begin(Sta_Pref_Namespace_Text_Value, false);
  Sta_Pref_Object.putString(Sta_Pref_Key_Ssid_Text_Value, Ssid_Buffer_Value);
  Sta_Pref_Object.putString(Sta_Pref_Key_Pass_Text_Value, Pass_Buffer_Value);
  Sta_Pref_Object.putBool(Sta_Pref_Key_Configured_Text_Value, true);
  Sta_Pref_Object.end();

  Serial.print("STA config saved. Target SSID: ");
  Serial.println(Ssid_Buffer_Value);

  // --------- 嘗試連線到目標 WiFi (STA) ---------
  // 目前 AP 已經在跑，這裡切成 AP+STA 模式
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(Ssid_Buffer_Value, Pass_Buffer_Value);

  Serial.print("Connecting to target WiFi (STA): ");
  Serial.println(Ssid_Buffer_Value);

  const uint32_t Connect_Timeout_Ms_Value = 10000;  // 最多等 10 秒
  uint32_t Start_Tick_Value = millis();
  bool Sta_Connected_Value = false;

  while (millis() - Start_Tick_Value < Connect_Timeout_Ms_Value) {
    if (WiFi.status() == WL_CONNECTED) {
      Sta_Connected_Value = true;
      break;
    }
    delay(200);
  }

  IPAddress Sta_Ip_Value(0,0,0,0);
  if (Sta_Connected_Value) {
    Sta_Ip_Value = WiFi.localIP();
    Serial.print("STA connected, IP: ");
    Serial.println(Sta_Ip_Value);
  } else {
    Serial.println("STA connect failed within timeout.");
  }

  // --------- 回應 HTML 成功 / 失敗頁面，並顯示目標網址 ---------
  String Html_Text_Value;
  Html_Text_Value.reserve(900);

  Html_Text_Value  = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  Html_Text_Value += "<title>Connect Config Saved</title>";
  Html_Text_Value += "<style>";
  Html_Text_Value += "body{background:#111;color:#eee;font-family:Arial,Helvetica,sans-serif;text-align:center;}";
  Html_Text_Value += "h2{margin-top:20px;}";
  Html_Text_Value += "p,li{font-size:14px;}";
  Html_Text_Value += "a{color:#4da3ff;text-decoration:none;}";
  Html_Text_Value += "a:hover{text-decoration:underline;}";
  Html_Text_Value += ".box{margin-top:30px;display:inline-block;padding:20px 30px;border:1px solid #444;border-radius:6px;background:#181818;text-align:left;}";
  Html_Text_Value += "</style></head><body>";

  Html_Text_Value += "<h2>Connect Config Saved</h2>";
  Html_Text_Value += "<div class='box'>";
  Html_Text_Value += "<p><b>Target WiFi SSID:</b> ";
  Html_Text_Value += Ssid_Buffer_Value;
  Html_Text_Value += "</p>";

  if (Sta_Connected_Value) {
    String Sta_Ip_Text_Value = Sta_Ip_Value.toString();

    Html_Text_Value += "<ol>";
    Html_Text_Value += "<li>Change your phone/PC WiFi to SSID: <b>";
    Html_Text_Value += Ssid_Buffer_Value;
    Html_Text_Value += "</b>.</li>";
    Html_Text_Value += "<li>Open the streaming page:<br>";
    Html_Text_Value += "<a href='http://";
    Html_Text_Value += Sta_Ip_Text_Value;
    Html_Text_Value += "/'>http://";
    Html_Text_Value += Sta_Ip_Text_Value;
    Html_Text_Value += "/</a>";
    Html_Text_Value += "</li>";
    Html_Text_Value += "</ol>";

    Html_Text_Value += "<p style='margin-top:12px;font-size:12px;'>If the page does not open automatically, please enter the URL manually in your browser.</p>";
  } else {
    Html_Text_Value += "<p>ESP32 tried to connect to the target WiFi but failed within timeout.</p>";
    Html_Text_Value += "<p>Please check SSID/password or signal quality, then try again.</p>";
  }

  Html_Text_Value += "</div>";
  Html_Text_Value += "</body></html>";

  httpd_resp_set_status(Request_Ptr, "200 OK");
  httpd_resp_set_type(Request_Ptr, "text/html");
  httpd_resp_sendstr(Request_Ptr, Html_Text_Value.c_str());

  return ESP_OK;
}


// ------------------------------------------------------
// Handler: /resetAp 清除 AP 設定，恢復預設 ESPNET
// ------------------------------------------------------
static esp_err_t ResetAp_Handler(httpd_req_t *Request_Ptr) {
  // 呼叫 ApConfigModule 裡的 Ap_Reset_Config_To_Default()
  bool Result_Value = Ap_Reset_Config_To_Default();

  httpd_resp_set_type(Request_Ptr, "text/plain");

  if (!Result_Value) {
    httpd_resp_set_status(Request_Ptr, "500 Internal Server Error");
    httpd_resp_sendstr(Request_Ptr, "Reset failed.");
    return ESP_OK;
  }

  httpd_resp_set_status(Request_Ptr, "200 OK");
  httpd_resp_sendstr(Request_Ptr,
                     "AP config reset to default (ESPNET).\nPlease reboot ESP32.");
  return ESP_OK;
}

// ------------------------------------------------------
// WebServer_Start：依 AP 設定狀態註冊不同路由
// ------------------------------------------------------
void WebServer_Start() {
  httpd_config_t Config_Value = HTTPD_DEFAULT_CONFIG();
  Config_Value.server_port = 80;
  Config_Value.uri_match_fn = httpd_uri_match_wildcard;

  httpd_handle_t Server_Handle_Value = NULL;
  esp_err_t Result_Value = httpd_start(&Server_Handle_Value, &Config_Value);
  if (Result_Value != ESP_OK) {
    Serial.printf("Error starting server: 0x%x\n", Result_Value);
    return;
  }

// ---------- 尚未完成 AP 設定：CAPTIVE 模式 ----------
if (!Ap_Config_Is_Done()) {
  // 1) /saveConfig (AP 模式用)
  httpd_uri_t SaveConfig_Uri_Value = {
    .uri       = "/saveConfig",
    .method    = HTTP_GET,
    .handler   = SaveConfig_Handler,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &SaveConfig_Uri_Value);

  // 2) /saveConnect (STA 連線設定用)
  httpd_uri_t SaveConnect_Uri_Value = {
    .uri       = "/saveConnect",
    .method    = HTTP_GET,
    .handler   = SaveConnect_Handler,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &SaveConnect_Uri_Value);

  // 3) 其它所有 GET → 設定頁
  httpd_uri_t Any_Uri_Value = {
    .uri       = "/*",
    .method    = HTTP_GET,
    .handler   = Index_Handler,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &Any_Uri_Value);

  Serial.println("HTTP server started in CAPTIVE mode.");
  return;
}


  // ---------- 已完成 AP 設定：NORMAL 模式 ----------
  httpd_uri_t Index_Uri_Value = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = Index_Handler,
    .user_ctx = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &Index_Uri_Value);

  httpd_uri_t Index_Html_Uri_Value = {
    .uri = "/index.html",
    .method = HTTP_GET,
    .handler = Index_Handler,
    .user_ctx = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &Index_Html_Uri_Value);

  httpd_uri_t Stream_Uri_Value = {
    .uri = "/stream",
    .method = HTTP_GET,
    .handler = Stream_Handler,
    .user_ctx = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &Stream_Uri_Value);

  httpd_uri_t Set_Uri_Value = {
    .uri = "/set",
    .method = HTTP_GET,
    .handler = Set_Handler,
    .user_ctx = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &Set_Uri_Value);

  httpd_uri_t Capture_Uri_Value = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = Capture_Handler,
    .user_ctx = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &Capture_Uri_Value);

  httpd_uri_t ResetAp_Uri_Value = {
    .uri = "/resetAp",
    .method = HTTP_GET,
    .handler = ResetAp_Handler,
    .user_ctx = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &ResetAp_Uri_Value);

  Serial.println("HTTP server started in NORMAL mode.");
}
