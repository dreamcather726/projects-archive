#include <Arduino.h>

static constexpr uint8_t RIGHT_PWM_PIN = 42;
static constexpr uint8_t RIGHT_DIR_PIN = 41;
static constexpr uint8_t LEFT_PWM_PIN = 1;
static constexpr uint8_t LEFT_DIR_PIN = 2;

static constexpr bool RIGHT_INVERT_DIRECTION = false;
static constexpr bool LEFT_INVERT_DIRECTION = true;
static constexpr uint8_t FULL_POWER_PWM = 255;
static constexpr uint32_t DIRECTION_DEAD_TIME_US = 1000;

static bool motors_running = false;

static void setOneMotor(uint8_t pwm_pin, uint8_t dir_pin, int pwm, bool invert)
{
  analogWrite(pwm_pin, 0);
  delayMicroseconds(DIRECTION_DEAD_TIME_US);

  pwm = constrain(pwm, -255, 255);
  bool forward = pwm >= 0;
  if (invert) {
    forward = !forward;
  }

  digitalWrite(dir_pin, forward ? HIGH : LOW);
  analogWrite(pwm_pin, abs(pwm));
}

static void stopMotors()
{
  analogWrite(RIGHT_PWM_PIN, 0);
  analogWrite(LEFT_PWM_PIN, 0);
  motors_running = false;
  Serial.println("STOP: both motors stopped");
}

static void startFullPower()
{
  setOneMotor(RIGHT_PWM_PIN, RIGHT_DIR_PIN,
              FULL_POWER_PWM, RIGHT_INVERT_DIRECTION);
  setOneMotor(LEFT_PWM_PIN, LEFT_DIR_PIN,
              FULL_POWER_PWM, LEFT_INVERT_DIRECTION);
  motors_running = true;
  Serial.println("RUN: both motors at PWM=255");
}

void setup()
{
  Serial.begin(115200);

  pinMode(RIGHT_PWM_PIN, OUTPUT);
  pinMode(RIGHT_DIR_PIN, OUTPUT);
  pinMode(LEFT_PWM_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN, OUTPUT);

  // Establish a known stopped state before applying full power.
  analogWrite(RIGHT_PWM_PIN, 0);
  analogWrite(LEFT_PWM_PIN, 0);
  digitalWrite(RIGHT_DIR_PIN, LOW);
  digitalWrite(LEFT_DIR_PIN, LOW);

  Serial.println("\n=== MOTOR FULL-POWER TEST ===");
  Serial.println("WARNING: lift the wheels before reset or power-on");
  Serial.println("Commands: s=emergency stop, r=run at full power again");

  // Requested behavior: start at maximum speed immediately after initialization.
  startFullPower();
}

void loop()
{
  if (!Serial.available()) {
    delay(1);
    return;
  }

  const char command = static_cast<char>(Serial.read());
  if (command == 's' || command == 'S') {
    stopMotors();
  } else if (command == 'r' || command == 'R') {
    startFullPower();
  }
}
