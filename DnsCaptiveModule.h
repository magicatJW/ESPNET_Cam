// ------------------------------------------------------
// DnsCaptiveModule.h
// Captive Portal 用 DNS 模組
// 功能：
//   * 啟動 DNS 伺服器，所有網域都指向指定 AP IP
//   * 在 loop 中呼叫 Process 函式，處理 DNS 封包
// ------------------------------------------------------
#ifndef DNS_CAPTIVE_MODULE_H
#define DNS_CAPTIVE_MODULE_H

#include <Arduino.h>
#include <WiFi.h>      // 為了 IPAddress 型別

// 啟動 Captive DNS：
//   Ap_Ip_Value = WiFi.softAPIP()
//   回傳 true 表示啟動成功
bool Dns_Captive_Start(IPAddress Ap_Ip_Value);

// 在 loop 中呼叫，處理 DNS 封包
void Dns_Captive_Process();

// 回傳目前 DNS 是否有啟動
bool Dns_Captive_Is_Running();

#endif  // DNS_CAPTIVE_MODULE_H
