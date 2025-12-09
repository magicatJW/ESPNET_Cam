// ------------------------------------------------------
// WebServerModule.cpp
// CATNET-CAM + AP 初次設定 + Captive Portal
// 版本說明：
//   * 首次啟動 (Ap_Config_Is_Done()==false)
//       - DNS Captive 由 DnsCaptiveModule 負責
//       - WebServer 只註冊 /* → AP 設定頁 + /saveConfig
//   * 完成設定後 (Ap_Config_Is_Done()==true)
//       - WebServer 提供：
//           /           → 主頁 (Start/Stop Stream + Resolution + Reset AP)
//           /index.html → 同 /
//           /stream     → MJPEG 串流
//           /set        → 僅解析 res 參數 (解析度)
//           /capture    → 拍照
//           /resetAp    → 清除 AP 設定，回到預設 (ESPNET)
// ------------------------------------------------------

#include "WebServerModule.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "ApConfigModule.h"
#include "HtmlMainPageModule.h"
#include "HtmlApSetupModule.h"

// ---------- MJPEG 串流常數 ----------
#define Stream_Content_Type_Text_Value   "multipart/x-mixed-replace;boundary=frame"
#define Stream_Boundary_Text_Value       "\r\n--frame\r\n"
#define Stream_Part_Header_Format_Value  "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n"

// ---------- 解析度對照表 ----------
typedef struct {
  const char *Name_Text_Value;
  framesize_t Frame_Size_Value;
} Resolution_Map_Item;

static const Resolution_Map_Item Resolution_Map_Table[] = {
  { "qqvga", FRAMESIZE_QQVGA },  // 160x120
  { "qvga",  FRAMESIZE_QVGA  },  // 320x240
  { "hvga",  FRAMESIZE_HVGA  },  // 480x320
  { "vga",   FRAMESIZE_VGA   },  // 640x480
  { "svga",  FRAMESIZE_SVGA  },  // 800x600
  { "xga",   FRAMESIZE_XGA   },  // 1024x768
  { "sxga",  FRAMESIZE_SXGA  },  // 1280x1024
  { "uxga",  FRAMESIZE_UXGA  }   // 1600x1200
};

#define Resolution_Count_Value  (sizeof(Resolution_Map_Table) / sizeof(Resolution_Map_Table[0]))

// ------------------------------------------------------
// 工具：找解析度索引
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
// MJPEG 串流 Handler (/stream)
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

    Result_Value = httpd_resp_send_chunk(Request_Ptr,
                                         Stream_Boundary_Text_Value,
                                         strlen(Stream_Boundary_Text_Value));
    if (Result_Value != ESP_OK) {
      esp_camera_fb_return(Frame_Ptr);
      break;
    }

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
// 拍照 Handler (/capture)
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
// 首頁 Handler：依 Ap_Config_Is_Done() 顯示不同 HTML
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
// /set Handler：僅支援 res=... (解析度)
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
                                  Query_Len_Value) != ESP_OK) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "No query");
    return ESP_OK;
  }

  char Res_Buffer_Value[16];
  if (httpd_query_key_value(Query_Buffer_Value, "res",
                            Res_Buffer_Value, sizeof(Res_Buffer_Value)) != ESP_OK) {
    httpd_resp_set_status(Request_Ptr, "400 Bad Request");
    httpd_resp_set_type(Request_Ptr, "text/plain");
    httpd_resp_sendstr(Request_Ptr, "Missing res");
    return ESP_OK;
  }

  // 把大寫轉小寫，方便比較
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
// /saveConfig Handler：儲存 AP SSID / 密碼不變，略
// 這裡假設你已經有 SaveConfig_Handler 實作，保持不動
// ------------------------------------------------------
static esp_err_t SaveConfig_Handler(httpd_req_t *Request_Ptr) {
  // ... 你之前那個版本即可（呼叫 Ap_Save_Config_To_Nvs）
  // 這裡不重貼，重點是 /resetAp 在下面
  // （如果你需要我也可以幫你補成新版）
  return ESP_OK;
}

// ------------------------------------------------------
// /resetAp Handler：清除 AP 設定，恢復預設 (ESPNET)
// ------------------------------------------------------
static esp_err_t ResetAp_Handler(httpd_req_t *Request_Ptr) {
  bool Result_Value = Ap_Reset_Config_To_Default();

  httpd_resp_set_type(Request_Ptr, "text/plain");
  if (!Result_Value) {
    httpd_resp_set_status(Request_Ptr, "500 Internal Server Error");
    httpd_resp_sendstr(Request_Ptr, "Reset failed.");
    return ESP_OK;
  }

  // 提醒使用者需要重啟 ESP32 才會再次啟動 ESPNET + Captive Portal
  httpd_resp_set_status(Request_Ptr, "200 OK");
  httpd_resp_sendstr(Request_Ptr,
                     "AP config reset to default (ESPNET).\nPlease reboot ESP32.");
  return ESP_OK;
}

// ------------------------------------------------------
// WebServer_Start：依「是否已設定」決定註冊哪些路由
// ------------------------------------------------------
void WebServer_Start() {
  httpd_config_t Config_Value = HTTPD_DEFAULT_CONFIG();
  Config_Value.server_port  = 80;
  Config_Value.uri_match_fn = httpd_uri_match_wildcard;

  httpd_handle_t Server_Handle_Value = NULL;
  esp_err_t Result_Value = httpd_start(&Server_Handle_Value, &Config_Value);
  if (Result_Value != ESP_OK) {
    Serial.printf("Error starting server: 0x%x\n", Result_Value);
    return;
  }

  // ---------- 尚未完成 AP 設定：Captive 模式 ----------
  if (!Ap_Config_Is_Done()) {
    httpd_uri_t Any_Uri_Value = {
      .uri       = "/*",
      .method    = HTTP_GET,
      .handler   = Index_Handler,     // 一律顯示 AP 設定頁
      .user_ctx  = NULL
    };
    httpd_register_uri_handler(Server_Handle_Value, &Any_Uri_Value);

    httpd_uri_t SaveConfig_Uri_Value = {
      .uri       = "/saveConfig",
      .method    = HTTP_GET,         // 或 POST，看你表單
      .handler   = SaveConfig_Handler,
      .user_ctx  = NULL
    };
    httpd_register_uri_handler(Server_Handle_Value, &SaveConfig_Uri_Value);

    Serial.println("HTTP server started in CAPTIVE mode.");
    return;
  }

  // ---------- 已完成 AP 設定：正常控制模式 ----------
  httpd_uri_t Index_Uri_Value = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = Index_Handler,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &Index_Uri_Value);

  httpd_uri_t Index_Html_Uri_Value = {
    .uri       = "/index.html",
    .method    = HTTP_GET,
    .handler   = Index_Handler,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &Index_Html_Uri_Value);

  httpd_uri_t Stream_Uri_Value = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = Stream_Handler,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &Stream_Uri_Value);

  httpd_uri_t Set_Uri_Value = {
    .uri       = "/set",
    .method    = HTTP_GET,
    .handler   = Set_Handler,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &Set_Uri_Value);

  httpd_uri_t Capture_Uri_Value = {
    .uri       = "/capture",
    .method    = HTTP_GET,
    .handler   = Capture_Handler,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &Capture_Uri_Value);

  httpd_uri_t ResetAp_Uri_Value = {
    .uri       = "/resetAp",
    .method    = HTTP_GET,
    .handler   = ResetAp_Handler,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(Server_Handle_Value, &ResetAp_Uri_Value);

  Serial.println("HTTP server started in NORMAL mode.");
}
