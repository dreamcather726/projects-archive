#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

// MAX98357 I2S amplifier pins used by the main project.
static constexpr uint8_t I2S_DATA_PIN = 40;  // DIN
static constexpr uint8_t I2S_WS_PIN = 39;    // LRC / WS
static constexpr uint8_t I2S_BCLK_PIN = 38;  // BCLK
static constexpr i2s_port_t I2S_PORT = I2S_NUM_0;
static constexpr uint32_t SAMPLE_RATE = 22050;
static constexpr uint16_t AUDIO_BUFFER_SAMPLES = 128;
static constexpr uint16_t SINE_TABLE_SIZE = 256;

struct MelodyEvent {
  uint16_t frequency_hz;
  uint16_t duration_ms;
};

// A short built-in melody. A frequency of zero represents a rest.
static const MelodyEvent MELODY[] = {
  {262, 280}, {294, 280}, {330, 280}, {392, 500}, {0, 100},
  {392, 280}, {330, 280}, {294, 280}, {262, 500}, {0, 160},
  {330, 280}, {349, 280}, {392, 500}, {0, 100},
  {392, 280}, {349, 280}, {330, 500}, {0, 160},
  {262, 220}, {330, 220}, {392, 220}, {523, 650}
};
static constexpr size_t MELODY_EVENT_COUNT = sizeof(MELODY) / sizeof(MELODY[0]);

static int16_t sine_table[SINE_TABLE_SIZE];
static uint32_t phase = 0;
static uint32_t phase_increment = 0;
static uint8_t volume = 75;
static bool playing = false;
static size_t melody_index = 0;
static uint32_t event_end_ms = 0;

static void printHelp()
{
  Serial.println("Commands: p=play/replay, s=stop, +=volume up, -=volume down, ?=help");
}

static bool initI2s()
{
  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
  config.sample_rate = SAMPLE_RATE;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 8;
  config.dma_buf_len = AUDIO_BUFFER_SAMPLES;
  config.use_apll = false;
  config.tx_desc_auto_clear = true;
  config.fixed_mclk = 0;

  if (i2s_driver_install(I2S_PORT, &config, 0, nullptr) != ESP_OK) {
    return false;
  }

  i2s_pin_config_t pins = {};
  pins.bck_io_num = I2S_BCLK_PIN;
  pins.ws_io_num = I2S_WS_PIN;
  pins.data_out_num = I2S_DATA_PIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  if (i2s_set_pin(I2S_PORT, &pins) != ESP_OK ||
      i2s_set_clk(I2S_PORT, SAMPLE_RATE,
                  I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO) != ESP_OK) {
    i2s_driver_uninstall(I2S_PORT);
    return false;
  }

  i2s_zero_dma_buffer(I2S_PORT);
  return true;
}

static void selectEvent(size_t index)
{
  const MelodyEvent &event = MELODY[index];
  phase = 0;
  phase_increment = event.frequency_hz == 0
      ? 0
      : static_cast<uint32_t>(
            (static_cast<double>(event.frequency_hz) * 4294967296.0) / SAMPLE_RATE);
  event_end_ms = millis() + event.duration_ms;
  Serial.printf("Event %u/%u: %u Hz, %u ms\n",
                static_cast<unsigned>(index + 1),
                static_cast<unsigned>(MELODY_EVENT_COUNT),
                event.frequency_hz, event.duration_ms);
}

static void startMelody()
{
  melody_index = 0;
  playing = true;
  selectEvent(melody_index);
  Serial.println("Music started");
}

static void stopMelody()
{
  playing = false;
  phase_increment = 0;
  phase = 0;
  i2s_zero_dma_buffer(I2S_PORT);
  Serial.println("Music stopped");
}

static void updateMelody()
{
  if (!playing || static_cast<int32_t>(millis() - event_end_ms) < 0) {
    return;
  }

  ++melody_index;
  if (melody_index >= MELODY_EVENT_COUNT) {
    stopMelody();
    Serial.println("PASS: melody playback completed");
    return;
  }
  selectEvent(melody_index);
}

static void writeAudioChunk()
{
  int16_t samples[AUDIO_BUFFER_SAMPLES];
  for (uint16_t i = 0; i < AUDIO_BUFFER_SAMPLES; ++i) {
    if (!playing || phase_increment == 0) {
      samples[i] = 0;
    } else {
      const uint8_t table_index = static_cast<uint8_t>(phase >> 24);
      samples[i] = static_cast<int16_t>(
          (static_cast<int32_t>(sine_table[table_index]) * volume) / 127);
      phase += phase_increment;
    }
  }

  size_t bytes_written = 0;
  i2s_write(I2S_PORT, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
}

static void handleSerial()
{
  while (Serial.available()) {
    const char command = static_cast<char>(Serial.read());
    switch (command) {
      case 'p':
      case 'P': startMelody(); break;
      case 's':
      case 'S': stopMelody(); break;
      case '+':
        volume = min(120, volume + 10);
        Serial.printf("Volume=%u/127\n", volume);
        break;
      case '-':
        volume = max(10, volume - 10);
        Serial.printf("Volume=%u/127\n", volume);
        break;
      case '?': printHelp(); break;
      default: break;
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== I2S MUSIC TEST ===");
  Serial.println("MAX98357 pins: BCLK=38 WS=39 DIN=40");

  for (uint16_t i = 0; i < SINE_TABLE_SIZE; ++i) {
    const float angle = 2.0f * PI * static_cast<float>(i) / SINE_TABLE_SIZE;
    sine_table[i] = static_cast<int16_t>(sinf(angle) * 14000.0f);
  }

  if (!initI2s()) {
    Serial.println("FAIL: I2S initialization failed");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("I2S initialized. The built-in melody will play once.");
  printHelp();
  startMelody();
}

void loop()
{
  handleSerial();
  updateMelody();
  writeAudioChunk();
}
