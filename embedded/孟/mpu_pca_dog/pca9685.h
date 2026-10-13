#pragma once

#include <Arduino.h>
#include <Wire.h>

class PCA9685 {
public:
    // PCA9685 固定提供 16 路独立 PWM 输出。
    static const uint8_t CHANNEL_COUNT = 16;

    // 每路舵机的角度与脉宽对应关系，可以分通道单独设置。
    struct ServoCalibration {
        float minPulseUs;
        float maxPulseUs;
        float minAngleDeg;
        float maxAngleDeg;
    };

    explicit PCA9685(uint8_t address = 0x40, TwoWire &wire = Wire);

    // 调用 begin() 前需要先完成 Wire.begin()。
    bool begin(float pwmFrequencyHz = 50.0f);

    // 设置所有通道共用的 PWM 频率，普通模拟舵机通常使用 50 Hz。
    bool setPWMFrequency(float frequencyHz);

    // 如果已测得板载晶振的实际频率，可在此设置以提高脉宽精度。
    // 修改后需要再次调用 setPWMFrequency() 重新计算分频值。
    void setOscillatorFrequency(uint32_t oscillatorFrequencyHz);

    // 底层 12 位 PWM 接口，onCount/offCount 的范围都是 0~4095。
    bool setChannelPWM(uint8_t channel, uint16_t onCount, uint16_t offCount);

    // 按微秒设置舵机高电平脉宽，可用于精确调试舵机中位和限位。
    bool setPulseWidthUs(uint8_t channel, float pulseWidthUs);

    // 建立某一通道的“角度 -> 脉宽”线性映射。
    bool setServoCalibration(uint8_t channel,
                             float minPulseUs,
                             float maxPulseUs,
                             float minAngleDeg = 0.0f,
                             float maxAngleDeg = 180.0f);

    // 根据该通道的校准参数设置舵机角度。
    bool setServoAngle(uint8_t channel, float angleDeg);

    // 停止单路或全部通道的 PWM 输出。
    bool disableChannel(uint8_t channel);
    bool disableAllChannels();

    float getPWMFrequency() const;
    uint32_t getOscillatorFrequency() const;
    const ServoCalibration &getServoCalibration(uint8_t channel) const;

private:
    static const uint8_t MODE1 = 0x00;
    static const uint8_t MODE2 = 0x01;
    static const uint8_t LED0_ON_L = 0x06;
    static const uint8_t PRESCALE = 0xFE;

    bool readRegister(uint8_t reg, uint8_t &value);
    bool writeRegister(uint8_t reg, uint8_t value);
    bool writePWMRegisters(uint8_t startRegister,
                           uint16_t onCount,
                           uint16_t offCount,
                           bool fullOff);

    TwoWire *_wire;
    uint8_t _address;
    uint32_t _oscillatorFrequencyHz;
    float _pwmFrequencyHz;
    ServoCalibration _calibration[CHANNEL_COUNT];
};
