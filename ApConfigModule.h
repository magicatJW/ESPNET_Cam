// ------------------------------------------------------
// ApConfigModule.h
// ESP32 AP 模式設定 + NVS 儲存
// ------------------------------------------------------
#ifndef AP_CONFIG_MODULE_H
#define AP_CONFIG_MODULE_H

#include <Arduino.h>

// 從 NVS 讀取 AP 設定
void Ap_Config_Load_From_Nvs();

// 啟動 AP 模式
bool Ap_Start_Access_Point();

// 儲存新的 AP 設定到 NVS
bool Ap_Save_Config_To_Nvs(const char *New_Ssid_Text_Ptr,
                           const char *New_Pass_Text_Ptr);

// 是否已完成 AP 設定
bool Ap_Config_Is_Done();

// 取得目前 AP SSID（除錯用）
const char *Ap_Get_Current_Ssid_Text_Ptr();

// **新功能**：重設 AP 設定為「未設定狀態」並回到預設 SSID / 密碼
// 僅影響 NVS 與程式中的變數，已啟動的 AP 仍維持原本 SSID，
// 需在 UI 提示使用者「請重新啟動 ESP32」。
bool Ap_Reset_Config_To_Default();

#endif  // AP_CONFIG_MODULE_H
