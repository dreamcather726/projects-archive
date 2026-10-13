#ifndef DOG_CONTROL_H
#define DOG_CONTROL_H

#include <Arduino.h>

// 为了防止网页误操作，微调和动作偏移都限制在比较温和的范围。
#define CENTER_TRIM_LIMIT_US 400
#define STAND_POSE_LIMIT_US 500
#define MOTION_OFFSET_LIMIT_US 500

// 四条腿，每条腿只控制大腿和小腿两个舵机。
#define LEG_COUNT 4
#define JOINT_COUNT 2
#define GAIT_FRAME_COUNT 20

enum LegIndex {
  LEG_RF = 0,
  LEG_LF = 1,
  LEG_RB = 2,
  LEG_LB = 3
};

enum JointIndex {
  JOINT_THIGH = 0,
  JOINT_CALF = 1
};

struct LegServo {
  const char* name;
  int thighChannel;
  int calfChannel;
  int thighGaitSign;
  int calfGaitSign;
};

typedef void (*ControlServiceCallback)();

extern LegServo legs[LEG_COUNT];
extern int centerTrimUs[LEG_COUNT][JOINT_COUNT];
extern int standPoseUs[LEG_COUNT][JOINT_COUNT];
extern int motionOffsetUs[LEG_COUNT][JOINT_COUNT];
extern bool stopRequested;

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
void setLegMotionOffset(int legIndex, int thighOffsetUs, int calfOffsetUs);
void moveLegHome(int legIndex);
void moveDogCenter();
void moveDogHome();
void walkDog(int stepCount, int frameDelayMs, int thighAmplitudeUs, int calfAmplitudeUs);
String getStateJson();

#endif
