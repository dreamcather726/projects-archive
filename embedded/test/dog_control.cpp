#include "dog_control.h"

#include "pca9685_servo.h"

// 如果实际接线不同，优先修改这里的通道映射。
// gaitSign 用来处理左右腿舵机镜像安装：正数保持方向，负数反向。
LegServo legs[LEG_COUNT] = {
  {"RF", 1, 2, 1, 1},
  {"LF", 5, 6, -1, -1},
  {"RB", 9, 10, 1, 1},
  {"LB", 13, 14, -1, -1}
};

// 每个关节的中位微调值，单位 us。装配后某个关节不正，就改这里或在网页上调。
int centerTrimUs[LEG_COUNT][JOINT_COUNT] = {
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0}
};

// 站立姿态是机械狗默认承重姿势，会保存到文件中供步态叠加使用。
int standPoseUs[LEG_COUNT][JOINT_COUNT] = {
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0}
};

// 当前动作偏移值，单位 us。最终输出 = 1500 + 中位微调 + 站立姿态 + 当前动作偏移。
int motionOffsetUs[LEG_COUNT][JOINT_COUNT] = {
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0}
};

bool stopRequested = false;

static ControlServiceCallback controlServiceCallback = nullptr;

void setControlServiceCallback(ControlServiceCallback callback)
{
  controlServiceCallback = callback;
}

int clampValue(int inputValue, int minValue, int maxValue)
{
  if (inputValue < minValue) return minValue;
  if (inputValue > maxValue) return maxValue;
  return inputValue;
}

bool isValidLeg(int legIndex)
{
  return legIndex >= 0 && legIndex < LEG_COUNT;
}

bool isValidJoint(int jointIndex)
{
  return jointIndex >= 0 && jointIndex < JOINT_COUNT;
}

int getJointChannel(int legIndex, int jointIndex)
{
  if (jointIndex == JOINT_THIGH) return legs[legIndex].thighChannel;
  return legs[legIndex].calfChannel;
}

int getJointOutputPulseUs(int legIndex, int jointIndex)
{
  int pulseUs = SERVO_CENTER_US + centerTrimUs[legIndex][jointIndex] + standPoseUs[legIndex][jointIndex] + motionOffsetUs[legIndex][jointIndex];
  return clampPulseUs(pulseUs);
}

int getDiagonalLegIndex(int legIndex)
{
  if (legIndex == LEG_RF) return LEG_LB;
  if (legIndex == LEG_LB) return LEG_RF;
  if (legIndex == LEG_LF) return LEG_RB;
  if (legIndex == LEG_RB) return LEG_LF;
  return -1;
}

static int getJointGaitSign(int legIndex, int jointIndex)
{
  if (jointIndex == JOINT_THIGH) return legs[legIndex].thighGaitSign;
  return legs[legIndex].calfGaitSign;
}

static void applyJointOutput(int legIndex, int jointIndex)
{
  if (!isValidLeg(legIndex) || !isValidJoint(jointIndex)) return;
  writeServoPulseUs(getJointChannel(legIndex, jointIndex), getJointOutputPulseUs(legIndex, jointIndex));
}

void setJointCenterTrim(int legIndex, int jointIndex, int trimUs)
{
  if (!isValidLeg(legIndex) || !isValidJoint(jointIndex)) return;
  centerTrimUs[legIndex][jointIndex] = clampValue(trimUs, -CENTER_TRIM_LIMIT_US, CENTER_TRIM_LIMIT_US);
  applyJointOutput(legIndex, jointIndex);
}

void setJointStandPose(int legIndex, int jointIndex, int poseUs)
{
  if (!isValidLeg(legIndex) || !isValidJoint(jointIndex)) return;
  standPoseUs[legIndex][jointIndex] = clampValue(poseUs, -STAND_POSE_LIMIT_US, STAND_POSE_LIMIT_US);
  applyJointOutput(legIndex, jointIndex);
}

void setJointMotionOffset(int legIndex, int jointIndex, int offsetUs)
{
  if (!isValidLeg(legIndex) || !isValidJoint(jointIndex)) return;
  motionOffsetUs[legIndex][jointIndex] = clampValue(offsetUs, -MOTION_OFFSET_LIMIT_US, MOTION_OFFSET_LIMIT_US);
  applyJointOutput(legIndex, jointIndex);
}

void setDiagonalJointMotionOffset(int legIndex, int jointIndex, int offsetUs)
{
  if (!isValidLeg(legIndex) || !isValidJoint(jointIndex)) return;
  int diagonalLegIndex = getDiagonalLegIndex(legIndex);
  int sourceMotionUs = offsetUs / getJointGaitSign(legIndex, jointIndex);

  // 对角绑定按物理动作同相，再换算成对角腿自己的脉宽方向。
  setJointMotionOffset(legIndex, jointIndex, offsetUs);
  if (isValidLeg(diagonalLegIndex)) {
    int diagonalOffsetUs = sourceMotionUs * getJointGaitSign(diagonalLegIndex, jointIndex);
    setJointMotionOffset(diagonalLegIndex, jointIndex, diagonalOffsetUs);
  }
}

void setAllJointMotionOffset(int sourceLegIndex, int jointIndex, int offsetUs)
{
  if (!isValidLeg(sourceLegIndex) || !isValidJoint(jointIndex)) return;
  int diagonalLegIndex = getDiagonalLegIndex(sourceLegIndex);
  int sourceMotionUs = offsetUs / getJointGaitSign(sourceLegIndex, jointIndex);

  // 全部绑定按对角步态的物理动作分相，再换算成每个镜像舵机自己的脉宽方向。
  for (int legIndex = 0; legIndex < LEG_COUNT; legIndex++) {
    int phaseSign = (legIndex == sourceLegIndex || legIndex == diagonalLegIndex) ? 1 : -1;
    int boundOffsetUs = sourceMotionUs * phaseSign * getJointGaitSign(legIndex, jointIndex);
    setJointMotionOffset(legIndex, jointIndex, boundOffsetUs);
  }
}

void setDiagonalCalfMotionOffset(int legIndex, int calfOffsetUs)
{
  setDiagonalJointMotionOffset(legIndex, JOINT_CALF, calfOffsetUs);
}

void setAllCalfMotionOffset(int sourceLegIndex, int calfOffsetUs)
{
  setAllJointMotionOffset(sourceLegIndex, JOINT_CALF, calfOffsetUs);
}

void setLegMotionOffset(int legIndex, int thighOffsetUs, int calfOffsetUs)
{
  if (!isValidLeg(legIndex)) return;
  setJointMotionOffset(legIndex, JOINT_THIGH, thighOffsetUs * legs[legIndex].thighGaitSign);
  setJointMotionOffset(legIndex, JOINT_CALF, calfOffsetUs * legs[legIndex].calfGaitSign);
}

void moveLegHome(int legIndex)
{
  if (!isValidLeg(legIndex)) return;
  motionOffsetUs[legIndex][JOINT_THIGH] = 0;
  motionOffsetUs[legIndex][JOINT_CALF] = 0;
  applyJointOutput(legIndex, JOINT_THIGH);
  applyJointOutput(legIndex, JOINT_CALF);
}

void moveDogCenter()
{
  // 临时抵消站立姿态，只让舵机回到 1500 + 中位微调，不修改已保存的站立姿态。
  for (int legIndex = 0; legIndex < LEG_COUNT; legIndex++) {
    motionOffsetUs[legIndex][JOINT_THIGH] = -standPoseUs[legIndex][JOINT_THIGH];
    motionOffsetUs[legIndex][JOINT_CALF] = -standPoseUs[legIndex][JOINT_CALF];
    applyJointOutput(legIndex, JOINT_THIGH);
    applyJointOutput(legIndex, JOINT_CALF);
    delay(20);
  }
}

void moveDogHome()
{
  // 整机回到站立姿态时会使用每个关节自己的中位微调和站立偏移。
  for (int legIndex = 0; legIndex < LEG_COUNT; legIndex++) {
    moveLegHome(legIndex);
    delay(20);
  }
}

static int getWaveValue(int frameIndex, int phaseOffset, int amplitudeUs)
{
  int phase = (frameIndex + phaseOffset) % GAIT_FRAME_COUNT;
  int halfFrameCount = GAIT_FRAME_COUNT / 2;

  if (phase < halfFrameCount) {
    return map(phase, 0, halfFrameCount - 1, -amplitudeUs, amplitudeUs);
  }

  return map(phase, halfFrameCount, GAIT_FRAME_COUNT - 1, amplitudeUs, -amplitudeUs);
}

static void moveLegGaitFrame(int legIndex, int frameIndex, int phaseOffset, int thighAmplitudeUs, int calfAmplitudeUs)
{
  int thighOffsetUs = getWaveValue(frameIndex, phaseOffset, thighAmplitudeUs);
  int calfOffsetUs = getWaveValue(frameIndex, phaseOffset + GAIT_FRAME_COUNT / 4, calfAmplitudeUs);
  setLegMotionOffset(legIndex, thighOffsetUs, calfOffsetUs);
}

static void moveDogGaitFrame(int frameIndex, int thighAmplitudeUs, int calfAmplitudeUs)
{
  // 对角步态：右前+左后同相，左前+右后反相。
  // 左腿在 setLegMotionOffset() 中按 gaitSign 反向，抵消实际舵机镜像安装。
  moveLegGaitFrame(LEG_RF, frameIndex, 0, thighAmplitudeUs, calfAmplitudeUs);
  moveLegGaitFrame(LEG_LB, frameIndex, 0, thighAmplitudeUs, calfAmplitudeUs);
  moveLegGaitFrame(LEG_LF, frameIndex, GAIT_FRAME_COUNT / 2, thighAmplitudeUs, calfAmplitudeUs);
  moveLegGaitFrame(LEG_RB, frameIndex, GAIT_FRAME_COUNT / 2, thighAmplitudeUs, calfAmplitudeUs);
}

static void waitAndServiceControl(int delayMs)
{
  unsigned long startTime = millis();
  while (millis() - startTime < (unsigned long)delayMs) {
    if (controlServiceCallback != nullptr) {
      controlServiceCallback();
    }
    if (stopRequested) return;
    delay(2);
  }
}

void walkDog(int stepCount, int frameDelayMs, int thighAmplitudeUs, int calfAmplitudeUs)
{
  if (stepCount <= 0) stepCount = 1;
  if (frameDelayMs < 20) frameDelayMs = 20;
  thighAmplitudeUs = clampValue(abs(thighAmplitudeUs), 0, MOTION_OFFSET_LIMIT_US);
  calfAmplitudeUs = clampValue(abs(calfAmplitudeUs), 0, MOTION_OFFSET_LIMIT_US);

  stopRequested = false;
  for (int stepIndex = 0; stepIndex < stepCount && !stopRequested; stepIndex++) {
    for (int frameIndex = 0; frameIndex < GAIT_FRAME_COUNT && !stopRequested; frameIndex++) {
      moveDogGaitFrame(frameIndex, thighAmplitudeUs, calfAmplitudeUs);
      waitAndServiceControl(frameDelayMs);
    }
  }

  moveDogHome();
}

String getStateJson()
{
  String json = "{\"legs\":[";
  for (int legIndex = 0; legIndex < LEG_COUNT; legIndex++) {
    if (legIndex > 0) json += ",";
    json += "{\"name\":\"";
    json += legs[legIndex].name;
    json += "\",\"joints\":[";
    for (int jointIndex = 0; jointIndex < JOINT_COUNT; jointIndex++) {
      if (jointIndex > 0) json += ",";
      json += "{\"trim\":";
      json += centerTrimUs[legIndex][jointIndex];
      json += ",\"stand\":";
      json += standPoseUs[legIndex][jointIndex];
      json += ",\"offset\":";
      json += motionOffsetUs[legIndex][jointIndex];
      json += ",\"pulse\":";
      json += getJointOutputPulseUs(legIndex, jointIndex);
      json += "}";
    }
    json += "]}";
  }
  json += "]}";
  return json;
}
