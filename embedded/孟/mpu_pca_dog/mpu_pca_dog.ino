/*
  ESP32 + MPU6050 + PCA9685 example

  I2C wiring:
    SDA -> GPIO 8
    SCL -> GPIO 9
    MPU6050 default address -> 0x68
    PCA9685 default address -> 0x40

  Serial commands (115200 baud):
    S <channel> <angle>       e.g. S 0 90
    P <channel> <pulse_us>    e.g. P 3 1500
    D <channel>               e.g. D 3
    X                         disable all channels
*/

#include <Wire.h>
#include <stdio.h>

#include "imu.h"
#include "pca9685.h"

namespace {
const uint8_t MPU6050_ADDRESS = 0x68;
const uint8_t PCA9685_ADDRESS = 0x40;
const float SERVO_FREQUENCY_HZ = 50.0f;

// 硬件定时器以 1 MHz 计数，每 1000 个计数（1 ms）进入一次中断。
const uint32_t TIMER_FREQUENCY_HZ = 1000000;
const uint32_t TIMER_BASE_TICK_US = 1000;
const uint16_t IMU_TASK_PERIOD_TICKS = 5;       // IMU 更新周期：5 ms / 200 Hz
const uint16_t SERVO_TASK_PERIOD_TICKS = 10;    // 舵机任务周期：10 ms / 100 Hz
const uint16_t PRINT_TASK_PERIOD_TICKS = 100;   // 数据输出周期：100 ms / 10 Hz

// 四足机器人实际使用的 12 个 PCA9685 通道。
// 顺序：右前、右后、左前、左后，每条腿依次为髋/大腿/小腿。
const uint8_t ROBOT_SERVO_CHANNELS[] = {
    0, 1, 2,
    8, 9, 10,
    4, 5, 6,
    12, 13, 14
};
const uint8_t ROBOT_SERVO_COUNT =
    sizeof(ROBOT_SERVO_CHANNELS) / sizeof(ROBOT_SERVO_CHANNELS[0]);
const float SERVO_CENTER_ANGLE_DEG = 90.0f;
const float SERVO_TEST_OFFSET_DEG = 5.0f;        // 上电自检只在中位附近微动 5 度
// 与参考程序 angle_to_pluse() 保持一致：0 度约 93 计数，180 度约 468 计数。
const uint16_t REFERENCE_SERVO_MIN_COUNT = 93;
const uint16_t REFERENCE_SERVO_MAX_COUNT = 468;
const uint16_t SERVO_TEST_SETTLE_TICKS = 30;    // 全部回中后等待 300 ms
const uint16_t SERVO_TEST_HOLD_TICKS = 15;      // 每个测试位置保持 150 ms
const uint16_t SERVO_TEST_GAP_TICKS = 8;        // 通道之间间隔 80 ms

// 中断函数只设置任务标志，loop() 根据标志执行具体任务。
enum TaskFlag : uint32_t {
    TASK_NONE = 0,
    TASK_IMU_UPDATE = 1UL << 0,
    TASK_SERVO_COMMAND = 1UL << 1,
    TASK_IMU_PRINT = 1UL << 2
};

// 舵机自检采用状态机，每次定时器调度只执行一小步，不使用 delay()。
enum ServoTestPhase : uint8_t {
    SERVO_TEST_IDLE,
    SERVO_TEST_CENTER_ALL,
    SERVO_TEST_MOVE_POSITIVE,
    SERVO_TEST_MOVE_NEGATIVE,
    SERVO_TEST_RESTORE_CENTER
};

PCA9685 servoController(PCA9685_ADDRESS, Wire);
// 两个设备分别记录初始化状态，一个设备失败不会阻止另一个工作。
bool imuReady = false;
bool pcaReady = false;
bool imuDataUpdatedSincePrint = false;
char commandBuffer[48];
size_t commandLength = 0;

hw_timer_t *taskTimer = nullptr;
portMUX_TYPE taskTimerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t scheduledTaskFlags = TASK_NONE;
volatile uint16_t imuTaskTicks = 0;
volatile uint16_t servoTaskTicks = 0;
volatile uint16_t printTaskTicks = 0;
ServoTestPhase servoTestPhase = SERVO_TEST_IDLE;
uint8_t servoTestChannelIndex = 0;
uint16_t servoTestWaitTicks = 0;

void startServoSelfTest();

// 定时器中断不能直接调用 Wire 或 Serial，否则可能发生阻塞或总线冲突。
void ARDUINO_ISR_ATTR onTaskTimer() {
    portENTER_CRITICAL_ISR(&taskTimerMux);

    // 使用显式赋值，兼容 C++20 对 volatile 自增操作的限制。
    imuTaskTicks = imuTaskTicks + 1;
    servoTaskTicks = servoTaskTicks + 1;
    printTaskTicks = printTaskTicks + 1;

    if (imuTaskTicks >= IMU_TASK_PERIOD_TICKS) {
        imuTaskTicks = 0;
        scheduledTaskFlags = scheduledTaskFlags | TASK_IMU_UPDATE;
    }

    if (servoTaskTicks >= SERVO_TASK_PERIOD_TICKS) {
        servoTaskTicks = 0;
        scheduledTaskFlags = scheduledTaskFlags | TASK_SERVO_COMMAND;
    }

    if (printTaskTicks >= PRINT_TASK_PERIOD_TICKS) {
        printTaskTicks = 0;
        scheduledTaskFlags = scheduledTaskFlags | TASK_IMU_PRINT;
    }

    portEXIT_CRITICAL_ISR(&taskTimerMux);
}

// 原子地取出并清除待执行标志，避免与定时器中断同时访问。
uint32_t takeScheduledTasks() {
    portENTER_CRITICAL(&taskTimerMux);
    const uint32_t tasks = scheduledTaskFlags;
    scheduledTaskFlags = TASK_NONE;
    portEXIT_CRITICAL(&taskTimerMux);
    return tasks;
}

bool startTaskTimer() {
    // 1 MHz 计数频率下，定时器的一个计数对应 1 us。
    taskTimer = timerBegin(TIMER_FREQUENCY_HZ);
    if (taskTimer == nullptr) {
        return false;
    }

    timerAttachInterrupt(taskTimer, &onTaskTimer);
    timerAlarm(taskTimer, TIMER_BASE_TICK_US, true, 0);

    // 启动后立即执行一轮任务，不必等待第一个调度周期。
    portENTER_CRITICAL(&taskTimerMux);
    scheduledTaskFlags = TASK_IMU_UPDATE | TASK_SERVO_COMMAND | TASK_IMU_PRINT;
    portEXIT_CRITICAL(&taskTimerMux);
    return true;
}

void printCommandHelp() {
    Serial.println("Servo commands:");
    Serial.println("  S <channel 0-15> <angle 0-180>");
    Serial.println("  P <channel 0-15> <pulse width us>");
    Serial.println("  D <channel 0-15>");
    Serial.println("  X (disable all channels)");
    Serial.println("  T (run the 12-servo self-test again)");
}

void processCommand(const char *command) {
    if (!pcaReady) {
        Serial.println("PCA9685 is not ready");
        return;
    }

    unsigned int channel = 0;
    float value = 0.0f;
    bool success = false;

    if (command[0] == 'T' || command[0] == 't') {
        startServoSelfTest();
        return;
    }

    // S 命令按角度控制，P 命令用于直接调试微秒脉宽。
    if (sscanf(command, " S %u %f", &channel, &value) == 2) {
        success = channel < PCA9685::CHANNEL_COUNT &&
                  servoController.setServoAngle(static_cast<uint8_t>(channel),
                                                value);
    } else if (sscanf(command, " P %u %f", &channel, &value) == 2) {
        success = channel < PCA9685::CHANNEL_COUNT &&
                  servoController.setPulseWidthUs(static_cast<uint8_t>(channel),
                                                  value);
    } else if (sscanf(command, " D %u", &channel) == 1) {
        success = channel < PCA9685::CHANNEL_COUNT &&
                  servoController.disableChannel(static_cast<uint8_t>(channel));
    } else if (command[0] == 'X' || command[0] == 'x') {
        // X 是紧急停止命令，同时取消正在进行的上电自检。
        servoTestPhase = SERVO_TEST_IDLE;
        success = servoController.disableAllChannels();
    } else {
        printCommandHelp();
        return;
    }

    Serial.println(success ? "OK" : "ERROR: invalid value or I2C write failed");
}

void readSerialCommands() {
    // 逐字节接收命令，避免 readStringUntil() 阻塞 IMU 的高频更新。
    while (Serial.available() > 0) {
        const char input = static_cast<char>(Serial.read());
        if (input == '\r') {
            continue;
        }

        if (input == '\n') {
            commandBuffer[commandLength] = '\0';
            if (commandLength > 0) {
                processCommand(commandBuffer);
            }
            commandLength = 0;
        } else if (commandLength < sizeof(commandBuffer) - 1) {
            commandBuffer[commandLength++] = input;
        } else {
            commandLength = 0;
            Serial.println("ERROR: command is too long");
        }
    }
}

void printImuData() {
    // 只显示 Pitch/Roll 两个姿态角，单位为度，保留 1 位小数。
    Serial.print("Pitch/Roll(deg): ");
    Serial.print(pitch, 1);
    Serial.print(", ");
    Serial.println(roll, 1);
}

void runImuTask() {
    if (imuReady && mpu6050_update()) {
        // 只有在 DMP 产生新数据后才允许打印，避免反复输出旧数据。
        imuDataUpdatedSincePrint = true;
    }
}

void startServoSelfTest() {
    if (!pcaReady) {
        Serial.println("ERROR: PCA9685 is not ready; self-test cannot start");
        return;
    }

    servoTestChannelIndex = 0;
    servoTestWaitTicks = 0;
    servoTestPhase = SERVO_TEST_CENTER_ALL;
    Serial.println("Starting 12-servo power-on self-test");
}

void runServoSelfTest() {
    if (!pcaReady || servoTestPhase == SERVO_TEST_IDLE) {
        return;
    }

    if (servoTestWaitTicks > 0) {
        --servoTestWaitTicks;
        return;
    }

    const uint8_t channel = ROBOT_SERVO_CHANNELS[servoTestChannelIndex];
    bool success = true;

    switch (servoTestPhase) {
        case SERVO_TEST_CENTER_ALL:
            // 先使 12 个舵机全部回到 90 度中位。
            for (uint8_t index = 0; index < ROBOT_SERVO_COUNT; ++index) {
                if (!servoController.setServoAngle(ROBOT_SERVO_CHANNELS[index],
                                                   SERVO_CENTER_ANGLE_DEG)) {
                    success = false;
                }
            }
            servoTestPhase = SERVO_TEST_MOVE_POSITIVE;
            servoTestWaitTicks = SERVO_TEST_SETTLE_TICKS;
            break;

        case SERVO_TEST_MOVE_POSITIVE:
            Serial.print("Testing servo channel ");
            Serial.println(channel);
            success = servoController.setServoAngle(
                channel, SERVO_CENTER_ANGLE_DEG + SERVO_TEST_OFFSET_DEG);
            servoTestPhase = SERVO_TEST_MOVE_NEGATIVE;
            servoTestWaitTicks = SERVO_TEST_HOLD_TICKS;
            break;

        case SERVO_TEST_MOVE_NEGATIVE:
            success = servoController.setServoAngle(
                channel, SERVO_CENTER_ANGLE_DEG - SERVO_TEST_OFFSET_DEG);
            servoTestPhase = SERVO_TEST_RESTORE_CENTER;
            servoTestWaitTicks = SERVO_TEST_HOLD_TICKS;
            break;

        case SERVO_TEST_RESTORE_CENTER:
            success = servoController.setServoAngle(channel,
                                                    SERVO_CENTER_ANGLE_DEG);
            ++servoTestChannelIndex;
            if (servoTestChannelIndex >= ROBOT_SERVO_COUNT) {
                servoTestPhase = SERVO_TEST_IDLE;
                Serial.println("12-servo power-on self-test complete");
            } else {
                servoTestPhase = SERVO_TEST_MOVE_POSITIVE;
                servoTestWaitTicks = SERVO_TEST_GAP_TICKS;
            }
            break;

        case SERVO_TEST_IDLE:
            break;
    }

    if (!success) {
        Serial.print("ERROR: servo self-test I2C write failed on channel ");
        Serial.println(channel);
        servoTestPhase = SERVO_TEST_IDLE;
    }
}

void runServoCommandTask() {
    // 自检和串口舵机命令共用同一个定时任务，避免同时写 PCA9685。
    runServoSelfTest();
    readSerialCommands();
}

void runImuPrintTask() {
    if (imuReady && imuDataUpdatedSincePrint) {
        imuDataUpdatedSincePrint = false;
        printImuData();
    }
}
}  // namespace

void setup() {
    Serial.begin(115200);
    delay(200);

    // mpu6050_init() 同时在 GPIO 8/9 上初始化 400 kHz 共享 I2C 总线。
    imuReady = mpu6050_init(MPU6050_ADDRESS);
    Serial.println(imuReady ? "MPU6050 ready" : "ERROR: MPU6050 not found/DMP failed");

    pcaReady = servoController.begin(SERVO_FREQUENCY_HZ) &&
               servoController.disableAllChannels();
    if (pcaReady) {
        // 初始化后已先关闭全部输出，等待定时器启动上电自检。
        // 将参考程序中已验证的 PCA9685 计数范围换算为微秒，
        // 使当前库的角度接口与原 angle_to_pluse() 的舅机位置保持一致。
        const float pwmTickUs =
            1000000.0f / (servoController.getPWMFrequency() * 4096.0f);
        const float servoMinPulseUs = REFERENCE_SERVO_MIN_COUNT * pwmTickUs;
        const float servoMaxPulseUs = REFERENCE_SERVO_MAX_COUNT * pwmTickUs;

        for (uint8_t channel = 0; channel < PCA9685::CHANNEL_COUNT; ++channel) {
            servoController.setServoCalibration(channel,
                                                servoMinPulseUs,
                                                servoMaxPulseUs,
                                                0.0f, 180.0f);
        }
        Serial.print("PCA9685 ready, actual PWM frequency: ");
        Serial.print(servoController.getPWMFrequency(), 4);
        Serial.println(" Hz");

        // 每次上电自动启动整机 12 舵机微动检查。
        startServoSelfTest();
    } else {
        Serial.println("ERROR: PCA9685 not found/configuration failed");
    }

    printCommandHelp();

    if (startTaskTimer()) {
        Serial.println("Hardware timer task scheduler ready");
    } else {
        Serial.println("ERROR: hardware timer initialization failed");
    }
}

void loop() {
    const uint32_t tasks = takeScheduledTasks();

    // IMU 任务优先执行，保证 DMP FIFO 能及时读取。
    if ((tasks & TASK_IMU_UPDATE) != 0) {
        runImuTask();
    }

    if ((tasks & TASK_SERVO_COMMAND) != 0) {
        runServoCommandTask();
    }

    if ((tasks & TASK_IMU_PRINT) != 0) {
        runImuPrintTask();
    }
}
