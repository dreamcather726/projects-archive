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
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define WIFI_AP_NAME "ServoDog-Setup"
#define WIFI_AP_PASSWORD "REDACTED_CONFIGURE_LOCALLY"

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
static void handleGaitApi();
static void handleSaveCenterApi();
static void handleSaveStandApi();
static void handleSaveGaitApi();
static String getControlPageHtml();

void connectWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_STA_NAME, WIFI_STA_PASSWORD);

  Serial.println();
  Serial.print("Connecting WiFi: ");
  Serial.println(WIFI_STA_NAME);

  unsigned long connectStartMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - connectStartMs < WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Assigned IP: ");
    Serial.println(WiFi.localIP());
    return;
  }

  // 找不到预设路由器时开启校准热点，手机可直接连接后访问 192.168.4.1。
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  if (WiFi.softAP(WIFI_AP_NAME, WIFI_AP_PASSWORD)) {
    Serial.print("WiFi fallback AP: ");
    Serial.println(WIFI_AP_NAME);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("WiFi fallback AP failed");
  }
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
  server.on("/api/gait", handleGaitApi);
  server.on("/api/save_center", handleSaveCenterApi);
  server.on("/api/save_stand", handleSaveStandApi);
  server.on("/api/save_gait", handleSaveGaitApi);
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
  if (walkingActive) {
    server.send(200, "text/plain", "already walking");
    return;
  }

  int stepCount = getRequiredIntArg("steps", 0);
  int frameDelayMs = getRequiredIntArg("delay", gaitFrameDelayMs);
  int thighAmplitudeUs = getRequiredIntArg("thigh", gaitThighAmplitudeUs);
  int calfAmplitudeUs = getRequiredIntArg("calf", gaitCalfAmplitudeUs);

  server.send(200, "text/plain", "forward diagonal gait started");
  if (stepCount <= 0) {
    walkDogContinuously(frameDelayMs, thighAmplitudeUs, calfAmplitudeUs);
  } else {
    walkDog(stepCount, frameDelayMs, thighAmplitudeUs, calfAmplitudeUs);
  }
}

static void handleStopApi()
{
  stopRequested = true;
  server.send(200, "text/plain", "stop");
}

static void handleGaitApi()
{
  int frameDelayMs = getRequiredIntArg("delay", gaitFrameDelayMs);
  int thighAmplitudeUs = getRequiredIntArg("thigh", gaitThighAmplitudeUs);
  int calfAmplitudeUs = getRequiredIntArg("calf", gaitCalfAmplitudeUs);
  setGaitParameters(frameDelayMs, thighAmplitudeUs, calfAmplitudeUs);
  server.send(200, "application/json", getStateJson());
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

static void handleSaveGaitApi()
{
  bool isSaved = saveGaitConfigToFile();
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
  <title>12自由度舵机狗零位校准</title>
  <style>
    * { box-sizing: border-box; }
    body { font-family: Arial, "Microsoft YaHei", sans-serif; margin: 0; background: #f3f5f8; color: #172033; }
    main { max-width: 980px; margin: auto; padding: 18px; }
    h1 { font-size: 24px; margin: 0 0 8px; }
    .notice { background: #fff8df; border: 1px solid #ead486; border-radius: 8px; padding: 10px 12px; margin: 12px 0; line-height: 1.6; }
    .bar { position: sticky; top: 0; z-index: 2; display: flex; gap: 8px; flex-wrap: wrap; padding: 10px 0; background: #f3f5f8; }
    button { padding: 10px 14px; border: 1px solid #aab4c0; border-radius: 7px; background: white; cursor: pointer; }
    button.primary { background: #2563eb; color: white; border-color: #2563eb; }
    .gait-panel { background: white; border: 1px solid #d7dde5; border-radius: 10px; padding: 14px; margin: 0 0 12px; }
    .gait-panel h2 { font-size: 18px; margin: 0 0 10px; }
    .gait-grid { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 14px; }
    .gait-control label { display: block; font-weight: 700; margin-bottom: 5px; }
    .gait-control .row { display: flex; align-items: center; gap: 8px; }
    .gait-control input[type=range] { flex: 1; }
    .gait-actions { display: flex; gap: 8px; flex-wrap: wrap; margin-top: 12px; }
    .legs { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 12px; }
    .leg { background: white; border: 1px solid #d7dde5; border-radius: 10px; padding: 14px; }
    .leg h2 { font-size: 18px; margin: 0 0 4px; }
    .joint { border-top: 1px solid #edf0f4; padding: 12px 0 4px; }
    .joint-head { display: flex; justify-content: space-between; gap: 8px; align-items: center; }
    .value { width: 82px; padding: 6px; border: 1px solid #aab4c0; border-radius: 5px; text-align: right; }
    input[type=range] { width: 100%; }
    .label { font-weight: 700; }
    .small { color: #5f6b7a; font-size: 13px; }
    #status { min-height: 20px; color: #176b37; padding: 2px 0 8px; }
    @media (max-width: 700px) { .legs, .gait-grid { grid-template-columns: 1fr; } }
  </style>
</head>
<body>
  <main>
    <h1>12自由度舵机狗 · 0位校准</h1>
    <div class="small">通道分配：左前 0/1/2，右前 4/5/6，左后 8/9/10，右后 12/13/14。</div>
    <div class="notice">
      校准前请架空机械狗，避免腿部突然动作。页面调整会立即输出，但不会立即写入 Flash；确认 12 个关节机械位置正确后再点击“保存0位”。
    </div>
    <div class="bar">
      <button onclick="centerDog()">重新输出校准位</button>
      <button class="primary" onclick="saveCenter()">保存0位</button>
      <button class="primary" onclick="walkForward()">向前走（对角步态）</button>
      <button onclick="stopDog()">停止</button>
      <button onclick="loadState()">刷新显示</button>
    </div>
    <div id="status"></div>
    <section class="gait-panel">
      <h2>步态实时调节</h2>
      <div class="gait-grid">
        <div class="gait-control">
          <label for="gaitSteps">运行周期 <span class="small">（0 为持续）</span></label>
          <div class="row">
            <input id="gaitSteps" type="range" min="0" max="100" step="1" value="0">
            <input id="gaitStepsNumber" class="value" type="number" min="0" max="100" step="1" value="0">
          </div>
        </div>
        <div class="gait-control">
          <label for="gaitDelay">刷新间隔 <span class="small">（越小越流畅）</span></label>
          <div class="row">
            <input id="gaitDelay" type="range" min="20" max="100" step="5" value="20">
            <input id="gaitDelayNumber" class="value" type="number" min="20" max="100" step="5" value="20">
            <span>ms</span>
          </div>
        </div>
        <div class="gait-control">
          <label for="gaitThigh">大腿摆幅 <span class="small">（前后步幅）</span></label>
          <div class="row">
            <input id="gaitThigh" type="range" min="0" max="300" step="5" value="160">
            <input id="gaitThighNumber" class="value" type="number" min="0" max="300" step="5" value="160">
            <span>us</span>
          </div>
        </div>
        <div class="gait-control">
          <label for="gaitCalf">小腿摆幅 <span class="small">（抬腿高度）</span></label>
          <div class="row">
            <input id="gaitCalf" type="range" min="0" max="300" step="5" value="120">
            <input id="gaitCalfNumber" class="value" type="number" min="0" max="300" step="5" value="120">
            <span>us</span>
          </div>
        </div>
      </div>
      <div class="gait-actions">
        <button class="primary" onclick="applyGait()">应用步态参数</button>
        <button onclick="saveGait()">保存步态参数</button>
      </div>
    </section>
    <div id="legs" class="legs"></div>
  </main>
<script>
const jointNames = ['髋关节', '大腿', '小腿'];
const legNames = {RF: '右前腿', LF: '左前腿', RB: '右后腿', LB: '左后腿'};

async function api(path) {
  const response = await fetch(path);
  if (!response.ok) throw new Error(await response.text());
  return response;
}

async function loadState() {
  const response = await api('/api/state');
  const state = await response.json();
  renderState(state);
  renderGaitControls(state.gait);
}

function setControlPair(rangeId, numberId, value) {
  document.getElementById(rangeId).value = value;
  document.getElementById(numberId).value = value;
}

function bindControlPair(rangeId, numberId, minValue, maxValue) {
  const range = document.getElementById(rangeId);
  const number = document.getElementById(numberId);
  range.addEventListener('input', () => number.value = range.value);
  number.addEventListener('input', () => {
    number.value = Math.max(minValue, Math.min(maxValue, Number(number.value) || minValue));
    range.value = number.value;
  });
}

function renderGaitControls(gait) {
  if (!gait) return;
  setControlPair('gaitDelay', 'gaitDelayNumber', gait.frameDelay);
  setControlPair('gaitThigh', 'gaitThighNumber', gait.thighAmplitude);
  setControlPair('gaitCalf', 'gaitCalfNumber', gait.calfAmplitude);
}

function renderState(state) {
  const root = document.getElementById('legs');
  root.innerHTML = '';
  state.legs.forEach((leg, legIndex) => {
    const box = document.createElement('div');
    box.className = 'leg';
    box.innerHTML = `<h2>${legNames[leg.name] || leg.name}（${leg.name}）</h2>`;
    leg.joints.forEach((joint, jointIndex) => {
      box.appendChild(makeCalibrationRow(legIndex, jointIndex, joint));
    });
    root.appendChild(box);
  });
}

function makeCalibrationRow(legIndex, jointIndex, joint) {
  const row = document.createElement('div');
  row.className = 'joint';
  row.innerHTML = `
    <div class="joint-head">
      <div><span class="label">${jointNames[jointIndex]}</span> <span class="small">PCA9685 CH${joint.channel}</span></div>
      <label><input class="value" type="number" min="-1000" max="1000" step="5" value="${joint.trim}"> us</label>
    </div>
    <input type="range" min="-1000" max="1000" step="5" value="${joint.trim}">
    <div class="small">当前脉宽：${joint.pulse} us（基准 1500 us）</div>
  `;
  const number = row.querySelector('input[type=number]');
  const slider = row.querySelector('input[type=range]');
  slider.addEventListener('input', () => number.value = slider.value);
  number.addEventListener('input', () => slider.value = number.value);
  slider.addEventListener('change', () => setTrim(legIndex, jointIndex, slider.value));
  number.addEventListener('change', () => {
    number.value = Math.max(-1000, Math.min(1000, Number(number.value) || 0));
    slider.value = number.value;
    setTrim(legIndex, jointIndex, number.value);
  });
  return row;
}

async function setTrim(legIndex, jointIndex, value) {
  showStatus('正在调整…');
  await api(`/api/trim?leg=${legIndex}&joint=${jointIndex}&trim=${value}`);
  await loadState();
  showStatus('调整已生效，尚未保存');
}

async function centerDog() {
  await api('/api/center');
  await loadState();
  showStatus('12 路舵机已输出校准位');
}

async function saveCenter() {
  await api('/api/save_center');
  await loadState();
  showStatus('12 路舵机0位已保存，上电后自动加载');
}

async function walkForward() {
  const steps = document.getElementById('gaitStepsNumber').value;
  await api(`/api/walk?steps=${steps}`);
  const runText = Number(steps) <= 0 ? '持续向前走' : `向前走 ${steps} 个周期`;
  showStatus(`正在${runText}：右前+左后同步，左前+右后同步`);
}

async function stopDog() {
  await api('/api/stop');
  showStatus('已发送停止命令');
}

async function applyGait() {
  const delay = document.getElementById('gaitDelayNumber').value;
  const thigh = document.getElementById('gaitThighNumber').value;
  const calf = document.getElementById('gaitCalfNumber').value;
  const response = await api(`/api/gait?delay=${delay}&thigh=${thigh}&calf=${calf}`);
  const state = await response.json();
  renderGaitControls(state.gait);
  showStatus('步态参数已实时生效，尚未保存');
}

async function saveGait() {
  await applyGait();
  await api('/api/save_gait');
  showStatus('步态参数已保存，下次上电自动加载');
}

function showStatus(text) {
  document.getElementById('status').textContent = text;
}

bindControlPair('gaitDelay', 'gaitDelayNumber', 20, 100);
bindControlPair('gaitThigh', 'gaitThighNumber', 0, 300);
bindControlPair('gaitCalf', 'gaitCalfNumber', 0, 300);
bindControlPair('gaitSteps', 'gaitStepsNumber', 0, 100);
loadState().catch(error => showStatus(`读取失败：${error.message}`));
</script>
</body>
</html>
)rawliteral";
  return html;
}
