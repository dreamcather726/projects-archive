#include "i2s_speaker.h"

#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <math.h>

static constexpr i2s_port_t SPEAKER_I2S_PORT = I2S_NUM_0;
static constexpr uint32_t SPEAKER_SAMPLE_RATE = 22050;
static constexpr uint16_t AUDIO_BUFFER_SAMPLE_COUNT = 64;
static constexpr uint16_t SINE_TABLE_SIZE = 256;
static constexpr uint8_t MAX_ACTIVE_NOTES = 8;
static constexpr int16_t MAX_MIXED_AMPLITUDE = 14000;

struct SpeakerVoice {
  uint8_t note;
  uint8_t velocity;
  uint32_t phase;
  uint32_t phase_increment;
  bool active;
};

static int16_t sine_table[SINE_TABLE_SIZE];
static uint32_t phase_increments[128];
static SpeakerVoice voices[MAX_ACTIVE_NOTES] = {};
static SemaphoreHandle_t audio_mutex = nullptr;
static TaskHandle_t audio_task_handle = nullptr;
static bool is_speaker_ready = false;

static void generate_audio_tables();
static void speaker_audio_task(void *parameter);

bool i2s_speaker_init()
{
  if (is_speaker_ready) {
    return true;
  }

  generate_audio_tables();
  audio_mutex = xSemaphoreCreateMutex();
  if (audio_mutex == nullptr) {
    return false;
  }

  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
  config.sample_rate = SPEAKER_SAMPLE_RATE;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 8;
  config.dma_buf_len = AUDIO_BUFFER_SAMPLE_COUNT;
  config.use_apll = false;
  config.tx_desc_auto_clear = true;
  config.fixed_mclk = 0;

  if (i2s_driver_install(SPEAKER_I2S_PORT, &config, 0, nullptr) != ESP_OK) {
    vSemaphoreDelete(audio_mutex);
    audio_mutex = nullptr;
    return false;
  }

  i2s_pin_config_t pins = {};
  pins.bck_io_num = I2S_SPEAKER_BCLK_PIN;
  pins.ws_io_num = I2S_SPEAKER_LRC_PIN;
  pins.data_out_num = I2S_SPEAKER_DATA_PIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  if (i2s_set_pin(SPEAKER_I2S_PORT, &pins) != ESP_OK
      || i2s_set_clk(SPEAKER_I2S_PORT,
                     SPEAKER_SAMPLE_RATE,
                     I2S_BITS_PER_SAMPLE_16BIT,
                     I2S_CHANNEL_MONO) != ESP_OK) {
    i2s_driver_uninstall(SPEAKER_I2S_PORT);
    vSemaphoreDelete(audio_mutex);
    audio_mutex = nullptr;
    return false;
  }

  i2s_zero_dma_buffer(SPEAKER_I2S_PORT);
  const BaseType_t task_result = xTaskCreatePinnedToCore(speaker_audio_task,
                                                         "i2s_speaker",
                                                         3072,
                                                         nullptr,
                                                         2,
                                                         &audio_task_handle,
                                                         0);
  if (task_result != pdPASS) {
    i2s_driver_uninstall(SPEAKER_I2S_PORT);
    vSemaphoreDelete(audio_mutex);
    audio_mutex = nullptr;
    return false;
  }

  is_speaker_ready = true;
  return true;
}

void i2s_speaker_note_on(uint8_t note, uint8_t velocity)
{
  if (!is_speaker_ready || note > 127 || velocity == 0 || audio_mutex == nullptr) {
    return;
  }

  xSemaphoreTake(audio_mutex, portMAX_DELAY);
  SpeakerVoice *free_voice = nullptr;
  for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
    if (voices[i].active && voices[i].note == note) {
      voices[i].velocity = velocity > 127 ? 127 : velocity;
      xSemaphoreGive(audio_mutex);
      return;
    }
    if (!voices[i].active && free_voice == nullptr) {
      free_voice = &voices[i];
    }
  }

  // 同时音符超过槽位时复用第一个声部。
  if (free_voice == nullptr) {
    free_voice = &voices[0];
  }
  free_voice->note = note;
  free_voice->velocity = velocity > 127 ? 127 : velocity;
  free_voice->phase = 0;
  free_voice->phase_increment = phase_increments[note];
  free_voice->active = true;
  xSemaphoreGive(audio_mutex);
}

void i2s_speaker_note_off(uint8_t note)
{
  if (!is_speaker_ready || audio_mutex == nullptr) {
    return;
  }

  xSemaphoreTake(audio_mutex, portMAX_DELAY);
  for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
    if (voices[i].active && voices[i].note == note) {
      voices[i].active = false;
    }
  }
  xSemaphoreGive(audio_mutex);
}

void i2s_speaker_all_notes_off()
{
  if (!is_speaker_ready || audio_mutex == nullptr) {
    return;
  }

  xSemaphoreTake(audio_mutex, portMAX_DELAY);
  for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
    voices[i].active = false;
  }
  xSemaphoreGive(audio_mutex);
}

bool i2s_speaker_is_ready()
{
  return is_speaker_ready;
}

uint8_t i2s_speaker_active_note_count()
{
  if (!is_speaker_ready || audio_mutex == nullptr) {
    return 0;
  }

  uint8_t active_count = 0;
  xSemaphoreTake(audio_mutex, portMAX_DELAY);
  for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
    if (voices[i].active) {
      active_count++;
    }
  }
  xSemaphoreGive(audio_mutex);
  return active_count;
}

static void generate_audio_tables()
{
  for (uint16_t i = 0; i < SINE_TABLE_SIZE; i++) {
    const float angle = 2.0f * PI * static_cast<float>(i) / SINE_TABLE_SIZE;
    sine_table[i] = static_cast<int16_t>(sinf(angle) * MAX_MIXED_AMPLITUDE);
  }

  for (uint16_t note = 0; note < 128; note++) {
    const float frequency_hz = 440.0f * powf(2.0f, (static_cast<float>(note) - 69.0f) / 12.0f);
    phase_increments[note] = static_cast<uint32_t>(
        (frequency_hz * 4294967296.0) / static_cast<double>(SPEAKER_SAMPLE_RATE));
  }
}

static void speaker_audio_task(void *parameter)
{
  (void)parameter;
  int16_t audio_buffer[AUDIO_BUFFER_SAMPLE_COUNT];

  while (true) {
    xSemaphoreTake(audio_mutex, portMAX_DELAY);
    for (uint16_t sample_index = 0; sample_index < AUDIO_BUFFER_SAMPLE_COUNT; sample_index++) {
      int32_t mixed_sample = 0;
      uint8_t active_count = 0;

      for (uint8_t voice_index = 0; voice_index < MAX_ACTIVE_NOTES; voice_index++) {
        SpeakerVoice &voice = voices[voice_index];
        if (!voice.active) {
          continue;
        }

        const uint8_t table_index = static_cast<uint8_t>(voice.phase >> 24);
        mixed_sample += (static_cast<int32_t>(sine_table[table_index]) * voice.velocity) >> 7;
        voice.phase += voice.phase_increment;
        active_count++;
      }

      if (active_count > 0) {
        mixed_sample /= active_count;
      }
      audio_buffer[sample_index] = static_cast<int16_t>(
          constrain(mixed_sample, -MAX_MIXED_AMPLITUDE, MAX_MIXED_AMPLITUDE));
    }
    xSemaphoreGive(audio_mutex);

    size_t bytes_written = 0;
    i2s_write(SPEAKER_I2S_PORT,
              audio_buffer,
              sizeof(audio_buffer),
              &bytes_written,
              portMAX_DELAY);
  }
}
