// ------------------------------------------------------
// HtmlApSetupModule.cpp
// AP / STA 設定頁面 HTML
// ------------------------------------------------------

#include "HtmlApSetupModule.h"

const char Html_ApSetup_Page_Text_Value[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESPNET - WiFi Setup</title>
  <style>
    body { background:#111; color:#eee; font-family:Arial, Helvetica, sans-serif; text-align:center; }
    h1 { margin-top:20px; }
    input {
      margin:6px;
      padding:6px 10px;
      font-size:14px;
      border-radius:4px;
      border:1px solid #555;
      background:#222;
      color:#eee;
      width:220px;
    }
    button {
      margin:6px;
      padding:6px 12px;
      font-size:14px;
      border-radius:4px;
      border:1px solid #555;
      background:#222;
      color:#eee;
      cursor:pointer;
    }
    button:hover { background:#333; }
    .box {
      margin-top:40px;
      display:inline-block;
      padding:20px 30px;
      border:1px solid #444;
      border-radius:6px;
      background:#181818;
      text-align:left;
    }
    label { font-size:14px; }
    .desc { font-size:12px; color:#aaa; margin-bottom:10px; }
  </style>
</head>
<body>
  <h1>ESPNET - WiFi Setup</h1>
  <div class="box">
    <p class="desc">
      Step 1: Connect to <b>ESPNET</b> first.<br>
      Step 2: Enter the WiFi SSID and password you want to use.<br>
      Step 3: Choose to run ESP32 as an Access Point or connect to an existing WiFi.
    </p>
    <form onsubmit="return false;">
      <label>Target SSID</label><br>
      <input type="text" id="ssid" maxlength="32" required><br>
      <label>Password (8~64 chars)</label><br>
      <input type="password" id="pass" minlength="8" maxlength="64" required><br><br>

      <button type="button" onclick="saveApConfig()">AP Config Save</button>
      <button type="button" onclick="saveConnectConfig()">Connect Config Save</button>
    </form>
  </div>

  <script>
    function getValues() {
      var ssid = document.getElementById('ssid').value.trim();
      var pass = document.getElementById('pass').value.trim();

      if (ssid.length === 0 || ssid.length > 32) {
        alert('Invalid SSID length (1~32).');
        return null;
      }
      if (pass.length < 8 || pass.length > 64) {
        alert('Invalid password length (8~64).');
        return null;
      }
      return { ssid: ssid, pass: pass };
    }

    function saveApConfig() {
      var v = getValues();
      if (!v) return;

      var url = '/saveConfig?ssid=' + encodeURIComponent(v.ssid) +
                '&pass=' + encodeURIComponent(v.pass);
      window.location.href = url;
    }

    function saveConnectConfig() {
      var v = getValues();
      if (!v) return;

      var url = '/saveConnect?ssid=' + encodeURIComponent(v.ssid) +
                '&pass=' + encodeURIComponent(v.pass);
      window.location.href = url;
    }
  </script>
</body>
</html>
)rawliteral";
