// ------------------------------------------------------
// HtmlMainPageModule.cpp
// CATNET-CAM 主控制頁面 HTML 內容
// 已精簡：
//   * Start / Stop Stream 按鈕
//   * 畫質選單 (Resolution)
//   * 恢復預設 AP 按鈕（呼叫 /resetAp）
// ------------------------------------------------------

#include "HtmlMainPageModule.h"

const char Html_Main_Page_Text_Value[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>CATNET-CAM</title>
  <style>
    body { background:#111; color:#eee; text-align:center; font-family:Arial, Helvetica, sans-serif; }
    h1 { margin-top:20px; }
    #stream { margin-top:20px; border:2px solid #444; max-width:100%%; }
    button, select {
      margin:8px;
      padding:8px 14px;
      font-size:14px;
      border-radius:4px;
      border:1px solid #555;
      background:#222;
      color:#eee;
      cursor:pointer;
    }
    button:hover, select:hover {
      background:#333;
    }
    .control-bar { margin-top:15px; }
    .label { margin:0 4px; }
  </style>
</head>
<body>
  <h1>CATNET-CAM</h1>

  <div class="control-bar">
    <button id="StreamToggleBtn" onclick="toggleStream()">Start Stream</button>
  </div>

  <div class="control-bar">
    <span class="label">Resolution:</span>
    <select id="ResSelect" onchange="changeRes()">
  <option value="qqvga">QQVGA 160x120</option>
  <option value="qvga" selected>QVGA 320x240</option>
  <option value="hvga">HVGA 480x320</option>
  <option value="vga">VGA 640x480</option>
  <option value="svga">SVGA 800x600</option>
  <option value="xga">XGA 1024x768</option>
  <option value="sxga">SXGA 1280x1024</option>
  <option value="uxga">UXGA 1600x1200</option>
</select>

  </div>

  <div class="control-bar">
    <button onclick="resetAp()">Reset AP to default (ESPNET)</button>
  </div>

  <img id="stream" src="" />

  <script>
    var streaming = false;

    function toggleStream() {
      var img = document.getElementById('stream');
      var btn = document.getElementById('StreamToggleBtn');

      if (!streaming) {
        img.src = '/stream';
        btn.textContent = 'Stop Stream';
        streaming = true;
      } else {
        img.src = '';
        btn.textContent = 'Start Stream';
        streaming = false;
      }
    }

    function changeRes() {
      var r = document.getElementById('ResSelect').value;
      fetch('/set?res=' + encodeURIComponent(r));
    }

    function resetAp() {
      if (!confirm('Reset AP config to default (ESPNET)?\\nDevice will need to be rebooted.')) {
        return;
      }
      fetch('/resetAp')
        .then(function(response) { return response.text(); })
        .then(function(text) {
          alert(text);
        })
        .catch(function(err) {
          alert('Reset failed: ' + err);
        });
    }
  </script>
</body>
</html>
)rawliteral";
