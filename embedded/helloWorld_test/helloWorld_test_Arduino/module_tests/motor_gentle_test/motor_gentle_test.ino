#include <Arduino.h>

static constexpr uint8_t RIGHT_PWM_PIN = 42;
static constexpr uint8_t RIGHT_DIR_PIN = 41;
static constexpr uint8_t LEFT_PWM_PIN = 1;
static constexpr uint8_t LEFT_DIR_PIN = 2;

static constexpr bool RIGHT_INVERT_DIRECTION = false;
static constexpr bool LEFT_INVERT_DIRECTION = true;
static constexpr uint8_t GENTLE_TARGET_PWM = 120;
static constexpr uint8_t RAMP_STEP = 5;
static constexpr uint32_t START_DELAY_MS = 1000;
static constexpr uint32_t RAMP_INTERVAL_MS = 50;
static constexpr uint32_t DIRECTION_DEAD_TIME_US = 1000;

static int current_pwm = 0;
static bool ramp_enabled = true;
static uint32_t start_requested_ms = 0;
static uint32_t last_ramp_ms = 0;

static void setOneMotor(uint8_t pwm_pin, uint8_t dir_pin, int pwm, bool invert)
{
  pwm = constrain(pwm, -255, 255);
  bool forward = pwm >= 0;
  if (invert) {
    forward = !forward;
  }

  digitalWrite(dir_pin, forward ? HIGH : LOW);
  analogWrite(pwm_pin, abs(pwm));
}

static void applyMotorPwm(int pwm)
{
  current_pwm = constrain(pwm, 0, GENTLE_TARGET_PWM);
  setOneMotor(RIGHT_PWM_PIN, RIGHT_DIR_PIN,
              current_pwm, RIGHT_INVERT_DIRECTION);
  setOneMotor(LEFT_PWM_PIN, LEFT_DIR_PIN,
              current_pwm, LEFT_INVERT_DIRECTION);
}

static void stopMotors()
{
  analogWrite(RIGHT_PWM_PIN, 0);
  analogWrite(LEFT_PWM_PIN, 0);
  current_pwm = 0;
  ramp_enabled = false;
  Serial.println("STOP: both motors stopped");
}

static void requestGentleStart()
{
  analogWrite(RIGHT_PWM_PIN, 0);
  analogWrite(LEFT_PWM_PIN, 0);
  current_pwm = 0;
  ramp_enabled = true;
  start_requested_ms = millis();
  last_ramp_ms = start_requested_ms;
  Serial.println("WAIT: motors will start after 1 second");
}

void setup()
{
  Serial.begin(115200);

  pinMode(RIGHT_PWM_PIN, OUTPUT);
  pinMode(RIGHT_DIR_PIN, OUTPUT);
  pinMode(LEFT_PWM_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN, OUTPUT);

  analogWrite(RIGHT_PWM_PIN, 0);
  analogWrite(LEFT_PWM_PIN, 0);
  digitalWrite(RIGHT_DIR_PIN, LOW);
  digitalWrite(LEFT_DIR_PIN, LOW);
  delayMicroseconds(DIRECTION_DEAD_TIME_US);

  Serial.println("\n=== MOTOR GENTLE-START TEST ===");
  Serial.println("WARNING: lift the wheels before reset or power-on");
  Serial.println("Commands: s=stop, r=restart the delayed gentle ramp");
  requestGentleStart();
}

void loop()
{
  if (Serial.available()) {
    const char command = static_cast<char>(Serial.read());
    if (command == 's' || command == 'S') {
      stopMotors();
    } else if (command == 'r' || command == 'R') {
      requestGentleStart();
    }
  }

  const uint32_t now_ms = millis();
  if (!ramp_enabled || now_ms - start_requested_ms < START_DELAY_MS ||
      now_ms - last_ramp_ms < RAMP_INTERVAL_MS) {
    delay(1);
    return;
  }
  last_ramp_ms = now_ms;

  if (current_pwm < GENTLE_TARGET_PWM) {
    applyMotorPwm(min(static_cast<int>(GENTLE_TARGET_PWM),
                      current_pwm + RAMP_STEP));
    Serial.printf("RAMP: PWM=%d\n", current_pwm);
  } else {
    ramp_enabled = false;
    Serial.printf("RUN: gentle target reached, PWM=%u\n", GENTLE_TARGET_PWM);
  }
}
