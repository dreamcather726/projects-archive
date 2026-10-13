#include "PS2_CTL.hpp"
#include "RobotDogController.hpp"
#include "config.h"
#include <WebServer.h>
#include <WiFi.h>

PS2_CTL ps2;
RobotDogController robot_dog;
WebServer debug_server(80);

static const char *LOCAL_AP_SSID = "REDACTED_CONFIGURE_LOCALLY";
static const char *LOCAL_AP_PASSWORD = "REDACTED_CONFIGURE_LOCALLY";

RobotDogController::MotionCommand parseMotionCommand(const String &command_name) {
  if (command_name == "forward") {
    return RobotDogController::MOTION_FORWARD;
  }
  if (command_name == "backward") {
    return RobotDogController::MOTION_BACKWARD;
  }
  if (command_name == "left") {
    return RobotDogController::MOTION_LEFT;
  }
  if (command_name == "right") {
    return RobotDogController::MOTION_RIGHT;
  }
  return RobotDogController::MOTION_STOP;
}

bool hasPoseAngleArgs() {
  return debug_server.hasArg("a0") &&
         debug_server.hasArg("a1") &&
         debug_server.hasArg("a2") &&
         debug_server.hasArg("a3");
}

uint32_t getActionDurationArg() {
  if (!debug_server.hasArg("durationMs")) {
    return 400;
  }
  return (uint32_t)debug_server.arg("durationMs").toInt();
}

void sendStatusJson() {
  String response = "{\"centers\":[";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    if (leg_index > 0) {
      response += ",";
    }
    response += robot_dog.getServoCenterAngle(leg_index);
  }
  response += "],\"currentAngles\":[";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    if (leg_index > 0) {
      response += ",";
    }
    response += robot_dog.getCurrentServoAngle(leg_index);
  }
  response += "],\"gaitDirections\":[";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    if (leg_index > 0) {
      response += ",";
    }
    response += robot_dog.getLegGaitDirection(leg_index);
  }
  response += "],\"params\":{\"restOffset\":";
  response += robot_dog.getRestOffsetDegrees();
  response += ",\"halfCrouchOffset\":";
  response += robot_dog.getHalfCrouchOffsetDegrees();
  response += ",\"crouchOffset\":";
  response += robot_dog.getCrouchOffsetDegrees();
  response += ",\"manualMaxOffset\":";
  response += robot_dog.getManualMaxOffsetDegrees();
  response += ",\"walkSwingOffset\":";
  response += robot_dog.getWalkSwingOffsetDegrees();
  response += ",\"walkPushOffset\":";
  response += robot_dog.getWalkPushOffsetDegrees();
  response += ",\"turnSwingOffset\":";
  response += robot_dog.getTurnSwingOffsetDegrees();
  response += ",\"turnPushOffset\":";
  response += robot_dog.getTurnPushOffsetDegrees();
  response += ",\"walkStepIntervalMs\":";
  response += robot_dog.getWalkStepIntervalMs();
  response += ",\"turnDirection\":";
  response += robot_dog.getTurnDirectionSign();
  response += "},\"motion\":{\"command\":";
  response += (int)robot_dog.getContinuousMotionCommand();
  response += ",\"actionActive\":";
  response += robot_dog.isActionPlaybackActive() ? "true" : "false";
  response += ",\"actionLoop\":";
  response += robot_dog.isActionPlaybackLoopEnabled() ? "true" : "false";
  response += ",\"playbackFrameIndex\":";
  response += robot_dog.getPlaybackFrameIndex();
  response += ",\"actionFrameCount\":";
  response += robot_dog.getActionFrameCount();
  response += ",\"actionFrameCapacity\":";
  response += robot_dog.getActionFrameCapacity();
  response += "},\"actionFrames\":[";
  for (uint8_t frame_index = 0; frame_index < robot_dog.getActionFrameCount(); frame_index++) {
    RobotDogController::ActionFrame action_frame;
    if (!robot_dog.getActionFrame(frame_index, &action_frame)) {
      continue;
    }
    if (frame_index > 0) {
      response += ",";
    }
    response += "{\"angles\":[";
    for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
      if (leg_index > 0) {
        response += ",";
      }
      response += action_frame.angles[leg_index];
    }
    response += "],\"durationMs\":";
    response += action_frame.duration_ms;
    response += "}";
  }
  response += "],\"network\":{\"apSsid\":\"";
  response += LOCAL_AP_SSID;
  response += "\",\"apIp\":\"";
  response += WiFi.softAPIP().toString();
  response += "\",\"mode\":\"AP\",\"apClientCount\":";
  response += WiFi.softAPgetStationNum();
  response += "}}";

  debug_server.send(200, "application/json", response);
}

String buildConfigHeader() {
  String config_text;
  config_text += "#ifndef ROBOT_DOG_CONFIG_H\r\n";
  config_text += "#define ROBOT_DOG_CONFIG_H\r\n\r\n";
  config_text += "#include <Arduino.h>\r\n\r\n";
  config_text += "static const uint8_t ROBOT_DOG_SERVO_COUNT = 4;\r\n\r\n";

  config_text += "static const uint8_t ROBOT_DOG_SERVO_PINS[ROBOT_DOG_SERVO_COUNT] = {\r\n";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    config_text += "    ";
    config_text += (int)ROBOT_DOG_SERVO_PINS[leg_index];
    config_text += leg_index + 1 < robot_dog.getLegCount() ? ",\r\n" : "\r\n";
  }
  config_text += "};\r\n\r\n";

  config_text += "static const int ROBOT_DOG_SERVO_CENTER_ANGLES[ROBOT_DOG_SERVO_COUNT] = {\r\n";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    config_text += "    ";
    config_text += robot_dog.getServoCenterAngle(leg_index);
    config_text += leg_index + 1 < robot_dog.getLegCount() ? ",\r\n" : "\r\n";
  }
  config_text += "};\r\n\r\n";

  config_text += "static const int ROBOT_DOG_GAIT_DIRECTIONS[ROBOT_DOG_SERVO_COUNT] = {\r\n";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    config_text += "    ";
    config_text += robot_dog.getLegGaitDirection(leg_index);
    config_text += leg_index + 1 < robot_dog.getLegCount() ? ",\r\n" : "\r\n";
  }
  config_text += "};\r\n\r\n";

  config_text += "static const int ROBOT_DOG_REST_OFFSET_DEGREES = ";
  config_text += robot_dog.getRestOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_HALF_CROUCH_OFFSET_DEGREES = ";
  config_text += robot_dog.getHalfCrouchOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_CROUCH_OFFSET_DEGREES = ";
  config_text += robot_dog.getCrouchOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_MANUAL_MAX_OFFSET_DEGREES = ";
  config_text += robot_dog.getManualMaxOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_WALK_SWING_OFFSET_DEGREES = ";
  config_text += robot_dog.getWalkSwingOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_WALK_PUSH_OFFSET_DEGREES = ";
  config_text += robot_dog.getWalkPushOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_TURN_SWING_OFFSET_DEGREES = ";
  config_text += robot_dog.getTurnSwingOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_TURN_PUSH_OFFSET_DEGREES = ";
  config_text += robot_dog.getTurnPushOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const uint32_t ROBOT_DOG_WALK_STEP_INTERVAL_MS = ";
  config_text += robot_dog.getWalkStepIntervalMs();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_TURN_DIRECTION_SIGN = ";
  config_text += robot_dog.getTurnDirectionSign();
  config_text += ";\r\n\r\n";

  uint8_t action_capacity = robot_dog.getActionFrameCapacity();
  config_text += "static const uint8_t ROBOT_DOG_ACTION_FRAME_COUNT = ";
  config_text += robot_dog.getActionFrameCount();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_ACTION_FRAME_ANGLES[16][ROBOT_DOG_SERVO_COUNT] = {\r\n";
  for (uint8_t frame_index = 0; frame_index < action_capacity; frame_index++) {
    RobotDogController::ActionFrame action_frame;
    bool has_frame = robot_dog.getActionFrame(frame_index, &action_frame);
    config_text += "    {";
    for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
      if (leg_index > 0) {
        config_text += ", ";
      }
      config_text += has_frame
        ? action_frame.angles[leg_index]
        : robot_dog.getServoCenterAngle(leg_index);
    }
    config_text += frame_index + 1 < action_capacity ? "},\r\n" : "}\r\n";
  }
  config_text += "};\r\n";
  config_text += "static const uint32_t ROBOT_DOG_ACTION_FRAME_DURATIONS_MS[16] = {\r\n";
  for (uint8_t frame_index = 0; frame_index < action_capacity; frame_index++) {
    RobotDogController::ActionFrame action_frame;
    bool has_frame = robot_dog.getActionFrame(frame_index, &action_frame);
    config_text += "    ";
    config_text += has_frame ? action_frame.duration_ms : 400;
    config_text += frame_index + 1 < action_capacity ? ",\r\n" : "\r\n";
  }
  config_text += "};\r\n\r\n";
  config_text += "#endif\r\n";

  return config_text;
}

void handleConfigExport() {
  debug_server.sendHeader("Content-Disposition", "attachment; filename=config.h");
  debug_server.send(200, "text/plain", buildConfigHeader());
}

void handleRootPage() {
  String page = R"rawliteral(
<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>RobotDog WiFi Control</title>
<style>
body{margin:0;font-family:Arial,"Microsoft YaHei",sans-serif;background:#eef2f7;color:#111827}
main{max-width:980px;margin:0 auto;padding:18px}
h1{font-size:22px;margin:8px 0 14px}
h2{font-size:17px;margin:18px 0 10px}
.bar{display:flex;gap:8px;flex-wrap:wrap;margin:14px 0}
button{border:0;border-radius:6px;padding:10px 13px;background:#2563eb;color:white;font-size:15px}
button.secondary{background:#475569}
button.save{background:#d97706}
button.danger{background:#dc2626}
button.motion{background:#059669}
button.pending{background:#7c3aed}
button.done{background:#16a34a}
button.failed{background:#b91c1c}
.row{background:white;border:1px solid #d8dee9;border-radius:8px;padding:12px;margin:10px 0}
.head{display:flex;justify-content:space-between;gap:10px;margin-bottom:10px;font-weight:700}
input[type=range]{width:100%}
.tools{display:flex;gap:8px;align-items:center;margin-top:8px;flex-wrap:wrap}
input[type=number]{width:86px;padding:8px;border:1px solid #cbd5e1;border-radius:6px}
.status{font-size:13px;color:#475569;margin-top:12px;line-height:1.7}
.param-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(168px,1fr));gap:10px}
.param label{display:block;font-size:13px;color:#475569;margin-bottom:6px}
.param input{width:100%;box-sizing:border-box}
.frame-row{display:grid;grid-template-columns:42px repeat(4,minmax(62px,1fr)) minmax(86px,1fr) auto;gap:8px;align-items:center}
.frame-row span{font-size:13px;color:#475569}
.frame-row input{width:100%;box-sizing:border-box}
@media(max-width:680px){.frame-row{grid-template-columns:1fr 1fr}.frame-row .tools{grid-column:1/-1}}
</style>
</head>
<body>
<main>
<h1>RobotDog WiFi 上位机</h1>
<div class="bar">
<button onclick="command('/stand', this)">站立</button>
<button class="secondary" onclick="command('/rest', this)">休息</button>
<button class="secondary" onclick="command('/half-crouch', this)">半蹲</button>
<button class="secondary" onclick="command('/crouch', this)">趴下</button>
<button class="save" onclick="window.location='/config'">导出 config.h</button>
<button class="danger" onclick="command('/reset', this)">恢复配置</button>
</div>
<h2>连续运动</h2>
<section class="row">
  <div class="bar">
    <button class="motion" onclick="startMotion('forward', this)">前进</button>
    <button class="motion" onclick="startMotion('backward', this)">后退</button>
    <button class="motion" onclick="startMotion('left', this)">左转</button>
    <button class="motion" onclick="startMotion('right', this)">右转</button>
    <button class="danger" onclick="startMotion('stop', this)">停止</button>
  </div>
</section>
<h2>舵机中位与前进方向</h2>
<section id="servos"></section>
<h2>动作参数</h2>
<section class="row">
  <div class="param-grid">
    <div class="param"><label>休息偏移角度</label><input id="restOffset" type="number" min="-45" max="45"></div>
    <div class="param"><label>半蹲偏移角度</label><input id="halfCrouchOffset" type="number" min="-45" max="45"></div>
    <div class="param"><label>趴下偏移角度</label><input id="crouchOffset" type="number" min="-45" max="45"></div>
    <div class="param"><label>手动最大偏移</label><input id="manualMaxOffset" type="number" min="0" max="45"></div>
    <div class="param"><label>前进摆腿角度</label><input id="walkSwingOffset" type="number" min="0" max="45"></div>
    <div class="param"><label>前进推地角度</label><input id="walkPushOffset" type="number" min="0" max="45"></div>
    <div class="param"><label>转向摆腿角度</label><input id="turnSwingOffset" type="number" min="0" max="45"></div>
    <div class="param"><label>转向推地角度</label><input id="turnPushOffset" type="number" min="0" max="45"></div>
    <div class="param"><label>步态间隔 ms</label><input id="walkStepIntervalMs" type="number" min="120" max="1000"></div>
    <div class="param"><label>转向方向修正</label><input id="turnDirection" type="number" min="-1" max="1" step="2"></div>
  </div>
  <div class="tools"><button onclick="setParams(this)">应用动作参数</button></div>
</section>
<h2>动作编辑与记忆播放</h2>
<section class="row">
  <div class="param-grid">
    <div class="param"><label>前左角度</label><input id="editA0" type="number" min="35" max="145"></div>
    <div class="param"><label>前右角度</label><input id="editA1" type="number" min="35" max="145"></div>
    <div class="param"><label>后左角度</label><input id="editA2" type="number" min="35" max="145"></div>
    <div class="param"><label>后右角度</label><input id="editA3" type="number" min="35" max="145"></div>
    <div class="param"><label>保持时间 ms</label><input id="editDuration" type="number" min="80" max="10000" value="400"></div>
  </div>
  <div class="tools">
    <button class="secondary" onclick="fillEditorFromCurrent()">读取当前姿态</button>
    <button onclick="previewEditorPose(this)">预览姿态</button>
    <button class="save" onclick="addEditorFrame(this)">加入记忆</button>
    <button class="save" onclick="addCurrentFrame(this)">记录当前</button>
    <button class="motion" onclick="playAction(false, this)">播放一次</button>
    <button class="motion" onclick="playAction(true, this)">循环播放</button>
    <button class="danger" onclick="command('/action/stop', this)">停止播放</button>
    <button class="danger" onclick="command('/action/clear', this)">清空记忆</button>
  </div>
  <div id="frames"></div>
</section>
<div class="status" id="status">Loading...</div>
</main>
<script>
const names = ["前左 Front Left", "前右 Front Right", "后左 Rear Left", "后右 Rear Right"];
const paramIds = [
  "restOffset",
  "halfCrouchOffset",
  "crouchOffset",
  "manualMaxOffset",
  "walkSwingOffset",
  "walkPushOffset",
  "turnSwingOffset",
  "turnPushOffset",
  "walkStepIntervalMs",
  "turnDirection"
];
const motionNames = ["停止", "前进", "后退", "左转", "右转"];
let latestState = null;
const setButtonState = (button, text, className) => {
  if (!button) {
    return;
  }
  if (!button.dataset.text) {
    button.dataset.text = button.textContent;
  }
  button.textContent = text;
  button.classList.remove('pending', 'done', 'failed');
  button.classList.add(className);
};
const restoreButton = (button) => {
  if (!button || !button.dataset.text) {
    return;
  }
  setTimeout(() => {
    button.textContent = button.dataset.text;
    button.classList.remove('pending', 'done', 'failed');
  }, 700);
};
const requestAndRefresh = (path, button) => {
  setButtonState(button, '发送中...', 'pending');
  return fetch(path).then(response => {
    if (!response.ok) {
      throw new Error('request failed');
    }
    setButtonState(button, '已完成', 'done');
    restoreButton(button);
    refresh();
  }).catch(() => {
    setButtonState(button, '失败', 'failed');
    restoreButton(button);
  });
};
const command = (path, button) => {
  requestAndRefresh(path, button);
};
const startMotion = (motion, button) => {
  requestAndRefresh(`/motion?command=${motion}`, button);
};
const setAngle = (index, angle, button) => {
  requestAndRefresh(`/set?leg=${index}&angle=${angle}`, button);
};
const setDirection = (index, direction, button) => {
  requestAndRefresh(`/direction?leg=${index}&direction=${direction}`, button);
};
const setParams = (button) => {
  const query = paramIds.map(id => `${id}=${encodeURIComponent(document.getElementById(id).value)}`).join('&');
  requestAndRefresh(`/params?${query}`, button);
};
const editorQuery = () => {
  const angles = [0, 1, 2, 3].map(index => document.getElementById(`editA${index}`).value);
  const durationMs = document.getElementById('editDuration').value;
  return `a0=${angles[0]}&a1=${angles[1]}&a2=${angles[2]}&a3=${angles[3]}&durationMs=${durationMs}`;
};
const fillEditorFromCurrent = () => {
  if (!latestState) {
    return;
  }
  latestState.currentAngles.forEach((angle, index) => {
    document.getElementById(`editA${index}`).value = angle;
  });
};
const previewEditorPose = (button) => {
  requestAndRefresh(`/pose-angles?${editorQuery()}`, button);
};
const addEditorFrame = (button) => {
  requestAndRefresh(`/action/add?${editorQuery()}`, button);
};
const addCurrentFrame = (button) => {
  const durationMs = document.getElementById('editDuration').value;
  requestAndRefresh(`/action/add?source=current&durationMs=${durationMs}`, button);
};
const frameQuery = (index) => {
  const angles = [0, 1, 2, 3].map(legIndex => document.getElementById(`frame${index}A${legIndex}`).value);
  const durationMs = document.getElementById(`frame${index}Duration`).value;
  return `index=${index}&a0=${angles[0]}&a1=${angles[1]}&a2=${angles[2]}&a3=${angles[3]}&durationMs=${durationMs}`;
};
const previewFrame = (index, button) => {
  requestAndRefresh(`/pose-angles?${frameQuery(index)}`, button);
};
const updateFrame = (index, button) => {
  requestAndRefresh(`/action/update?${frameQuery(index)}`, button);
};
const deleteFrame = (index, button) => {
  requestAndRefresh(`/action/delete?index=${index}`, button);
};
const playAction = (loop, button) => {
  requestAndRefresh(`/action/play?loop=${loop ? 1 : 0}`, button);
};
const renderFrames = (state) => {
  const frameHtml = state.actionFrames.map((frame, index) => `
    <div class="row frame-row">
      <span>#${index + 1}</span>
      ${frame.angles.map((angle, legIndex) => `<input id="frame${index}A${legIndex}" type="number" min="35" max="145" value="${angle}">`).join('')}
      <input id="frame${index}Duration" type="number" min="80" max="10000" value="${frame.durationMs}">
      <div class="tools">
        <button class="secondary" onclick="previewFrame(${index}, this)">预览</button>
        <button onclick="updateFrame(${index}, this)">更新</button>
        <button class="danger" onclick="deleteFrame(${index}, this)">删除</button>
      </div>
    </div>
  `).join('');
  document.getElementById('frames').innerHTML = frameHtml || '<div class="status">暂无动作帧</div>';
};
const refresh = () => {
  fetch('/status').then(response => response.json()).then(state => {
    latestState = state;
    document.getElementById('servos').innerHTML = state.centers.map((angle, index) => `
      <div class="row">
        <div class="head"><span>${names[index]}</span><span>中位 ${angle}°｜当前 ${state.currentAngles[index]}°</span></div>
        <input type="range" min="35" max="145" value="${angle}" oninput="this.nextElementSibling.querySelector('input').value=this.value" onchange="setAngle(${index}, this.value, null)">
        <div class="tools">
          <button class="secondary" onclick="setAngle(${index}, ${angle - 1}, this)">-1</button>
          <button class="secondary" onclick="setAngle(${index}, ${angle + 1}, this)">+1</button>
          <input type="number" min="35" max="145" value="${angle}" onchange="setAngle(${index}, this.value, null)">
          <button class="${state.gaitDirections[index] > 0 ? 'save' : 'secondary'}" onclick="setDirection(${index}, 1, this)">前进正向</button>
          <button class="${state.gaitDirections[index] < 0 ? 'save' : 'secondary'}" onclick="setDirection(${index}, -1, this)">前进反向</button>
        </div>
      </div>
    `).join('');
    paramIds.forEach(id => document.getElementById(id).value = state.params[id]);
    if (!document.activeElement || !document.activeElement.id.startsWith('edit')) {
      fillEditorFromCurrent();
    }
    renderFrames(state);
    const activeMotion = motionNames[state.motion.command] || '未知';
    const actionText = state.motion.actionActive
      ? `动作播放中 #${state.motion.playbackFrameIndex + 1}${state.motion.actionLoop ? ' 循环' : ''}`
      : '动作未播放';
    document.getElementById('status').innerHTML =
      `本地热点：${state.network.apSsid}  地址：http://${state.network.apIp}<br>` +
      `网络模式：热点直连｜已连接设备：${state.network.apClientCount}<br>` +
      `连续运动：${activeMotion}｜记忆帧：${state.motion.actionFrameCount}/${state.motion.actionFrameCapacity}｜${actionText}`;
  });
};
refresh();
</script>
</body>
</html>
)rawliteral";

  debug_server.send(200, "text/html", page);
}

void handleSetServoCenter() {
  if (!debug_server.hasArg("leg") || !debug_server.hasArg("angle")) {
    debug_server.send(400, "text/plain", "missing leg or angle");
    return;
  }

  uint8_t leg_index = (uint8_t)debug_server.arg("leg").toInt();
  int angle_degrees = debug_server.arg("angle").toInt();
  robot_dog.setServoCenterAngle(leg_index, angle_degrees);
  sendStatusJson();
}

void handleSetLegDirection() {
  if (!debug_server.hasArg("leg") || !debug_server.hasArg("direction")) {
    debug_server.send(400, "text/plain", "missing leg or direction");
    return;
  }

  uint8_t leg_index = (uint8_t)debug_server.arg("leg").toInt();
  int direction_sign = debug_server.arg("direction").toInt();
  robot_dog.setLegGaitDirection(leg_index, direction_sign);
  sendStatusJson();
}

void handleSetMotionParameters() {
  if (!debug_server.hasArg("restOffset") ||
      !debug_server.hasArg("halfCrouchOffset") ||
      !debug_server.hasArg("crouchOffset") ||
      !debug_server.hasArg("manualMaxOffset") ||
      !debug_server.hasArg("walkSwingOffset") ||
      !debug_server.hasArg("walkPushOffset") ||
      !debug_server.hasArg("turnSwingOffset") ||
      !debug_server.hasArg("turnPushOffset") ||
      !debug_server.hasArg("walkStepIntervalMs") ||
      !debug_server.hasArg("turnDirection")) {
    debug_server.send(400, "text/plain", "missing motion parameter");
    return;
  }

  robot_dog.setMotionParameters(
    debug_server.arg("restOffset").toInt(),
    debug_server.arg("halfCrouchOffset").toInt(),
    debug_server.arg("crouchOffset").toInt(),
    debug_server.arg("manualMaxOffset").toInt(),
    debug_server.arg("walkSwingOffset").toInt(),
    debug_server.arg("walkPushOffset").toInt(),
    debug_server.arg("turnSwingOffset").toInt(),
    debug_server.arg("turnPushOffset").toInt(),
    (uint32_t)debug_server.arg("walkStepIntervalMs").toInt(),
    debug_server.arg("turnDirection").toInt()
  );
  sendStatusJson();
}

void handleResetCenters() {
  robot_dog.resetServoCenterAngles();
  sendStatusJson();
}

void handleStandPose() {
  robot_dog.applyStandPose();
  sendStatusJson();
}

void handleRestPose() {
  robot_dog.applyRestPose();
  sendStatusJson();
}

void handleHalfCrouchPose() {
  robot_dog.applyHalfCrouchPose();
  sendStatusJson();
}

void handleCrouchPose() {
  robot_dog.applyCrouchPose();
  sendStatusJson();
}

void handleMotionCommand() {
  if (!debug_server.hasArg("command")) {
    debug_server.send(400, "text/plain", "missing command");
    return;
  }

  RobotDogController::MotionCommand motion_command =
    parseMotionCommand(debug_server.arg("command"));
  if (motion_command == RobotDogController::MOTION_STOP) {
    robot_dog.stopContinuousMotion();
  } else {
    robot_dog.startContinuousMotion(motion_command);
  }
  sendStatusJson();
}

void handlePoseAngles() {
  if (!hasPoseAngleArgs()) {
    debug_server.send(400, "text/plain", "missing pose angles");
    return;
  }

  robot_dog.setPoseAngles(
    debug_server.arg("a0").toInt(),
    debug_server.arg("a1").toInt(),
    debug_server.arg("a2").toInt(),
    debug_server.arg("a3").toInt()
  );
  sendStatusJson();
}

void handleAddActionFrame() {
  bool added = false;
  if (debug_server.hasArg("source") && debug_server.arg("source") == "current") {
    added = robot_dog.addActionFrame(
      robot_dog.getCurrentServoAngle(0),
      robot_dog.getCurrentServoAngle(1),
      robot_dog.getCurrentServoAngle(2),
      robot_dog.getCurrentServoAngle(3),
      getActionDurationArg()
    );
  } else if (hasPoseAngleArgs()) {
    added = robot_dog.addActionFrame(
      debug_server.arg("a0").toInt(),
      debug_server.arg("a1").toInt(),
      debug_server.arg("a2").toInt(),
      debug_server.arg("a3").toInt(),
      getActionDurationArg()
    );
  } else {
    debug_server.send(400, "text/plain", "missing action frame angles");
    return;
  }

  if (!added) {
    debug_server.send(400, "text/plain", "action frame storage full");
    return;
  }
  sendStatusJson();
}

void handleUpdateActionFrame() {
  if (!debug_server.hasArg("index") || !hasPoseAngleArgs()) {
    debug_server.send(400, "text/plain", "missing action frame update args");
    return;
  }

  bool updated = robot_dog.updateActionFrame(
    (uint8_t)debug_server.arg("index").toInt(),
    debug_server.arg("a0").toInt(),
    debug_server.arg("a1").toInt(),
    debug_server.arg("a2").toInt(),
    debug_server.arg("a3").toInt(),
    getActionDurationArg()
  );
  if (!updated) {
    debug_server.send(400, "text/plain", "invalid action frame index");
    return;
  }
  sendStatusJson();
}

void handleDeleteActionFrame() {
  if (!debug_server.hasArg("index")) {
    debug_server.send(400, "text/plain", "missing action frame index");
    return;
  }

  bool removed = robot_dog.removeActionFrame((uint8_t)debug_server.arg("index").toInt());
  if (!removed) {
    debug_server.send(400, "text/plain", "invalid action frame index");
    return;
  }
  sendStatusJson();
}

void handleClearActionFrames() {
  robot_dog.clearActionFrames();
  sendStatusJson();
}

void handlePlayActionFrames() {
  bool loop_enabled = debug_server.hasArg("loop") && debug_server.arg("loop").toInt() != 0;
  robot_dog.playActionFrames(loop_enabled);
  sendStatusJson();
}

void handleStopActionFrames() {
  robot_dog.stopActionPlayback();
  sendStatusJson();
}

void connectControlWifi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(LOCAL_AP_SSID, LOCAL_AP_PASSWORD);

  Serial.print("RobotDog AP SSID: ");
  Serial.println(LOCAL_AP_SSID);
  Serial.print("RobotDog AP password: ");
  Serial.println(LOCAL_AP_PASSWORD);
  Serial.print("RobotDog control URL: http://");
  Serial.println(WiFi.softAPIP());
}

void initDebugServer() {
  connectControlWifi();

  debug_server.on("/", handleRootPage);
  debug_server.on("/status", sendStatusJson);
  debug_server.on("/set", handleSetServoCenter);
  debug_server.on("/direction", handleSetLegDirection);
  debug_server.on("/params", handleSetMotionParameters);
  debug_server.on("/config", handleConfigExport);
  debug_server.on("/reset", handleResetCenters);
  debug_server.on("/stand", handleStandPose);
  debug_server.on("/rest", handleRestPose);
  debug_server.on("/half-crouch", handleHalfCrouchPose);
  debug_server.on("/crouch", handleCrouchPose);
  debug_server.on("/motion", handleMotionCommand);
  debug_server.on("/pose-angles", handlePoseAngles);
  debug_server.on("/action/add", handleAddActionFrame);
  debug_server.on("/action/update", handleUpdateActionFrame);
  debug_server.on("/action/delete", handleDeleteActionFrame);
  debug_server.on("/action/clear", handleClearActionFrames);
  debug_server.on("/action/play", handlePlayActionFrames);
  debug_server.on("/action/stop", handleStopActionFrames);
  debug_server.begin();
}

void setup() {
  // 先接管舵机并回到已保存中位，减少外部供电上电时的乱动时间。
  robot_dog.init();
  delay(300);

  // 初始化 PS2 串口接收和 WiFi 调试网页。
  ps2.init();
  initDebugServer();
}

void loop() {
  debug_server.handleClient();

  // 持续读取 PS2 数据包，并在内部解析成按键、摇杆和模式状态。
  ps2.PS2_Task();

  PS2KeyValueObjectTypeDef ps2_keyvalue;

  // 只有收到过完整 PS2 数据帧后，才把最新手柄状态交给机械狗控制器。
  if (ps2.getKeyValue(&ps2_keyvalue)) {
    robot_dog.update(ps2_keyvalue);
  }

  robot_dog.tick();
}
