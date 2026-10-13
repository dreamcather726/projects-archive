#include "wifi_control.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "center_storage.h"
#include "dog_control.h"

// ESP32 连接到你的路由器 WiFi，网页访问路由器分配给 ESP32 的 IP 地址。
// 例如串口打印 IP: 192.168.1.23，就在浏览器打开 http://192.168.1.23
#define WIFI_STA_NAME "具身智能机器人-2.4G"
#define WIFI_STA_PASSWORD "REDACTED_CONFIGURE_LOCALLY"

static WebServer server(80);

static int getRequiredIntArg(const String& name, int defaultValue);
static void handleRootPage();
static void handleStateApi();
static void handleTrimApi();
static void handleStandApi();
static void handleOffsetApi();
static void handleJointBindApi();
static void handleCalfBindApi();
static void handleCenterApi();
static void handleHomeApi();
static void handleWalkApi();
static void handleStopApi();
static void handleSaveCenterApi();
static void handleSaveStandApi();
static String getControlPageHtml();

void connectWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_STA_NAME, WIFI_STA_PASSWORD);

  Serial.println();
  Serial.print("Connecting WiFi: ");
  Serial.println(WIFI_STA_NAME);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Assigned IP: ");
  Serial.println(WiFi.localIP());
}

void setupWebServer()
{
  setControlServiceCallback(handleWifiClient);

  server.on("/", handleRootPage);
  server.on("/api/state", handleStateApi);
  server.on("/api/trim", handleTrimApi);
  server.on("/api/stand", handleStandApi);
  server.on("/api/offset", handleOffsetApi);
  server.on("/api/joint_bind", handleJointBindApi);
  server.on("/api/calf_bind", handleCalfBindApi);
  server.on("/api/center", handleCenterApi);
  server.on("/api/home", handleHomeApi);
  server.on("/api/walk", handleWalkApi);
  server.on("/api/stop", handleStopApi);
  server.on("/api/save_center", handleSaveCenterApi);
  server.on("/api/save_stand", handleSaveStandApi);
  server.begin();
}

void handleWifiClient()
{
  server.handleClient();
}

static int getRequiredIntArg(const String& name, int defaultValue)
{
  if (!server.hasArg(name)) return defaultValue;
  return server.arg(name).toInt();
}

static void handleRootPage()
{
  server.send(200, "text/html; charset=utf-8", getControlPageHtml());
}

static void handleStateApi()
{
  server.send(200, "application/json", getStateJson());
}

static void handleTrimApi()
{
  int legIndex = getRequiredIntArg("leg", -1);
  int jointIndex = getRequiredIntArg("joint", -1);
  int trimUs = getRequiredIntArg("trim", 0);

  if (!isValidLeg(legIndex) || !isValidJoint(jointIndex)) {
    server.send(400, "text/plain", "bad leg or joint");
    return;
  }

  setJointCenterTrim(legIndex, jointIndex, trimUs);
  saveCenterTrimToFile();
  server.send(200, "application/json", getStateJson());
}

static void handleStandApi()
{
  int legIndex = getRequiredIntArg("leg", -1);
  int jointIndex = getRequiredIntArg("joint", -1);
  int poseUs = getRequiredIntArg("stand", 0);

  if (!isValidLeg(legIndex) || !isValidJoint(jointIndex)) {
    server.send(400, "text/plain", "bad leg or joint");
    return;
  }

  setJointStandPose(legIndex, jointIndex, poseUs);
  server.send(200, "application/json", getStateJson());
}

static void handleOffsetApi()
{
  int legIndex = getRequiredIntArg("leg", -1);
  int jointIndex = getRequiredIntArg("joint", -1);
  int offsetUs = getRequiredIntArg("offset", 0);

  if (!isValidLeg(legIndex) || !isValidJoint(jointIndex)) {
    server.send(400, "text/plain", "bad leg or joint");
    return;
  }

  setJointMotionOffset(legIndex, jointIndex, offsetUs);
  server.send(200, "application/json", getStateJson());
}

static void handleJointBindApi()
{
  int legIndex = getRequiredIntArg("leg", -1);
  int jointIndex = getRequiredIntArg("joint", -1);
  int offsetUs = getRequiredIntArg("offset", 0);
  String mode = server.hasArg("mode") ? server.arg("mode") : "none";

  if (!isValidLeg(legIndex) || !isValidJoint(jointIndex)) {
    server.send(400, "text/plain", "bad leg or joint");
    return;
  }

  if (mode == "diagonal") {
    setDiagonalJointMotionOffset(legIndex, jointIndex, offsetUs);
  } else if (mode == "all") {
    setAllJointMotionOffset(legIndex, jointIndex, offsetUs);
  } else {
    setJointMotionOffset(legIndex, jointIndex, offsetUs);
  }

  server.send(200, "application/json", getStateJson());
}

static void handleCalfBindApi()
{
  int legIndex = getRequiredIntArg("leg", -1);
  int offsetUs = getRequiredIntArg("offset", 0);
  String mode = server.hasArg("mode") ? server.arg("mode") : "none";

  if (!isValidLeg(legIndex)) {
    server.send(400, "text/plain", "bad leg");
    return;
  }

  if (mode == "diagonal") {
    setDiagonalCalfMotionOffset(legIndex, offsetUs);
  } else if (mode == "all") {
    setAllCalfMotionOffset(legIndex, offsetUs);
  } else {
    setJointMotionOffset(legIndex, JOINT_CALF, offsetUs);
  }

  server.send(200, "application/json", getStateJson());
}

static void handleCenterApi()
{
  moveDogCenter();
  server.send(200, "application/json", getStateJson());
}

static void handleHomeApi()
{
  moveDogHome();
  server.send(200, "application/json", getStateJson());
}

static void handleWalkApi()
{
  int stepCount = getRequiredIntArg("steps", 1);
  int frameDelayMs = getRequiredIntArg("delay", 80);
  int thighAmplitudeUs = getRequiredIntArg("thigh", 160);
  int calfAmplitudeUs = getRequiredIntArg("calf", 120);

  server.send(200, "text/plain", "walk started");
  walkDog(stepCount, frameDelayMs, thighAmplitudeUs, calfAmplitudeUs);
}

static void handleStopApi()
{
  stopRequested = true;
  server.send(200, "text/plain", "stop");
}

static void handleSaveCenterApi()
{
  bool isSaved = saveCenterTrimToFile();
  server.send(isSaved ? 200 : 500, "text/plain", isSaved ? "saved" : "save failed");
}

static void handleSaveStandApi()
{
  bool isSaved = saveStandPoseToFile();
  server.send(isSaved ? 200 : 500, "text/plain", isSaved ? "saved" : "save failed");
}

static String getControlPageHtml()
{
  String html = R"rawliteral(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Dog Servo WiFi Control</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 16px; background: #f6f7f9; color: #1f2933; }
    h1 { font-size: 22px; margin: 0 0 12px; }
    .bar { display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 14px; }
    button { padding: 9px 12px; border: 1px solid #aab4c0; border-radius: 6px; background: white; }
    button.primary { background: #2563eb; color: white; border-color: #2563eb; }
    button.danger { background: #dc2626; color: white; border-color: #dc2626; }
    select { padding: 8px 10px; border: 1px solid #aab4c0; border-radius: 6px; background: white; }
    .leg { background: white; border: 1px solid #d7dde5; border-radius: 8px; padding: 12px; margin: 10px 0; }
    .leg h2 { font-size: 18px; margin: 0 0 10px; }
    .joint { display: grid; grid-template-columns: 64px 1fr 62px; gap: 8px; align-items: center; margin: 10px 0; }
    input[type=range] { width: 100%; }
    .label { font-weight: 700; }
    .small { color: #5f6b7a; font-size: 13px; }
    .walk { background: white; border: 1px solid #d7dde5; border-radius: 8px; padding: 12px; margin-top: 12px; }
    .walk input { width: 72px; margin-right: 8px; }
  </style>
</head>
<body>
  <h1>机械狗舵机 WiFi 控制</h1>
  <div class="small">中位微调用来修正装配误差；站立姿态用于默认承重；动作偏移用于临时摆动关节。</div>
  <div class="bar">
    <button class="primary" onclick="centerDog()">回到中位</button>
    <button onclick="homeDog()">回到站立</button>
    <button onclick="saveCenter()">保存中位</button>
    <button onclick="saveStand()">保存站立</button>
    <button class="danger" onclick="stopDog()">停止</button>
    <label class="small">大腿绑定
      <select id="thighBindMode">
        <option value="none">不绑定</option>
        <option value="diagonal">对角同向</option>
        <option value="all">全部绑定</option>
      </select>
    </label>
    <label class="small">小腿绑定
      <select id="calfBindMode">
        <option value="none">不绑定</option>
        <option value="diagonal">对角同向</option>
        <option value="all">全部绑定</option>
      </select>
    </label>
  </div>
  <div id="legs"></div>
  <div class="walk">
    <div class="label">步态测试</div>
    <div style="margin-top:8px">
      步数 <input id="steps" type="number" value="2">
      帧间隔ms <input id="delay" type="number" value="80">
      大腿幅度us <input id="thigh" type="number" value="160">
      小腿幅度us <input id="calf" type="number" value="120">
      <button onclick="walkDog()">开始</button>
    </div>
  </div>
<script>
const jointNames = ['大腿', '小腿'];

async function api(path) {
  const response = await fetch(path);
  if (!response.ok) throw new Error(await response.text());
  return response;
}

async function loadState() {
  const response = await api('/api/state');
  const state = await response.json();
  renderState(state);
}

function renderState(state) {
  const root = document.getElementById('legs');
  root.innerHTML = '';
  state.legs.forEach((leg, legIndex) => {
    const box = document.createElement('div');
    box.className = 'leg';
    box.innerHTML = `<h2>${leg.name}</h2>`;
    leg.joints.forEach((joint, jointIndex) => {
      box.appendChild(makeSliderRow(legIndex, jointIndex, 'trim', '中位微调', joint.trim, -400, 400));
      box.appendChild(makeSliderRow(legIndex, jointIndex, 'stand', '站立姿态', joint.stand, -500, 500));
      box.appendChild(makeSliderRow(legIndex, jointIndex, 'offset', '动作偏移', joint.offset, -500, 500));
      const pulse = document.createElement('div');
      pulse.className = 'small';
      pulse.textContent = `${jointNames[jointIndex]} 当前输出：${joint.pulse}us`;
      box.appendChild(pulse);
    });
    root.appendChild(box);
  });
}

function makeSliderRow(legIndex, jointIndex, type, title, value, min, max) {
  const row = document.createElement('div');
  row.className = 'joint';
  row.innerHTML = `
    <div>${jointNames[jointIndex]}<br><span class="small">${title}</span></div>
    <input type="range" min="${min}" max="${max}" step="5" value="${value}">
    <span>${value}us</span>
  `;
  const slider = row.querySelector('input');
  const text = row.querySelector('span:last-child');
  slider.addEventListener('input', () => {
    text.textContent = `${slider.value}us`;
  });
  slider.addEventListener('change', async () => {
    if (type === 'offset') {
      const bindMode = jointIndex === 0
        ? document.getElementById('thighBindMode').value
        : document.getElementById('calfBindMode').value;
      await api(`/api/joint_bind?mode=${bindMode}&leg=${legIndex}&joint=${jointIndex}&offset=${slider.value}`);
    } else {
      await api(`/api/${type}?leg=${legIndex}&joint=${jointIndex}&${type}=${slider.value}`);
    }
    await loadState();
  });
  return row;
}

async function homeDog() {
  await api('/api/home');
  await loadState();
}

async function centerDog() {
  await api('/api/center');
  await loadState();
}

async function stopDog() {
  await api('/api/stop');
}

async function saveCenter() {
  await api('/api/save_center');
  await loadState();
  alert('中位已保存');
}

async function saveStand() {
  await api('/api/save_stand');
  await loadState();
  alert('站立姿态已保存');
}

async function walkDog() {
  const steps = document.getElementById('steps').value;
  const delay = document.getElementById('delay').value;
  const thigh = document.getElementById('thigh').value;
  const calf = document.getElementById('calf').value;
  await api(`/api/walk?steps=${steps}&delay=${delay}&thigh=${thigh}&calf=${calf}`);
  await loadState();
}

loadState();
</script>
</body>
</html>
)rawliteral";
  return html;
}
