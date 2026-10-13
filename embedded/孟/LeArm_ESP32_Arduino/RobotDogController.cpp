#include "RobotDogController.hpp"
#include "config.h"
#include <ESP32Servo.h>
#include <Preferences.h>
#include <stddef.h>

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
static const char *SETTINGS_NAMESPACE = "robotdog";
static const char *SETTINGS_KEY = "settings";
static const char *MOTION_PARAMETER_SETTINGS_KEY = "motion_params";
static const char *TURN_DIRECTION_SETTINGS_KEY = "turn_dirs";
static const char *LEG_MOTION_SETTINGS_KEY = "leg_motion";
static const uint32_t SETTINGS_STORAGE_MAGIC = 0x52444732;
static const uint16_t SETTINGS_STORAGE_VERSION = 2;
static const uint32_t MOTION_PARAMETER_STORAGE_MAGIC = 0x4D504152;
static const uint16_t MOTION_PARAMETER_STORAGE_VERSION = 1;
static const uint32_t TURN_DIRECTION_STORAGE_MAGIC = 0x54444952;
static const uint16_t TURN_DIRECTION_STORAGE_VERSION = 2;
static const uint32_t LEG_MOTION_STORAGE_MAGIC = 0x4C4D4F54;
static const uint16_t LEG_MOTION_STORAGE_VERSION = 1;
static Servo leg_servos[SERVO_COUNT];
static int servo_center_angles[SERVO_COUNT];
static int current_servo_angles[SERVO_COUNT];
static int leg_gait_directions[SERVO_COUNT];
static int leg_forward_offsets_degrees[SERVO_COUNT];
static int leg_backward_offsets_degrees[SERVO_COUNT];
static int rest_pose_angles[SERVO_COUNT];
static int half_crouch_pose_angles[SERVO_COUNT];
static int crouch_pose_angles[SERVO_COUNT];
static int rest_offset_degrees = ROBOT_DOG_REST_OFFSET_DEGREES;
static int half_crouch_offset_degrees = ROBOT_DOG_HALF_CROUCH_OFFSET_DEGREES;
static int crouch_offset_degrees = ROBOT_DOG_CROUCH_OFFSET_DEGREES;
static int manual_max_offset_degrees = ROBOT_DOG_MANUAL_MAX_OFFSET_DEGREES;
static int forward_swing_offset_degrees = ROBOT_DOG_FORWARD_SWING_OFFSET_DEGREES;
static int forward_push_offset_degrees = ROBOT_DOG_FORWARD_PUSH_OFFSET_DEGREES;
static uint32_t forward_step_interval_ms = ROBOT_DOG_FORWARD_STEP_INTERVAL_MS;
static int backward_swing_offset_degrees = ROBOT_DOG_BACKWARD_SWING_OFFSET_DEGREES;
static int backward_push_offset_degrees = ROBOT_DOG_BACKWARD_PUSH_OFFSET_DEGREES;
static uint32_t backward_step_interval_ms = ROBOT_DOG_BACKWARD_STEP_INTERVAL_MS;
static int left_turn_swing_offset_degrees = ROBOT_DOG_LEFT_TURN_SWING_OFFSET_DEGREES;
static int left_turn_push_offset_degrees = ROBOT_DOG_LEFT_TURN_PUSH_OFFSET_DEGREES;
static uint32_t left_turn_step_interval_ms = ROBOT_DOG_LEFT_TURN_STEP_INTERVAL_MS;
static int right_turn_swing_offset_degrees = ROBOT_DOG_RIGHT_TURN_SWING_OFFSET_DEGREES;
static int right_turn_push_offset_degrees = ROBOT_DOG_RIGHT_TURN_PUSH_OFFSET_DEGREES;
static uint32_t right_turn_step_interval_ms = ROBOT_DOG_RIGHT_TURN_STEP_INTERVAL_MS;
static int left_turn_leg_directions[SERVO_COUNT];
static int right_turn_leg_directions[SERVO_COUNT];

struct StoredSettings
{
    uint32_t magic;
    uint16_t version;
    uint16_t size_bytes;
    int center_angles[SERVO_COUNT];
    int gait_directions[SERVO_COUNT];
    int rest_angles[SERVO_COUNT];
    int half_crouch_angles[SERVO_COUNT];
    int crouch_angles[SERVO_COUNT];
    int rest_offset_degrees;
    int half_crouch_offset_degrees;
    int crouch_offset_degrees;
    int manual_max_offset_degrees;
    int walk_swing_offset_degrees;
    int walk_push_offset_degrees;
    int turn_swing_offset_degrees;
    int turn_push_offset_degrees;
    uint32_t walk_step_interval_ms;
    int turn_direction_sign;
    uint8_t action_frame_count;
    uint8_t reserved[3];
    RobotDogController::ActionFrame action_frames[ACTION_FRAME_CAPACITY];
    uint32_t checksum;
};

struct LegMotionSettings
{
    uint32_t magic;
    uint16_t version;
    uint16_t size_bytes;
    int forward_offsets_degrees[SERVO_COUNT];
    int backward_offsets_degrees[SERVO_COUNT];
    uint32_t checksum;
};

struct MotionParameterSettings
{
    uint32_t magic;
    uint16_t version;
    uint16_t size_bytes;
    int forward_swing_offset_degrees;
    int forward_push_offset_degrees;
    uint32_t forward_step_interval_ms;
    int backward_swing_offset_degrees;
    int backward_push_offset_degrees;
    uint32_t backward_step_interval_ms;
    int left_turn_swing_offset_degrees;
    int left_turn_push_offset_degrees;
    uint32_t left_turn_step_interval_ms;
    int right_turn_swing_offset_degrees;
    int right_turn_push_offset_degrees;
    uint32_t right_turn_step_interval_ms;
    uint32_t checksum;
};

struct TurnDirectionSettings
{
    uint32_t magic;
    uint16_t version;
    uint16_t size_bytes;
    int left_leg_directions[SERVO_COUNT];
    int right_leg_directions[SERVO_COUNT];
    uint32_t checksum;
};

struct LegacyTurnDirectionSettingsV1
{
    uint32_t magic;
    uint16_t version;
    uint16_t size_bytes;
    int left_direction_sign;
    int right_direction_sign;
    uint32_t checksum;
};

static uint32_t calculateSettingsChecksum(const StoredSettings &stored_settings)
{
    const uint8_t *stored_bytes = reinterpret_cast<const uint8_t *>(&stored_settings);
    uint32_t checksum = 2166136261UL;
    for (size_t byte_index = 0;
         byte_index < offsetof(StoredSettings, checksum);
         byte_index++)
    {
        checksum ^= stored_bytes[byte_index];
        checksum *= 16777619UL;
    }
    return checksum;
}

static bool isStoredSettingsValid(const StoredSettings &stored_settings)
{
    return stored_settings.magic == SETTINGS_STORAGE_MAGIC &&
        stored_settings.version == SETTINGS_STORAGE_VERSION &&
        stored_settings.size_bytes == sizeof(StoredSettings) &&
        stored_settings.action_frame_count <= ACTION_FRAME_CAPACITY &&
        stored_settings.checksum == calculateSettingsChecksum(stored_settings);
}

static uint32_t calculateLegMotionSettingsChecksum(
    const LegMotionSettings &motion_settings
)
{
    const uint8_t *stored_bytes = reinterpret_cast<const uint8_t *>(&motion_settings);
    uint32_t checksum = 2166136261UL;
    for (size_t byte_index = 0;
         byte_index < offsetof(LegMotionSettings, checksum);
         byte_index++)
    {
        checksum ^= stored_bytes[byte_index];
        checksum *= 16777619UL;
    }
    return checksum;
}

static bool isLegMotionSettingsValid(const LegMotionSettings &motion_settings)
{
    if (motion_settings.magic != LEG_MOTION_STORAGE_MAGIC ||
        motion_settings.version != LEG_MOTION_STORAGE_VERSION ||
        motion_settings.size_bytes != sizeof(LegMotionSettings) ||
        motion_settings.checksum != calculateLegMotionSettingsChecksum(motion_settings))
    {
        return false;
    }

    for (uint8_t leg_index = 0; leg_index < SERVO_COUNT; leg_index++)
    {
        if (motion_settings.forward_offsets_degrees[leg_index] < -MAX_MOTION_OFFSET_DEGREES ||
            motion_settings.forward_offsets_degrees[leg_index] > MAX_MOTION_OFFSET_DEGREES ||
            motion_settings.backward_offsets_degrees[leg_index] < -MAX_MOTION_OFFSET_DEGREES ||
            motion_settings.backward_offsets_degrees[leg_index] > MAX_MOTION_OFFSET_DEGREES)
        {
            return false;
        }
    }
    return true;
}

static uint32_t calculateMotionParameterSettingsChecksum(
    const MotionParameterSettings &motion_settings
)
{
    const uint8_t *stored_bytes = reinterpret_cast<const uint8_t *>(&motion_settings);
    uint32_t checksum = 2166136261UL;
    for (size_t byte_index = 0;
         byte_index < offsetof(MotionParameterSettings, checksum);
         byte_index++)
    {
        checksum ^= stored_bytes[byte_index];
        checksum *= 16777619UL;
    }
    return checksum;
}

static bool isMotionParameterSettingsValid(
    const MotionParameterSettings &motion_settings
)
{
    return motion_settings.magic == MOTION_PARAMETER_STORAGE_MAGIC &&
        motion_settings.version == MOTION_PARAMETER_STORAGE_VERSION &&
        motion_settings.size_bytes == sizeof(MotionParameterSettings) &&
        motion_settings.checksum ==
            calculateMotionParameterSettingsChecksum(motion_settings);
}

static uint32_t calculateTurnDirectionSettingsChecksum(
    const TurnDirectionSettings &direction_settings
)
{
    const uint8_t *stored_bytes =
        reinterpret_cast<const uint8_t *>(&direction_settings);
    uint32_t checksum = 2166136261UL;
    for (size_t byte_index = 0;
         byte_index < offsetof(TurnDirectionSettings, checksum);
         byte_index++)
    {
        checksum ^= stored_bytes[byte_index];
        checksum *= 16777619UL;
    }
    return checksum;
}

static bool isTurnDirectionSettingsValid(
    const TurnDirectionSettings &direction_settings
)
{
    return direction_settings.magic == TURN_DIRECTION_STORAGE_MAGIC &&
        direction_settings.version == TURN_DIRECTION_STORAGE_VERSION &&
        direction_settings.size_bytes == sizeof(TurnDirectionSettings) &&
        direction_settings.checksum ==
            calculateTurnDirectionSettingsChecksum(direction_settings);
}

static uint32_t calculateLegacyTurnDirectionSettingsChecksum(
    const LegacyTurnDirectionSettingsV1 &direction_settings
)
{
    const uint8_t *stored_bytes =
        reinterpret_cast<const uint8_t *>(&direction_settings);
    uint32_t checksum = 2166136261UL;
    for (size_t byte_index = 0;
         byte_index < offsetof(LegacyTurnDirectionSettingsV1, checksum);
         byte_index++)
    {
        checksum ^= stored_bytes[byte_index];
        checksum *= 16777619UL;
    }
    return checksum;
}

static bool isLegacyTurnDirectionSettingsValid(
    const LegacyTurnDirectionSettingsV1 &direction_settings
)
{
    return direction_settings.magic == TURN_DIRECTION_STORAGE_MAGIC &&
        direction_settings.version == 1 &&
        direction_settings.size_bytes == sizeof(LegacyTurnDirectionSettingsV1) &&
        direction_settings.checksum ==
            calculateLegacyTurnDirectionSettingsChecksum(direction_settings);
}

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
    ps2_l1_was_pressed = false;
    ps2_r1_was_pressed = false;
    ps2_r2_was_pressed = false;
    has_saved_settings = false;
    settings_dirty = false;

    loadServoSettings();
    loadActionFrames();
    loadSavedSettings();
    loadMotionParameterSettings();
    loadTurnDirectionSettings();
    initializeLegMotionOffsets();
    loadLegMotionOffsets();
    settings_dirty = false;
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
    bool l1_pressed = ps2_keyvalue.bit_l1 != 0;
    bool r1_pressed = ps2_keyvalue.bit_r1 != 0;
    bool r2_pressed = ps2_keyvalue.bit_r2 != 0;
    bool l1_pressed_now = l1_pressed && !ps2_l1_was_pressed;
    bool r1_pressed_now = r1_pressed && !ps2_r1_was_pressed;
    bool r2_pressed_now = r2_pressed && !ps2_r2_was_pressed;

    // 每次收到手柄数据都更新按键历史。只响应从松开到按下的瞬间，
    // 避免长按时反复从动作的第一帧重新播放。
    ps2_l1_was_pressed = l1_pressed;
    ps2_r1_was_pressed = r1_pressed;
    ps2_r2_was_pressed = r2_pressed;

    // 录制动作快捷键：L1=停止，R2=循环播放，R1=播放一次。
    // 同时按下多个快捷键时，停止优先，其次是循环播放。
    if (l1_pressed_now)
    {
        stopActionPlayback();
        return;
    }

    if (r2_pressed_now)
    {
        playActionFrames(true);
        return;
    }

    if (r1_pressed_now)
    {
        playActionFrames(false);
        return;
    }

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

    if (ps2_keyvalue.mode == PS2_COORDINATE_MODE ||
        ps2_keyvalue.bit_up || ps2_keyvalue.bit_down)
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
    settings_dirty = true;
}

int RobotDogController::getPresetPoseAngle(
    PresetPose preset_pose,
    uint8_t leg_index
) const
{
    if (leg_index >= LEG_COUNT)
    {
        return getCenterAngle(0);
    }

    if (preset_pose == PRESET_REST)
    {
        return rest_pose_angles[leg_index];
    }
    if (preset_pose == PRESET_HALF_CROUCH)
    {
        return half_crouch_pose_angles[leg_index];
    }
    return crouch_pose_angles[leg_index];
}

void RobotDogController::setPresetPoseAngles(
    PresetPose preset_pose,
    int front_left_angle,
    int front_right_angle,
    int rear_left_angle,
    int rear_right_angle
)
{
    int new_angles[LEG_COUNT] = {
        front_left_angle,
        front_right_angle,
        rear_left_angle,
        rear_right_angle
    };
    int *preset_angles = crouch_pose_angles;
    if (preset_pose == PRESET_REST)
    {
        preset_angles = rest_pose_angles;
    }
    else if (preset_pose == PRESET_HALF_CROUCH)
    {
        preset_angles = half_crouch_pose_angles;
    }

    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        preset_angles[leg_index] = clampServoAngle(new_angles[leg_index]);
    }
    settings_dirty = true;
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

int RobotDogController::getForwardSwingOffsetDegrees(void) const
{
    return forward_swing_offset_degrees;
}

int RobotDogController::getForwardPushOffsetDegrees(void) const
{
    return forward_push_offset_degrees;
}

uint32_t RobotDogController::getForwardStepIntervalMs(void) const
{
    return forward_step_interval_ms;
}

int RobotDogController::getBackwardSwingOffsetDegrees(void) const
{
    return backward_swing_offset_degrees;
}

int RobotDogController::getBackwardPushOffsetDegrees(void) const
{
    return backward_push_offset_degrees;
}

uint32_t RobotDogController::getBackwardStepIntervalMs(void) const
{
    return backward_step_interval_ms;
}

int RobotDogController::getLeftTurnSwingOffsetDegrees(void) const
{
    return left_turn_swing_offset_degrees;
}

int RobotDogController::getLeftTurnPushOffsetDegrees(void) const
{
    return left_turn_push_offset_degrees;
}

uint32_t RobotDogController::getLeftTurnStepIntervalMs(void) const
{
    return left_turn_step_interval_ms;
}

int RobotDogController::getRightTurnSwingOffsetDegrees(void) const
{
    return right_turn_swing_offset_degrees;
}

int RobotDogController::getRightTurnPushOffsetDegrees(void) const
{
    return right_turn_push_offset_degrees;
}

uint32_t RobotDogController::getRightTurnStepIntervalMs(void) const
{
    return right_turn_step_interval_ms;
}

int RobotDogController::getLeftTurnLegDirection(uint8_t leg_index) const
{
    if (leg_index >= LEG_COUNT)
    {
        return 1;
    }
    return left_turn_leg_directions[leg_index];
}

int RobotDogController::getRightTurnLegDirection(uint8_t leg_index) const
{
    if (leg_index >= LEG_COUNT)
    {
        return 1;
    }
    return right_turn_leg_directions[leg_index];
}

void RobotDogController::setTurnLegDirection(
    MotionCommand motion_command,
    uint8_t leg_index,
    int direction_sign
)
{
    if (leg_index >= LEG_COUNT ||
        (motion_command != MOTION_LEFT && motion_command != MOTION_RIGHT))
    {
        return;
    }

    int normalized_direction = direction_sign < 0 ? -1 : 1;
    if (motion_command == MOTION_LEFT)
    {
        left_turn_leg_directions[leg_index] = normalized_direction;
    }
    else
    {
        right_turn_leg_directions[leg_index] = normalized_direction;
    }
    settings_dirty = true;
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

    int normalized_direction = direction_sign < 0 ? -1 : 1;
    leg_gait_directions[leg_index] = normalized_direction;
    int forward_magnitude = abs(leg_forward_offsets_degrees[leg_index]);
    int backward_magnitude = abs(leg_backward_offsets_degrees[leg_index]);
    leg_forward_offsets_degrees[leg_index] = -forward_magnitude * normalized_direction;
    leg_backward_offsets_degrees[leg_index] = backward_magnitude * normalized_direction;
    settings_dirty = true;
}

int RobotDogController::getLegForwardOffsetDegrees(uint8_t leg_index) const
{
    if (leg_index >= LEG_COUNT)
    {
        return 0;
    }
    return leg_forward_offsets_degrees[leg_index];
}

int RobotDogController::getLegBackwardOffsetDegrees(uint8_t leg_index) const
{
    if (leg_index >= LEG_COUNT)
    {
        return 0;
    }
    return leg_backward_offsets_degrees[leg_index];
}

void RobotDogController::setLegMotionOffsets(
    uint8_t leg_index,
    int forward_offset_degrees,
    int backward_offset_degrees
)
{
    if (leg_index >= LEG_COUNT)
    {
        return;
    }

    leg_forward_offsets_degrees[leg_index] = constrain(
        forward_offset_degrees,
        -MAX_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    leg_backward_offsets_degrees[leg_index] = constrain(
        backward_offset_degrees,
        -MAX_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    if (leg_forward_offsets_degrees[leg_index] < 0)
    {
        leg_gait_directions[leg_index] = 1;
    }
    else if (leg_forward_offsets_degrees[leg_index] > 0)
    {
        leg_gait_directions[leg_index] = -1;
    }
    settings_dirty = true;
}

void RobotDogController::setMotionParameters(
    int new_rest_offset_degrees,
    int new_half_crouch_offset_degrees,
    int new_crouch_offset_degrees,
    int new_manual_max_offset_degrees,
    int new_forward_swing_offset_degrees,
    int new_forward_push_offset_degrees,
    uint32_t new_forward_step_interval_ms,
    int new_backward_swing_offset_degrees,
    int new_backward_push_offset_degrees,
    uint32_t new_backward_step_interval_ms,
    int new_left_turn_swing_offset_degrees,
    int new_left_turn_push_offset_degrees,
    uint32_t new_left_turn_step_interval_ms,
    int new_right_turn_swing_offset_degrees,
    int new_right_turn_push_offset_degrees,
    uint32_t new_right_turn_step_interval_ms
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
    int previous_forward_swing_offset_degrees = forward_swing_offset_degrees;
    int next_forward_swing_offset_degrees = constrain(
        new_forward_swing_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    forward_swing_offset_degrees = next_forward_swing_offset_degrees;
    if (next_forward_swing_offset_degrees != previous_forward_swing_offset_degrees)
    {
        for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
        {
            int gait_direction = leg_gait_directions[leg_index] < 0 ? -1 : 1;
            int forward_sign = leg_forward_offsets_degrees[leg_index] < 0
                ? -1
                : (leg_forward_offsets_degrees[leg_index] > 0 ? 1 : -gait_direction);
            leg_forward_offsets_degrees[leg_index] =
                forward_sign * next_forward_swing_offset_degrees;
        }
    }

    int previous_backward_swing_offset_degrees = backward_swing_offset_degrees;
    int next_backward_swing_offset_degrees = constrain(
        new_backward_swing_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    backward_swing_offset_degrees = next_backward_swing_offset_degrees;
    if (next_backward_swing_offset_degrees != previous_backward_swing_offset_degrees)
    {
        for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
        {
            int gait_direction = leg_gait_directions[leg_index] < 0 ? -1 : 1;
            int backward_sign = leg_backward_offsets_degrees[leg_index] < 0
                ? -1
                : (leg_backward_offsets_degrees[leg_index] > 0 ? 1 : gait_direction);
            leg_backward_offsets_degrees[leg_index] =
                backward_sign * next_backward_swing_offset_degrees;
        }
    }

    forward_push_offset_degrees = constrain(
        new_forward_push_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    forward_step_interval_ms = constrain(
        new_forward_step_interval_ms,
        MIN_WALK_STEP_INTERVAL_MS,
        MAX_WALK_STEP_INTERVAL_MS
    );
    backward_push_offset_degrees = constrain(
        new_backward_push_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    backward_step_interval_ms = constrain(
        new_backward_step_interval_ms,
        MIN_WALK_STEP_INTERVAL_MS,
        MAX_WALK_STEP_INTERVAL_MS
    );
    left_turn_swing_offset_degrees = constrain(
        new_left_turn_swing_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    left_turn_push_offset_degrees = constrain(
        new_left_turn_push_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    left_turn_step_interval_ms = constrain(
        new_left_turn_step_interval_ms,
        MIN_WALK_STEP_INTERVAL_MS,
        MAX_WALK_STEP_INTERVAL_MS
    );
    right_turn_swing_offset_degrees = constrain(
        new_right_turn_swing_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    right_turn_push_offset_degrees = constrain(
        new_right_turn_push_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    right_turn_step_interval_ms = constrain(
        new_right_turn_step_interval_ms,
        MIN_WALK_STEP_INTERVAL_MS,
        MAX_WALK_STEP_INTERVAL_MS
    );
    settings_dirty = true;
}

bool RobotDogController::saveSettings(void)
{
    StoredSettings stored_settings = {};
    stored_settings.magic = SETTINGS_STORAGE_MAGIC;
    stored_settings.version = SETTINGS_STORAGE_VERSION;
    stored_settings.size_bytes = sizeof(StoredSettings);
    stored_settings.rest_offset_degrees = rest_offset_degrees;
    stored_settings.half_crouch_offset_degrees = half_crouch_offset_degrees;
    stored_settings.crouch_offset_degrees = crouch_offset_degrees;
    stored_settings.manual_max_offset_degrees = manual_max_offset_degrees;
    // 保持主设置结构为 v2，旧固件保存的数据仍然可以读取。
    stored_settings.walk_swing_offset_degrees = forward_swing_offset_degrees;
    stored_settings.walk_push_offset_degrees = forward_push_offset_degrees;
    stored_settings.turn_swing_offset_degrees = left_turn_swing_offset_degrees;
    stored_settings.turn_push_offset_degrees = left_turn_push_offset_degrees;
    stored_settings.walk_step_interval_ms = forward_step_interval_ms;
    // 旧版只有一个转向修正值，继续写入左转第一条腿以保持存储结构兼容。
    stored_settings.turn_direction_sign = left_turn_leg_directions[0];
    stored_settings.action_frame_count = action_frame_count;

    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        stored_settings.center_angles[leg_index] = servo_center_angles[leg_index];
        stored_settings.gait_directions[leg_index] = leg_gait_directions[leg_index];
        stored_settings.rest_angles[leg_index] = rest_pose_angles[leg_index];
        stored_settings.half_crouch_angles[leg_index] = half_crouch_pose_angles[leg_index];
        stored_settings.crouch_angles[leg_index] = crouch_pose_angles[leg_index];
    }
    for (uint8_t frame_index = 0; frame_index < action_frame_count; frame_index++)
    {
        stored_settings.action_frames[frame_index] = action_frames[frame_index];
    }
    stored_settings.checksum = calculateSettingsChecksum(stored_settings);

    Preferences preferences;
    if (!preferences.begin(SETTINGS_NAMESPACE, false))
    {
        return false;
    }

    size_t written_bytes = preferences.putBytes(
        SETTINGS_KEY,
        &stored_settings,
        sizeof(stored_settings)
    );
    preferences.end();
    if (written_bytes != sizeof(stored_settings))
    {
        return false;
    }

    StoredSettings verified_settings = {};
    if (!preferences.begin(SETTINGS_NAMESPACE, true))
    {
        return false;
    }
    bool is_saved = preferences.getBytesLength(SETTINGS_KEY) == sizeof(verified_settings) &&
        preferences.getBytes(
            SETTINGS_KEY,
            &verified_settings,
            sizeof(verified_settings)
        ) == sizeof(verified_settings) &&
        isStoredSettingsValid(verified_settings) &&
        verified_settings.checksum == stored_settings.checksum;
    preferences.end();

    bool all_settings_saved = is_saved &&
        saveMotionParameterSettings() &&
        saveTurnDirectionSettings() &&
        saveLegMotionOffsets();
    has_saved_settings = all_settings_saved;
    settings_dirty = !all_settings_saved;
    return all_settings_saved;
}

bool RobotDogController::hasSavedSettings(void) const
{
    return has_saved_settings;
}

bool RobotDogController::hasUnsavedSettings(void) const
{
    return settings_dirty;
}

void RobotDogController::resetServoCenterAngles(void)
{
    clearSavedSettings();
    loadServoSettings();
    loadActionFrames();
    initializeLegMotionOffsets();
    has_saved_settings = false;
    settings_dirty = false;

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
    applyPresetPose(PRESET_REST);
}

void RobotDogController::applyHalfCrouchPose(void)
{
    stopActionPlayback();
    stopContinuousMotionWithoutPose();
    applyPresetPose(PRESET_HALF_CROUCH);
}

void RobotDogController::applyCrouchPose(void)
{
    stopActionPlayback();
    stopContinuousMotionWithoutPose();
    applyPresetPose(PRESET_CROUCH);
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

void RobotDogController::previewMotionStep(
    MotionCommand motion_command,
    uint8_t step_index
)
{
    if (motion_command == MOTION_STOP)
    {
        stopContinuousMotion();
        return;
    }

    stopActionPlayback();
    stopContinuousMotionWithoutPose();
    if (motion_command == MOTION_FORWARD || motion_command == MOTION_BACKWARD)
    {
        applyWalkMotionStep(motion_command, step_index);
    }
    else if (motion_command == MOTION_LEFT || motion_command == MOTION_RIGHT)
    {
        applyTurnMotionStep(motion_command, step_index);
    }
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
    settings_dirty = true;
    return true;
}

void RobotDogController::clearActionFrames(void)
{
    stopActionPlayback();
    action_frame_count = 0;
    settings_dirty = true;
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
        rest_pose_angles[leg_index] = clampServoAngle(
            ROBOT_DOG_REST_POSE_ANGLES[leg_index]
        );
        half_crouch_pose_angles[leg_index] = clampServoAngle(
            ROBOT_DOG_HALF_CROUCH_POSE_ANGLES[leg_index]
        );
        crouch_pose_angles[leg_index] = clampServoAngle(
            ROBOT_DOG_CROUCH_POSE_ANGLES[leg_index]
        );
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
    forward_swing_offset_degrees = constrain(
        ROBOT_DOG_FORWARD_SWING_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    forward_push_offset_degrees = constrain(
        ROBOT_DOG_FORWARD_PUSH_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    forward_step_interval_ms = constrain(
        ROBOT_DOG_FORWARD_STEP_INTERVAL_MS,
        MIN_WALK_STEP_INTERVAL_MS,
        MAX_WALK_STEP_INTERVAL_MS
    );
    backward_swing_offset_degrees = constrain(
        ROBOT_DOG_BACKWARD_SWING_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    backward_push_offset_degrees = constrain(
        ROBOT_DOG_BACKWARD_PUSH_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    backward_step_interval_ms = constrain(
        ROBOT_DOG_BACKWARD_STEP_INTERVAL_MS,
        MIN_WALK_STEP_INTERVAL_MS,
        MAX_WALK_STEP_INTERVAL_MS
    );
    left_turn_swing_offset_degrees = constrain(
        ROBOT_DOG_LEFT_TURN_SWING_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    left_turn_push_offset_degrees = constrain(
        ROBOT_DOG_LEFT_TURN_PUSH_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    left_turn_step_interval_ms = constrain(
        ROBOT_DOG_LEFT_TURN_STEP_INTERVAL_MS,
        MIN_WALK_STEP_INTERVAL_MS,
        MAX_WALK_STEP_INTERVAL_MS
    );
    right_turn_swing_offset_degrees = constrain(
        ROBOT_DOG_RIGHT_TURN_SWING_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    right_turn_push_offset_degrees = constrain(
        ROBOT_DOG_RIGHT_TURN_PUSH_OFFSET_DEGREES,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    right_turn_step_interval_ms = constrain(
        ROBOT_DOG_RIGHT_TURN_STEP_INTERVAL_MS,
        MIN_WALK_STEP_INTERVAL_MS,
        MAX_WALK_STEP_INTERVAL_MS
    );
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        left_turn_leg_directions[leg_index] =
            ROBOT_DOG_LEFT_TURN_DIRECTIONS[leg_index] < 0 ? -1 : 1;
        right_turn_leg_directions[leg_index] =
            ROBOT_DOG_RIGHT_TURN_DIRECTIONS[leg_index] < 0 ? -1 : 1;
    }
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

void RobotDogController::loadSavedSettings(void)
{
    StoredSettings stored_settings = {};
    Preferences preferences;
    if (!preferences.begin(SETTINGS_NAMESPACE, true))
    {
        return;
    }

    bool has_stored_settings = preferences.getBytesLength(SETTINGS_KEY) ==
        sizeof(stored_settings) && preferences.getBytes(
            SETTINGS_KEY,
            &stored_settings,
            sizeof(stored_settings)
        ) == sizeof(stored_settings);
    preferences.end();
    if (!has_stored_settings || !isStoredSettingsValid(stored_settings))
    {
        return;
    }

    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        servo_center_angles[leg_index] = clampServoAngle(
            stored_settings.center_angles[leg_index]
        );
        current_servo_angles[leg_index] = servo_center_angles[leg_index];
        leg_gait_directions[leg_index] =
            stored_settings.gait_directions[leg_index] < 0 ? -1 : 1;
        rest_pose_angles[leg_index] = clampServoAngle(
            stored_settings.rest_angles[leg_index]
        );
        half_crouch_pose_angles[leg_index] = clampServoAngle(
            stored_settings.half_crouch_angles[leg_index]
        );
        crouch_pose_angles[leg_index] = clampServoAngle(
            stored_settings.crouch_angles[leg_index]
        );
    }

    rest_offset_degrees = constrain(
        stored_settings.rest_offset_degrees,
        MIN_POSE_OFFSET_DEGREES,
        MAX_POSE_OFFSET_DEGREES
    );
    half_crouch_offset_degrees = constrain(
        stored_settings.half_crouch_offset_degrees,
        MIN_POSE_OFFSET_DEGREES,
        MAX_POSE_OFFSET_DEGREES
    );
    crouch_offset_degrees = constrain(
        stored_settings.crouch_offset_degrees,
        MIN_POSE_OFFSET_DEGREES,
        MAX_POSE_OFFSET_DEGREES
    );
    manual_max_offset_degrees = constrain(
        stored_settings.manual_max_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    forward_swing_offset_degrees = constrain(
        stored_settings.walk_swing_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    backward_swing_offset_degrees = forward_swing_offset_degrees;
    forward_push_offset_degrees = constrain(
        stored_settings.walk_push_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    backward_push_offset_degrees = forward_push_offset_degrees;
    left_turn_swing_offset_degrees = constrain(
        stored_settings.turn_swing_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    right_turn_swing_offset_degrees = left_turn_swing_offset_degrees;
    left_turn_push_offset_degrees = constrain(
        stored_settings.turn_push_offset_degrees,
        MIN_MOTION_OFFSET_DEGREES,
        MAX_MOTION_OFFSET_DEGREES
    );
    right_turn_push_offset_degrees = left_turn_push_offset_degrees;
    forward_step_interval_ms = constrain(
        stored_settings.walk_step_interval_ms,
        MIN_WALK_STEP_INTERVAL_MS,
        MAX_WALK_STEP_INTERVAL_MS
    );
    backward_step_interval_ms = forward_step_interval_ms;
    left_turn_step_interval_ms = forward_step_interval_ms;
    right_turn_step_interval_ms = forward_step_interval_ms;
    int legacy_turn_direction_sign =
        stored_settings.turn_direction_sign < 0 ? -1 : 1;
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        left_turn_leg_directions[leg_index] = legacy_turn_direction_sign;
        right_turn_leg_directions[leg_index] = legacy_turn_direction_sign;
    }

    action_frame_count = 0;
    for (uint8_t frame_index = 0;
         frame_index < stored_settings.action_frame_count;
         frame_index++)
    {
        addActionFrame(
            stored_settings.action_frames[frame_index].angles[LEG_FRONT_LEFT],
            stored_settings.action_frames[frame_index].angles[LEG_FRONT_RIGHT],
            stored_settings.action_frames[frame_index].angles[LEG_REAR_LEFT],
            stored_settings.action_frames[frame_index].angles[LEG_REAR_RIGHT],
            stored_settings.action_frames[frame_index].duration_ms
        );
    }
    has_saved_settings = true;
}

void RobotDogController::initializeLegMotionOffsets(void)
{
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        int gait_direction = leg_gait_directions[leg_index] < 0 ? -1 : 1;
        leg_forward_offsets_degrees[leg_index] =
            -forward_swing_offset_degrees * gait_direction;
        leg_backward_offsets_degrees[leg_index] =
            backward_swing_offset_degrees * gait_direction;
    }
}

void RobotDogController::loadMotionParameterSettings(void)
{
    MotionParameterSettings motion_settings = {};
    Preferences preferences;
    if (!preferences.begin(SETTINGS_NAMESPACE, true))
    {
        return;
    }

    bool has_stored_settings =
        preferences.getBytesLength(MOTION_PARAMETER_SETTINGS_KEY) ==
            sizeof(motion_settings) &&
        preferences.getBytes(
            MOTION_PARAMETER_SETTINGS_KEY,
            &motion_settings,
            sizeof(motion_settings)
        ) == sizeof(motion_settings);
    preferences.end();
    if (!has_stored_settings || !isMotionParameterSettingsValid(motion_settings))
    {
        return;
    }

    setMotionParameters(
        rest_offset_degrees,
        half_crouch_offset_degrees,
        crouch_offset_degrees,
        manual_max_offset_degrees,
        motion_settings.forward_swing_offset_degrees,
        motion_settings.forward_push_offset_degrees,
        motion_settings.forward_step_interval_ms,
        motion_settings.backward_swing_offset_degrees,
        motion_settings.backward_push_offset_degrees,
        motion_settings.backward_step_interval_ms,
        motion_settings.left_turn_swing_offset_degrees,
        motion_settings.left_turn_push_offset_degrees,
        motion_settings.left_turn_step_interval_ms,
        motion_settings.right_turn_swing_offset_degrees,
        motion_settings.right_turn_push_offset_degrees,
        motion_settings.right_turn_step_interval_ms
    );
}

void RobotDogController::loadLegMotionOffsets(void)
{
    LegMotionSettings motion_settings = {};
    Preferences preferences;
    if (!preferences.begin(SETTINGS_NAMESPACE, true))
    {
        return;
    }

    bool has_stored_settings = preferences.getBytesLength(LEG_MOTION_SETTINGS_KEY) ==
        sizeof(motion_settings) && preferences.getBytes(
            LEG_MOTION_SETTINGS_KEY,
            &motion_settings,
            sizeof(motion_settings)
        ) == sizeof(motion_settings);
    preferences.end();
    if (!has_stored_settings || !isLegMotionSettingsValid(motion_settings))
    {
        return;
    }

    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        leg_forward_offsets_degrees[leg_index] =
            motion_settings.forward_offsets_degrees[leg_index];
        leg_backward_offsets_degrees[leg_index] =
            motion_settings.backward_offsets_degrees[leg_index];
    }
}

void RobotDogController::loadTurnDirectionSettings(void)
{
    TurnDirectionSettings direction_settings = {};
    LegacyTurnDirectionSettingsV1 legacy_settings = {};
    Preferences preferences;
    if (!preferences.begin(SETTINGS_NAMESPACE, true))
    {
        return;
    }

    size_t stored_size = preferences.getBytesLength(TURN_DIRECTION_SETTINGS_KEY);
    bool has_stored_settings = stored_size == sizeof(direction_settings) &&
        preferences.getBytes(
            TURN_DIRECTION_SETTINGS_KEY,
            &direction_settings,
            sizeof(direction_settings)
        ) == sizeof(direction_settings);
    bool has_legacy_settings = !has_stored_settings &&
        stored_size == sizeof(legacy_settings) &&
        preferences.getBytes(
            TURN_DIRECTION_SETTINGS_KEY,
            &legacy_settings,
            sizeof(legacy_settings)
        ) == sizeof(legacy_settings);
    preferences.end();

    if (has_stored_settings && isTurnDirectionSettingsValid(direction_settings))
    {
        for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
        {
            left_turn_leg_directions[leg_index] =
                direction_settings.left_leg_directions[leg_index] < 0 ? -1 : 1;
            right_turn_leg_directions[leg_index] =
                direction_settings.right_leg_directions[leg_index] < 0 ? -1 : 1;
        }
        return;
    }

    if (has_legacy_settings && isLegacyTurnDirectionSettingsValid(legacy_settings))
    {
        int left_direction = legacy_settings.left_direction_sign < 0 ? -1 : 1;
        int right_direction = legacy_settings.right_direction_sign < 0 ? -1 : 1;
        for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
        {
            left_turn_leg_directions[leg_index] = left_direction;
            right_turn_leg_directions[leg_index] = right_direction;
        }
    }
}

bool RobotDogController::saveLegMotionOffsets(void) const
{
    LegMotionSettings motion_settings = {};
    motion_settings.magic = LEG_MOTION_STORAGE_MAGIC;
    motion_settings.version = LEG_MOTION_STORAGE_VERSION;
    motion_settings.size_bytes = sizeof(LegMotionSettings);
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        motion_settings.forward_offsets_degrees[leg_index] =
            leg_forward_offsets_degrees[leg_index];
        motion_settings.backward_offsets_degrees[leg_index] =
            leg_backward_offsets_degrees[leg_index];
    }
    motion_settings.checksum = calculateLegMotionSettingsChecksum(motion_settings);

    Preferences preferences;
    if (!preferences.begin(SETTINGS_NAMESPACE, false))
    {
        return false;
    }
    size_t written_bytes = preferences.putBytes(
        LEG_MOTION_SETTINGS_KEY,
        &motion_settings,
        sizeof(motion_settings)
    );
    preferences.end();
    return written_bytes == sizeof(motion_settings);
}

bool RobotDogController::saveMotionParameterSettings(void) const
{
    MotionParameterSettings motion_settings = {};
    motion_settings.magic = MOTION_PARAMETER_STORAGE_MAGIC;
    motion_settings.version = MOTION_PARAMETER_STORAGE_VERSION;
    motion_settings.size_bytes = sizeof(MotionParameterSettings);
    motion_settings.forward_swing_offset_degrees = forward_swing_offset_degrees;
    motion_settings.forward_push_offset_degrees = forward_push_offset_degrees;
    motion_settings.forward_step_interval_ms = forward_step_interval_ms;
    motion_settings.backward_swing_offset_degrees = backward_swing_offset_degrees;
    motion_settings.backward_push_offset_degrees = backward_push_offset_degrees;
    motion_settings.backward_step_interval_ms = backward_step_interval_ms;
    motion_settings.left_turn_swing_offset_degrees = left_turn_swing_offset_degrees;
    motion_settings.left_turn_push_offset_degrees = left_turn_push_offset_degrees;
    motion_settings.left_turn_step_interval_ms = left_turn_step_interval_ms;
    motion_settings.right_turn_swing_offset_degrees = right_turn_swing_offset_degrees;
    motion_settings.right_turn_push_offset_degrees = right_turn_push_offset_degrees;
    motion_settings.right_turn_step_interval_ms = right_turn_step_interval_ms;
    motion_settings.checksum =
        calculateMotionParameterSettingsChecksum(motion_settings);

    Preferences preferences;
    if (!preferences.begin(SETTINGS_NAMESPACE, false))
    {
        return false;
    }
    size_t written_bytes = preferences.putBytes(
        MOTION_PARAMETER_SETTINGS_KEY,
        &motion_settings,
        sizeof(motion_settings)
    );
    preferences.end();
    return written_bytes == sizeof(motion_settings);
}

bool RobotDogController::saveTurnDirectionSettings(void) const
{
    TurnDirectionSettings direction_settings = {};
    direction_settings.magic = TURN_DIRECTION_STORAGE_MAGIC;
    direction_settings.version = TURN_DIRECTION_STORAGE_VERSION;
    direction_settings.size_bytes = sizeof(TurnDirectionSettings);
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        direction_settings.left_leg_directions[leg_index] =
            left_turn_leg_directions[leg_index];
        direction_settings.right_leg_directions[leg_index] =
            right_turn_leg_directions[leg_index];
    }
    direction_settings.checksum =
        calculateTurnDirectionSettingsChecksum(direction_settings);

    Preferences preferences;
    if (!preferences.begin(SETTINGS_NAMESPACE, false))
    {
        return false;
    }
    size_t written_bytes = preferences.putBytes(
        TURN_DIRECTION_SETTINGS_KEY,
        &direction_settings,
        sizeof(direction_settings)
    );
    preferences.end();
    return written_bytes == sizeof(direction_settings);
}

void RobotDogController::clearSavedSettings(void)
{
    Preferences preferences;
    if (!preferences.begin(SETTINGS_NAMESPACE, false))
    {
        return;
    }

    preferences.clear();
    preferences.end();
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
        last_step_ms = millis() - getMotionStepIntervalMs(motion_command);
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
    bool has_joystick_forward_input = ps2_keyvalue.mode == PS2_COORDINATE_MODE &&
        isJoystickOutsideDeadZone(ps2_keyvalue.left_joystick_y);
    bool has_joystick_turn_input = ps2_keyvalue.mode == PS2_COORDINATE_MODE &&
        isJoystickOutsideDeadZone(ps2_keyvalue.right_joystick_x);
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
        turn_strength = mapJoystickToOffset(ps2_keyvalue.right_joystick_x, 100);
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

void RobotDogController::applyPresetPose(PresetPose preset_pose)
{
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        writeServoAngle(
            leg_index,
            getPresetPoseAngle(preset_pose, leg_index)
        );
    }
}

void RobotDogController::applyCurrentMotionStep(void)
{
    uint32_t current_ms = millis();
    if (current_ms - last_step_ms < getMotionStepIntervalMs(active_motion_command))
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

    gait_step_index = (gait_step_index + 1) % 2;
}

uint32_t RobotDogController::getMotionStepIntervalMs(
    MotionCommand motion_command
) const
{
    if (motion_command == MOTION_BACKWARD)
    {
        return backward_step_interval_ms;
    }
    if (motion_command == MOTION_LEFT)
    {
        return left_turn_step_interval_ms;
    }
    if (motion_command == MOTION_RIGHT)
    {
        return right_turn_step_interval_ms;
    }
    return forward_step_interval_ms;
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
    const uint8_t *swing_legs = is_diagonal_a_swing_step ? DIAGONAL_A_LEGS : DIAGONAL_B_LEGS;
    const uint8_t *push_legs = is_diagonal_a_swing_step ? DIAGONAL_B_LEGS : DIAGONAL_A_LEGS;
    int push_offset_degrees = motion_command == MOTION_FORWARD
        ? forward_push_offset_degrees
        : backward_push_offset_degrees;
    for (uint8_t pair_index = 0; pair_index < 2; pair_index++)
    {
        uint8_t swing_leg = swing_legs[pair_index];
        uint8_t push_leg = push_legs[pair_index];
        int swing_offset = motion_command == MOTION_FORWARD
            ? leg_forward_offsets_degrees[swing_leg]
            : leg_backward_offsets_degrees[swing_leg];
        int push_motion_offset = motion_command == MOTION_FORWARD
            ? leg_forward_offsets_degrees[push_leg]
            : leg_backward_offsets_degrees[push_leg];
        int push_direction = push_motion_offset > 0
            ? -1
            : (push_motion_offset < 0 ? 1 : 0);
        target_angles[swing_leg] += swing_offset;
        target_angles[push_leg] += push_direction * push_offset_degrees;
    }

    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        writeServoAngle(leg_index, target_angles[leg_index]);
    }
}

void RobotDogController::applyTurnMotionStep(MotionCommand motion_command, uint8_t step_index)
{
    int target_angles[LEG_COUNT];
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        target_angles[leg_index] = getCenterAngle(leg_index);
    }

    bool is_swing_step = step_index % 2 == 0;
    int turn_swing_offset_degrees = motion_command == MOTION_LEFT
        ? left_turn_swing_offset_degrees
        : right_turn_swing_offset_degrees;
    int turn_push_offset_degrees = motion_command == MOTION_LEFT
        ? left_turn_push_offset_degrees
        : right_turn_push_offset_degrees;
    for (uint8_t leg_index = 0; leg_index < LEG_COUNT; leg_index++)
    {
        bool is_left_side = leg_index == LEG_FRONT_LEFT ||
            leg_index == LEG_REAR_LEFT;
        bool use_forward_direction = motion_command == MOTION_LEFT
            ? is_left_side
            : !is_left_side;
        int calibrated_motion_offset = use_forward_direction
            ? leg_forward_offsets_degrees[leg_index]
            : leg_backward_offsets_degrees[leg_index];
        int calibrated_direction = calibrated_motion_offset < 0 ? -1 : 1;
        calibrated_direction *= motion_command == MOTION_LEFT
            ? left_turn_leg_directions[leg_index]
            : right_turn_leg_directions[leg_index];
        int turn_offset = is_swing_step
            ? calibrated_direction * turn_swing_offset_degrees
            : -calibrated_direction * turn_push_offset_degrees;
        target_angles[leg_index] += turn_offset;
    }

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
    settings_dirty = true;
    return true;
}
