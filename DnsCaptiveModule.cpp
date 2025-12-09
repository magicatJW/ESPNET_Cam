// ------------------------------------------------------
// DnsCaptiveModule.cpp
// Captive Portal 用 DNS 模組實作
// ------------------------------------------------------

#include "DnsCaptiveModule.h"
#include <DNSServer.h>

// DNS 監聽 Port
#define Dns_Server_Port_Value   53

// ------------------------------------------------------
// 靜態物件與變數
// ------------------------------------------------------

static DNSServer  Dns_Server_Object;
static bool       Dns_Captive_Running_Value = false;

// ------------------------------------------------------
// Dns_Captive_Start
// ------------------------------------------------------
bool Dns_Captive_Start(IPAddress Ap_Ip_Value) {
  // 若已啟動就不用再啟
  if (Dns_Captive_Running_Value) {
    return true;
  }

  // 所有網域都導到 Ap_Ip_Value
  // "*" 代表 wildcard domain
  bool Start_Result_Value = Dns_Server_Object.start(
      Dns_Server_Port_Value, "*", Ap_Ip_Value);

  if (!Start_Result_Value) {
    Serial.println("Dns_Captive_Start: DNS start failed.");
    Dns_Captive_Running_Value = false;
    return false;
  }

  Dns_Captive_Running_Value = true;

  Serial.print("DNS Captive started. All domains -> ");
  Serial.println(Ap_Ip_Value);

  return true;
}

// ------------------------------------------------------
// Dns_Captive_Process
// ------------------------------------------------------
void Dns_Captive_Process() {
  if (!Dns_Captive_Running_Value) {
    return;
  }

  // 處理來自手機 / PC 的 DNS Query
  Dns_Server_Object.processNextRequest();
}

// ------------------------------------------------------
// Dns_Captive_Is_Running
// ------------------------------------------------------
bool Dns_Captive_Is_Running() {
  return Dns_Captive_Running_Value;
}
