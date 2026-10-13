#include "RobotDogController.hpp"
#include "config.h"
#include <ESP32Servo.h>

#define SERVO_PWM_FREQUENCY_HZ 50

static const uint8_t SERVO_COUNT = ROBOT_DOG_SERVO_COUNT;
static const uint8_t ACTION_FRAME_CAPACITY = 16;
// 常见舵机使用 500us~2500us 脉宽表示 0~180 度。
static const int SERVO_MIN_PULSE_US = 500;
static const int SERVO_MAX_PULSE_US = 2500;
// 机械结构安全角度范围，避免舵机打到机械限位。
static const int SERVO_MIN_ANGLE_DEGREES = 35;
static const int SERVO_MAX_ANGLE_DEGREES = 145;
static const int MIN_POSE_OFFSET_DEGREES = -45;
static const int MAX_POSE_OFFSET_DEGREES = 45;
static const int MIN_MOTION_OFFSET_DEGREES = 0;
static const int MAX_MOTION_OFFSET_DEGREES = 45;
static const uint32_t MIN_WALK_STEP_INTERVAL_MS = 120;
static const uint32_t MAX_WALK_STEP_INTERVAL_MS = 1000;
static const uint32_t MIN_ACTION_FRAME_DURATION_MS = 80;
static const uint32_t MAX_ACTION_FRAME_DURATION_MS = 10000;
static const int JOYSTICK_CENTER = 127;
static const int JOYSTICK_DEAD_ZONE = 18;
static Servo leg_servos[SERVO_COUNT];
static int servo_center_angles[SERVO_COUNT];
static int current_servo_angles[SERVO_COUNT];
static int leg_gait_directions[SERVO_COUNT];
static int rest_offset_degrees = ROBOT_DOG_REST_OFFSET_DEGREES;
static int half_crouch_offset_degrees = ROBOT_DOG_HALF_CROUCH_OFFSET_DEGREES;
static int crouch_offset_degrees = ROBOT_DOG_CROUCH_OFFSET_DEGREES;
static int manual_max_offset_degrees = ROBOT_DOG_MANUAL_MAX_OFFSET_DEGREES;
static int walk_swing_offset_degrees = ROBOT_DOG_WALK_SWING_OFFSET_DEGREES;
static int walk_push_offset_degrees = ROBOT_DOG_WALK_PUSH_OFFSET_DEGREES;
static int turn_swing_offset_degrees = ROBOT_DOG_TURN_SWING_OFFSET_DEGREES;
static int turn_push_offset_degrees = ROBOT_DOG_TURN_PUSH_OFFSET_DEGREES;
static uint32_t walk_step_interval_ms = ROBOT_DOG_WALK_STEP_INTERVAL_MS;
static int turn_direction_sign = ROBOT_DOG_TURN_DIRECTION_SIGN < 0 ? -1 : 1;

void RobotDogController::init(void)
{
    last_step_ms = 0;
    gait_step_index = 0;
    active_motion_command = MOTION_STOP;
    continuous_motion_started_by_ps2 = false;
    action_frame_count = 0;
    action_playback_active = false;
    action_playback_loop_enabled = false;
    playback_frame_index = 0;
    playback_frame_start_ms = 0;

    loadServoSettings();
    loadActionFrames();
    initializeServoPwm();

    applyStandPose();
}

void RobotDogController::tick(void)
{
    updateActionPlayback();

    if (!action_playback_active && active_motion_command != MOTION_STOP)
    {
        applyCurrentMotionStep();
    }
}

void RobotDogController::update(const PS2KeyValueObjectTypeDef &ps2_keyvalue)
{
    // 右侧功能键：叉=站立，方块=休息，圆圈=半蹲，三角=趴下。
    if (ps2_keyvalue.bit_cross)
    {
        applyStandPose();
        return;
    }

    if (ps2_keyvalue.bit_square)
    {
        applyRestPose();
        return;
    }

    if (ps2_keyvalue.bit_circle)
    {
        applyHalfCrouchPose();
        return;
    }

    if (ps2_keyvalue.bit_triangle)
    {
        applyCrouchPose();
        return;
    }

    if (ps2_keyvalue.mode == PS2_COORDINATE_MODE)
    {
        applyWalkGait(ps2_keyvalue);
        return;
    }

    if (ps2_keyvalue.bit_left || ps2_keyvalue.bit_right ||
        ps2_keyvalue.bit_up || ps2_keyvalue.bit_down)
    {
        applyManualPose(ps2_keyvalue);
        return;
    }

    if (continuous_motion_started_by_ps2 && active_motion_command != MOTION_STOP)
    {
        stopContinuousMotion();
    }
}

uint8_t RobotDogController::getLegCount(void) const
{
    return LEG_COUNT;
}

int RobotDogController::getServoCenterAngle(uint8_t leg_index) const
{
    return getCenterAngle(leg_index);
}

int RobotDogController::getCurrentServoAngle(uint8_t leg_index) const
{
    if (leg_index >= LEG_COUNT)
    {
        return ROBOT_DOG_SERVO_CENTER_ANGLES[0];
    }

    return current_servo_angles[leg_index];
}

void RobotDogController::setServoCenterAngle(uint8_t leg_index, int angle_degrees)
{
    if (leg_index >= LEG_COUNT)
    {
        return;
    }

    servo_center_angles[leg_index] = clampServoAngle(angle_degrees);
    writeServoAngle(leg_index, servo_center_angles[leg_index]);
}

int RobotDogController::getRestOffsetDegrees(void) const
{
    return rest_offset_degrees;
}

int RobotDogController::getHalfCrouchOffsetDegrees(void) const
{
    return half_crouch_offset_degrees;
}

int RobotDogController::getCrouchOffsetDegrees(void) const
{
    return crouch_offset_degrees;
}

int RobotDogController::getManualMaxOffsetDegrees(void) const
{
    return manual_max_offset_degrees;
}

int RobotDogController::getWalkMaxOffsetDegrees(void) const
{
    return walk_swing_offset_degrees;
}

int RobotDogController::getWalkSwingOffsetDegrees(void) const
{
    return walk_swing_offset_degrees;
}

int RobotDogController::getWalkPushOffsetDegrees(void) const
{
    return walk_push_offset_degrees;
}

int RobotDogController::getTurnSwingOffsetDegrees(void) const
{
    return turn_swing_offset_degrees;
}

int RobotDogController::getTurnPushOffsetDegrees(void) const
{
    return turn_push_offset_degrees;
}

int RobotDogController::getTurnDirectionSign(void) const
{
    return turn_direction_sign;
}

uint32_t RobotDogController::getWalkStepIntervalMs(void) const
{
    return walk_step_interval_ms;
}

int RobotDogController::getLegGaitDirection(uint8_t leg_index) const
{
    if (leg_index >= LEG_COUNT)
    {
        return 1;
    }

    return leg_gait_directions[leg_index];
}

void RobotDogController::setLegGaitDirection(uint8_t leg_index, int direction_sign)
{
    if (leg_index >= LEG_COUNT)
    {
        return;
    }

    leg_gait_directions[leg_index] = direction_sign < 0 ? -1 : 1;
}

void RobotDogController::setMotionParameters(
    int new_rest_offset_degrees,
    int new_half_crouch_offset_degrees,
    int new_crouch_offset_degrees,
    int new_manual_max_offset_degrees,
    int new_walk_swing_offset_degrees,
    int new_walk_push_offset_degrees,
    int new_turn_swing_offset_degrees,
    int new_turn_push_offset_degrees,
    uint32_t new_walk_step_interval_ms,
    int new_turn_direction_sign
)
{
    rest_offset_degrees = constrain(
        new_rest_offset_degrees,
        MIN_POSE_OFFSET_DEGREES,
        MAX_POSE_OFFSET_DEGREES
    );
    half_crouch_offset_degrees = constrain(
        new_half_crouch_offset_degrees,
        MIN_POSE_OFFSET_DEGREES,
        MAX_POSE_OFFSET_DEGREES
    );
    crouch_offset_degrees = constrain(
        new_crouch_offset_degrees,
        MIN_POSE_OFFSET_DEGREES,
        MAX_POSE_OFFSET_DEGREES
    );
    manual_max_offset_degrees = constrain(
        new_manual_max_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    walk_swing_offset_degrees = constrain(
        new_walk_swing_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    walk_push_offset_degrees = constrain(
        new_walk_push_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    turn_swing_offset_degrees = constrain(
        new_turn_swing_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    turn_push_offset_degrees = constrain(
        new_turn_push_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    walk_step_interval_ms = constrain(
        new_walk_step_interval_ms,
        MIN_WALK_STEP_INTERVAL_MS,
        MAX_WALK_STEP_INTERVAL_MS
    );
    turn_direction_sign = new_turn_direction_sign < 0 ? -1 : 1;
}

void RobotDogController::resetServoCenterAngles(void)
{
    loadServoSettings();
    loadActionFrames();

    applyStandPose();
}

void RobotDogController::applyStandPose(void)
{
    stopActionPlayback();
    stopContinuousMotionWithoutPose();

    // 站立姿态：4 个自由度全部回到已保存的中位。
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        writeServoAngle(leg_index, getCenterAngle(leg_index));
    }
}

void RobotDogController::applyRestPose(void)
{
    stopActionPlayback();
    stopContinuousMotionWithoutPose();
    applyOffsetPose(rest_offset_degrees);
}

void RobotDogController::applyHalfCrouchPose(void)
{
    stopActionPlayback();
    stopContinuousMotionWithoutPose();
    applyOffsetPose(half_crouch_offset_degrees);
}

void RobotDogController::applyCrouchPose(void)
{
    stopActionPlayback();
    stopContinuousMotionWithoutPose();
    applyOffsetPose(crouch_offset_degrees);
}

void RobotDogController::setPoseAngles(
    int front_left_angle,
    int front_right_angle,
    int rear_left_angle,
    int rear_right_angle
)
{
    stopActionPlayback();
    stopContinuousMotionWithoutPose();

    writeServoAngle(LEG_FRONT_LEFT, front_left_angle);
    writeServoAngle(LEG_FRONT_RIGHT, front_right_angle);
    writeServoAngle(LEG_REAR_LEFT, rear_left_angle);
    writeServoAngle(LEG_REAR_RIGHT, rear_right_angle);
}

void RobotDogController::startContinuousMotion(MotionCommand motion_command)
{
    setContinuousMotion(motion_command, false);
}

void RobotDogController::stopContinuousMotion(void)
{
    stopContinuousMotionWithoutPose();
    applyStandPose();
}

RobotDogController::MotionCommand RobotDogController::getContinuousMotionCommand(void) const
{
    return active_motion_command;
}

uint8_t RobotDogController::getActionFrameCapacity(void) const
{
    return ACTION_FRAME_CAPACITY;
}

uint8_t RobotDogController::getActionFrameCount(void) const
{
    return action_frame_count;
}

bool RobotDogController::getActionFrame(uint8_t frame_index, ActionFrame *action_frame) const
{
    if (frame_index >= action_frame_count || action_frame == NULL)
    {
        return false;
    }

    *action_frame = action_frames[frame_index];
    return true;
}

bool RobotDogController::addActionFrame(
    int front_left_angle,
    int front_right_angle,
    int rear_left_angle,
    int rear_right_angle,
    uint32_t duration_ms
)
{
    if (action_frame_count >= ACTION_FRAME_CAPACITY)
    {
        return false;
    }

    if (!setActionFrame(
            action_frame_count,
            front_left_angle,
            front_right_angle,
            rear_left_angle,
            rear_right_angle,
            duration_ms
        ))
    {
        return false;
    }

    action_frame_count++;
    return true;
}

bool RobotDogController::updateActionFrame(
    uint8_t frame_index,
    int front_left_angle,
    int front_right_angle,
    int rear_left_angle,
    int rear_right_angle,
    uint32_t duration_ms
)
{
    if (frame_index >= action_frame_count)
    {
        return false;
    }

    return setActionFrame(
        frame_index,
        front_left_angle,
        front_right_angle,
        rear_left_angle,
        rear_right_angle,
        duration_ms
    );
}

bool RobotDogController::removeActionFrame(uint8_t frame_index)
{
    if (frame_index >= action_frame_count)
    {
        return false;
    }

    for (uint8_t next_frame_index = frame_index + 1;
         next_frame_index < action_frame_count;
         next_frame_index++)
    {
        action_frames[next_frame_index - 1] = action_frames[next_frame_index];
    }

    action_frame_count--;
    if (action_frame_count == 0)
    {
        stopActionPlayback();
    }
    else if (playback_frame_index >= action_frame_count)
    {
        playback_frame_index = action_frame_count - 1;
    }
    return true;
}

void RobotDogController::clearActionFrames(void)
{
    stopActionPlayback();
    action_frame_count = 0;
}

void RobotDogController::playActionFrames(bool loop_enabled)
{
    if (action_frame_count == 0)
    {
        return;
    }

    stopContinuousMotionWithoutPose();
    action_playback_active = true;
    action_playback_loop_enabled = loop_enabled;
    playback_frame_index = 0;
    playback_frame_start_ms = millis();

    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        writeServoAngle(leg_index, action_frames[0].angles[leg_index]);
    }
}

void RobotDogController::stopActionPlayback(void)
{
    action_playback_active = false;
    action_playback_loop_enabled = false;
    playback_frame_index = 0;
    playback_frame_start_ms = millis();
}

bool RobotDogController::isActionPlaybackActive(void) const
{
    return action_playback_active;
}

bool RobotDogController::isActionPlaybackLoopEnabled(void) const
{
    return action_playback_loop_enabled;
}

uint8_t RobotDogController::getPlaybackFrameIndex(void) const
{
    return playback_frame_index;
}

void RobotDogController::writeServoAngle(uint8_t leg_index, int angle_degrees)
{
    if (leg_index >= LEG_COUNT)
    {
        return;
    }

    // 所有角度输出都先经过安全限幅，再交给 ESP32Servo 生成舵机 PWM。
    int safe_angle_degrees = clampServoAngle(angle_degrees);
    current_servo_angles[leg_index] = safe_angle_degrees;
    leg_servos[leg_index].write(safe_angle_degrees);
}

int RobotDogController::clampServoAngle(int angle_degrees) const
{
    if (angle_degrees < SERVO_MIN_ANGLE_DEGREES)
    {
        return SERVO_MIN_ANGLE_DEGREES;
    }

    if (angle_degrees > SERVO_MAX_ANGLE_DEGREES)
    {
        return SERVO_MAX_ANGLE_DEGREES;
    }

    return angle_degrees;
}

int RobotDogController::getCenterAngle(uint8_t leg_index) const
{
    if (leg_index >= LEG_COUNT)
    {
        return ROBOT_DOG_SERVO_CENTER_ANGLES[0];
    }

    return servo_center_angles[leg_index];
}

void RobotDogController::loadServoSettings(void)
{
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        servo_center_angles[leg_index] = clampServoAngle(
            ROBOT_DOG_SERVO_CENTER_ANGLES[leg_index]
        );
        leg_gait_directions[leg_index] = ROBOT_DOG_GAIT_DIRECTIONS[leg_index] < 0 ? -1 : 1;
        current_servo_angles[leg_index] = servo_center_angles[leg_index];
    }
    rest_offset_degrees = constrain(
        ROBOT_DOG_REST_OFFSET_DEGREES,
        MIN_POSE_OFFSET_DEGREES,
        MAX_POSE_OFFSET_DEGREES
    );
    half_crouch_offset_degrees = constrain(
        ROBOT_DOG_HALF_CROUCH_OFFSET_DEGREES,
        MIN_POSE_OFFSET_DEGREES,
        MAX_POSE_OFFSET_DEGREES
    );
    crouch_offset_degrees = constrain(
        ROBOT_DOG_CROUCH_OFFSET_DEGREES,
        MIN_POSE_OFFSET_DEGREES,
        MAX_POSE_OFFSET_DEGREES
    );
    manual_max_offset_degrees = constrain(
        ROBOT_DOG_MANUAL_MAX_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    walk_swing_offset_degrees = constrain(
        ROBOT_DOG_WALK_SWING_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    walk_push_offset_degrees = constrain(
        ROBOT_DOG_WALK_PUSH_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    turn_swing_offset_degrees = constrain(
        ROBOT_DOG_TURN_SWING_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    turn_push_offset_degrees = constrain(
        ROBOT_DOG_TURN_PUSH_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    walk_step_interval_ms = constrain(
        ROBOT_DOG_WALK_STEP_INTERVAL_MS,
        MIN_WALK_STEP_INTERVAL_MS,
        MAX_WALK_STEP_INTERVAL_MS
    );
    turn_direction_sign = ROBOT_DOG_TURN_DIRECTION_SIGN < 0 ? -1 : 1;
}

void RobotDogController::loadActionFrames(void)
{
    action_frame_count = 0;
    uint8_t config_frame_count = ROBOT_DOG_ACTION_FRAME_COUNT;
    if (config_frame_count > ACTION_FRAME_CAPACITY)
    {
        config_frame_count = ACTION_FRAME_CAPACITY;
    }

    for (uint8_t frame_index = 0; frame_index < config_frame_count; frame_index++)
    {
        if (addActionFrame(
                ROBOT_DOG_ACTION_FRAME_ANGLES[frame_index][LEG_FRONT_LEFT],
                ROBOT_DOG_ACTION_FRAME_ANGLES[frame_index][LEG_FRONT_RIGHT],
                ROBOT_DOG_ACTION_FRAME_ANGLES[frame_index][LEG_REAR_LEFT],
                ROBOT_DOG_ACTION_FRAME_ANGLES[frame_index][LEG_REAR_RIGHT],
                ROBOT_DOG_ACTION_FRAME_DURATIONS_MS[frame_index]
            ))
        {
            continue;
        }
        break;
    }
}

int RobotDogController::mapJoystickToOffset(uint8_t joystick_value, int max_offset_degrees) const
{
    // 摇杆原始值约为 0~255，中位值约为 127，死区用于过滤手柄轻微抖动。
    int offset = (int)joystick_value - JOYSTICK_CENTER;
    if (abs(offset) <= JOYSTICK_DEAD_ZONE)
    {
        return 0;
    }

    return map(offset, -JOYSTICK_CENTER, JOYSTICK_CENTER, -max_offset_degrees, max_offset_degrees);
}

bool RobotDogController::isJoystickOutsideDeadZone(uint8_t joystick_value) const
{
    return abs((int)joystick_value - JOYSTICK_CENTER) > JOYSTICK_DEAD_ZONE;
}

void RobotDogController::initializeServoPwm(void)
{
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        leg_servos[leg_index].setPeriodHertz(SERVO_PWM_FREQUENCY_HZ);
        leg_servos[leg_index].attach(
            ROBOT_DOG_SERVO_PINS[leg_index],
            SERVO_MIN_PULSE_US,
            SERVO_MAX_PULSE_US
        );
    }
}

void RobotDogController::stopContinuousMotionWithoutPose(void)
{
    active_motion_command = MOTION_STOP;
    continuous_motion_started_by_ps2 = false;
    gait_step_index = 0;
}

void RobotDogController::setContinuousMotion(MotionCommand motion_command, bool started_by_ps2)
{
    if (motion_command == MOTION_STOP)
    {
        stopContinuousMotion();
        return;
    }

    stopActionPlayback();

    if (active_motion_command != motion_command ||
        continuous_motion_started_by_ps2 != started_by_ps2)
    {
        gait_step_index = 0;
        last_step_ms = millis() - walk_step_interval_ms;
    }

    active_motion_command = motion_command;
    continuous_motion_started_by_ps2 = started_by_ps2;
}

void RobotDogController::applyManualPose(const PS2KeyValueObjectTypeDef &ps2_keyvalue)
{
    stopActionPlayback();
    stopContinuousMotionWithoutPose();

    // 方向键控制整体姿态偏移：左右影响左右两侧角度，上下影响前后两侧角度。
    int x_offset = 0;
    int y_offset = 0;

    if (ps2_keyvalue.bit_left)
    {
        x_offset = -manual_max_offset_degrees;
    }
    else if (ps2_keyvalue.bit_right)
    {
        x_offset = manual_max_offset_degrees;
    }

    if (ps2_keyvalue.bit_up)
    {
        y_offset = manual_max_offset_degrees;
    }
    else if (ps2_keyvalue.bit_down)
    {
        y_offset = -manual_max_offset_degrees;
    }

    writeServoAngle(LEG_FRONT_LEFT, getCenterAngle(LEG_FRONT_LEFT) + y_offset + x_offset);
    writeServoAngle(LEG_FRONT_RIGHT, getCenterAngle(LEG_FRONT_RIGHT) + y_offset - x_offset);
    writeServoAngle(LEG_REAR_LEFT, getCenterAngle(LEG_REAR_LEFT) - y_offset + x_offset);
    writeServoAngle(LEG_REAR_RIGHT, getCenterAngle(LEG_REAR_RIGHT) - y_offset - x_offset);
}

void RobotDogController::applyWalkGait(const PS2KeyValueObjectTypeDef &ps2_keyvalue)
{
    bool has_joystick_forward_input = isJoystickOutsideDeadZone(ps2_keyvalue.left_joystick_y);
    bool has_joystick_turn_input = isJoystickOutsideDeadZone(ps2_keyvalue.left_joystick_x);
    bool has_button_forward_input = ps2_keyvalue.bit_up || ps2_keyvalue.bit_down;
    bool has_button_turn_input = ps2_keyvalue.bit_left || ps2_keyvalue.bit_right;

    if (!has_joystick_forward_input && !has_joystick_turn_input &&
        !has_button_forward_input && !has_button_turn_input)
    {
        if (continuous_motion_started_by_ps2 && active_motion_command != MOTION_STOP)
        {
            stopContinuousMotion();
        }
        return;
    }

    int forward_strength = 0;
    int turn_strength = 0;
    if (has_button_forward_input)
    {
        forward_strength = ps2_keyvalue.bit_up ? 100 : -100;
    }
    else if (has_joystick_forward_input)
    {
        forward_strength = -mapJoystickToOffset(ps2_keyvalue.left_joystick_y, 100);
    }

    if (has_button_turn_input)
    {
        turn_strength = ps2_keyvalue.bit_right ? 100 : -100;
    }
    else if (has_joystick_turn_input)
    {
        turn_strength = mapJoystickToOffset(ps2_keyvalue.left_joystick_x, 100);
    }

    MotionCommand next_motion_command = MOTION_STOP;

    if (abs(forward_strength) >= abs(turn_strength))
    {
        next_motion_command = forward_strength >= 0 ? MOTION_FORWARD : MOTION_BACKWARD;
    }
    else
    {
        next_motion_command = turn_strength >= 0 ? MOTION_RIGHT : MOTION_LEFT;
    }

    setContinuousMotion(next_motion_command, true);
    applyCurrentMotionStep();
}

void RobotDogController::applyOffsetPose(int offset_degrees)
{
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        writeServoAngle(leg_index, getCenterAngle(leg_index) + offset_degrees);
    }
}

void RobotDogController::applyCurrentMotionStep(void)
{
    uint32_t current_ms = millis();
    if (current_ms - last_step_ms < walk_step_interval_ms)
    {
        return;
    }

    last_step_ms = current_ms;

    if (active_motion_command == MOTION_FORWARD || active_motion_command == MOTION_BACKWARD)
    {
        applyWalkMotionStep(active_motion_command, gait_step_index);
    }
    else if (active_motion_command == MOTION_LEFT || active_motion_command == MOTION_RIGHT)
    {
        applyTurnMotionStep(active_motion_command, gait_step_index);
    }

    gait_step_index = (gait_step_index + 1) % (LEG_COUNT * 2);
}

void RobotDogController::applyWalkMotionStep(MotionCommand motion_command, uint8_t step_index)
{
    static const uint8_t DIAGONAL_A_LEGS[] = {
        LEG_FRONT_LEFT,
        LEG_REAR_RIGHT
    };
    static const uint8_t DIAGONAL_B_LEGS[] = {
        LEG_FRONT_RIGHT,
        LEG_REAR_LEFT
    };
    int target_angles[LEG_COUNT];
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        target_angles[leg_index] = getCenterAngle(leg_index);
    }

    bool is_diagonal_a_swing_step = step_index % 2 == 0;
    int walk_direction = motion_command == MOTION_FORWARD ? 1 : -1;
    int swing_offset = -walk_direction * walk_swing_offset_degrees;
    int push_offset = walk_direction * walk_push_offset_degrees;

    const uint8_t *swing_legs = is_diagonal_a_swing_step ? DIAGONAL_A_LEGS : DIAGONAL_B_LEGS;
    const uint8_t *push_legs = is_diagonal_a_swing_step ? DIAGONAL_B_LEGS : DIAGONAL_A_LEGS;
    for (uint8_t pair_index = 0; pair_index < 2; pair_index++)
    {
        uint8_t swing_leg = swing_legs[pair_index];
        uint8_t push_leg = push_legs[pair_index];
        target_angles[swing_leg] += swing_offset * leg_gait_directions[swing_leg];
        target_angles[push_leg] += push_offset * leg_gait_directions[push_leg];
    }

    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        writeServoAngle(leg_index, target_angles[leg_index]);
    }
}

void RobotDogController::applyTurnMotionStep(MotionCommand motion_command, uint8_t step_index)
{
    static const uint8_t LEG_SEQUENCE[LEG_COUNT] = {
        LEG_FRONT_LEFT,
        LEG_REAR_RIGHT,
        LEG_FRONT_RIGHT,
        LEG_REAR_LEFT
    };
    int target_angles[LEG_COUNT];
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        target_angles[leg_index] = getCenterAngle(leg_index);
    }

    uint8_t active_leg = LEG_SEQUENCE[(step_index / 2) % LEG_COUNT];
    bool is_swing_step = step_index % 2 == 0;
    bool is_left_leg = active_leg == LEG_FRONT_LEFT || active_leg == LEG_REAR_LEFT;
    int turn_command_sign = motion_command == MOTION_LEFT ? 1 : -1;
    int side_walk_direction = is_left_leg
        ? -turn_command_sign * turn_direction_sign
        : turn_command_sign * turn_direction_sign;
    int leg_offset = is_swing_step
        ? -side_walk_direction * turn_swing_offset_degrees
        : side_walk_direction * turn_push_offset_degrees;

    target_angles[active_leg] += leg_offset * leg_gait_directions[active_leg];

    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        writeServoAngle(leg_index, target_angles[leg_index]);
    }
}

void RobotDogController::updateActionPlayback(void)
{
    if (!action_playback_active)
    {
        return;
    }

    if (action_frame_count == 0)
    {
        stopActionPlayback();
        return;
    }

    uint32_t current_ms = millis();
    if (current_ms - playback_frame_start_ms <
        action_frames[playback_frame_index].duration_ms)
    {
        return;
    }

    playback_frame_index++;
    if (playback_frame_index >= action_frame_count)
    {
        if (!action_playback_loop_enabled)
        {
            action_playback_active = false;
            playback_frame_index = action_frame_count - 1;
            return;
        }
        playback_frame_index = 0;
    }

    playback_frame_start_ms = current_ms;
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        writeServoAngle(leg_index, action_frames[playback_frame_index].angles[leg_index]);
    }
}

bool RobotDogController::setActionFrame(
    uint8_t frame_index,
    int front_left_angle,
    int front_right_angle,
    int rear_left_angle,
    int rear_right_angle,
    uint32_t duration_ms
)
{
    if (frame_index >= ACTION_FRAME_CAPACITY)
    {
        return false;
    }

    action_frames[frame_index].angles[LEG_FRONT_LEFT] = clampServoAngle(front_left_angle);
    action_frames[frame_index].angles[LEG_FRONT_RIGHT] = clampServoAngle(front_right_angle);
    action_frames[frame_index].angles[LEG_REAR_LEFT] = clampServoAngle(rear_left_angle);
    action_frames[frame_index].angles[LEG_REAR_RIGHT] = clampServoAngle(rear_right_angle);
    action_frames[frame_index].duration_ms = constrain(
        duration_ms,
        MIN_ACTION_FRAME_DURATION_MS,
        MAX_ACTION_FRAME_DURATION_MS
    );
    return true;
}
