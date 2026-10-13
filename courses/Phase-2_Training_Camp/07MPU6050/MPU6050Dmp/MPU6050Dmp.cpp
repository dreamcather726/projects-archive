// 引入本函数库自己的头文件，保证声明与实现使用相同的函数格式。
#include "MPU6050Dmp.h"

// Wire.h 提供 ESP32 的 I2C 通信功能。
#include <Wire.h>
// I2Cdev.h 是 Electronic Cats MPU6050 库依赖的 I2C 辅助库。
#include "I2Cdev.h"
// 该头文件提供 DMP 初始化、FIFO、四元数和姿态角相关函数。
#include "MPU6050_6Axis_MotionApps20.h"

// 以下变量只在本 .cpp 文件内部使用。
// static 可以把变量限制在当前文件中，避免主程序直接修改底层数据。

// 创建 Electronic Cats 库提供的 MPU6050 对象。
// 自己编写的函数会通过这个对象调用传感器底层功能。
static MPU6050 mpu;

// connectionOk 记录 mpu.testConnection() 是否成功。
static bool connectionOk = false;

// dmpReady 记录 DMP 是否已经初始化、校准并开启。
static bool dmpReady = false;

// dmpStatus 保存 dmpInitialize() 返回的状态码。
// 初始值 0xFF 表示还没有执行 DMP 初始化。
static uint8_t dmpStatus = 0xFF;

// FIFO 每次输出一包二进制数据，本数组负责暂存当前数据包。
static uint8_t fifoBuffer[64];

// quaternion 保存从 FIFO 数据中提取出的四元数。
static Quaternion quaternion;

// gravity 保存根据四元数计算出的重力方向向量。
static VectorFloat gravity;

// ypr[0]、ypr[1]、ypr[2] 依次保存弧度制的 Yaw、Pitch、Roll。
static float ypr[3] = {0.0f, 0.0f, 0.0f};

// 下面三个变量保存已经换算成角度的姿态值。
// get 函数会把这些结果提供给主程序。
static float yaw = 0.0f;
static float pitch = 0.0f;
static float roll = 0.0f;

// 初始化函数的四个参数由头文件中的默认值或主程序传入。
bool mpuDmpBegin(int sdaPin,
                 int sclPin,
                 uint32_t i2cClock,
                 uint8_t calibrationLoops) {
  // 第一步：清除上一次运行留下的状态和角度。
  // 如果重新初始化失败，主程序就不会误用上一次的成功状态。
  connectionOk = false;
  dmpReady = false;
  dmpStatus = 0xFF;
  yaw = 0.0f;
  pitch = 0.0f;
  roll = 0.0f;

  // 第二步：按主程序给出的 SDA、SCL 引脚启动 I2C。
  // setClock() 设置通信速度，本程序默认使用 400 kHz。
  Wire.begin(sdaPin, sclPin);
  Wire.setClock(i2cClock);

  // 第三步：初始化并唤醒 MPU6050，使芯片进入可通信状态。
  mpu.initialize();

  // 第四步：检查 ESP32 能否通过 I2C 正常访问 MPU6050。
  // 连接失败时立即返回 false，不继续执行 DMP 初始化。
  if (!mpu.testConnection()) {
    return false;
  }

  // 基础连接测试通过，保存状态供 mpuDmpIsConnected() 查询。
  connectionOk = true;

  // 第五步：初始化 MPU6050 内部的 DMP，并保存返回状态码。
  // 状态码为 0 才表示初始化成功。
  dmpStatus = mpu.dmpInitialize();

  if (dmpStatus != 0) {
    return false;
  }

  // 第六步：先把加速度计和陀螺仪的六个旧偏移量清零。
  // 这样自动校准会从统一的初始状态开始。
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);

  // 分别校准加速度计和陀螺仪。
  // calibrationLoops 默认等于 6；校准期间移动模块会影响结果。
  mpu.CalibrateAccel(calibrationLoops);
  mpu.CalibrateGyro(calibrationLoops);

  // 第七步：校准完成后开启 DMP，并把准备状态改为 true。
  // 到这里，mpuDmpUpdate() 才允许读取 FIFO 数据。
  mpu.setDMPEnabled(true);
  dmpReady = true;

  // 所有初始化步骤均成功完成。
  return true;
}

bool mpuDmpUpdate() {
  // DMP 没有准备完成时不能读取 FIFO，直接返回 false。
  if (!dmpReady) {
    return false;
  }

  // 尝试取得最新的一包完整 FIFO 数据，并存入 fifoBuffer。
  // 暂时没有完整数据包时返回 false，不覆盖上一次有效角度。
  if (!mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    return false;
  }

  // 第一步：从 FIFO 原始数据包中提取四元数。
  mpu.dmpGetQuaternion(&quaternion, fifoBuffer);

  // 第二步：根据四元数计算重力在传感器坐标系中的方向。
  mpu.dmpGetGravity(&gravity, &quaternion);

  // 第三步：结合四元数和重力向量得到三个弧度制姿态角。
  mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity);

  // DMP 返回的是弧度，乘 180/PI 后换算成角度。
  // 数组下标 0、1、2 分别对应 Yaw、Pitch、Roll。
  yaw = ypr[0] * 180.0f / PI;
  pitch = ypr[1] * 180.0f / PI;
  roll = ypr[2] * 180.0f / PI;

  // 三个姿态角都已更新，主程序现在可以读取新结果。
  return true;
}

// 以下函数只返回内部保存的状态或结果，不会重新读取传感器。

bool mpuDmpIsConnected() {
  return connectionOk;
}

bool mpuDmpIsReady() {
  return dmpReady;
}

uint8_t mpuDmpGetStatus() {
  return dmpStatus;
}

float mpuDmpGetYaw() {
  return yaw;
}

float mpuDmpGetPitch() {
  return pitch;
}

float mpuDmpGetRoll() {
  return roll;
}

void mpuDmpPrintOffsets() {
  // Electronic Cats 库的这个函数会直接把六个有效偏移量输出到串口。
  mpu.PrintActiveOffsets();
}
