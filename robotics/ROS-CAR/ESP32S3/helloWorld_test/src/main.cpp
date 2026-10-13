#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>
#include <button.h>
#include <i2s_speaker.h>
#include <motor_driver.h>
#include <wheel_encoder.h>
#include "speed_pid.h"
#include "serial_sender.h"
#include "serial_receive.h"
#include <Ticker.h>
#include <arm_servo.h>
#define ENABLE_MPU6050 1 // MPU6050 相关功能开关，0=关闭，1=开启。开启后会发送 IMU 数据帧，并在串口调试信息中打印姿态角。
#if ENABLE_MPU6050
#include "imu.h"
#endif
#include <math.h>
#include <string.h>
#define PID_INTERVAL_US 50000
#define ENABLE_ENCODER_TEST_COUNTER 0 // Encoder 测试计数器开关，1=开启，0=关闭。开启后会模拟编码器数据以测试 PID 控制和串口通信功能，无需实际连接编码器硬件。

static constexpr int8_t TFT_CS_PIN = 47;
static constexpr int8_t TFT_DC_PIN = 21;
static constexpr int8_t TFT_RST_PIN = -1;
static constexpr int8_t TFT_SDA_PIN = 20;
static constexpr int8_t TFT_SCL_PIN = 19;
static constexpr uint16_t TFT_NATIVE_WIDTH = 240;
static constexpr uint16_t TFT_NATIVE_HEIGHT = 320;
static constexpr uint8_t TFT_ROTATION = 3;
static constexpr uint32_t TFT_UPDATE_INTERVAL_MS = 50;

static constexpr uint8_t BUZZER_PIN = 48;
// 当前 Arduino Core 的 analogWrite() 从通道 7 向下分配电机 PWM，因此蜂鸣器固定使用通道 0。
static constexpr uint8_t BUZZER_LEDC_CHANNEL = 0;
static constexpr uint8_t BUZZER_LEDC_RESOLUTION_BITS = 10;
static constexpr uint16_t BUZZER_LEDC_INITIAL_FREQUENCY_HZ = 2000;
static const uint16_t BUZZER_MELODY_FREQUENCIES_HZ[] = {262, 262, 392, 392, 440, 440, 392};
static const uint16_t BUZZER_MELODY_DURATIONS_MS[] = {180, 180, 180, 180, 180, 180, 360};
static constexpr uint8_t BUZZER_MELODY_NOTE_COUNT =
    sizeof(BUZZER_MELODY_FREQUENCIES_HZ) / sizeof(BUZZER_MELODY_FREQUENCIES_HZ[0]);
static constexpr uint16_t BUZZER_MELODY_GAP_MS = 30;
static constexpr bool AUDIO_SELF_TEST_AT_STARTUP = true;

static constexpr uint8_t BUTTON_1_PIN = 10;
static constexpr uint8_t BUTTON_2_PIN = 11;
static constexpr uint8_t BUTTON_3_PIN = 15;
static constexpr uint8_t BUTTON_4_PIN = 16;
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 5;
static constexpr uint32_t BUTTON_DEBUG_PRINT_INTERVAL_MS = 500;
static constexpr float BUTTON_MOTOR_SPEED_STEP_CM_S = 5.0f;
static constexpr float BUTTON_MOTOR_SPEED_MAX_CM_S = 60.0f;

static constexpr uint8_t BATTERY_ADC_PIN = 14;
// 默认按电阻分压后 ADC 电压的 3 倍换算电池电压，需根据实际分压电阻校准。
static constexpr float BATTERY_VOLTAGE_DIVIDER_RATIO = 10.0f;
static constexpr uint8_t BATTERY_ADC_SAMPLE_COUNT = 16;
static constexpr uint32_t BATTERY_UPDATE_INTERVAL_MS = 500;

static constexpr uint8_t LINE_SENSOR_ADDRESS = 0x27;
static constexpr uint8_t LINE_READ_COMMAND = 0x01;
static constexpr uint8_t LINE_DATA_LENGTH = 32;
static constexpr uint16_t LINE_THRESHOLD = 200;
static constexpr bool LINE_HIGH_VALUE_IS_ACTIVE = false;
static constexpr uint32_t LINE_I2C_CLOCK_HZ = 100000;
static constexpr uint32_t LINE_READ_INTERVAL_MS = 100;
static constexpr uint32_t LINE_RESPONSE_DELAY_MS = 10;
#if ENABLE_MPU6050
static constexpr uint8_t MPU6050_INIT_MAX_ATTEMPTS = 3;
// 完整校准会阻塞约 20 秒；默认关闭以保证开机后按键快速可用。
static constexpr bool MPU6050_CALIBRATE_AT_STARTUP = false;
#endif

static Adafruit_ST7789 screen(TFT_CS_PIN,
                             TFT_DC_PIN,
                             TFT_SDA_PIN,
                             TFT_SCL_PIN,
                             TFT_RST_PIN);

// 下位机发送给上位机的功能码
static constexpr uint8_t FRAME_FUNC_ENCODER = 0x01;
#if ENABLE_MPU6050
static constexpr uint8_t FRAME_FUNC_IMU = 0x02;
#endif

// 上位机发送给下位机的功能码
static constexpr uint8_t CMD_FUNC_WHEEL_SPEED = 0x10;
static constexpr uint8_t CMD_FUNC_PID_CONTROL = 0x11;

static constexpr float CMD_SPEED_MAX_CM_S = 200.0f;
static constexpr uint32_t CMD_SPEED_TIMEOUT_MS = 500;
static constexpr int PWM_DEADBAND = 5;
static constexpr float DEFAULT_PID_KP = 2.5f;
static constexpr float DEFAULT_PID_KI =0.65f;
static constexpr float DEFAULT_PID_KD = 0.5f;
static constexpr float PID_OUTPUT_MAX = 255.0f;
static constexpr float PID_OUTPUT_MIN = -255.0f;
static constexpr float TARGET_SPEED_STEP_MAX_CM_S = 8.0f;
static constexpr int PWM_STEP_MAX = 12;
static constexpr int32_t TEST_ENCODER_COUNTS_PER_SEC = 20;
static constexpr uint32_t TEST_ENCODER_UPDATE_MS = 100;

static int16_t read_i16_le(const uint8_t *bytes);// 读取 16 位有符号整数，小端序
static float read_f32_le(const uint8_t *bytes);// 读取 32 位浮点数，小端序
static int16_t clamp_speed_cm_s_x100(float speed_cm_s);// 速度转换为 int16，单位 cm/s * 100
static int limit_pwm_step(int target_pwm, int current_pwm, int max_step);// 限制 PWM 单次变化量
static float limit_speed_step(float target_speed_cm_s, float current_speed_cm_s, float max_step_cm_s);// 限制目标速度单次变化量
static void apply_speed_command(float left_speed_cm_s, float right_speed_cm_s);// 应用目标速度命令
static void update_encoder_test_data(uint32_t now_ms);// 更新编码器测试数据
static void apply_pid_parameters(float kp, float ki, float kd);// 更新左右轮 PID 参数
static void PID_control(float left_speed_cm_s, float right_speed_cm_s);// 执行左右轮速度闭环
static void tft_init();// 初始化 TFT 并绘制固定界面
static void tft_update(uint32_t now_ms);// 定时刷新屏幕上的车辆状态
static void auxiliary_io_init();// 初始化按键、无源蜂鸣器和电池 ADC
static void buttons_update();// 读取带去抖的按键状态，新按下时短鸣
static void battery_update(uint32_t now_ms);// 定时采样并计算电池电压
static void line_sensor_update(uint32_t now_ms);// 非阻塞读取八路循迹数据
static bool line_sensor_read_response();// 读取循迹模块的 32 字节响应
static void stop_motors();// 立即停止左右电机并清空 PID 状态
static void report_text_data();// 以纯文本 key=value 格式上报车辆和传感器数据
static void buzzer_melody_start();// 从头非阻塞播放无源蜂鸣器旋律
static void buzzer_melody_update(uint32_t now_ms);// 推进蜂鸣器旋律状态机
static void buzzer_tone_start(uint16_t frequency_hz);// 使用独立 LEDC 通道启动蜂鸣器
static void buzzer_tone_stop();// 停止蜂鸣器专用 LEDC 输出
static void audio_self_test_update();// 开机后依次测试蜂鸣器和 I2S 喇叭
static void adjust_motor_speed(float delta_cm_s);// 按固定步进增减左右轮目标速度

// 是否每秒打印一次调试信息
static constexpr bool DEBUG_SENSOR_PRINT = false;
// 开启后每 500ms 打印按键原始电平和去抖状态，并暂停二进制传感器帧发送。
static constexpr bool DEBUG_BUTTON_PRINT = true;
static constexpr bool ENABLE_TEXT_DATA_REPORT = true;
static constexpr bool ENABLE_BINARY_DATA_REPORT = false;

// PID 控制器（左右轮各一套）
static PID left_speed_pid;
static PID right_speed_pid;

// 目标速度（cm/s），后续可通过串口指令修改
static float target_left_speed_cm_s = 0.0f;
static float target_right_speed_cm_s = 0.0f;
static float requested_left_speed_cm_s = 0.0f;
static float requested_right_speed_cm_s = 0.0f;
static uint32_t last_speed_command_ms = 0;
static bool has_speed_command_timed_out = false;

// 最近一次计算得到的状态（用于发送/打印）
static int32_t latest_left_encoder_count = 0;
static int32_t latest_right_encoder_count = 0;
static float latest_left_speed_cm_s = 0.0f;
static float latest_right_speed_cm_s = 0.0f;
static int latest_left_pwm = 0;
static int latest_right_pwm = 0;
static Button buttons[4];
static const uint8_t BUTTON_PINS[4] = {BUTTON_1_PIN, BUTTON_2_PIN, BUTTON_3_PIN, BUTTON_4_PIN};
static bool latest_button_states[4] = {false, false, false, false};
static float latest_battery_voltage = 0.0f;
static uint16_t latest_line_raw[8] = {0};
static uint8_t latest_line_states[8] = {0};
static bool is_line_data_valid = false;
static bool is_buzzer_melody_playing = false;
static bool is_buzzer_gap_active = false;
static uint8_t buzzer_melody_note_index = 0;
static uint32_t buzzer_melody_deadline_ms = 0;
static bool is_buzzer_output_ready = false;
static uint8_t audio_self_test_phase = 0;

#if ENABLE_MPU6050
extern float roll, pitch, yaw;
extern int16_t ax, ay, az;
extern int16_t gx, gy, gz;
static bool is_mpu6050_ready = false;
#endif

// 用 10ms tick 
static Ticker control_ticker;

// 由定时回调置位，在 loop() 里读取并清零
// 必须用 volatile，避免编译器优化导致读不到置位结果
static volatile bool is_pid_update_due = false;
static volatile bool is_sensor_send_due = false;
static volatile bool is_debug_print_due = false;
static void on_tick_10ms();
ArmServo armServo;
void setup()
{
  Serial.begin(115200);
  tft_init();
  auxiliary_io_init();
  if (i2s_speaker_init()) {
    Serial.println("I2S speaker ready: DIN=40 LRC=39 BCLK=38 stereo");
  } else {
    Serial.println("I2S speaker init failed");
  }
  //机械臂初始化
  armServo.begin();
  // 底盘初始化
  wheel_encoder_init();
  wheel_encoder_speed_init();
  motor_init();
#if ENABLE_MPU6050
  for (uint8_t attempt = 1; attempt <= MPU6050_INIT_MAX_ATTEMPTS; attempt++) {
    Serial.printf("MPU6050 init %u/%u...\n", attempt, MPU6050_INIT_MAX_ATTEMPTS);
    if (mpu6050_init()) {
      is_mpu6050_ready = true;
      break;
    }
    delay(500);
  }
  if (is_mpu6050_ready && MPU6050_CALIBRATE_AT_STARTUP) {
    mpu6050_calibrate();
  } else if (!is_mpu6050_ready) {
    Serial.println("MPU6050 unavailable; continuing without IMU");
  }
#endif
  // 循迹模块、MPU6050 和 PCA9685 共用 GPIO 8/9 的 I2C 总线。
  Wire.setClock(LINE_I2C_CLOCK_HZ);
  Wire.setTimeOut(10);
  // PID 参数初始化
  left_speed_pid = {};
  right_speed_pid = {};
  apply_pid_parameters(DEFAULT_PID_KP, DEFAULT_PID_KI, DEFAULT_PID_KD);

  target_left_speed_cm_s = 0.0f;
  target_right_speed_cm_s = 0.0f;

  // 单实例定时器：10ms tick
  control_ticker.attach_ms(10, on_tick_10ms);
}

void loop()
{
  const uint32_t now_ms = millis();
  buttons_update();
  buzzer_melody_update(now_ms);
  audio_self_test_update();
  battery_update(now_ms);
  line_sensor_update(now_ms);
  buttons_update();

  // 1) 接收上位机命令：
  //    0x10 = 左右轮目标速度，单位 cm/s * 100，int16 小端。
  //    0x11 = PID_control 调参，依次为 Kp/Ki/Kd，float32 小端。
  serial_receive_update(Serial);
  SerialReceiveFrame received_frame;
  while (serial_receive_take_frame(&received_frame)) {
    if (received_frame.func == CMD_FUNC_WHEEL_SPEED && received_frame.payload_len >= 4) {
      const int16_t left_speed_raw_x100 = read_i16_le(&received_frame.payload[0]);
      const int16_t right_speed_raw_x100 = read_i16_le(&received_frame.payload[2]);
      apply_speed_command(static_cast<float>(left_speed_raw_x100) / 100.0f,
                          static_cast<float>(right_speed_raw_x100) / 100.0f);
      last_speed_command_ms = now_ms;
      has_speed_command_timed_out = false;
    } else if (received_frame.func == CMD_FUNC_PID_CONTROL && received_frame.payload_len >= 12) {
      const float kp = read_f32_le(&received_frame.payload[0]);
      const float ki = read_f32_le(&received_frame.payload[4]);
      const float kd = read_f32_le(&received_frame.payload[8]);
      apply_pid_parameters(kp, ki, kd);
    }
  }

  // 2) 如果上位机超过 500ms 没有继续发送速度命令，自动停车。
  if (last_speed_command_ms != 0 && (now_ms - last_speed_command_ms) > CMD_SPEED_TIMEOUT_MS) {
   if (!has_speed_command_timed_out) {
      // 停止电机 + 清空 PID
      requested_left_speed_cm_s = 0;
      requested_right_speed_cm_s = 0;
      target_left_speed_cm_s = 0;
      target_right_speed_cm_s = 0;
      PID_Clear(&left_speed_pid);
      PID_Clear(&right_speed_pid);
      set_right_motor_pwm(0);
      set_left_motor_pwm(0);
      latest_left_pwm = 0;
      latest_right_pwm = 0;
      has_speed_command_timed_out = true;
    }
  }

  // 3) 每 50ms 执行一次 PID 速度闭环。
  if (is_pid_update_due) {
    is_pid_update_due = false;

#if ENABLE_ENCODER_TEST_COUNTER
    update_encoder_test_data(now_ms);
#else
    int32_t left_encoder_count = 0;
    int32_t right_encoder_count = 0;
    float left_speed_cm_s = 0.0f;
    float right_speed_cm_s = 0.0f;

    // 读取编码器：总计数 + 速度（cm/s）
    wheel_encoder_get_counts(&left_encoder_count, &right_encoder_count);
    const bool is_speed_updated = wheel_encoder_get_speed_cm_s(&left_speed_cm_s, &right_speed_cm_s, PID_INTERVAL_US);
#if ENABLE_MPU6050
    if (is_mpu6050_ready) {
      mpu6050_update();
    }
#endif
    latest_left_encoder_count = left_encoder_count;
    latest_right_encoder_count = right_encoder_count;
    if (is_speed_updated) {
      latest_left_speed_cm_s = left_speed_cm_s;
      latest_right_speed_cm_s = right_speed_cm_s;
      PID_control(left_speed_cm_s, right_speed_cm_s);
    }
#endif
  }

  // 4) 按配置上报纯文本数据；二进制帧默认关闭。
  if (is_sensor_send_due) {
    is_sensor_send_due = false;
    if (ENABLE_TEXT_DATA_REPORT) {
      report_text_data();
      buttons_update();
    }

    if (ENABLE_BINARY_DATA_REPORT && !DEBUG_SENSOR_PRINT && !DEBUG_BUTTON_PRINT) {
      uint8_t encoder_payload[10];
      ss_pack_int_le(latest_left_encoder_count, 3, &encoder_payload[0]);//打包左轮总计数
      ss_pack_int_le(latest_right_encoder_count, 3, &encoder_payload[3]);//打包右轮总计数
      ss_pack_int_le(clamp_speed_cm_s_x100(latest_left_speed_cm_s), 2, &encoder_payload[6]);//打包左轮实际速度
      ss_pack_int_le(clamp_speed_cm_s_x100(latest_right_speed_cm_s), 2, &encoder_payload[8]);//打包右轮实际速度
      ss_send(Serial, FRAME_FUNC_ENCODER, encoder_payload, 10, false);//发送编码器总计数和实际速度
#if ENABLE_MPU6050
      if (is_mpu6050_ready) {
        uint8_t imu_payload[18];
        ss_pack_mpu6050_bundle(ax,ay,az,gx,gy,gz,roll,pitch,yaw,imu_payload);//打包IMU数据帧
        ss_send(Serial, FRAME_FUNC_IMU, imu_payload, 18, false);//发送IMU数据帧
      }
#endif
    }
  }



  // 5) 如果打开 DEBUG_SENSOR_PRINT，每 1 秒打印一次调试信息。
  if (is_debug_print_due && DEBUG_SENSOR_PRINT) {
    is_debug_print_due = false;

    Serial.print("[ENC] countL=");
    Serial.print(latest_left_encoder_count);
    Serial.print(" countR=");
    Serial.print(latest_right_encoder_count);
    Serial.print(" vL_cm_s=");
    Serial.print(latest_left_speed_cm_s);
    Serial.print(" vR_cm_s=");
    Serial.print(latest_right_speed_cm_s);
    Serial.print(" pwmL=");
    Serial.print(latest_left_pwm);
    Serial.print(" pwmR=");
    Serial.print(latest_right_pwm);
#if ENABLE_MPU6050
    if (is_mpu6050_ready) {
      Serial.print(" roll=");
      Serial.print(roll);
      Serial.print(" pitch=");
      Serial.print(pitch);
      Serial.print(" yaw=");
      Serial.println(yaw);
    } else {
      Serial.println(" imu=OFF");
    }
#else
    Serial.println();
#endif
  }

  // 6) 局部刷新 TFT，不阻塞串口通信和 PID 控制。
  tft_update(now_ms);
  buttons_update();
}

// 每 10ms 触发一次：用计数器合成 100ms 与 1s 的“到期标记”
static void on_tick_10ms()
{
  static uint16_t pid_tick_count = 0;
  static uint16_t sensor_send_tick_count = 0;
  static uint16_t debug_print_tick_count = 0;

  pid_tick_count++;
  sensor_send_tick_count++;
  debug_print_tick_count++;

  if (pid_tick_count >= 5) {
    pid_tick_count = 0;
    is_pid_update_due = true;
  }

  if (sensor_send_tick_count >= 5) {
    sensor_send_tick_count = 0;
    is_sensor_send_due = true;
  }

  if (debug_print_tick_count >= 100) {
    debug_print_tick_count = 0;
    is_debug_print_due = true;
  }
}

static int16_t read_i16_le(const uint8_t *bytes)
{
  return static_cast<int16_t>(static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8));
}

static float read_f32_le(const uint8_t *bytes)
{
  const uint32_t raw = static_cast<uint32_t>(bytes[0])
                     | (static_cast<uint32_t>(bytes[1]) << 8)
                     | (static_cast<uint32_t>(bytes[2]) << 16)
                     | (static_cast<uint32_t>(bytes[3]) << 24);
  float value = 0.0f;
  memcpy(&value, &raw, sizeof(value));
  return value;
}

static int16_t clamp_speed_cm_s_x100(float speed_cm_s)
{
  const int32_t scaled_speed = static_cast<int32_t>(lroundf(speed_cm_s * 100.0f));
  if (scaled_speed > 32767) return 32767;
  if (scaled_speed < -32768) return -32768;
  return static_cast<int16_t>(scaled_speed);
}

static int limit_pwm_step(int target_pwm, int current_pwm, int max_step)
{
  const int delta_pwm = target_pwm - current_pwm;
  if (delta_pwm > max_step) return current_pwm + max_step;
  if (delta_pwm < -max_step) return current_pwm - max_step;
  return target_pwm;
}

static float limit_speed_step(float target_speed_cm_s, float current_speed_cm_s, float max_step_cm_s)
{
  const float delta_speed_cm_s = target_speed_cm_s - current_speed_cm_s;
  if (delta_speed_cm_s > max_step_cm_s) return current_speed_cm_s + max_step_cm_s;
  if (delta_speed_cm_s < -max_step_cm_s) return current_speed_cm_s - max_step_cm_s;
  return target_speed_cm_s;
}

static void apply_speed_command(float left_speed_cm_s, float right_speed_cm_s)
{
  const float new_left_speed_cm_s = constrain(left_speed_cm_s, -CMD_SPEED_MAX_CM_S, CMD_SPEED_MAX_CM_S);
  const float new_right_speed_cm_s = constrain(right_speed_cm_s, -CMD_SPEED_MAX_CM_S, CMD_SPEED_MAX_CM_S);

  const bool is_left_direction_changed = (requested_left_speed_cm_s * new_left_speed_cm_s) < 0.0f;
  const bool is_right_direction_changed = (requested_right_speed_cm_s * new_right_speed_cm_s) < 0.0f;
  if (is_left_direction_changed || (fabs(new_left_speed_cm_s) < 0.01f && fabs(requested_left_speed_cm_s) >= 0.01f)) {
    PID_Clear(&left_speed_pid);
  }
  if (is_right_direction_changed || (fabs(new_right_speed_cm_s) < 0.01f && fabs(requested_right_speed_cm_s) >= 0.01f)) {
    PID_Clear(&right_speed_pid);
  }

  requested_left_speed_cm_s = new_left_speed_cm_s;
  requested_right_speed_cm_s = new_right_speed_cm_s;
}

static void update_encoder_test_data(uint32_t now_ms)
{
  static uint32_t last_test_update_ms = 0;
  if (last_test_update_ms == 0) {
    last_test_update_ms = now_ms;
    return;
  }

  const uint32_t elapsed_ms = now_ms - last_test_update_ms;
  if (elapsed_ms < TEST_ENCODER_UPDATE_MS) {
    return;
  }

  const int32_t step_count = static_cast<int32_t>(
      (static_cast<uint32_t>(TEST_ENCODER_COUNTS_PER_SEC) * elapsed_ms)
      / 1000U);
  if (step_count <= 0) {
    return;
  }

  latest_left_encoder_count += step_count;
  latest_right_encoder_count += step_count;
  latest_left_speed_cm_s = TEST_ENCODER_COUNTS_PER_SEC * WHEEL_CM_PER_COUNT;
  latest_right_speed_cm_s = TEST_ENCODER_COUNTS_PER_SEC * WHEEL_CM_PER_COUNT;
  last_test_update_ms = now_ms;
}

static void apply_pid_parameters(float kp, float ki, float kd)
{
  if (!isfinite(kp) || !isfinite(ki) || !isfinite(kd)) {
    return;
  }

  PID_Init(&left_speed_pid, kp, ki, kd, PID_OUTPUT_MAX, PID_OUTPUT_MIN);
  PID_Init(&right_speed_pid, kp, ki, kd, PID_OUTPUT_MAX, PID_OUTPUT_MIN);
  PID_Clear(&left_speed_pid);
  PID_Clear(&right_speed_pid);
}

static void PID_control(float left_speed_cm_s, float right_speed_cm_s)
{
  target_left_speed_cm_s = limit_speed_step(requested_left_speed_cm_s,
                                           target_left_speed_cm_s,
                                           TARGET_SPEED_STEP_MAX_CM_S);
  target_right_speed_cm_s = limit_speed_step(requested_right_speed_cm_s,
                                            target_right_speed_cm_s,
                                            TARGET_SPEED_STEP_MAX_CM_S);

  if (fabs(target_left_speed_cm_s) < 0.01f) PID_Clear(&left_speed_pid);
  if (fabs(target_right_speed_cm_s) < 0.01f) PID_Clear(&right_speed_pid);

  const float left_pwm_float = PID_IncPIDCal(&left_speed_pid, left_speed_cm_s, target_left_speed_cm_s);
  const float right_pwm_float = PID_IncPIDCal(&right_speed_pid, right_speed_cm_s, target_right_speed_cm_s);
  int left_pwm = constrain(static_cast<int>(left_pwm_float), -255, 255);
  int right_pwm = constrain(static_cast<int>(right_pwm_float), -255, 255);

  if (abs(left_pwm) < PWM_DEADBAND) left_pwm = 0;
  if (abs(right_pwm) < PWM_DEADBAND) right_pwm = 0;
  left_pwm = limit_pwm_step(left_pwm, latest_left_pwm, PWM_STEP_MAX);
  right_pwm = limit_pwm_step(right_pwm, latest_right_pwm, PWM_STEP_MAX);

  set_left_motor_pwm(left_pwm);
  set_right_motor_pwm(right_pwm);

  latest_left_pwm = left_pwm;
  latest_right_pwm = right_pwm;
}

static void tft_init()
{
  screen.init(TFT_NATIVE_WIDTH, TFT_NATIVE_HEIGHT);
  screen.setRotation(TFT_ROTATION);
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextWrap(false);

  screen.setTextSize(2);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(8, 8);
  screen.print("ROS CAR STATUS");
  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(196, 8);
  screen.print("BAT:");

  screen.drawFastHLine(8, 30, 304, ST77XX_BLUE);
  screen.drawFastHLine(8, 72, 304, ST77XX_BLUE);
  screen.drawFastHLine(8, 142, 304, ST77XX_BLUE);

  screen.setTextSize(1);
  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(8, 178);
  screen.print("K10:I2S  K11:BUZ  K15:SPD+  K16:SPD-");
}

static void tft_update(uint32_t now_ms)
{
  static uint32_t last_update_ms = 0;
  if (now_ms - last_update_ms < TFT_UPDATE_INTERVAL_MS) {
    return;
  }
  last_update_ms = now_ms;

  char text[64];

  screen.setTextSize(2);
  screen.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  screen.setCursor(244, 8);
  snprintf(text, sizeof(text), "%4.1fV", static_cast<double>(latest_battery_voltage));
  screen.print(text);

  screen.setTextSize(1);
  screen.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  screen.setCursor(8, 40);
  snprintf(text, sizeof(text), "ENC L:%11ld  R:%11ld        ",
           static_cast<long>(latest_left_encoder_count),
           static_cast<long>(latest_right_encoder_count));
  screen.print(text);

  screen.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  screen.setCursor(8, 56);
  snprintf(text, sizeof(text), "SPD L:%7.2f  R:%7.2f cm/s       ",
           static_cast<double>(latest_left_speed_cm_s),
           static_cast<double>(latest_right_speed_cm_s));
  screen.print(text);

  screen.setTextSize(2);
  screen.setTextColor(is_line_data_valid ? ST77XX_GREEN : ST77XX_RED, ST77XX_BLACK);
  screen.setCursor(8, 82);
  if (is_line_data_valid) {
    snprintf(text, sizeof(text), "LINE: %u%u%u%u%u%u%u%u      ",
             latest_line_states[0], latest_line_states[1],
             latest_line_states[2], latest_line_states[3],
             latest_line_states[4], latest_line_states[5],
             latest_line_states[6], latest_line_states[7]);
  } else {
    snprintf(text, sizeof(text), "LINE: --------      ");
  }
  screen.print(text);

  screen.setTextSize(1);
  screen.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  screen.setCursor(8, 108);
  if (is_line_data_valid) {
    snprintf(text, sizeof(text), "RAW0-3: %5u %5u %5u %5u              ",
             latest_line_raw[0], latest_line_raw[1],
             latest_line_raw[2], latest_line_raw[3]);
  } else {
    snprintf(text, sizeof(text), "RAW0-3: ----- ----- ----- -----              ");
  }
  screen.print(text);

  screen.setCursor(8, 124);
  if (is_line_data_valid) {
    snprintf(text, sizeof(text), "RAW4-7: %5u %5u %5u %5u              ",
             latest_line_raw[4], latest_line_raw[5],
             latest_line_raw[6], latest_line_raw[7]);
  } else {
    snprintf(text, sizeof(text), "RAW4-7: ----- ----- ----- -----              ");
  }
  screen.print(text);

  screen.setTextSize(2);
  screen.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);
  screen.setCursor(8, 152);
  snprintf(text, sizeof(text), "KEY: %u   %u   %u   %u      ",
           latest_button_states[0] ? 1U : 0U,
           latest_button_states[1] ? 1U : 0U,
           latest_button_states[2] ? 1U : 0U,
           latest_button_states[3] ? 1U : 0U);
  screen.print(text);

  screen.setTextSize(1);
#if ENABLE_MPU6050
  screen.setTextColor(is_mpu6050_ready ? ST77XX_GREEN : ST77XX_RED, ST77XX_BLACK);
  screen.setCursor(8, 188);
  if (is_mpu6050_ready) {
    snprintf(text, sizeof(text), "RPY  R:%6.1f  P:%6.1f  Y:%6.1f          ",
             static_cast<double>(roll),
             static_cast<double>(pitch),
             static_cast<double>(yaw));
  } else {
    snprintf(text, sizeof(text), "RPY  IMU ERROR                            ");
  }
  screen.print(text);

  screen.setCursor(8, 202);
  if (is_mpu6050_ready) {
    snprintf(text, sizeof(text), "GYRO X:%6d  Y:%6d  Z:%6d              ", gx, gy, gz);
  } else {
    snprintf(text, sizeof(text), "GYRO IMU ERROR                           ");
  }
  screen.print(text);
#else
  screen.setTextColor(ST77XX_RED, ST77XX_BLACK);
  screen.setCursor(8, 188);
  screen.print("RPY  IMU DISABLED                         ");
  screen.setCursor(8, 202);
  screen.print("GYRO IMU DISABLED                        ");
#endif

  screen.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  screen.setCursor(8, 218);
  snprintf(text, sizeof(text), "BAT ADC: GPIO14  SCALE:x%.2f             ",
           static_cast<double>(BATTERY_VOLTAGE_DIVIDER_RATIO));
  screen.print(text);
}

static void auxiliary_io_init()
{
  for (uint8_t i = 0; i < 4; i++) {
    button_init(&buttons[i], BUTTON_PINS[i], true, true, BUTTON_DEBOUNCE_MS);
    latest_button_states[i] = button_is_pressed(&buttons[i]);
  }

  pinMode(BUZZER_PIN, OUTPUT);
  const double buzzer_frequency = ledcSetup(BUZZER_LEDC_CHANNEL,
                                             BUZZER_LEDC_INITIAL_FREQUENCY_HZ,
                                             BUZZER_LEDC_RESOLUTION_BITS);
  if (buzzer_frequency > 0.0) {
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
    ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
    is_buzzer_output_ready = true;
    Serial.println("Buzzer ready: GPIO48 LEDC channel 0");
  } else {
    Serial.println("Buzzer LEDC setup failed");
  }

  pinMode(BATTERY_ADC_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
}

static void buttons_update()
{
  static uint32_t last_debug_print_ms = 0;
  bool has_state_change = false;
  for (uint8_t i = 0; i < 4; i++) {
    const bool was_pressed = latest_button_states[i];
    button_update_pressed(&buttons[i]);
    latest_button_states[i] = button_is_pressed(&buttons[i]);

    if (latest_button_states[i] != was_pressed) {
      has_state_change = true;
      if (latest_button_states[i]) {
        switch (i) {
          case 0:
            if (i2s_speaker_play_sweep()) {
              if (DEBUG_BUTTON_PRINT) Serial.println("[ACTION] I2S sweep started");
            } else if (DEBUG_BUTTON_PRINT) {
              Serial.println("[ACTION] I2S sweep busy or speaker unavailable");
            }
            break;
          case 1:
            if (is_buzzer_output_ready) {
              buzzer_melody_start();
              if (DEBUG_BUTTON_PRINT) Serial.println("[ACTION] buzzer melody started");
            } else if (DEBUG_BUTTON_PRINT) {
              Serial.println("[ACTION] buzzer unavailable");
            }
            break;
          case 2:
            adjust_motor_speed(BUTTON_MOTOR_SPEED_STEP_CM_S);
            break;
          case 3:
            adjust_motor_speed(-BUTTON_MOTOR_SPEED_STEP_CM_S);
            break;
          default:
            break;
        }
      }
    }
  }

  const uint32_t now_ms = millis();
  if (DEBUG_BUTTON_PRINT
      && (has_state_change || (now_ms - last_debug_print_ms) >= BUTTON_DEBUG_PRINT_INTERVAL_MS)) {
    last_debug_print_ms = now_ms;
    Serial.print("[KEY]");
    for (uint8_t i = 0; i < 4; i++) {
      Serial.printf(" GPIO%u raw=%u pressed=%u",
                    BUTTON_PINS[i],
                    digitalRead(BUTTON_PINS[i]) == HIGH ? 1U : 0U,
                    latest_button_states[i] ? 1U : 0U);
    }
    Serial.println();
  }
}

static void buzzer_melody_start()
{
  if (!is_buzzer_output_ready) {
    return;
  }

  buzzer_tone_stop();
  buzzer_melody_note_index = 0;
  is_buzzer_gap_active = false;
  is_buzzer_melody_playing = true;
  buzzer_tone_start(BUZZER_MELODY_FREQUENCIES_HZ[0]);
  buzzer_melody_deadline_ms = millis() + BUZZER_MELODY_DURATIONS_MS[0];
}

static void buzzer_melody_update(uint32_t now_ms)
{
  if (!is_buzzer_melody_playing
      || static_cast<int32_t>(now_ms - buzzer_melody_deadline_ms) < 0) {
    return;
  }

  if (!is_buzzer_gap_active) {
    buzzer_tone_stop();
    is_buzzer_gap_active = true;
    buzzer_melody_deadline_ms = now_ms + BUZZER_MELODY_GAP_MS;
    return;
  }

  buzzer_melody_note_index++;
  if (buzzer_melody_note_index >= BUZZER_MELODY_NOTE_COUNT) {
    is_buzzer_melody_playing = false;
    is_buzzer_gap_active = false;
    buzzer_tone_stop();
    return;
  }

  is_buzzer_gap_active = false;
  buzzer_tone_start(BUZZER_MELODY_FREQUENCIES_HZ[buzzer_melody_note_index]);
  buzzer_melody_deadline_ms = now_ms + BUZZER_MELODY_DURATIONS_MS[buzzer_melody_note_index];
}

static void buzzer_tone_start(uint16_t frequency_hz)
{
  if (is_buzzer_output_ready) {
    ledcWriteTone(BUZZER_LEDC_CHANNEL, frequency_hz);
  }
}

static void buzzer_tone_stop()
{
  if (is_buzzer_output_ready) {
    ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
  }
}

static void audio_self_test_update()
{
  if (!AUDIO_SELF_TEST_AT_STARTUP || audio_self_test_phase >= 3) {
    return;
  }

  if (audio_self_test_phase == 0) {
    Serial.println("[SELFTEST] buzzer melody");
    if (is_buzzer_output_ready) {
      buzzer_melody_start();
    }
    audio_self_test_phase = 1;
    return;
  }

  if (audio_self_test_phase == 1 && !is_buzzer_melody_playing) {
    Serial.println("[SELFTEST] I2S sweep");
    i2s_speaker_play_sweep();
    audio_self_test_phase = 2;
    return;
  }

  if (audio_self_test_phase == 2 && !i2s_speaker_is_sweep_playing()) {
    Serial.println("[SELFTEST] audio complete");
    audio_self_test_phase = 3;
  }
}

static void adjust_motor_speed(float delta_cm_s)
{
  const float current_speed_cm_s = max(0.0f,
      (requested_left_speed_cm_s + requested_right_speed_cm_s) * 0.5f);
  const float new_speed_cm_s = constrain(current_speed_cm_s + delta_cm_s,
                                         0.0f,
                                         BUTTON_MOTOR_SPEED_MAX_CM_S);

  if (new_speed_cm_s < 0.01f) {
    stop_motors();
  } else {
    apply_speed_command(new_speed_cm_s, new_speed_cm_s);
    last_speed_command_ms = 0;
    has_speed_command_timed_out = false;
  }

  if (DEBUG_BUTTON_PRINT) {
    Serial.printf("[ACTION] motor target %.1f cm/s\n", static_cast<double>(new_speed_cm_s));
  }
}

static void stop_motors()
{
  requested_left_speed_cm_s = 0.0f;
  requested_right_speed_cm_s = 0.0f;
  target_left_speed_cm_s = 0.0f;
  target_right_speed_cm_s = 0.0f;
  PID_Clear(&left_speed_pid);
  PID_Clear(&right_speed_pid);
  set_left_motor_pwm(0);
  set_right_motor_pwm(0);
  latest_left_pwm = 0;
  latest_right_pwm = 0;
  last_speed_command_ms = 0;
  has_speed_command_timed_out = false;
}

static void report_text_data()
{
  Serial.printf("[DATA] enc_l=%ld,enc_r=%ld,speed_l=%.2f,speed_r=%.2f,cmd_l=%.1f,cmd_r=%.1f,pwm_l=%d,pwm_r=%d,bat=%.2f",
                static_cast<long>(latest_left_encoder_count),
                static_cast<long>(latest_right_encoder_count),
                static_cast<double>(latest_left_speed_cm_s),
                static_cast<double>(latest_right_speed_cm_s),
                static_cast<double>(requested_left_speed_cm_s),
                static_cast<double>(requested_right_speed_cm_s),
                latest_left_pwm,
                latest_right_pwm,
                static_cast<double>(latest_battery_voltage));

  Serial.printf(",keys=%u%u%u%u,i2s_sweep=%u,buzzer_music=%u,line_ok=%u,line_bits=%u%u%u%u%u%u%u%u,line_raw=",
                latest_button_states[0] ? 1U : 0U,
                latest_button_states[1] ? 1U : 0U,
                latest_button_states[2] ? 1U : 0U,
                latest_button_states[3] ? 1U : 0U,
                i2s_speaker_is_sweep_playing() ? 1U : 0U,
                is_buzzer_melody_playing ? 1U : 0U,
                is_line_data_valid ? 1U : 0U,
                latest_line_states[0], latest_line_states[1],
                latest_line_states[2], latest_line_states[3],
                latest_line_states[4], latest_line_states[5],
                latest_line_states[6], latest_line_states[7]);
  for (uint8_t i = 0; i < 8; i++) {
    if (i > 0) {
      Serial.print('|');
    }
    Serial.print(latest_line_raw[i]);
  }

#if ENABLE_MPU6050
  Serial.printf(",imu_ok=%u,roll=%.1f,pitch=%.1f,yaw=%.1f,gx=%d,gy=%d,gz=%d",
                is_mpu6050_ready ? 1U : 0U,
                static_cast<double>(roll),
                static_cast<double>(pitch),
                static_cast<double>(yaw),
                gx, gy, gz);
#else
  Serial.print(",imu_ok=0,roll=0.0,pitch=0.0,yaw=0.0,gx=0,gy=0,gz=0");
#endif

  Serial.println();
}

static void battery_update(uint32_t now_ms)
{
  static uint32_t last_update_ms = 0;
  if (now_ms - last_update_ms < BATTERY_UPDATE_INTERVAL_MS) {
    return;
  }
  last_update_ms = now_ms;

  uint32_t millivolt_sum = 0;
  for (uint8_t i = 0; i < BATTERY_ADC_SAMPLE_COUNT; i++) {
    millivolt_sum += analogReadMilliVolts(BATTERY_ADC_PIN);
  }

  const float adc_voltage = static_cast<float>(millivolt_sum)
                          / static_cast<float>(BATTERY_ADC_SAMPLE_COUNT)
                          / 1000.0f;
  latest_battery_voltage = adc_voltage * BATTERY_VOLTAGE_DIVIDER_RATIO;
}

static void line_sensor_update(uint32_t now_ms)
{
  static uint32_t last_request_ms = 0;
  static uint32_t request_start_ms = 0;
  static bool is_response_pending = false;

  if (is_response_pending) {
    if (now_ms - request_start_ms < LINE_RESPONSE_DELAY_MS) {
      return;
    }

    is_line_data_valid = line_sensor_read_response();
    is_response_pending = false;
    return;
  }

  if (now_ms - last_request_ms < LINE_READ_INTERVAL_MS) {
    return;
  }
  last_request_ms = now_ms;

  Wire.beginTransmission(LINE_SENSOR_ADDRESS);
  Wire.write(LINE_READ_COMMAND);
  if (Wire.endTransmission() != 0) {
    is_line_data_valid = false;
    return;
  }

  request_start_ms = now_ms;
  is_response_pending = true;
}

static bool line_sensor_read_response()
{
  const uint8_t received_bytes = Wire.requestFrom(LINE_SENSOR_ADDRESS, LINE_DATA_LENGTH);
  if (received_bytes != LINE_DATA_LENGTH) {
    while (Wire.available() > 0) {
      Wire.read();
    }
    return false;
  }

  // 前 16 字节不是八路循迹通道数据。
  for (uint8_t i = 0; i < 16; i++) {
    Wire.read();
  }

  for (uint8_t i = 0; i < 8; i++) {
    const uint8_t high_byte = Wire.read();
    const uint8_t low_byte = Wire.read();
    const uint16_t raw_value = (static_cast<uint16_t>(high_byte) << 8) | low_byte;
    latest_line_raw[i] = raw_value;
    latest_line_states[i] = LINE_HIGH_VALUE_IS_ACTIVE
                          ? (raw_value >= LINE_THRESHOLD ? 1U : 0U)
                          : (raw_value < LINE_THRESHOLD ? 1U : 0U);
  }

  return true;
}
