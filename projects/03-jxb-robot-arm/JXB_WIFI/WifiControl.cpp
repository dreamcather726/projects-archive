#include "WifiControl.h"

#include <WiFi.h>
#include <WebServer.h>

static WebServer server(80);

static const char* wifiSsid = "";
static const char* wifiPassword = "";
static int servoCount = 0;
static int* currentAngles = nullptr;
static int* targetAngles = nullptr;
static const int* presetA = nullptr;
static const int* presetB = nullptr;
static bool* wifiStopFlag = nullptr;

static void waitWifiConnected();
static void printControlCommands();
static void handleRoot();
static void handleSetServo();
static void handleMoveAll();
static void handlePreset();
static void handleStop();
static void handleRun();
static void handleStatus();
static void handleNotFound();
static void copyTarget(const int angles[]);
static bool isValidChannel(int ch);

void startWifiControl(
  const char* ssid,
  const char* password,
  int count,
  int current[],
  int target[],
  const int positionA[],
  const int positionB[],
  bool* stopFlag
) {
  wifiSsid = ssid;
  wifiPassword = password;
  servoCount = count;
  currentAngles = current;
  targetAngles = target;
  presetA = positionA;
  presetB = positionB;
  wifiStopFlag = stopFlag;

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPassword);
  waitWifiConnected();

  Serial.print("WiFi SSID: ");
  Serial.println(wifiSsid);
  Serial.print("Control page: http://");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/set", HTTP_GET, handleSetServo);
  server.on("/move", HTTP_GET, handleMoveAll);
  server.on("/preset", HTTP_GET, handlePreset);
  server.on("/stop", HTTP_GET, handleStop);
  server.on("/run", HTTP_GET, handleRun);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  printControlCommands();
}

void handleWifiControlClient() {
  server.handleClient();
}

static void waitWifiConnected() {
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
}

static void printControlCommands() {
  IPAddress ip = WiFi.localIP();

  Serial.println();
  Serial.println("===== JXB WiFi Control Commands =====");
  Serial.println("Open control page:");
  Serial.print("http://");
  Serial.print(ip);
  Serial.println("/");
  Serial.println();
  Serial.println("Set one servo:");
  Serial.print("http://");
  Serial.print(ip);
  Serial.println("/set?ch=0&angle=90");
  Serial.println();
  Serial.println("Move all servos:");
  Serial.print("http://");
  Serial.print(ip);
  Serial.println("/move?s0=90&s1=0&s2=180&s3=130");
  Serial.println();
  Serial.println("Preset:");
  Serial.print("http://");
  Serial.print(ip);
  Serial.println("/preset?name=A");
  Serial.print("http://");
  Serial.print(ip);
  Serial.println("/preset?name=B");
  Serial.println();
  Serial.println("Stop:");
  Serial.print("http://");
  Serial.print(ip);
  Serial.println("/stop");
  Serial.println();
  Serial.println("Run:");
  Serial.print("http://");
  Serial.print(ip);
  Serial.println("/run");
  Serial.println();
  Serial.println("Status:");
  Serial.print("http://");
  Serial.print(ip);
  Serial.println("/status");
  Serial.println("=====================================");
  Serial.println();
}

static void handleRoot() {
  String html =
    "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>JXB Servo Panel</title>"
    "<style>"
    "*{box-sizing:border-box}body{font-family:Arial,sans-serif;margin:0;background:#eef2f5;color:#1c2733}"
    ".top{background:#17212b;color:white;padding:18px 20px}.wrap{max-width:620px;margin:0 auto}"
    "h1{font-size:24px;line-height:1.2;margin:0}.sub{font-size:13px;color:#b8c4d0;margin-top:6px}"
    "main{max-width:620px;margin:18px auto;padding:0 14px 24px}.panel{background:white;border:1px solid #d6dee7;border-radius:8px;padding:16px;margin-bottom:14px}"
    ".servo{display:grid;grid-template-columns:88px 1fr 56px;gap:12px;align-items:center;padding:12px 0;border-top:1px solid #edf1f5}"
    ".servo:first-child{border-top:0}.name{font-weight:700}.value{font-variant-numeric:tabular-nums;text-align:right;background:#eef4fa;border-radius:6px;padding:7px 8px;color:#1769aa}"
    "input[type=range]{width:100%;accent-color:#1769aa}.actions{display:grid;grid-template-columns:1fr 1fr;gap:10px}"
    "button{height:44px;border:0;border-radius:7px;background:#1769aa;color:white;font-weight:700;font-size:14px}"
    "button.alt{background:#2f7d59}button.stop{background:#b53b35}button.run{background:#53616f}"
    ".status{font-size:13px;line-height:1.55;color:#41505f;white-space:pre-wrap;background:#f7f9fb;border:1px solid #e1e7ee;border-radius:7px;padding:12px;margin-top:12px}"
    "@media(max-width:420px){.servo{grid-template-columns:1fr 54px}.servo input{grid-column:1/3}.name{grid-column:1/2}.value{grid-column:2/3}.actions{grid-template-columns:1fr}}"
    "</style></head><body><header class='top'><div class='wrap'><h1>JXB Servo Panel</h1><div class='sub'>WiFi control ready</div></div></header><main><section class='panel'>";

  for (int i = 0; i < servoCount; i++) {
    html += "<div class='servo'><div class='name'>Servo ";
    html += String(i);
    html += "</div><input id='s";
    html += String(i);
    html += "' type='range' min='0' max='180' value='";
    html += String(targetAngles[i]);
    html += "' oninput='v";
    html += String(i);
    html += ".textContent=this.value' onchange='setServo(";
    html += String(i);
    html += ",this.value)'><div class='value'><span id='v";
    html += String(i);
    html += "'>";
    html += String(targetAngles[i]);
    html += "</span> </div></div>";
  }

  html +=
    "</section><section class='panel'><div class='actions'>"
    "<button onclick=\"preset('A')\">Position A</button>"
    "<button onclick=\"preset('B')\">Position B</button>"
    "<button class='run' onclick='run()'>Run</button>"
    "<button class='stop' onclick='stop()'>Stop</button>"
    "</div><div id='status' class='status'>Loading status...</div></section>"
    "<script>"
    "function get(u){return fetch(u).then(r=>r.text()).then(t=>status.textContent=t)}"
    "function setServo(ch,angle){get('/set?ch='+ch+'&angle='+angle)}"
    "function preset(name){get('/preset?name='+name);setTimeout(refresh,200)}"
    "function stop(){get('/stop')}"
    "function run(){get('/run')}"
    "function refresh(){fetch('/status').then(r=>r.json()).then(s=>{"
    "status.textContent='current: '+s.current.join(', ')+'\\ntarget: '+s.target.join(', ')+'\\nstop: '+s.stop;"
    "s.target.forEach((a,i)=>{document.getElementById('s'+i).value=a;document.getElementById('v'+i).textContent=a})"
    "})}"
    "setInterval(refresh,1000);refresh();"
    "</script></main></body></html>";

  server.send(200, "text/html", html);
}

static void handleSetServo() {
  if (!server.hasArg("ch") || !server.hasArg("angle")) {
    server.send(400, "text/plain", "Missing ch or angle");
    return;
  }

  int ch = server.arg("ch").toInt();
  int angle = server.arg("angle").toInt();
  if (!isValidChannel(ch)) {
    server.send(400, "text/plain", "Invalid channel");
    return;
  }

  targetAngles[ch] = constrain(angle, 0, 180);
  *wifiStopFlag = false;
  server.send(200, "text/plain", "OK");
}

static void handleMoveAll() {
  bool updated = false;
  for (int i = 0; i < servoCount; i++) {
    String argName = "s" + String(i);
    if (server.hasArg(argName)) {
      targetAngles[i] = constrain(server.arg(argName).toInt(), 0, 180);
      updated = true;
    }
  }

  if (!updated) {
    server.send(400, "text/plain", "Use /move?s0=90&s1=0&s2=180&s3=130");
    return;
  }

  *wifiStopFlag = false;
  server.send(200, "text/plain", "OK");
}

static void handlePreset() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain", "Missing name");
    return;
  }

  String name = server.arg("name");
  if (name == "A" || name == "a") {
    copyTarget(presetA);
  } else if (name == "B" || name == "b") {
    copyTarget(presetB);
  } else {
    server.send(400, "text/plain", "Unknown preset");
    return;
  }

  *wifiStopFlag = false;
  server.send(200, "text/plain", "OK");
}

static void handleStop() {
  *wifiStopFlag = true;
  for (int i = 0; i < servoCount; i++) {
    targetAngles[i] = currentAngles[i];
  }
  server.send(200, "text/plain", "Stopped");
}

static void handleRun() {
  *wifiStopFlag = false;
  server.send(200, "text/plain", "Running");
}

static void handleStatus() {
  String json = "{\"current\":[";
  for (int i = 0; i < servoCount; i++) {
    if (i > 0) json += ",";
    json += String(currentAngles[i]);
  }

  json += "],\"target\":[";
  for (int i = 0; i < servoCount; i++) {
    if (i > 0) json += ",";
    json += String(targetAngles[i]);
  }

  json += "],\"stop\":";
  json += *wifiStopFlag ? "true" : "false";
  json += "}";
  server.send(200, "application/json", json);
}

static void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

static void copyTarget(const int angles[]) {
  for (int i = 0; i < servoCount; i++) {
    targetAngles[i] = angles[i];
  }
}

static bool isValidChannel(int ch) {
  return ch >= 0 && ch < servoCount;
}
