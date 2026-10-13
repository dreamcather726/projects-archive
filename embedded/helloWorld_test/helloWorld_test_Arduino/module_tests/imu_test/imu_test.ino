#include <Arduino.h>
#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>

// MPU6050 I2C: SDA=8, SCL=9, address=0x68.
static constexpr uint8_t IMU_SDA_PIN = 8;
static constexpr uint8_t IMU_SCL_PIN = 9;
static constexpr uint8_t IMU_ADDRESS = 0x68;
static constexpr uint32_t PRINT_INTERVAL_MS = 200;

static MPU6050 imu(IMU_ADDRESS);
static bool imu_ready = false;
static uint8_t fifo_buffer[64];

static void printRawData()
{
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  imu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  Serial.printf("ACC=%6d,%6d,%6d  GYRO=%6d,%6d,%6d", ax, ay, az, gx, gy, gz);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== MPU6050 TEST ===");
  Serial.println("Keep the board still while it initializes.");

  Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);
  Wire.setClock(400000);
  imu.initialize();

  if (!imu.testConnection()) {
    Serial.println("FAIL: MPU6050 not found. Check 3.3V, GND, SDA=8, SCL=9 and AD0.");
    return;
  }

  const uint8_t status = imu.dmpInitialize();
  if (status != 0) {
    Serial.printf("FAIL: DMP initialization returned error %u.\n", status);
    return;
  }

  imu.CalibrateAccel(6);
  imu.CalibrateGyro(6);
  imu.setDMPEnabled(true);
  imu.resetFIFO();
  imu_ready = true;
  Serial.println("PASS: MPU6050 connected. Rotate the board to observe R/P/Y changes.");
}

void loop()
{
  if (!imu_ready) {
    delay(1000);
    return;
  }

  static uint32_t last_print_ms = 0;
  if (millis() - last_print_ms < PRINT_INTERVAL_MS) {
    delay(1);
    return;
  }
  last_print_ms = millis();

  if (!imu.dmpGetCurrentFIFOPacket(fifo_buffer)) {
    Serial.println("WAIT: no complete DMP packet");
    return;
  }

  Quaternion quaternion;
  VectorFloat gravity;
  float ypr[3];
  imu.dmpGetQuaternion(&quaternion, fifo_buffer);
  imu.dmpGetGravity(&gravity, &quaternion);
  imu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity);

  const float yaw = ypr[0] * 180.0f / PI;
  const float pitch = ypr[1] * 180.0f / PI;
  const float roll = ypr[2] * 180.0f / PI;
  Serial.printf("R/P/Y=%7.2f,%7.2f,%7.2f  ", roll, pitch, yaw);
  printRawData();
  Serial.println();
}
