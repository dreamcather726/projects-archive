#ifndef __ROBOT_DOG_CONTROLLER_H__
#define __ROBOT_DOG_CONTROLLER_H__

#include <Arduino.h>
#include "PS2_CTL.hpp"

class RobotDogController
{
    public:
        enum MotionCommand
        {
            MOTION_STOP = 0,
            MOTION_FORWARD,
            MOTION_BACKWARD,
            MOTION_LEFT,
            MOTION_RIGHT
        };

        struct ActionFrame
        {
            int angles[4];
            uint32_t duration_ms;
        };

        void init(void);
        void tick(void);
        void update(const PS2KeyValueObjectTypeDef &ps2_keyvalue);
        uint8_t getLegCount(void) const;
        int getServoCenterAngle(uint8_t leg_index) const;
        int getCurrentServoAngle(uint8_t leg_index) const;
        void setServoCenterAngle(uint8_t leg_index, int angle_degrees);
        int getRestOffsetDegrees(void) const;
        int getHalfCrouchOffsetDegrees(void) const;
        int getCrouchOffsetDegrees(void) const;
        int getManualMaxOffsetDegrees(void) const;
        int getWalkMaxOffsetDegrees(void) const;
        int getWalkSwingOffsetDegrees(void) const;
        int getWalkPushOffsetDegrees(void) const;
        int getTurnSwingOffsetDegrees(void) const;
        int getTurnPushOffsetDegrees(void) const;
        int getTurnDirectionSign(void) const;
        uint32_t getWalkStepIntervalMs(void) const;
        int getLegGaitDirection(uint8_t leg_index) const;
        void setLegGaitDirection(uint8_t leg_index, int direction_sign);
        void setMotionParameters(
            int rest_offset_degrees,
            int half_crouch_offset_degrees,
            int crouch_offset_degrees,
            int manual_max_offset_degrees,
            int walk_swing_offset_degrees,
            int walk_push_offset_degrees,
            int turn_swing_offset_degrees,
            int turn_push_offset_degrees,
            uint32_t walk_step_interval_ms,
            int turn_direction_sign
        );
        void resetServoCenterAngles(void);
        void applyStandPose(void);
        void applyRestPose(void);
        void applyHalfCrouchPose(void);
        void applyCrouchPose(void);
        void setPoseAngles(
            int front_left_angle,
            int front_right_angle,
            int rear_left_angle,
            int rear_right_angle
        );
        void startContinuousMotion(MotionCommand motion_command);
        void stopContinuousMotion(void);
        MotionCommand getContinuousMotionCommand(void) const;
        uint8_t getActionFrameCapacity(void) const;
        uint8_t getActionFrameCount(void) const;
        bool getActionFrame(uint8_t frame_index, ActionFrame *action_frame) const;
        bool addActionFrame(
            int front_left_angle,
            int front_right_angle,
            int rear_left_angle,
            int rear_right_angle,
            uint32_t duration_ms
        );
        bool updateActionFrame(
            uint8_t frame_index,
            int front_left_angle,
            int front_right_angle,
            int rear_left_angle,
            int rear_right_angle,
            uint32_t duration_ms
        );
        bool removeActionFrame(uint8_t frame_index);
        void clearActionFrames(void);
        void playActionFrames(bool loop_enabled);
        void stopActionPlayback(void);
        bool isActionPlaybackActive(void) const;
        bool isActionPlaybackLoopEnabled(void) const;
        uint8_t getPlaybackFrameIndex(void) const;

    private:
        enum LegIndex
        {
            LEG_FRONT_LEFT = 0,
            LEG_FRONT_RIGHT,
            LEG_REAR_LEFT,
            LEG_REAR_RIGHT,
            LEG_COUNT
        };

        void writeServoAngle(uint8_t leg_index, int angle_degrees);
        int clampServoAngle(int angle_degrees) const;
        int getCenterAngle(uint8_t leg_index) const;
        void loadServoSettings(void);
        void loadActionFrames(void);
        int mapJoystickToOffset(uint8_t joystick_value, int max_offset_degrees) const;
        bool isJoystickOutsideDeadZone(uint8_t joystick_value) const;
        void initializeServoPwm(void);
        void stopContinuousMotionWithoutPose(void);
        void setContinuousMotion(MotionCommand motion_command, bool started_by_ps2);
        void applyManualPose(const PS2KeyValueObjectTypeDef &ps2_keyvalue);
        void applyWalkGait(const PS2KeyValueObjectTypeDef &ps2_keyvalue);
        void applyOffsetPose(int offset_degrees);
        void applyCurrentMotionStep(void);
        void applyWalkMotionStep(MotionCommand motion_command, uint8_t step_index);
        void applyTurnMotionStep(MotionCommand motion_command, uint8_t step_index);
        void updateActionPlayback(void);
        bool setActionFrame(
            uint8_t frame_index,
            int front_left_angle,
            int front_right_angle,
            int rear_left_angle,
            int rear_right_angle,
            uint32_t duration_ms
        );

        uint32_t last_step_ms;
        uint8_t gait_step_index;
        MotionCommand active_motion_command;
        bool continuous_motion_started_by_ps2;
        ActionFrame action_frames[16];
        uint8_t action_frame_count;
        bool action_playback_active;
        bool action_playback_loop_enabled;
        uint8_t playback_frame_index;
        uint32_t playback_frame_start_ms;
};

#endif
