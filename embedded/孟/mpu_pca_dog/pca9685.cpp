#include "pca9685.h"

#include <math.h>

namespace {
// MODE1/MODE2 中使用的位控制标志。
const uint8_t MODE1_RESTART = 0x80;
const uint8_t MODE1_SLEEP = 0x10;
const uint8_t MODE1_AI = 0x20;
const uint8_t MODE2_OUTDRV = 0x04;
const uint16_t PWM_MAX_COUNT = 4095;
const uint16_t PWM_FULL_OFF = 0x1000;
const uint32_t DEFAULT_OSCILLATOR_HZ = 25000000UL;
}

PCA9685::PCA9685(uint8_t address, TwoWire &wire)
    : _wire(&wire),
      _address(address),
      _oscillatorFrequencyHz(DEFAULT_OSCILLATOR_HZ),
      _pwmFrequencyHz(0.0f) {
    // 先为每路通道设置通用的 0~180 度校准值，
    // 实际机械结构使用前应根据舵机型号重新校准。
    for (uint8_t channel = 0; channel < CHANNEL_COUNT; ++channel) {
        _calibration[channel].minPulseUs = 500.0f;
        _calibration[channel].maxPulseUs = 2500.0f;
        _calibration[channel].minAngleDeg = 0.0f;
        _calibration[channel].maxAngleDeg = 180.0f;
    }
}

bool PCA9685::begin(float pwmFrequencyHz) {
    // PCA9685 使用 7 位 I2C 地址。
    if (_address > 0x7F) {
        return false;
    }

    _wire->beginTransmission(_address);
    if (_wire->endTransmission() != 0) {
        return false;
    }

    // 使用图腾柱输出，并打开寄存器自动地址递增，
    // 便于在一次 I2C 传输中写入某通道的 4 个 PWM 寄存器。
    if (!writeRegister(MODE2, MODE2_OUTDRV) || !writeRegister(MODE1, MODE1_AI)) {
        return false;
    }

    delay(1);
    return setPWMFrequency(pwmFrequencyHz);
}

bool PCA9685::setPWMFrequency(float frequencyHz) {
    if (frequencyHz <= 0.0f) {
        return false;
    }

    // 数据手册分频公式：prescale = oscillator / (4096 * frequency) - 1。
    const float prescaleValue =
        static_cast<float>(_oscillatorFrequencyHz) / (4096.0f * frequencyHz) - 1.0f;
    const long roundedPrescale = lroundf(prescaleValue);
    if (roundedPrescale < 3 || roundedPrescale > 255) {
        return false;
    }

    uint8_t oldMode = 0;
    if (!readRegister(MODE1, oldMode)) {
        return false;
    }

    // PRESCALE 只能在睡眠状态下修改，写入后再唤醒并重启 PWM。
    const uint8_t sleepMode = (oldMode & ~MODE1_RESTART) | MODE1_SLEEP | MODE1_AI;
    if (!writeRegister(MODE1, sleepMode) ||
        !writeRegister(PRESCALE, static_cast<uint8_t>(roundedPrescale)) ||
        !writeRegister(MODE1, (oldMode & ~MODE1_SLEEP) | MODE1_AI)) {
        return false;
    }

    delay(1);
    if (!writeRegister(MODE1,
                       (oldMode & ~MODE1_SLEEP) | MODE1_RESTART | MODE1_AI)) {
        return false;
    }

    // 记录分频取整后的实际频率，脉宽换算使用该值而非理论值。
    _pwmFrequencyHz = static_cast<float>(_oscillatorFrequencyHz) /
                      (4096.0f * static_cast<float>(roundedPrescale + 1));
    return true;
}

void PCA9685::setOscillatorFrequency(uint32_t oscillatorFrequencyHz) {
    if (oscillatorFrequencyHz > 0) {
        _oscillatorFrequencyHz = oscillatorFrequencyHz;
    }
}

bool PCA9685::setChannelPWM(uint8_t channel,
                            uint16_t onCount,
                            uint16_t offCount) {
    if (channel >= CHANNEL_COUNT || onCount > PWM_MAX_COUNT ||
        offCount > PWM_MAX_COUNT) {
        return false;
    }

    return writePWMRegisters(LED0_ON_L + 4 * channel,
                             onCount,
                             offCount,
                             false);
}

bool PCA9685::setPulseWidthUs(uint8_t channel, float pulseWidthUs) {
    if (channel >= CHANNEL_COUNT || _pwmFrequencyHz <= 0.0f ||
        pulseWidthUs <= 0.0f) {
        return false;
    }

    const float periodUs = 1000000.0f / _pwmFrequencyHz;
    if (pulseWidthUs >= periodUs) {
        return false;
    }

    // 将微秒脉宽换算为 PCA9685 的 0~4095 计数值，并四舍五入到最近的计数格。
    long ticks = lroundf(pulseWidthUs * _pwmFrequencyHz * 4096.0f / 1000000.0f);
    if (ticks < 1) {
        ticks = 1;
    } else if (ticks > PWM_MAX_COUNT) {
        ticks = PWM_MAX_COUNT;
    }

    return setChannelPWM(channel, 0, static_cast<uint16_t>(ticks));
}

bool PCA9685::setServoCalibration(uint8_t channel,
                                  float minPulseUs,
                                  float maxPulseUs,
                                  float minAngleDeg,
                                  float maxAngleDeg) {
    if (channel >= CHANNEL_COUNT || minPulseUs <= 0.0f ||
        maxPulseUs <= minPulseUs || minAngleDeg == maxAngleDeg) {
        return false;
    }

    if (_pwmFrequencyHz > 0.0f &&
        maxPulseUs >= 1000000.0f / _pwmFrequencyHz) {
        return false;
    }

    _calibration[channel].minPulseUs = minPulseUs;
    _calibration[channel].maxPulseUs = maxPulseUs;
    _calibration[channel].minAngleDeg = minAngleDeg;
    _calibration[channel].maxAngleDeg = maxAngleDeg;
    return true;
}

bool PCA9685::setServoAngle(uint8_t channel, float angleDeg) {
    if (channel >= CHANNEL_COUNT) {
        return false;
    }

    const ServoCalibration &calibration = _calibration[channel];
    const float lowerAngle = min(calibration.minAngleDeg, calibration.maxAngleDeg);
    const float upperAngle = max(calibration.minAngleDeg, calibration.maxAngleDeg);
    if (angleDeg < lowerAngle || angleDeg > upperAngle) {
        return false;
    }

    // 在用户设定的角度和脉宽范围之间做线性插值。
    const float ratio = (angleDeg - calibration.minAngleDeg) /
                        (calibration.maxAngleDeg - calibration.minAngleDeg);
    const float pulseWidthUs = calibration.minPulseUs +
                               ratio * (calibration.maxPulseUs -
                                        calibration.minPulseUs);
    return setPulseWidthUs(channel, pulseWidthUs);
}

bool PCA9685::disableChannel(uint8_t channel) {
    if (channel >= CHANNEL_COUNT) {
        return false;
    }

    return writePWMRegisters(LED0_ON_L + 4 * channel, 0, 0, true);
}

bool PCA9685::disableAllChannels() {
    // 不使用 ALL_LED_OFF_H 的全局强制关闭位。
    // 全局关闭位一旦置位，后续只改某一通道的寄存器也不会恢复输出。
    // 逐通道设置 FULL_OFF 后，setChannelPWM() 可以正常重新启用任意单路。
    bool success = true;
    for (uint8_t channel = 0; channel < CHANNEL_COUNT; ++channel) {
        if (!disableChannel(channel)) {
            success = false;
        }
    }
    return success;
}

float PCA9685::getPWMFrequency() const {
    return _pwmFrequencyHz;
}

uint32_t PCA9685::getOscillatorFrequency() const {
    return _oscillatorFrequencyHz;
}

const PCA9685::ServoCalibration &PCA9685::getServoCalibration(
    uint8_t channel) const {
    // Invalid input returns channel 0 without touching hardware.
    return _calibration[channel < CHANNEL_COUNT ? channel : 0];
}

bool PCA9685::readRegister(uint8_t reg, uint8_t &value) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) {
        return false;
    }

    if (_wire->requestFrom(_address, static_cast<uint8_t>(1)) != 1) {
        return false;
    }

    value = _wire->read();
    return true;
}

bool PCA9685::writeRegister(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value);
    return _wire->endTransmission() == 0;
}

bool PCA9685::writePWMRegisters(uint8_t startRegister,
                                uint16_t onCount,
                                uint16_t offCount,
                                bool fullOff) {
    // 四个字节依次是 ON_L、ON_H、OFF_L、OFF_H。
    // fullOff 通过 OFF_H 的第 4 位强制关闭输出。
    _wire->beginTransmission(_address);
    _wire->write(startRegister);
    _wire->write(static_cast<uint8_t>(onCount & 0xFF));
    _wire->write(static_cast<uint8_t>((onCount >> 8) & 0x0F));
    _wire->write(static_cast<uint8_t>(offCount & 0xFF));
    _wire->write(static_cast<uint8_t>(fullOff ? PWM_FULL_OFF >> 8
                                              : (offCount >> 8) & 0x0F));
    return _wire->endTransmission() == 0;
}
