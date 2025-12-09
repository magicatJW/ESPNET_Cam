// ------------------------------------------------------
// ApConfigModule.cpp
// ESP32 AP 模式設定 + NVS 儲存
// ------------------------------------------------------

#include "ApConfigModule.h"
#include <WiFi.h>
#include <Preferences.h>

// ------------------------------------------------------
// #define：集中管理
// ------------------------------------------------------
#define Ap_Pref_Namespace_Text_Value         "esp_ap_cfg"
#define Ap_Pref_Key_Configured_Text_Value    "ap_cfg_ok"
#define Ap_Pref_Key_Ssid_Text_Value          "ap_ssid"
#define Ap_Pref_Key_Pass_Text_Value          "ap_pass"

#define Ap_Default_Ssid_Text_Value           "ESPNET"
#define Ap_Default_Pass_Text_Value           "12345678"  // 至少 8 碼

// ------------------------------------------------------
// 靜態物件與變數
// ------------------------------------------------------
static Preferences   Ap_Pref_Object;
static bool          Ap_Config_Is_Done_Value = false;
static String        Ap_Ssid_String_Value;
static String        Ap_Pass_String_Value;

// ------------------------------------------------------
void Ap_Config_Load_From_Nvs() {
  Ap_Pref_Object.begin(Ap_Pref_Namespace_Text_Value, false);

  bool Config_Flag_Value = Ap_Pref_Object.getBool(
      Ap_Pref_Key_Configured_Text_Value, false);

  if (Config_Flag_Value) {
    String Saved_Ssid_Value = Ap_Pref_Object.getString(
        Ap_Pref_Key_Ssid_Text_Value, Ap_Default_Ssid_Text_Value);
    String Saved_Pass_Value = Ap_Pref_Object.getString(
        Ap_Pref_Key_Pass_Text_Value, Ap_Default_Pass_Text_Value);

    Ap_Ssid_String_Value       = Saved_Ssid_Value;
    Ap_Pass_String_Value       = Saved_Pass_Value;
    Ap_Config_Is_Done_Value    = true;

    Serial.println("AP config found in NVS.");
    Serial.print("AP SSID: ");
    Serial.println(Ap_Ssid_String_Value);
  } else {
    Ap_Ssid_String_Value       = Ap_Default_Ssid_Text_Value;
    Ap_Pass_String_Value       = Ap_Default_Pass_Text_Value;
    Ap_Config_Is_Done_Value    = false;

    Serial.println("No AP config in NVS, use default.");
    Serial.print("Default AP SSID: ");
    Serial.println(Ap_Ssid_String_Value);
  }
}

// ------------------------------------------------------
bool Ap_Start_Access_Point() {
  WiFi.mode(WIFI_AP);

  bool Ap_Result_Value = WiFi.softAP(
      Ap_Ssid_String_Value.c_str(),
      Ap_Pass_String_Value.c_str()
  );

  if (!Ap_Result_Value) {
    Serial.println("AP start failed.");
    return false;
  }

  IPAddress Ap_Ip_Value = WiFi.softAPIP();
  Serial.print("AP started. SSID: ");
  Serial.println(Ap_Ssid_String_Value);
  Serial.print("AP IP: ");
  Serial.println(Ap_Ip_Value);

  return true;
}

// ------------------------------------------------------
bool Ap_Save_Config_To_Nvs(const char *New_Ssid_Text_Ptr,
                           const char *New_Pass_Text_Ptr) {
  if (New_Ssid_Text_Ptr == nullptr || New_Pass_Text_Ptr == nullptr) {
    Serial.println("Ap_Save_Config_To_Nvs: null pointer.");
    return false;
  }

  size_t Ssid_Len_Value = strlen(New_Ssid_Text_Ptr);
  size_t Pass_Len_Value = strlen(New_Pass_Text_Ptr);

  if (Ssid_Len_Value == 0 || Ssid_Len_Value > 32) {
    Serial.println("Ap_Save_Config_To_Nvs: invalid SSID length.");
    return false;
  }
  if (Pass_Len_Value < 8 || Pass_Len_Value > 64) {
    Serial.println("Ap_Save_Config_To_Nvs: invalid password length.");
    return false;
  }

  Ap_Pref_Object.putString(Ap_Pref_Key_Ssid_Text_Value, New_Ssid_Text_Ptr);
  Ap_Pref_Object.putString(Ap_Pref_Key_Pass_Text_Value, New_Pass_Text_Ptr);
  Ap_Pref_Object.putBool(Ap_Pref_Key_Configured_Text_Value, true);

  Ap_Ssid_String_Value    = New_Ssid_Text_Ptr;
  Ap_Pass_String_Value    = New_Pass_Text_Ptr;
  Ap_Config_Is_Done_Value = true;

  Serial.println("AP config saved to NVS.");
  Serial.print("New AP SSID: ");
  Serial.println(Ap_Ssid_String_Value);

  return true;
}

// ------------------------------------------------------
bool Ap_Config_Is_Done() {
  return Ap_Config_Is_Done_Value;
}

// ------------------------------------------------------
const char *Ap_Get_Current_Ssid_Text_Ptr() {
  return Ap_Ssid_String_Value.c_str();
}

// ------------------------------------------------------
// 新增：重設 AP 設定為預設值
// ------------------------------------------------------
bool Ap_Reset_Config_To_Default() {
  // 清空此命名空間所有 key
  Ap_Pref_Object.clear();

  // 邏輯上改回「未設定狀態」
  Ap_Ssid_String_Value       = Ap_Default_Ssid_Text_Value;
  Ap_Pass_String_Value       = Ap_Default_Pass_Text_Value;
  Ap_Config_Is_Done_Value    = false;

  Serial.println("AP config reset to default.");
  Serial.print("AP SSID (default): ");
  Serial.println(Ap_Ssid_String_Value);

  // 注意：目前已啟動的 AP 不會自動切回 ESPNET，需重新啟動 ESP32
  return true;
}
