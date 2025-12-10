#include "HtmlApSetupModule.h"

const char Html_ApSetup_Page_Text_Value[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESPNET - AP Setup</title>
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
      margin-top:10px;
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
  </style>
</head>
<body>
  <h1>ESPNET - AP Setup</h1>
  <div class="box">
    <form action="/saveConfig" method="GET">
      <label>AP SSID</label><br>
      <input type="text" name="ssid" maxlength="32" required><br>
      <label>AP Password (8~64 chars)</label><br>
      <input type="password" name="pass" minlength="8" maxlength="64" required><br>
      <button type="submit">Save Config</button>
    </form>
    <p style="margin-top:12px; font-size:12px;">
      After saving, please reboot ESP32 and connect using the new SSID / password.
    </p>
  </div>
</body>
</html>
)rawliteral";
