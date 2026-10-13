#ifndef DOG_CONTROL_H
#define DOG_CONTROL_H

#include <Arduino.h>

// 为了防止网页误操作，微调和动作偏移都限制在比较温和的范围。
#define CENTER_TRIM_LIMIT_US 1000
#define STAND_POSE_LIMIT_US 500
#define MOTION_OFFSET_LIMIT_US 500
#define GAIT_MIN_FRAME_DELAY_MS 20
#define GAIT_MAX_FRAME_DELAY_MS 100
#define GAIT_DEFAULT_FRAME_DELAY_MS 20
#define GAIT_DEFAULT_THIGH_AMPLITUDE_US 160
#define GAIT_DEFAULT_CALF_AMPLITUDE_US 120

// 四条腿，每条腿控制髋关节、大腿和小腿三个舵机，共 12 自由度。
#define LEG_COUNT 4
#define JOINT_COUNT 3
// 50 Hz 舵机每 20 ms 接收一个平滑目标；80 帧对应约 1.6 秒一个完整步态周期。
#define GAIT_FRAME_COUNT 80

enum LegIndex {
  LEG_RF = 0,
  LEG_LF = 1,
  LEG_RB = 2,
  LEG_LB = 3
};

enum JointIndex {
  JOINT_HIP = 0,
  JOINT_THIGH = 1,
  JOINT_CALF = 2
};

struct LegServo {
  const char* name;
  int channels[JOINT_COUNT];
  int gaitSigns[JOINT_COUNT];
};

typedef void (*ControlServiceCallback)();

extern LegServo legs[LEG_COUNT];
extern int centerTrimUs[LEG_COUNT][JOINT_COUNT];
extern int standPoseUs[LEG_COUNT][JOINT_COUNT];
extern int motionOffsetUs[LEG_COUNT][JOINT_COUNT];
extern bool stopRequested;
extern bool walkingActive;
extern int gaitFrameDelayMs;
extern int gaitThighAmplitudeUs;
extern int gaitCalfAmplitudeUs;

void setControlServiceCallback(ControlServiceCallback callback);
int clampValue(int inputValue, int minValue, int maxValue);
bool isValidLeg(int legIndex);
bool isValidJoint(int jointIndex);
int getJointChannel(int legIndex, int jointIndex);
int getJointOutputPulseUs(int legIndex, int jointIndex);
int getDiagonalLegIndex(int legIndex);
void setJointCenterTrim(int legIndex, int jointIndex, int trimUs);
void setJointStandPose(int legIndex, int jointIndex, int poseUs);
void setJointMotionOffset(int legIndex, int jointIndex, int offsetUs);
void setDiagonalJointMotionOffset(int legIndex, int jointIndex, int offsetUs);
void setAllJointMotionOffset(int sourceLegIndex, int jointIndex, int offsetUs);
void setDiagonalCalfMotionOffset(int legIndex, int calfOffsetUs);
void setAllCalfMotionOffset(int sourceLegIndex, int calfOffsetUs);
void setLegMotionOffset(int legIndex, int hipOffsetUs, int thighOffsetUs, int calfOffsetUs);
void moveLegHome(int legIndex);
void moveDogCenter();
void moveDogHome();
void setGaitParameters(int frameDelayMs, int thighAmplitudeUs, int calfAmplitudeUs);
void walkDog(int stepCount, int frameDelayMs, int thighAmplitudeUs, int calfAmplitudeUs);
void walkDogContinuously(int frameDelayMs, int thighAmplitudeUs, int calfAmplitudeUs);
String getStateJson();

#endif
