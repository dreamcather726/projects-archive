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
    90,
    90,
    90
};

static const int ROBOT_DOG_GAIT_DIRECTIONS[ROBOT_DOG_SERVO_COUNT] = {
    1,
    1,
    1,
    1
};

static const int ROBOT_DOG_REST_OFFSET_DEGREES = -20;
static const int ROBOT_DOG_HALF_CROUCH_OFFSET_DEGREES = -12;
static const int ROBOT_DOG_CROUCH_OFFSET_DEGREES = -35;
static const int ROBOT_DOG_MANUAL_MAX_OFFSET_DEGREES = 25;
static const int ROBOT_DOG_WALK_SWING_OFFSET_DEGREES = 45;
static const int ROBOT_DOG_WALK_PUSH_OFFSET_DEGREES = 10;
static const int ROBOT_DOG_TURN_SWING_OFFSET_DEGREES = 35;
static const int ROBOT_DOG_TURN_PUSH_OFFSET_DEGREES = 12;
static const uint32_t ROBOT_DOG_WALK_STEP_INTERVAL_MS = 260;
static const int ROBOT_DOG_TURN_DIRECTION_SIGN = 1;

static const uint8_t ROBOT_DOG_ACTION_FRAME_COUNT = 0;
static const int ROBOT_DOG_ACTION_FRAME_ANGLES[16][ROBOT_DOG_SERVO_COUNT] = {
    {90, 90, 90, 90}
};
static const uint32_t ROBOT_DOG_ACTION_FRAME_DURATIONS_MS[16] = {
    400
};

#endif
