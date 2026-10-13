#ifndef ROBOT_DOG_CONFIG_H
#define ROBOT_DOG_CONFIG_H

#include <Arduino.h>

static const uint8_t ROBOT_DOG_SERVO_COUNT = 4;

static const uint8_t ROBOT_DOG_SERVO_PINS[ROBOT_DOG_SERVO_COUNT] = {
    19,
    18,
    5,
    4
};

static const int ROBOT_DOG_SERVO_CENTER_ANGLES[ROBOT_DOG_SERVO_COUNT] = {
    90,
    99,
    82,
    82
};

static const int ROBOT_DOG_GAIT_DIRECTIONS[ROBOT_DOG_SERVO_COUNT] = {
    1,
    -1,
    1,
    -1
};

static const int ROBOT_DOG_REST_POSE_ANGLES[ROBOT_DOG_SERVO_COUNT] = {
    40,
    145,
    129,
    35
};

static const int ROBOT_DOG_HALF_CROUCH_POSE_ANGLES[ROBOT_DOG_SERVO_COUNT] = {
    100,
    85,
    129,
    35
};

static const int ROBOT_DOG_CROUCH_POSE_ANGLES[ROBOT_DOG_SERVO_COUNT] = {
    90,
    94,
    82,
    90
};

static const int ROBOT_DOG_REST_OFFSET_DEGREES = -20;
static const int ROBOT_DOG_HALF_CROUCH_OFFSET_DEGREES = -12;
static const int ROBOT_DOG_CROUCH_OFFSET_DEGREES = -35;
static const int ROBOT_DOG_MANUAL_MAX_OFFSET_DEGREES = 25;
static const int ROBOT_DOG_FORWARD_SWING_OFFSET_DEGREES = 15;
static const int ROBOT_DOG_FORWARD_PUSH_OFFSET_DEGREES = 10;
static const uint32_t ROBOT_DOG_FORWARD_STEP_INTERVAL_MS = 200;
static const int ROBOT_DOG_BACKWARD_SWING_OFFSET_DEGREES = 15;
static const int ROBOT_DOG_BACKWARD_PUSH_OFFSET_DEGREES = 10;
static const uint32_t ROBOT_DOG_BACKWARD_STEP_INTERVAL_MS = 200;
static const int ROBOT_DOG_LEFT_TURN_SWING_OFFSET_DEGREES = 15;
static const int ROBOT_DOG_LEFT_TURN_PUSH_OFFSET_DEGREES = 10;
static const uint32_t ROBOT_DOG_LEFT_TURN_STEP_INTERVAL_MS = 200;
static const int ROBOT_DOG_RIGHT_TURN_SWING_OFFSET_DEGREES = 15;
static const int ROBOT_DOG_RIGHT_TURN_PUSH_OFFSET_DEGREES = 10;
static const uint32_t ROBOT_DOG_RIGHT_TURN_STEP_INTERVAL_MS = 200;
static const int ROBOT_DOG_LEFT_TURN_DIRECTIONS[ROBOT_DOG_SERVO_COUNT] = {
    1,
    1,
    1,
    1
};
static const int ROBOT_DOG_RIGHT_TURN_DIRECTIONS[ROBOT_DOG_SERVO_COUNT] = {
    1,
    1,
    1,
    1
};

static const uint8_t ROBOT_DOG_ACTION_FRAME_COUNT = 0;
static const int ROBOT_DOG_ACTION_FRAME_ANGLES[16][ROBOT_DOG_SERVO_COUNT] = {
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82},
    {90, 99, 82, 82}
};
static const uint32_t ROBOT_DOG_ACTION_FRAME_DURATIONS_MS[16] = {
    400,
    400,
    400,
    400,
    400,
    400,
    400,
    400,
    400,
    400,
    400,
    400,
    400,
    400,
    400,
    400
};

#endif
