#include <Arduino.h>

static constexpr uint8_t RIGHT_PWM_PIN = 42;
static constexpr uint8_t RIGHT_DIR_PIN = 41;
static constexpr uint8_t LEFT_PWM_PIN = 1;
static constexpr uint8_t LEFT_DIR_PIN = 2;
static constexpr bool RIGHT_INVERT_DIRECTION = false;
static constexpr bool LEFT_INVERT_DIRECTION = true;
static constexpr uint32_t DIRECTION_DEAD_TIME_US = 1000;
static constexpr uint32_t AUTO_STOP_MS = 3000;

static int test_pwm = 80;
static int left_output = 0;
static int right_output = 0;

static void setOneMotor(uint8_t pwm_pin, uint8_t dir_pin, int pwm, bool invert)
{
  // Remove drive before changing direction to protect the H-bridge.
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

static void drive(int left_pwm, int right_pwm)
{
  left_output = constrain(left_pwm, -255, 255);
  right_output = constrain(right_pwm, -255, 255);
  setOneMotor(LEFT_PWM_PIN, LEFT_DIR_PIN, left_output, LEFT_INVERT_DIRECTION);
  setOneMotor(RIGHT_PWM_PIN, RIGHT_DIR_PIN, right_output, RIGHT_INVERT_DIRECTION);
  Serial.printf("OUTPUT  left=%d right=%d\n", left_output, right_output);
}

static void stopMotors()
{
  left_output = 0;
  right_output = 0;
  analogWrite(LEFT_PWM_PIN, 0);
  analogWrite(RIGHT_PWM_PIN, 0);
  Serial.println("STOP");
}

static void printHelp()
{
  Serial.println("Commands: f=both forward, b=both reverse, l=left only, r=right only");
  Serial.println("          s=stop, +=PWM+20, -=PWM-20, ?=help");
  Serial.println("Every movement stops automatically after 3 seconds.");
}

void setup()
{
  Serial.begin(115200);
  pinMode(RIGHT_DIR_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN, OUTPUT);
  pinMode(RIGHT_PWM_PIN, OUTPUT);
  pinMode(LEFT_PWM_PIN, OUTPUT);
  stopMotors();
  delay(500);

  Serial.println("\n=== MOTOR TEST ===");
  Serial.println("WARNING: Lift the wheels off the ground before sending a movement command.");
  printHelp();
}

void loop()
{
  static uint32_t movement_started_ms = 0;

  if ((left_output != 0 || right_output != 0) &&
      millis() - movement_started_ms >= AUTO_STOP_MS) {
    stopMotors();
  }

  if (!Serial.available()) {
    delay(1);
    return;
  }

  const char command = static_cast<char>(Serial.read());
  switch (command) {
    case 'f':
    case 'F': drive(test_pwm, test_pwm); movement_started_ms = millis(); break;
    case 'b':
    case 'B': drive(-test_pwm, -test_pwm); movement_started_ms = millis(); break;
    case 'l':
    case 'L': drive(test_pwm, 0); movement_started_ms = millis(); break;
    case 'r':
    case 'R': drive(0, test_pwm); movement_started_ms = millis(); break;
    case 's':
    case 'S': stopMotors(); break;
    case '+': test_pwm = min(255, test_pwm + 20); Serial.printf("TEST PWM=%d\n", test_pwm); break;
    case '-': test_pwm = max(20, test_pwm - 20); Serial.printf("TEST PWM=%d\n", test_pwm); break;
    case '?': printHelp(); break;
    default: break;  // Ignore line endings from the serial monitor.
  }
}
