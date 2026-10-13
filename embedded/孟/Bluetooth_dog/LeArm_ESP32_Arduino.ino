#include "PS2_CTL.hpp"
#include "RobotDogController.hpp"
#include "config.h"
#include <WebServer.h>
#include <WiFi.h>

PS2_CTL ps2;
RobotDogController robot_dog;
WebServer debug_server(80);

static const char *CONTROL_WIFI_SSID = "REDACTED_CONFIGURE_LOCALLY";
static const char *CONTROL_WIFI_PASSWORD = "REDACTED_CONFIGURE_LOCALLY";
static const uint32_t CONTROL_WIFI_CONNECT_TIMEOUT_MS = 15000;

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

bool parsePresetPose(
    const String &pose_name,
    RobotDogController::PresetPose *preset_pose) {
  if (preset_pose == NULL) {
    return false;
  }
  if (pose_name == "rest") {
    *preset_pose = RobotDogController::PRESET_REST;
    return true;
  }
  if (pose_name == "halfCrouch") {
    *preset_pose = RobotDogController::PRESET_HALF_CROUCH;
    return true;
  }
  if (pose_name == "crouch") {
    *preset_pose = RobotDogController::PRESET_CROUCH;
    return true;
  }
  return false;
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
  response += "],\"legMotionOffsets\":{\"forward\":[";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    if (leg_index > 0) {
      response += ",";
    }
    response += robot_dog.getLegForwardOffsetDegrees(leg_index);
  }
  response += "],\"backward\":[";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    if (leg_index > 0) {
      response += ",";
    }
    response += robot_dog.getLegBackwardOffsetDegrees(leg_index);
  }
  response += "]},\"poses\":{\"rest\":[";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    if (leg_index > 0) {
      response += ",";
    }
    response += robot_dog.getPresetPoseAngle(RobotDogController::PRESET_REST, leg_index);
  }
  response += "],\"halfCrouch\":[";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    if (leg_index > 0) {
      response += ",";
    }
    response += robot_dog.getPresetPoseAngle(
      RobotDogController::PRESET_HALF_CROUCH,
      leg_index
    );
  }
  response += "],\"crouch\":[";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    if (leg_index > 0) {
      response += ",";
    }
    response += robot_dog.getPresetPoseAngle(RobotDogController::PRESET_CROUCH, leg_index);
  }
  response += "]},\"storage\":{\"saved\":";
  response += robot_dog.hasSavedSettings() ? "true" : "false";
  response += ",\"dirty\":";
  response += robot_dog.hasUnsavedSettings() ? "true" : "false";
  response += "},\"params\":{\"restOffset\":";
  response += robot_dog.getRestOffsetDegrees();
  response += ",\"halfCrouchOffset\":";
  response += robot_dog.getHalfCrouchOffsetDegrees();
  response += ",\"crouchOffset\":";
  response += robot_dog.getCrouchOffsetDegrees();
  response += ",\"manualMaxOffset\":";
  response += robot_dog.getManualMaxOffsetDegrees();
  response += ",\"forwardSwingOffset\":";
  response += robot_dog.getForwardSwingOffsetDegrees();
  response += ",\"forwardPushOffset\":";
  response += robot_dog.getForwardPushOffsetDegrees();
  response += ",\"forwardStepIntervalMs\":";
  response += robot_dog.getForwardStepIntervalMs();
  response += ",\"backwardSwingOffset\":";
  response += robot_dog.getBackwardSwingOffsetDegrees();
  response += ",\"backwardPushOffset\":";
  response += robot_dog.getBackwardPushOffsetDegrees();
  response += ",\"backwardStepIntervalMs\":";
  response += robot_dog.getBackwardStepIntervalMs();
  response += ",\"leftTurnSwingOffset\":";
  response += robot_dog.getLeftTurnSwingOffsetDegrees();
  response += ",\"leftTurnPushOffset\":";
  response += robot_dog.getLeftTurnPushOffsetDegrees();
  response += ",\"leftTurnStepIntervalMs\":";
  response += robot_dog.getLeftTurnStepIntervalMs();
  response += ",\"rightTurnSwingOffset\":";
  response += robot_dog.getRightTurnSwingOffsetDegrees();
  response += ",\"rightTurnPushOffset\":";
  response += robot_dog.getRightTurnPushOffsetDegrees();
  response += ",\"rightTurnStepIntervalMs\":";
  response += robot_dog.getRightTurnStepIntervalMs();
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    response += ",\"leftTurnDirection";
    response += leg_index;
    response += "\":";
    response += robot_dog.getLeftTurnLegDirection(leg_index);
    response += ",\"rightTurnDirection";
    response += leg_index;
    response += "\":";
    response += robot_dog.getRightTurnLegDirection(leg_index);
  }
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
  response += "],\"network\":{\"wifiSsid\":\"";
  response += CONTROL_WIFI_SSID;
  response += "\",\"localIp\":\"";
  response += WiFi.localIP().toString();
  response += "\",\"mode\":\"STA\",\"connected\":";
  response += WiFi.status() == WL_CONNECTED ? "true" : "false";
  response += ",\"rssi\":";
  response += WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  response += "}}";

  debug_server.sendHeader("Cache-Control", "no-store");
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

  config_text += "static const int ROBOT_DOG_REST_POSE_ANGLES[ROBOT_DOG_SERVO_COUNT] = {\r\n";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    config_text += "    ";
    config_text += robot_dog.getPresetPoseAngle(RobotDogController::PRESET_REST, leg_index);
    config_text += leg_index + 1 < robot_dog.getLegCount() ? ",\r\n" : "\r\n";
  }
  config_text += "};\r\n\r\n";

  config_text += "static const int ROBOT_DOG_HALF_CROUCH_POSE_ANGLES[ROBOT_DOG_SERVO_COUNT] = {\r\n";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    config_text += "    ";
    config_text += robot_dog.getPresetPoseAngle(
      RobotDogController::PRESET_HALF_CROUCH,
      leg_index
    );
    config_text += leg_index + 1 < robot_dog.getLegCount() ? ",\r\n" : "\r\n";
  }
  config_text += "};\r\n\r\n";

  config_text += "static const int ROBOT_DOG_CROUCH_POSE_ANGLES[ROBOT_DOG_SERVO_COUNT] = {\r\n";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    config_text += "    ";
    config_text += robot_dog.getPresetPoseAngle(RobotDogController::PRESET_CROUCH, leg_index);
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
  config_text += "static const int ROBOT_DOG_FORWARD_SWING_OFFSET_DEGREES = ";
  config_text += robot_dog.getForwardSwingOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_FORWARD_PUSH_OFFSET_DEGREES = ";
  config_text += robot_dog.getForwardPushOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const uint32_t ROBOT_DOG_FORWARD_STEP_INTERVAL_MS = ";
  config_text += robot_dog.getForwardStepIntervalMs();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_BACKWARD_SWING_OFFSET_DEGREES = ";
  config_text += robot_dog.getBackwardSwingOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_BACKWARD_PUSH_OFFSET_DEGREES = ";
  config_text += robot_dog.getBackwardPushOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const uint32_t ROBOT_DOG_BACKWARD_STEP_INTERVAL_MS = ";
  config_text += robot_dog.getBackwardStepIntervalMs();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_LEFT_TURN_SWING_OFFSET_DEGREES = ";
  config_text += robot_dog.getLeftTurnSwingOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_LEFT_TURN_PUSH_OFFSET_DEGREES = ";
  config_text += robot_dog.getLeftTurnPushOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const uint32_t ROBOT_DOG_LEFT_TURN_STEP_INTERVAL_MS = ";
  config_text += robot_dog.getLeftTurnStepIntervalMs();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_RIGHT_TURN_SWING_OFFSET_DEGREES = ";
  config_text += robot_dog.getRightTurnSwingOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_RIGHT_TURN_PUSH_OFFSET_DEGREES = ";
  config_text += robot_dog.getRightTurnPushOffsetDegrees();
  config_text += ";\r\n";
  config_text += "static const uint32_t ROBOT_DOG_RIGHT_TURN_STEP_INTERVAL_MS = ";
  config_text += robot_dog.getRightTurnStepIntervalMs();
  config_text += ";\r\n";
  config_text += "static const int ROBOT_DOG_LEFT_TURN_DIRECTIONS[ROBOT_DOG_SERVO_COUNT] = {\r\n";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    config_text += "    ";
    config_text += robot_dog.getLeftTurnLegDirection(leg_index);
    config_text += leg_index + 1 < robot_dog.getLegCount() ? ",\r\n" : "\r\n";
  }
  config_text += "};\r\n";
  config_text += "static const int ROBOT_DOG_RIGHT_TURN_DIRECTIONS[ROBOT_DOG_SERVO_COUNT] = {\r\n";
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    config_text += "    ";
    config_text += robot_dog.getRightTurnLegDirection(leg_index);
    config_text += leg_index + 1 < robot_dog.getLegCount() ? ",\r\n" : "\r\n";
  }
  config_text += "};\r\n\r\n";

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
button{border:0;border-radius:6px;padding:10px 13px;background:#2563eb;color:white;font-size:15px;cursor:pointer;transition:transform .12s,box-shadow .12s,background .12s}
button:active{transform:translateY(1px)}
button.active{box-shadow:0 0 0 3px #bfdbfe;background:#1d4ed8}
button.secondary{background:#475569}
.secondary.active,.motion.active{background:#1d4ed8}
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
.feedback{display:none;padding:10px 12px;border-radius:6px;margin:10px 0;font-size:14px;font-weight:700}
.feedback.show{display:block}
.feedback.success{background:#dcfce7;color:#166534;border:1px solid #86efac}
.feedback.error{background:#fee2e2;color:#991b1b;border:1px solid #fca5a5}
.feedback.info{background:#dbeafe;color:#1e40af;border:1px solid #93c5fd}
.storage-state{display:inline-flex;align-items:center;padding:7px 10px;border-radius:6px;background:#e2e8f0;color:#334155;font-size:13px;font-weight:700}
.storage-state.saved{background:#dcfce7;color:#166534}
.storage-state.dirty{background:#fef3c7;color:#92400e}
.param-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(168px,1fr));gap:10px}
.param label{display:block;font-size:13px;color:#475569;margin-bottom:6px}
.param input,.param select{width:100%;box-sizing:border-box}
.param select{padding:8px;border:1px solid #cbd5e1;border-radius:6px;background:white}
.motion-param-groups{display:grid;grid-template-columns:repeat(2,minmax(260px,1fr));gap:12px;margin-top:12px}
.motion-param-group{border:1px solid #dbe3ef;border-radius:8px;padding:12px;background:#f8fafc}
.motion-param-group h3{font-size:15px;margin:0 0 10px;color:#1e3a5f}
.motion-param-group .param-grid{grid-template-columns:repeat(3,minmax(100px,1fr))}
.frame-row{display:grid;grid-template-columns:42px repeat(4,minmax(62px,1fr)) minmax(86px,1fr) auto;gap:8px;align-items:center}
.frame-row span{font-size:13px;color:#475569}
.frame-row input{width:100%;box-sizing:border-box}
.pose-grid{display:grid;grid-template-columns:repeat(4,minmax(130px,1fr));gap:10px}
.angle-control{display:grid;grid-template-columns:40px minmax(58px,1fr) 40px;gap:5px;align-items:center}
.angle-control input{width:100%;box-sizing:border-box;text-align:center}
.angle-control button{padding:8px 4px;background:#475569}
@media(max-width:680px){.frame-row{grid-template-columns:1fr 1fr}.frame-row .tools{grid-column:1/-1}}
@media(max-width:680px){.pose-grid{grid-template-columns:1fr 1fr}}
@media(max-width:680px){.motion-param-groups{grid-template-columns:1fr}.motion-param-group .param-grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<main>
<h1>RobotDog WiFi 上位机</h1>
<div id="feedback" class="feedback" role="status"></div>
<div class="bar">
<button id="poseStandButton" onclick="command('/stand', this, '已切换到站立')">站立</button>
<button id="poseRestButton" class="secondary" onclick="command('/rest', this, '已切换到休息')">休息</button>
<button id="poseHalfCrouchButton" class="secondary" onclick="command('/half-crouch', this, '已切换到半蹲')">半蹲</button>
<button id="poseCrouchButton" class="secondary" onclick="command('/crouch', this, '已切换到趴下')">趴下</button>
<button class="save" onclick="command('/save', this, '设置已写入设备并校验成功')">保存到设备</button>
<button class="save" onclick="window.location='/config'">导出 config.h</button>
<button class="danger" onclick="command('/reset', this, '已恢复固件默认配置')">恢复出厂配置</button>
<span id="storageState" class="storage-state">读取保存状态...</span>
</div>
<h2>连续运动</h2>
<section class="row">
  <div class="bar">
    <button class="motion" data-motion="1" onclick="startMotion('forward', this)">前进</button>
    <button class="motion" data-motion="2" onclick="startMotion('backward', this)">后退</button>
    <button class="motion" data-motion="3" onclick="startMotion('left', this)">左转</button>
    <button class="motion" data-motion="4" onclick="startMotion('right', this)">右转</button>
    <button class="danger" onclick="startMotion('stop', this)">停止</button>
  </div>
</section>
<h2>慢速单步调试</h2>
<section class="row">
  <div class="bar">
    <button onclick="stepMotion('forward', this)">前进下一步</button>
    <button onclick="stepMotion('backward', this)">后退下一步</button>
    <button onclick="stepMotion('left', this)">左转下一步</button>
    <button onclick="stepMotion('right', this)">右转下一步</button>
    <button class="danger" onclick="command('/stand', this, '已回到站立位置')">回到站立</button>
  </div>
  <div class="status">每按一次只执行一个步态相位。切换运动方向时会从第 1 步重新开始，建议先将机器狗架空。</div>
</section>
<h2>每条腿前进 / 后退调试</h2>
<section id="gaitLegs"></section>
<h2>舵机中位与前进方向</h2>
<section id="servos"></section>
<h2>姿态微调</h2>
<section id="poseSettings"></section>
<h2>动作参数</h2>
<section class="row">
  <div class="param-grid">
    <div class="param"><label>手动最大偏移</label><input id="manualMaxOffset" type="number" min="0" max="45"></div>
  </div>
  <div class="motion-param-groups">
    <div class="motion-param-group">
      <h3>前进</h3>
      <div class="param-grid">
        <div class="param"><label>摆腿角度</label><input id="forwardSwingOffset" type="number" min="0" max="45"></div>
        <div class="param"><label>推地角度</label><input id="forwardPushOffset" type="number" min="0" max="45"></div>
        <div class="param"><label>频率（间隔 ms）</label><input id="forwardStepIntervalMs" type="number" min="120" max="1000"></div>
      </div>
    </div>
    <div class="motion-param-group">
      <h3>后退</h3>
      <div class="param-grid">
        <div class="param"><label>摆腿角度</label><input id="backwardSwingOffset" type="number" min="0" max="45"></div>
        <div class="param"><label>推地角度</label><input id="backwardPushOffset" type="number" min="0" max="45"></div>
        <div class="param"><label>频率（间隔 ms）</label><input id="backwardStepIntervalMs" type="number" min="120" max="1000"></div>
      </div>
    </div>
    <div class="motion-param-group">
      <h3>左转</h3>
      <div class="param-grid">
        <div class="param"><label>摆腿角度</label><input id="leftTurnSwingOffset" type="number" min="0" max="45"></div>
        <div class="param"><label>推地角度</label><input id="leftTurnPushOffset" type="number" min="0" max="45"></div>
        <div class="param"><label>频率（间隔 ms）</label><input id="leftTurnStepIntervalMs" type="number" min="120" max="1000"></div>
        <div class="param"><label>前左方向</label><select id="leftTurnDirection0"><option value="1">正向</option><option value="-1">反向</option></select></div>
        <div class="param"><label>前右方向</label><select id="leftTurnDirection1"><option value="1">正向</option><option value="-1">反向</option></select></div>
        <div class="param"><label>后左方向</label><select id="leftTurnDirection2"><option value="1">正向</option><option value="-1">反向</option></select></div>
        <div class="param"><label>后右方向</label><select id="leftTurnDirection3"><option value="1">正向</option><option value="-1">反向</option></select></div>
      </div>
    </div>
    <div class="motion-param-group">
      <h3>右转</h3>
      <div class="param-grid">
        <div class="param"><label>摆腿角度</label><input id="rightTurnSwingOffset" type="number" min="0" max="45"></div>
        <div class="param"><label>推地角度</label><input id="rightTurnPushOffset" type="number" min="0" max="45"></div>
        <div class="param"><label>频率（间隔 ms）</label><input id="rightTurnStepIntervalMs" type="number" min="120" max="1000"></div>
        <div class="param"><label>前左方向</label><select id="rightTurnDirection0"><option value="1">正向</option><option value="-1">反向</option></select></div>
        <div class="param"><label>前右方向</label><select id="rightTurnDirection1"><option value="1">正向</option><option value="-1">反向</option></select></div>
        <div class="param"><label>后左方向</label><select id="rightTurnDirection2"><option value="1">正向</option><option value="-1">反向</option></select></div>
        <div class="param"><label>后右方向</label><select id="rightTurnDirection3"><option value="1">正向</option><option value="-1">反向</option></select></div>
      </div>
    </div>
  </div>
  <div class="status">频率使用相邻步态的间隔表示：数值越小，动作越快。范围 120～1000 ms。</div>
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
    <button class="danger" onclick="command('/action/clear', this, '动作记忆已清空并保存到设备')">清空记忆</button>
  </div>
  <div class="status">动作帧新增、更新、删除后会自动保存到设备。手柄 R1 播放一次，R2 循环播放，L1 停止播放。</div>
  <div id="frames"></div>
</section>
<div class="status" id="status">Loading...</div>
</main>
<script>
const names = ["前左 Front Left", "前右 Front Right", "后左 Rear Left", "后右 Rear Right"];
const paramIds = [
  "manualMaxOffset",
  "forwardSwingOffset",
  "forwardPushOffset",
  "forwardStepIntervalMs",
  "backwardSwingOffset",
  "backwardPushOffset",
  "backwardStepIntervalMs",
  "leftTurnSwingOffset",
  "leftTurnPushOffset",
  "leftTurnStepIntervalMs",
  "leftTurnDirection0",
  "leftTurnDirection1",
  "leftTurnDirection2",
  "leftTurnDirection3",
  "rightTurnSwingOffset",
  "rightTurnPushOffset",
  "rightTurnStepIntervalMs",
  "rightTurnDirection0",
  "rightTurnDirection1",
  "rightTurnDirection2",
  "rightTurnDirection3"
];
const poseDefinitions = [
  {key: 'rest', label: '休息 / 坐下'},
  {key: 'halfCrouch', label: '半蹲'},
  {key: 'crouch', label: '趴下'}
];
const motionNames = ["停止", "前进", "后退", "左转", "右转"];
let latestState = null;
let feedbackTimer = null;
let lastStepMotion = null;
const motionStepCounters = {forward: 0, backward: 0, left: 0, right: 0};
const showFeedback = (message, type = 'info') => {
  const feedback = document.getElementById('feedback');
  feedback.textContent = message;
  feedback.className = `feedback show ${type}`;
  clearTimeout(feedbackTimer);
  feedbackTimer = setTimeout(() => feedback.className = 'feedback', 3200);
};
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
const requestAndRefresh = (path, button, successMessage = '操作已完成') => {
  setButtonState(button, '发送中...', 'pending');
  showFeedback('正在发送控制指令...', 'info');
  return fetch(path).then(async response => {
    if (!response.ok) {
      throw new Error(await response.text() || 'request failed');
    }
    setButtonState(button, '已完成', 'done');
    showFeedback(successMessage, 'success');
    restoreButton(button);
    return refresh();
  }).catch(error => {
    setButtonState(button, '失败', 'failed');
    showFeedback(`操作失败：${error.message}`, 'error');
    restoreButton(button);
  });
};
const command = (path, button, successMessage) => {
  requestAndRefresh(path, button, successMessage);
};
const startMotion = (motion, button) => {
  const motionLabel = motion === 'stop' ? '运动已停止' : `${button.textContent}已启动`;
  requestAndRefresh(`/motion?command=${motion}`, button, motionLabel);
};
const stepMotion = (motion, button) => {
  if (lastStepMotion !== motion) {
    motionStepCounters[motion] = 0;
    lastStepMotion = motion;
  }
  const step = motionStepCounters[motion];
  const stepCount = 2;
  motionStepCounters[motion] = (step + 1) % stepCount;
  requestAndRefresh(
    `/motion-step?command=${motion}&step=${step}`,
    button,
    `${button.textContent}：第 ${step + 1} 步`
  );
};
const applyLegMotionOffsets = (index, button) => {
  const forwardOffset = document.getElementById(`gaitForward${index}`).value;
  const backwardOffset = document.getElementById(`gaitBackward${index}`).value;
  requestAndRefresh(
    `/gait-leg?leg=${index}&forwardOffset=${forwardOffset}&backwardOffset=${backwardOffset}`,
    button,
    `${names[index]}前进/后退参数已应用`
  );
};
const adjustLegMotionOffset = (index, direction, delta, button) => {
  const inputId = direction === 'forward' ? `gaitForward${index}` : `gaitBackward${index}`;
  const input = document.getElementById(inputId);
  input.value = Math.max(-45, Math.min(45, Number(input.value) + delta));
  applyLegMotionOffsets(index, button);
};
const setAngle = (index, angle, button) => {
  requestAndRefresh(`/set?leg=${index}&angle=${angle}`, button, `${names[index]}中位已调整`);
};
const setDirection = (index, direction, button) => {
  requestAndRefresh(`/direction?leg=${index}&direction=${direction}`, button);
};
const setParams = (button) => {
  const query = paramIds.map(id => `${id}=${encodeURIComponent(document.getElementById(id).value)}`).join('&');
  requestAndRefresh(`/params?${query}`, button, '动作参数已应用');
};
const presetPoseQuery = poseKey => {
  const angles = [0, 1, 2, 3].map(index => document.getElementById(`${poseKey}A${index}`).value);
  return `pose=${poseKey}&a0=${angles[0]}&a1=${angles[1]}&a2=${angles[2]}&a3=${angles[3]}`;
};
const applyPresetPose = (poseKey, button) => {
  const pose = poseDefinitions.find(item => item.key === poseKey);
  requestAndRefresh(`/pose-settings?${presetPoseQuery(poseKey)}`, button, `${pose.label}角度已应用`);
};
const adjustPresetPose = (poseKey, legIndex, delta, button) => {
  const input = document.getElementById(`${poseKey}A${legIndex}`);
  input.value = Math.max(35, Math.min(145, Number(input.value) + delta));
  applyPresetPose(poseKey, button);
};
const renderPresetPoses = state => {
  document.getElementById('poseSettings').innerHTML = poseDefinitions.map(pose => `
    <div class="row">
      <div class="head"><span>${pose.label}</span><span>四舵机独立角度</span></div>
      <div class="pose-grid">
        ${state.poses[pose.key].map((angle, legIndex) => `
          <div class="param">
            <label>${names[legIndex]}</label>
            <div class="angle-control">
              <button onclick="adjustPresetPose('${pose.key}', ${legIndex}, -1, this)">-1</button>
              <input id="${pose.key}A${legIndex}" type="number" min="35" max="145" value="${angle}" onchange="applyPresetPose('${pose.key}', null)">
              <button onclick="adjustPresetPose('${pose.key}', ${legIndex}, 1, this)">+1</button>
            </div>
          </div>
        `).join('')}
      </div>
      <div class="tools"><button onclick="applyPresetPose('${pose.key}', this)">应用并预览</button></div>
    </div>
  `).join('');
};
const renderGaitLegs = state => {
  document.getElementById('gaitLegs').innerHTML = names.map((name, index) => `
    <div class="row">
      <div class="head"><span>${name}</span><span>中位 ${state.centers[index]}°</span></div>
      <div class="pose-grid">
        <div class="param">
          <label>前进摆动偏移（目标 ${state.centers[index] + state.legMotionOffsets.forward[index]}°）</label>
          <div class="angle-control">
            <button onclick="adjustLegMotionOffset(${index}, 'forward', -1, this)">-1</button>
            <input id="gaitForward${index}" type="number" min="-45" max="45" value="${state.legMotionOffsets.forward[index]}" onchange="applyLegMotionOffsets(${index}, null)">
            <button onclick="adjustLegMotionOffset(${index}, 'forward', 1, this)">+1</button>
          </div>
        </div>
        <div class="param">
          <label>后退摆动偏移（目标 ${state.centers[index] + state.legMotionOffsets.backward[index]}°）</label>
          <div class="angle-control">
            <button onclick="adjustLegMotionOffset(${index}, 'backward', -1, this)">-1</button>
            <input id="gaitBackward${index}" type="number" min="-45" max="45" value="${state.legMotionOffsets.backward[index]}" onchange="applyLegMotionOffsets(${index}, null)">
            <button onclick="adjustLegMotionOffset(${index}, 'backward', 1, this)">+1</button>
          </div>
        </div>
      </div>
      <div class="tools"><button onclick="applyLegMotionOffsets(${index}, this)">应用这条腿</button></div>
    </div>
  `).join('');
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
  requestAndRefresh(`/action/add?${editorQuery()}`, button, '动作帧已加入并保存到设备');
};
const addCurrentFrame = (button) => {
  const durationMs = document.getElementById('editDuration').value;
  requestAndRefresh(
    `/action/add?source=current&durationMs=${durationMs}`,
    button,
    '当前姿态已记录并保存到设备'
  );
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
  requestAndRefresh(`/action/update?${frameQuery(index)}`, button, '动作帧已更新并保存到设备');
};
const deleteFrame = (index, button) => {
  requestAndRefresh(`/action/delete?index=${index}`, button, '动作帧已删除并同步到设备');
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
    renderGaitLegs(state);
    renderPresetPoses(state);
    paramIds.forEach(id => document.getElementById(id).value = state.params[id]);
    if (!document.activeElement || !document.activeElement.id.startsWith('edit')) {
      fillEditorFromCurrent();
    }
    renderFrames(state);
    const activeMotion = motionNames[state.motion.command] || '未知';
    document.querySelectorAll('[data-motion]').forEach(button => {
      button.classList.toggle('active', Number(button.dataset.motion) === state.motion.command);
    });
    const poseButtons = {
      rest: document.getElementById('poseRestButton'),
      halfCrouch: document.getElementById('poseHalfCrouchButton'),
      crouch: document.getElementById('poseCrouchButton')
    };
    Object.values(poseButtons).forEach(button => button.classList.remove('active'));
    document.getElementById('poseStandButton').classList.remove('active');
    if (state.motion.command === 0 && !state.motion.actionActive) {
      const activePose = poseDefinitions.find(pose =>
        state.poses[pose.key].every((angle, index) => angle === state.currentAngles[index])
      );
      if (activePose) {
        poseButtons[activePose.key].classList.add('active');
      } else if (state.centers.every((angle, index) => angle === state.currentAngles[index])) {
        document.getElementById('poseStandButton').classList.add('active');
      }
    }
    const storageState = document.getElementById('storageState');
    if (state.storage.dirty) {
      storageState.textContent = '有未保存修改';
      storageState.className = 'storage-state dirty';
    } else if (state.storage.saved) {
      storageState.textContent = '已从设备加载';
      storageState.className = 'storage-state saved';
    } else {
      storageState.textContent = '当前为固件默认配置';
      storageState.className = 'storage-state';
    }
    const actionText = state.motion.actionActive
      ? `动作播放中 #${state.motion.playbackFrameIndex + 1}${state.motion.actionLoop ? ' 循环' : ''}`
      : '动作未播放';
    document.getElementById('status').innerHTML =
      `连接 WiFi：${state.network.wifiSsid}  地址：http://${state.network.localIp}<br>` +
      `网络模式：路由器 WiFi｜连接状态：${state.network.connected ? '已连接' : '未连接'}｜信号：${state.network.rssi} dBm<br>` +
      `连续运动：${activeMotion}｜记忆帧：${state.motion.actionFrameCount}/${state.motion.actionFrameCapacity}｜${actionText}`;
  }).catch(error => showFeedback(`状态读取失败：${error.message}`, 'error'));
};
refresh();
</script>
</body>
</html>
)rawliteral";

  debug_server.sendHeader("Cache-Control", "no-store");
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
  if (!debug_server.hasArg("manualMaxOffset") ||
      !debug_server.hasArg("forwardSwingOffset") ||
      !debug_server.hasArg("forwardPushOffset") ||
      !debug_server.hasArg("forwardStepIntervalMs") ||
      !debug_server.hasArg("backwardSwingOffset") ||
      !debug_server.hasArg("backwardPushOffset") ||
      !debug_server.hasArg("backwardStepIntervalMs") ||
      !debug_server.hasArg("leftTurnSwingOffset") ||
      !debug_server.hasArg("leftTurnPushOffset") ||
      !debug_server.hasArg("leftTurnStepIntervalMs") ||
      !debug_server.hasArg("rightTurnSwingOffset") ||
      !debug_server.hasArg("rightTurnPushOffset") ||
      !debug_server.hasArg("rightTurnStepIntervalMs") ||
      !debug_server.hasArg("leftTurnDirection0") ||
      !debug_server.hasArg("leftTurnDirection1") ||
      !debug_server.hasArg("leftTurnDirection2") ||
      !debug_server.hasArg("leftTurnDirection3") ||
      !debug_server.hasArg("rightTurnDirection0") ||
      !debug_server.hasArg("rightTurnDirection1") ||
      !debug_server.hasArg("rightTurnDirection2") ||
      !debug_server.hasArg("rightTurnDirection3")) {
    debug_server.send(400, "text/plain", "missing motion parameter");
    return;
  }

  robot_dog.setMotionParameters(
    robot_dog.getRestOffsetDegrees(),
    robot_dog.getHalfCrouchOffsetDegrees(),
    robot_dog.getCrouchOffsetDegrees(),
    debug_server.arg("manualMaxOffset").toInt(),
    debug_server.arg("forwardSwingOffset").toInt(),
    debug_server.arg("forwardPushOffset").toInt(),
    (uint32_t)debug_server.arg("forwardStepIntervalMs").toInt(),
    debug_server.arg("backwardSwingOffset").toInt(),
    debug_server.arg("backwardPushOffset").toInt(),
    (uint32_t)debug_server.arg("backwardStepIntervalMs").toInt(),
    debug_server.arg("leftTurnSwingOffset").toInt(),
    debug_server.arg("leftTurnPushOffset").toInt(),
    (uint32_t)debug_server.arg("leftTurnStepIntervalMs").toInt(),
    debug_server.arg("rightTurnSwingOffset").toInt(),
    debug_server.arg("rightTurnPushOffset").toInt(),
    (uint32_t)debug_server.arg("rightTurnStepIntervalMs").toInt()
  );
  for (uint8_t leg_index = 0; leg_index < robot_dog.getLegCount(); leg_index++) {
    robot_dog.setTurnLegDirection(
      RobotDogController::MOTION_LEFT,
      leg_index,
      debug_server.arg(String("leftTurnDirection") + String(leg_index)).toInt()
    );
    robot_dog.setTurnLegDirection(
      RobotDogController::MOTION_RIGHT,
      leg_index,
      debug_server.arg(String("rightTurnDirection") + String(leg_index)).toInt()
    );
  }
  sendStatusJson();
}

void handleSetPresetPose() {
  if (!debug_server.hasArg("pose") || !hasPoseAngleArgs()) {
    debug_server.send(400, "text/plain", "missing preset pose args");
    return;
  }

  RobotDogController::PresetPose preset_pose;
  if (!parsePresetPose(debug_server.arg("pose"), &preset_pose)) {
    debug_server.send(400, "text/plain", "invalid preset pose");
    return;
  }

  robot_dog.setPresetPoseAngles(
    preset_pose,
    debug_server.arg("a0").toInt(),
    debug_server.arg("a1").toInt(),
    debug_server.arg("a2").toInt(),
    debug_server.arg("a3").toInt()
  );
  if (preset_pose == RobotDogController::PRESET_REST) {
    robot_dog.applyRestPose();
  } else if (preset_pose == RobotDogController::PRESET_HALF_CROUCH) {
    robot_dog.applyHalfCrouchPose();
  } else {
    robot_dog.applyCrouchPose();
  }
  sendStatusJson();
}

void handleSaveSettings() {
  if (!robot_dog.saveSettings()) {
    debug_server.send(500, "text/plain", "failed to save settings");
    return;
  }
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

void handleMotionStep() {
  if (!debug_server.hasArg("command") || !debug_server.hasArg("step")) {
    debug_server.send(400, "text/plain", "missing motion step args");
    return;
  }

  RobotDogController::MotionCommand motion_command =
    parseMotionCommand(debug_server.arg("command"));
  if (motion_command == RobotDogController::MOTION_STOP) {
    debug_server.send(400, "text/plain", "invalid motion step command");
    return;
  }

  int step_count = 2;
  int step_index = debug_server.arg("step").toInt();
  if (step_index < 0) {
    step_index = 0;
  }
  robot_dog.previewMotionStep(motion_command, (uint8_t)(step_index % step_count));
  sendStatusJson();
}

void handleSetLegMotionOffsets() {
  if (!debug_server.hasArg("leg") ||
      !debug_server.hasArg("forwardOffset") ||
      !debug_server.hasArg("backwardOffset")) {
    debug_server.send(400, "text/plain", "missing leg motion args");
    return;
  }

  int leg_index = debug_server.arg("leg").toInt();
  if (leg_index < 0 || leg_index >= robot_dog.getLegCount()) {
    debug_server.send(400, "text/plain", "invalid leg index");
    return;
  }
  robot_dog.setLegMotionOffsets(
    (uint8_t)leg_index,
    debug_server.arg("forwardOffset").toInt(),
    debug_server.arg("backwardOffset").toInt()
  );
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

bool persistActionFrames() {
  if (robot_dog.saveSettings()) {
    return true;
  }

  debug_server.send(
    500,
    "text/plain",
    "action changed in memory but failed to save to device"
  );
  return false;
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
  if (!persistActionFrames()) {
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
  if (!persistActionFrames()) {
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
  if (!persistActionFrames()) {
    return;
  }
  sendStatusJson();
}

void handleClearActionFrames() {
  robot_dog.clearActionFrames();
  if (!persistActionFrames()) {
    return;
  }
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
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(CONTROL_WIFI_SSID, CONTROL_WIFI_PASSWORD);

  Serial.print("RobotDog WiFi SSID: ");
  Serial.println(CONTROL_WIFI_SSID);
  Serial.print("Connecting");
  uint32_t connect_start_ms = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - connect_start_ms < CONTROL_WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("RobotDog WiFi connect failed. Check SSID and password.");
    return;
  }

  Serial.print("RobotDog control URL: http://");
  Serial.println(WiFi.localIP());
}

void initDebugServer() {
  connectControlWifi();

  debug_server.on("/", handleRootPage);
  debug_server.on("/status", sendStatusJson);
  debug_server.on("/set", handleSetServoCenter);
  debug_server.on("/direction", handleSetLegDirection);
  debug_server.on("/params", handleSetMotionParameters);
  debug_server.on("/pose-settings", handleSetPresetPose);
  debug_server.on("/save", handleSaveSettings);
  debug_server.on("/config", handleConfigExport);
  debug_server.on("/reset", handleResetCenters);
  debug_server.on("/stand", handleStandPose);
  debug_server.on("/rest", handleRestPose);
  debug_server.on("/half-crouch", handleHalfCrouchPose);
  debug_server.on("/crouch", handleCrouchPose);
  debug_server.on("/motion", handleMotionCommand);
  debug_server.on("/motion-step", handleMotionStep);
  debug_server.on("/gait-leg", handleSetLegMotionOffsets);
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
