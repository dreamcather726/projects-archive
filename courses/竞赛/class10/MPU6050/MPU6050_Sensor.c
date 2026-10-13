/*
  MPU6050_Sensor.c
  Version: v1.0.1

  总体说明：
  - 使用 ESP32 自带的 driver/i2c.h 底层驱动
  - 不直接使用 C++ 的 Wire 对象，因此可以保持真正的 .c 文件
  - 功能包括初始化、地址检测、寄存器读写、唤醒和 14 字节数据解析
  - 实际代码采用行尾注释，方便青少年把“代码、硬件、现象”对应起来
*/

#include "MPU6050_Sensor.h"                     // 引入本封装的函数声明、寄存器地址和数据结构

#include <stddef.h>                              // 提供 NULL 等标准定义
#include <string.h>                              // 提供 memset()，用于清空配置结构体

#include "driver/i2c.h"                         // 提供 ESP32 底层 I2C 主机通信函数
#include "driver/gpio.h"                        // 提供 ESP32 GPIO 编号和上拉配置类型

#include "freertos/FreeRTOS.h"                  // 提供 FreeRTOS 基础类型和时间换算宏
#include "freertos/task.h"                      // 提供 vTaskDelay() 延时函数

static const i2c_port_t MPU6050_I2C_PORT =        // 定义本封装使用的 I2C 控制器
    I2C_NUM_0;                                    // 使用 ESP32 的第 0 号 I2C 控制器

static const TickType_t MPU6050_I2C_TIMEOUT =     // 定义每次 I2C 操作的最长等待时间
    pdMS_TO_TICKS(1000);                          // 把 1000 毫秒换算成 FreeRTOS 时钟节拍

static uint8_t g_mpu6050_address =                // 保存当前正在使用的 MPU6050 地址
    MPU6050_DEFAULT_ADDRESS;                      // 初始默认地址为 0x68

static bool g_i2c_initialized = false;            // 记录 I2C 驱动是否已经成功初始化

static bool execute_i2c_command(                  // 内部函数：把组织好的 I2C 命令真正发送到总线
    i2c_cmd_handle_t command                      // 接收一个已经创建并填写好的 I2C 命令对象
) {                                               // 内部执行函数开始
    if (command == NULL || !g_i2c_initialized) { // 命令无效或 I2C 未初始化时不能继续通信
        return false;                             // 返回 false 表示执行失败
    }                                             // 参数与状态检查结束

    esp_err_t result = i2c_master_cmd_begin(      // 启动底层 I2C 命令并等待执行结果
        MPU6050_I2C_PORT,                         // 指定使用 I2C_NUM_0 控制器
        command,                                  // 传入需要执行的命令链
        MPU6050_I2C_TIMEOUT                       // 设置最长等待时间为 1000 毫秒
    );                                            // 底层命令执行调用结束

    return result == ESP_OK;                      // ESP_OK 表示成功，否则返回失败
}                                                 // execute_i2c_command() 结束

int16_t mpu6050_combine_bytes(                    // 把传感器返回的高低字节组合成有符号数据
    uint8_t high_byte,                            // 接收高 8 位数据
    uint8_t low_byte                              // 接收低 8 位数据
) {                                               // 字节组合函数开始
    uint16_t value =                              // 创建无符号 16 位临时变量
        ((uint16_t)high_byte << 8) |              // 把高字节左移 8 位，放到结果的高半部分
        (uint16_t)low_byte;                       // 使用按位或加入低字节

    return (int16_t)value;                        // 转成有符号 16 位数并返回
}                                                 // mpu6050_combine_bytes() 结束

bool mpu6050_probe(void) {                        // 检查当前地址是否有 I2C 设备回应
    if (!g_i2c_initialized) {                     // I2C 尚未初始化时无法检测设备
        return false;                             // 返回 false 表示检测失败
    }                                             // 初始化状态检查结束

    i2c_cmd_handle_t command =                    // 创建变量保存 I2C 命令链
        i2c_cmd_link_create();                    // 在内存中创建一个新的 I2C 命令对象

    if (command == NULL) {                        // 如果命令对象创建失败
        return false;                             // 返回 false，避免继续使用空指针
    }                                             // 命令对象检查结束

    i2c_master_start(command);                    // 在命令链中加入 I2C 起始信号 START

    i2c_master_write_byte(                        // 在命令链中加入一个待发送字节
        command,                                  // 指定当前命令对象
        (g_mpu6050_address << 1) |                // 把 7 位设备地址左移，为读写位留出位置
            I2C_MASTER_WRITE,                     // 最低位设为写模式，用于地址探测
        true                                      // 要求从设备返回 ACK 确认信号
    );                                            // 写地址命令结束

    i2c_master_stop(command);                     // 在命令链末尾加入停止信号 STOP

    bool success =                                // 创建变量保存命令执行结果
        execute_i2c_command(command);             // 把完整命令链发送到 I2C 总线

    i2c_cmd_link_delete(command);                 // 删除命令对象并释放占用的内存

    return success;                               // 返回设备是否成功回应
}                                                 // mpu6050_probe() 结束

bool mpu6050_write_register(                      // 向 MPU6050 的一个寄存器写入一个字节
    uint8_t register_address,                     // 接收目标寄存器地址
    uint8_t value                                 // 接收需要写入的数据
) {                                               // 写寄存器函数开始
    if (!g_i2c_initialized) {                     // I2C 未初始化时禁止通信
        return false;                             // 返回 false 表示写入失败
    }                                             // 初始化状态检查结束

    i2c_cmd_handle_t command =                    // 创建 I2C 命令链变量
        i2c_cmd_link_create();                    // 分配一个新的命令对象

    if (command == NULL) {                        // 如果命令对象创建失败
        return false;                             // 返回 false，防止空指针操作
    }                                             // 命令对象检查结束

    i2c_master_start(command);                    // 加入 I2C 起始信号

    i2c_master_write_byte(                        // 发送 MPU6050 地址和写入方向位
        command,                                  // 指定当前命令对象
        (g_mpu6050_address << 1) |                // 把 7 位地址左移一位
            I2C_MASTER_WRITE,                     // 最低位为 0，表示主机要写数据
        true                                      // 要求设备返回 ACK
    );                                            // 写设备地址命令结束

    i2c_master_write_byte(                        // 发送需要操作的寄存器地址
        command,                                  // 指定当前命令对象
        register_address,                         // 指定目标寄存器
        true                                      // 要求设备返回 ACK
    );                                            // 写寄存器地址命令结束

    i2c_master_write_byte(                        // 发送真正需要写入的寄存器数据
        command,                                  // 指定当前命令对象
        value,                                    // 写入一个字节的数据
        true                                      // 要求设备返回 ACK
    );                                            // 写寄存器数据命令结束

    i2c_master_stop(command);                     // 加入 I2C 停止信号

    bool success =                                // 创建变量保存写入结果
        execute_i2c_command(command);             // 执行整条 I2C 写入命令

    i2c_cmd_link_delete(command);                 // 删除命令对象，避免内存泄漏

    return success;                               // 返回寄存器写入是否成功
}                                                 // mpu6050_write_register() 结束

bool mpu6050_read_bytes(                          // 从一个起始寄存器连续读取多个字节
    uint8_t start_register,                       // 接收连续读取的起始寄存器地址
    uint8_t *buffer,                              // 接收用于保存数据的数组地址
    uint8_t length                                // 接收需要读取的字节数量
) {                                               // 连续读取函数开始
    if (                                           // 开始检查所有必要条件
        !g_i2c_initialized ||                     // 条件一：I2C 必须已经初始化
        buffer == NULL ||                         // 条件二：数据缓冲区不能是空指针
        length == 0                               // 条件三：读取长度不能为 0
    ) {                                           // 任意条件不满足时进入错误处理
        return false;                             // 返回 false 表示无法读取
    }                                             // 参数检查结束

    i2c_cmd_handle_t command =                    // 创建 I2C 命令链变量
        i2c_cmd_link_create();                    // 分配新的命令对象

    if (command == NULL) {                        // 命令对象创建失败时
        return false;                             // 返回 false，防止使用空指针
    }                                             // 命令对象检查结束

    i2c_master_start(command);                    // 加入第一次 START，准备发送起始寄存器

    i2c_master_write_byte(                        // 发送设备地址，并进入写模式
        command,                                  // 指定当前命令对象
        (g_mpu6050_address << 1) |                // 把设备地址左移一位
            I2C_MASTER_WRITE,                     // 最低位为写模式
        true                                      // 要求设备返回 ACK
    );                                            // 写设备地址命令结束

    i2c_master_write_byte(                        // 告诉 MPU6050 要从哪个寄存器开始读取
        command,                                  // 指定当前命令对象
        start_register,                           // 发送起始寄存器地址
        true                                      // 要求设备返回 ACK
    );                                            // 写起始寄存器命令结束

    i2c_master_start(command);                    // 加入重复起始信号，不释放总线就切换方向

    i2c_master_write_byte(                        // 再次发送设备地址，这次进入读取模式
        command,                                  // 指定当前命令对象
        (g_mpu6050_address << 1) |                // 把设备地址左移一位
            I2C_MASTER_READ,                      // 最低位为 1，表示主机要读取数据
        true                                      // 要求设备返回 ACK
    );                                            // 读设备地址命令结束

    if (length > 1) {                             // 多于 1 个字节时先读取前面的字节
        i2c_master_read(                          // 连续读取 length-1 个字节
            command,                              // 指定当前命令对象
            buffer,                               // 把数据写入 buffer 数组
            length - 1,                           // 最后一个字节需要单独读取
            I2C_MASTER_ACK                        // 每读完一个前置字节都回复 ACK
        );                                        // 连续读取前置字节命令结束
    }                                             // 多字节读取判断结束

    i2c_master_read_byte(                         // 单独读取最后一个字节
        command,                                  // 指定当前命令对象
        buffer + length - 1,                      // 把最后一个字节存到数组末尾
        I2C_MASTER_NACK                           // 最后回复 NACK，表示主机不再继续读取
    );                                            // 最后一个字节读取命令结束

    i2c_master_stop(command);                     // 加入 STOP，正常结束本次通信

    bool success =                                // 创建变量保存读取执行结果
        execute_i2c_command(command);             // 执行完整的写寄存器地址加连续读取命令

    i2c_cmd_link_delete(command);                 // 删除命令对象并释放内存

    return success;                               // 返回连续读取是否成功
}                                                 // mpu6050_read_bytes() 结束

bool mpu6050_read_register(                       // 从指定寄存器读取一个字节
    uint8_t register_address,                     // 接收目标寄存器地址
    uint8_t *value                                // 接收保存结果的变量地址
) {                                               // 单寄存器读取函数开始
    if (value == NULL) {                          // 保存结果的指针不能是空指针
        return false;                             // 返回 false 表示参数错误
    }                                             // 指针检查结束

    return mpu6050_read_bytes(                    // 复用连续读取函数完成单字节读取
        register_address,                         // 从指定寄存器开始
        value,                                    // 把结果写入 value 指向的位置
        1                                         // 只读取 1 个字节
    );                                            // 返回连续读取函数的成功或失败结果
}                                                 // mpu6050_read_register() 结束

bool mpu6050_read_who_am_i(uint8_t *value) {      // 读取 MPU6050 的身份寄存器
    return mpu6050_read_register(                 // 复用单寄存器读取函数
        MPU6050_REG_WHO_AM_I,                     // 指定寄存器地址 0x75
        value                                     // 保存读取到的身份值
    );                                            // 返回身份寄存器读取结果
}                                                 // mpu6050_read_who_am_i() 结束

bool mpu6050_wake(void) {                         // 关闭 MPU6050 默认休眠状态
    return mpu6050_write_register(                // 复用写寄存器函数
        MPU6050_REG_PWR_MGMT_1,                   // 指定电源管理寄存器 0x6B
        0x00                                      // 写入 0x00，清除休眠位
    );                                            // 返回唤醒操作是否成功
}                                                 // mpu6050_wake() 结束

bool mpu6050_read_data(MPU6050_Data *data) {      // 一次读取并解析完整传感器数据
    if (data == NULL) {                           // 数据结构指针不能是空指针
        return false;                             // 返回 false 表示参数无效
    }                                             // 数据指针检查结束

    uint8_t raw_data[14];                         // 创建 14 字节数组保存 MPU6050 原始数据

    bool success = mpu6050_read_bytes(            // 从加速度寄存器开始连续读取数据
        MPU6050_REG_ACCEL_XOUT_H,                 // 指定起始寄存器为 0x3B
        raw_data,                                 // 把结果保存到 raw_data 数组
        sizeof(raw_data)                          // 自动取得数组长度 14
    );                                            // 连续读取调用结束

    if (!success) {                               // 如果底层读取失败
        return false;                             // 返回 false，不解析无效数据
    }                                             // 读取结果判断结束

    data->accel_x =                               // 保存 X 轴加速度结果
        mpu6050_combine_bytes(                    // 组合 X 轴高低两个字节
            raw_data[0],                          // 第 0 字节是 X 轴高字节
            raw_data[1]                           // 第 1 字节是 X 轴低字节
        );                                        // X 轴字节组合结束

    data->accel_y =                               // 保存 Y 轴加速度结果
        mpu6050_combine_bytes(                    // 组合 Y 轴高低两个字节
            raw_data[2],                          // 第 2 字节是 Y 轴高字节
            raw_data[3]                           // 第 3 字节是 Y 轴低字节
        );                                        // Y 轴字节组合结束

    data->accel_z =                               // 保存 Z 轴加速度结果
        mpu6050_combine_bytes(                    // 组合 Z 轴高低两个字节
            raw_data[4],                          // 第 4 字节是 Z 轴高字节
            raw_data[5]                           // 第 5 字节是 Z 轴低字节
        );                                        // Z 轴字节组合结束

    data->temperature_raw =                       // 保存温度传感器原始值
        mpu6050_combine_bytes(                    // 组合温度高低两个字节
            raw_data[6],                          // 第 6 字节是温度高字节
            raw_data[7]                           // 第 7 字节是温度低字节
        );                                        // 温度字节组合结束

    data->gyro_x =                                // 保存 X 轴陀螺仪结果
        mpu6050_combine_bytes(                    // 组合 X 轴陀螺仪高低字节
            raw_data[8],                          // 第 8 字节是陀螺仪 X 高字节
            raw_data[9]                           // 第 9 字节是陀螺仪 X 低字节
        );                                        // 陀螺仪 X 字节组合结束

    data->gyro_y =                                // 保存 Y 轴陀螺仪结果
        mpu6050_combine_bytes(                    // 组合 Y 轴陀螺仪高低字节
            raw_data[10],                         // 第 10 字节是陀螺仪 Y 高字节
            raw_data[11]                          // 第 11 字节是陀螺仪 Y 低字节
        );                                        // 陀螺仪 Y 字节组合结束

    data->gyro_z =                                // 保存 Z 轴陀螺仪结果
        mpu6050_combine_bytes(                    // 组合 Z 轴陀螺仪高低字节
            raw_data[12],                         // 第 12 字节是陀螺仪 Z 高字节
            raw_data[13]                          // 第 13 字节是陀螺仪 Z 低字节
        );                                        // 陀螺仪 Z 字节组合结束

    data->temperature_c =                         // 保存换算后的摄氏温度
        data->temperature_raw /                   // 使用温度原始值参与计算
        340.0F +                                  // 按 MPU6050 温度公式除以 340
        36.53F;                                   // 再加上 36.53 得到摄氏温度

    return true;                                  // 所有数据解析成功，返回 true
}                                                 // mpu6050_read_data() 结束

bool mpu6050_init(                                // 初始化底层 I2C 并启动 MPU6050
    int sda_pin,                                  // 接收 SDA GPIO 编号
    int scl_pin,                                  // 接收 SCL GPIO 编号
    uint8_t device_address,                       // 接收 MPU6050 I2C 地址
    uint32_t clock_hz,                            // 接收 I2C 时钟频率
    uint8_t *who_am_i                             // 接收可选的身份值保存地址
) {                                               // 初始化函数开始
    if (                                           // 开始检查初始化参数
        sda_pin < 0 ||                            // SDA GPIO 编号不能小于 0
        scl_pin < 0 ||                            // SCL GPIO 编号不能小于 0
        clock_hz == 0                             // I2C 时钟频率不能为 0
    ) {                                           // 任意参数无效时进入错误处理
        return false;                             // 返回 false 表示初始化失败
    }                                             // 参数检查结束

    if (g_i2c_initialized) {                      // 如果本封装此前已经初始化过 I2C
        i2c_driver_delete(                        // 删除旧的 I2C 驱动配置
            MPU6050_I2C_PORT                      // 指定删除 I2C_NUM_0 驱动
        );                                        // 删除旧驱动调用结束

        g_i2c_initialized = false;                // 把初始化标志恢复为 false
    }                                             // 旧驱动处理结束

    g_mpu6050_address =                           // 保存本次要使用的设备地址
        device_address;                           // 地址通常为 0x68 或 0x69

    i2c_config_t config;                          // 创建 ESP32 I2C 配置结构体
    memset(&config, 0, sizeof(config));            // 先把整个配置结构清零，避免残留随机值

    config.mode =                                 // 设置 I2C 工作模式
        I2C_MODE_MASTER;                          // ESP32 作为主机主动访问 MPU6050

    config.sda_io_num =                           // 设置 SDA GPIO 字段
        (gpio_num_t)sda_pin;                      // 把整数引脚转换成 ESP32 GPIO 类型

    config.scl_io_num =                           // 设置 SCL GPIO 字段
        (gpio_num_t)scl_pin;                      // 把整数引脚转换成 ESP32 GPIO 类型

    config.sda_pullup_en =                        // 设置 SDA 内部上拉选项
        GPIO_PULLUP_ENABLE;                       // 开启 SDA 内部上拉

    config.scl_pullup_en =                        // 设置 SCL 内部上拉选项
        GPIO_PULLUP_ENABLE;                       // 开启 SCL 内部上拉

    config.master.clk_speed =                     // 设置主机 I2C 时钟速度
        clock_hz;                                 // 使用调用者传入的频率，例如 100000

    config.clk_flags = 0;                         // 使用 ESP-IDF 4.x 的默认 I2C 时钟来源

    esp_err_t result =                            // 创建变量保存底层配置结果
        i2c_param_config(                         // 把配置参数写入 ESP32 I2C 控制器
            MPU6050_I2C_PORT,                     // 指定 I2C_NUM_0
            &config                               // 传入配置结构体地址
        );                                        // I2C 参数配置调用结束

    if (result != ESP_OK) {                       // 如果参数配置没有成功
        return false;                             // 返回 false 表示初始化失败
    }                                             // 参数配置结果判断结束

    result = i2c_driver_install(                  // 安装并启动 ESP32 I2C 驱动
        MPU6050_I2C_PORT,                         // 指定 I2C_NUM_0
        I2C_MODE_MASTER,                          // 以主机模式安装
        0,                                        // 主机模式不需要接收缓冲区
        0,                                        // 主机模式不需要发送缓冲区
        0                                         // 不使用额外中断分配标志
    );                                            // I2C 驱动安装调用结束

    if (result != ESP_OK) {                       // 如果驱动安装失败
        return false;                             // 返回 false 表示初始化失败
    }                                             // 驱动安装结果判断结束

    g_i2c_initialized = true;                     // 标记 I2C 驱动已经可用

    vTaskDelay(pdMS_TO_TICKS(200));               // 等待 200 毫秒，让总线和传感器稳定

    if (!mpu6050_probe()) {                       // 检查当前地址是否有设备回应
        mpu6050_deinit();                         // 检测失败时释放已经安装的 I2C 驱动
        return false;                             // 返回 false 表示没有发现设备
    }                                             // 地址检测结束

    uint8_t identity = 0;                         // 创建变量保存 WHO_AM_I 身份值

    if (!mpu6050_read_who_am_i(                  // 尝试读取身份寄存器
            &identity                            // 把结果保存到 identity
        )) {                                      // 身份寄存器读取失败时进入错误处理
        mpu6050_deinit();                         // 删除 I2C 驱动并释放资源
        return false;                             // 返回 false 表示初始化失败
    }                                             // 身份寄存器读取判断结束

    if (who_am_i != NULL) {                       // 调用者提供了身份值保存地址时
        *who_am_i = identity;                     // 把读取结果写回调用者变量
    }                                             // 可选身份值写回结束

    if (!mpu6050_wake()) {                        // 向电源管理寄存器写入 0x00 唤醒设备
        mpu6050_deinit();                         // 唤醒失败时释放 I2C 驱动
        return false;                             // 返回 false 表示初始化失败
    }                                             // 唤醒结果判断结束

    vTaskDelay(pdMS_TO_TICKS(100));               // 等待 100 毫秒，让传感器进入正常工作状态

    return true;                                  // 地址、身份和唤醒都成功，返回 true
}                                                 // mpu6050_init() 结束

void mpu6050_deinit(void) {                       // 删除 I2C 驱动并恢复未初始化状态
    if (!g_i2c_initialized) {                     // 如果 I2C 本来就没有初始化
        return;                                   // 直接结束函数，不重复删除驱动
    }                                             // 初始化状态检查结束

    i2c_driver_delete(                            // 调用 ESP32 底层函数删除 I2C 驱动
        MPU6050_I2C_PORT                          // 指定删除 I2C_NUM_0
    );                                            // 驱动删除调用结束

    g_i2c_initialized = false;                    // 更新标志，表示总线已经释放
}                                                 // mpu6050_deinit() 结束
