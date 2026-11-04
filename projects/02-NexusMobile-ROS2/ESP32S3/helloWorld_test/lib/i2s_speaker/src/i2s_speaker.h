#pragma once

#include <Arduino.h>

static constexpr uint8_t I2S_SPEAKER_DATA_PIN = 40; // MAX98357 DIN
static constexpr uint8_t I2S_SPEAKER_LRC_PIN = 39;  // MAX98357 LRC / WS
static constexpr uint8_t I2S_SPEAKER_BCLK_PIN = 38; // MAX98357 BCLK

// 初始化 I2S 和后台音频生成任务。
bool i2s_speaker_init();

// 播放/停止 MIDI 音符，note 范围 0~127，velocity 范围 1~127。
void i2s_speaker_note_on(uint8_t note, uint8_t velocity = 80);
void i2s_speaker_note_off(uint8_t note);
void i2s_speaker_all_notes_off();

// 非阻塞播放一段上升后下降的测试音阶。正在播放时再次调用会返回 false。
bool i2s_speaker_play_sweep();
bool i2s_speaker_is_sweep_playing();

bool i2s_speaker_is_ready();
uint8_t i2s_speaker_active_note_count();
