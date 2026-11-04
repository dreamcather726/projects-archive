# ESP32-S3 ESP-IDF API 速查表

> 适用于本项目当前使用的 ESP-IDF 6.0.2。示例使用 C++，所有返回 `esp_err_t` 的初始化和配置函数建议用 `ESP_ERROR_CHECK()` 检查。

## 1. 头文件和 CMake 依赖

| 功能 | 头文件 | `main/CMakeLists.txt` 依赖 |
| --- | --- | --- |
| GPIO | `#include "driver/gpio.h"` | `esp_driver_gpio` |
| PWM（LEDC） | `#include "driver/ledc.h"` | `esp_driver_ledc` |
| UART | `#include "driver/uart.h"` | `esp_driver_uart` |
| I2C Master | `#include "driver/i2c_master.h"` | `esp_driver_i2c` |
| 日志 | `#include "esp_log.h"` | `log` |
| 错误检查 | `#include "esp_err.h"` | `esp_common` |
| FreeRTOS 任务/延时 | `#include "freertos/FreeRTOS.h"`、`#include "freertos/task.h"` | `freertos` |
| 微秒计时 | `#include "esp_timer.h"` | `esp_timer` |

需要使用这些驱动时，可把构建配置写成：

```cmake
idf_component_register(
    SRCS
        "main.cpp"
        "../lib/LED.cpp"
        "../lib/motor.cpp"
        "../lib/uart.cpp"
    INCLUDE_DIRS
        "."
        "../include"
    PRIV_REQUIRES
        esp_driver_gpio
        esp_driver_ledc
        esp_driver_i2c
        esp_driver_uart
        esp_timer
        log
        esp_common
        freertos
)
```

## 2. ESP32-S3 引脚注意事项

ESP32-S3 有 GPIO0～21 和 GPIO26～48，但并不是每个引脚都适合随意使用。

| GPIO | 默认用途或限制 | 建议 |
| --- | --- | --- |
| GPIO0、3、45、46 | 启动配置 Strapping 引脚 | 外接电路不能在复位时强行拉成错误电平 |
| GPIO19、20 | 默认用于 USB-JTAG | 改成普通 GPIO 后会影响 USB-JTAG |
| GPIO26～32 | 通常连接 SPI Flash/PSRAM | 不要作为普通 GPIO 使用 |
| GPIO33～37 | 八线 Flash/PSRAM 板卡可能占用 | 先确认模组型号和原理图 |
| GPIO17、18 | 本项目分别分配给 UART1 TX、RX | 使用串口协议时不要再配置成 LED、PWM 或普通 GPIO |
| 其他 GPIO | 通常可通过 GPIO Matrix 分配给外设 | 仍需以开发板原理图为准 |

## 3. GPIO

### 3.1 `gpio_config_t` 参数

| 字段 | 类型 | 常用值 | 含义 |
| --- | --- | --- | --- |
| `pin_bit_mask` | `uint64_t` | `1ULL << GPIO_NUM_4` | 要配置的引脚位掩码，可用 `\|` 一次选择多个引脚 |
| `mode` | `gpio_mode_t` | `GPIO_MODE_INPUT`、`GPIO_MODE_OUTPUT`、`GPIO_MODE_INPUT_OUTPUT`、`GPIO_MODE_OUTPUT_OD` | 输入、输出、双向或开漏模式 |
| `pull_up_en` | `gpio_pullup_t` | `GPIO_PULLUP_ENABLE` / `DISABLE` | 内部上拉 |
| `pull_down_en` | `gpio_pulldown_t` | `GPIO_PULLDOWN_ENABLE` / `DISABLE` | 内部下拉 |
| `intr_type` | `gpio_int_type_t` | `GPIO_INTR_DISABLE`、`POSEDGE`、`NEGEDGE`、`ANYEDGE`、`LOW_LEVEL`、`HIGH_LEVEL` | 中断触发方式 |

### 3.2 常用 GPIO 函数

| 函数 | 用途 | 参数定义 | 返回值/注意事项 |
| --- | --- | --- | --- |
| `gpio_config(&cfg)` | 一次配置模式、上下拉和中断 | `cfg`：`gpio_config_t` 指针 | `ESP_OK` 成功；会覆盖引脚原有配置 |
| `gpio_set_direction(pin, mode)` | 单独设置方向 | `pin`：GPIO 编号；`mode`：输入/输出模式 | 返回 `esp_err_t` |
| `gpio_set_level(pin, level)` | 设置输出电平 | `level`：`0` 低电平，非 `0` 高电平 | 引脚必须具有输出能力 |
| `gpio_get_level(pin)` | 读取输入电平 | `pin`：GPIO 编号 | 返回 `0` 或 `1`；未启用输入时总是 `0` |
| `gpio_set_pull_mode(pin, pull)` | 设置上下拉 | `GPIO_PULLUP_ONLY`、`GPIO_PULLDOWN_ONLY`、`GPIO_PULLUP_PULLDOWN`、`GPIO_FLOATING` | 返回 `esp_err_t` |
| `gpio_reset_pin(pin)` | 恢复引脚默认状态 | `pin`：GPIO 编号 | 返回 `esp_err_t` |
| `gpio_install_isr_service(flags)` | 安装 GPIO 中断服务 | `flags`：常用 `0`，也可传 `ESP_INTR_FLAG_*` | 整个程序通常只调用一次 |
| `gpio_isr_handler_add(pin, fn, arg)` | 给指定引脚注册中断回调 | `fn`：`void (*)(void *)`；`arg`：传给回调的参数 | 必须先安装中断服务 |
| `gpio_isr_handler_remove(pin)` | 删除引脚中断回调 | `pin`：GPIO 编号 | 返回 `esp_err_t` |

### 3.3 输出和输入示例

```cpp
#include "driver/gpio.h"
#include "esp_err.h"

constexpr gpio_num_t LED_PIN = GPIO_NUM_4;
constexpr gpio_num_t LED_PIN = GPIO_NUM_5;

void gpio_init(void)
{
    gpio_config_t output_cfg = {};
    output_cfg.pin_bit_mask = 1ULL << LED_PIN;
    output_cfg.mode = GPIO_MODE_OUTPUT;
    output_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    output_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    output_cfg.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&output_cfg));

    gpio_config_t input_cfg = {};
    input_cfg.pin_bit_mask = 1ULL << LED_PIN;
    input_cfg.mode = GPIO_MODE_INPUT;
    input_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    input_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    input_cfg.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&input_cfg));

    ESP_ERROR_CHECK(gpio_set_level(LED_PIN, 1));
    int LED_level = gpio_get_level(LED_PIN);
}
```

中断回调运行在 ISR 环境中，不要在里面延时、做复杂计算或直接调用普通日志函数。通常只发送队列、信号量或任务通知，再由任务处理业务。

## 4. PWM（LEDC）

ESP-IDF 通常使用 LEDC 外设产生普通 PWM。ESP32-S3 使用 `LEDC_LOW_SPEED_MODE`。

### 4.1 定时器参数 `ledc_timer_config_t`

| 字段 | 常用值 | 含义 |
| --- | --- | --- |
| `speed_mode` | `LEDC_LOW_SPEED_MODE` | ESP32-S3 的 LEDC 速度模式 |
| `duty_resolution` | `LEDC_TIMER_8_BIT`、`10_BIT`、`12_BIT` | 占空比分辨率；位数越高，可用的频率上限通常越低 |
| `timer_num` | `LEDC_TIMER_0`～`LEDC_TIMER_3` | 通道使用的定时器 |
| `freq_hz` | 例如 `1000`、`5000` | PWM 频率，单位 Hz |
| `clk_cfg` | `LEDC_AUTO_CLK` | 自动选择合适时钟源 |

### 4.2 通道参数 `ledc_channel_config_t`

| 字段 | 常用值 | 含义 |
| --- | --- | --- |
| `gpio_num` | 例如 `GPIO_NUM_4` | PWM 输出引脚 |
| `speed_mode` | 与定时器一致 | 速度模式 |
| `channel` | `LEDC_CHANNEL_0`～`LEDC_CHANNEL_7` | PWM 通道 |
| `timer_sel` | 例如 `LEDC_TIMER_0` | 绑定的 LEDC 定时器 |
| `duty` | `0`～`(1 << 分辨率位数) - 1` | 初始占空比计数值 |
| `hpoint` | 通常为 `0` | PWM 高电平开始点，普通用法保持 `0` |
| `flags.output_invert` | `0` 或 `1` | 是否反相输出 |

例如 10 位分辨率的安全计数范围是 `0～1023`，约 50% 占空比可使用 `512`。

### 4.3 常用 PWM 函数

| 函数 | 用途 | 参数定义 | 注意事项 |
| --- | --- | --- | --- |
| `ledc_timer_config(&cfg)` | 配置频率和分辨率 | `cfg`：定时器配置 | 应先于通道配置调用 |
| `ledc_channel_config(&cfg)` | 配置通道和输出 GPIO | `cfg`：通道配置 | 通道与定时器的模式必须一致 |
| `ledc_set_duty(mode, channel, duty)` | 写入新占空比 | `duty`：占空比计数值 | 调用后还要 `ledc_update_duty()` |
| `ledc_update_duty(mode, channel)` | 使新占空比生效 | 模式、通道 | 与 `ledc_set_duty()` 配对 |
| `ledc_set_duty_and_update(mode, channel, duty, hpoint)` | 设置并立即更新占空比 | `hpoint` 普通场景用 `0` | 控制同一通道时是更方便的接口 |
| `ledc_set_freq(mode, timer, hz)` | 动态修改 PWM 频率 | `hz`：新频率，单位 Hz | 返回 `esp_err_t` |
| `ledc_stop(mode, channel, idle_level)` | 停止 PWM | `idle_level`：停止后的输出电平 | 返回 `esp_err_t` |

### 4.4 PWM 最小示例

```cpp
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

void pwm_init(void)
{
    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_cfg.duty_resolution = LEDC_TIMER_10_BIT;
    timer_cfg.timer_num = LEDC_TIMER_0;
    timer_cfg.freq_hz = 5000;
    timer_cfg.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t channel_cfg = {};
    channel_cfg.gpio_num = GPIO_NUM_4;
    channel_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_cfg.channel = LEDC_CHANNEL_0;
    channel_cfg.timer_sel = LEDC_TIMER_0;
    channel_cfg.duty = 0;
    channel_cfg.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));

    // 10 位分辨率下，512 约等于 50% 占空比。
    ESP_ERROR_CHECK(ledc_set_duty_and_update(
        LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512, 0));
}
```

PWM GPIO 不能直接驱动电机。小车电机必须经过 H 桥或电机驱动芯片，PWM 通常接驱动芯片的 `EN/PWM` 输入。

## 5. I2C Master（新版驱动）

### 5.1 总线参数 `i2c_master_bus_config_t`

| 字段 | 常用值 | 含义 |
| --- | --- | --- |
| `i2c_port` | `-1` | 自动选择可用 I2C 控制器 |
| `sda_io_num` | 例如 `GPIO_NUM_8` | SDA 数据引脚 |
| `scl_io_num` | 例如 `GPIO_NUM_9` | SCL 时钟引脚 |
| `clk_source` | `I2C_CLK_SRC_DEFAULT` | I2C 时钟源 |
| `glitch_ignore_cnt` | `7` | 过滤短毛刺的常用值 |
| `intr_priority` | `0` | 由驱动自动选择中断优先级 |
| `trans_queue_depth` | 同步传输可用 `0` | 异步传输队列深度 |
| `flags.enable_internal_pullup` | `true` / `false` | 是否启用内部弱上拉；正式硬件仍建议外接上拉 |

### 5.2 设备参数 `i2c_device_config_t`

| 字段 | 常用值 | 含义 |
| --- | --- | --- |
| `dev_addr_length` | `I2C_ADDR_BIT_LEN_7` | 常见器件使用 7 位地址 |
| `device_address` | 例如 `0x68` | 原始 7 位地址，不包含读写位，不要左移 |
| `scl_speed_hz` | `100000` 或 `400000` | 100 kHz 标准模式或 400 kHz 快速模式 |
| `scl_wait_us` | `0` | 使用默认时钟拉伸等待值 |
| `flags.disable_ack_check` | `false` | 保持 ACK 检查开启 |

### 5.3 常用 I2C 函数

| 函数 | 用途 | 参数定义 | 常见返回值 |
| --- | --- | --- | --- |
| `i2c_new_master_bus(&cfg, &bus)` | 创建主机总线 | 总线配置、总线句柄输出地址 | `ESP_OK`、`ESP_ERR_NO_MEM`、`ESP_ERR_NOT_FOUND` |
| `i2c_master_bus_add_device(bus, &cfg, &dev)` | 在总线上添加设备 | 总线句柄、设备配置、设备句柄输出地址 | `ESP_OK` 或参数/内存错误 |
| `i2c_master_probe(bus, address, timeout_ms)` | 检查某个地址是否应答 | 7 位地址；超时单位 ms | `ESP_OK`、`ESP_ERR_NOT_FOUND`、`ESP_ERR_TIMEOUT` |
| `i2c_master_transmit(dev, data, size, timeout_ms)` | 写数据 | 缓冲区、字节数、超时 | `ESP_OK`、NACK、超时或参数错误 |
| `i2c_master_receive(dev, data, size, timeout_ms)` | 读数据 | 接收缓冲区、字节数、超时 | `ESP_OK` 或超时/参数错误 |
| `i2c_master_transmit_receive(dev, tx, tx_size, rx, rx_size, timeout_ms)` | 写寄存器地址后重复起始并读取 | 写缓冲区及长度、读缓冲区及长度、超时 | 读取传感器寄存器最常用 |
| `i2c_master_bus_rm_device(dev)` | 删除设备 | 设备句柄 | 不再使用设备时调用 |
| `i2c_del_master_bus(bus)` | 删除总线 | 总线句柄 | 应先删除该总线上的设备 |

`timeout_ms` 的单位是毫秒，`-1` 表示永久等待；实际项目建议使用有限超时，例如 `100` ms，避免总线异常时永久卡住。

### 5.4 I2C 初始化和寄存器读取示例

```cpp
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

static i2c_master_bus_handle_t i2c_bus = nullptr;
static i2c_master_dev_handle_t sensor = nullptr;

void i2c_init(void)
{
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = -1;
    bus_cfg.sda_io_num = GPIO_NUM_8;
    bus_cfg.scl_io_num = GPIO_NUM_9;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &i2c_bus));

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = 0x68;
    dev_cfg.scl_speed_hz = 400000;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_cfg, &sensor));
}

esp_err_t read_register(uint8_t reg, uint8_t *data, size_t size)
{
    return i2c_master_transmit_receive(
        sensor,
        &reg,
        sizeof(reg),
        data,
        size,
        100);
}
```

SDA 和 SCL 是开漏信号，必须上拉。内部上拉较弱，电机环境噪声较大时尤其建议使用合适的外部上拉电阻。

## 6. 常用系统函数

| 函数/宏 | 使用方式 | 参数定义 | 说明 |
| --- | --- | --- | --- |
| `ESP_LOGE(TAG, fmt, ...)` | 错误日志 | 标签、格式字符串、可变参数 | 严重错误 |
| `ESP_LOGW(TAG, fmt, ...)` | 警告日志 | 同上 | 可恢复异常 |
| `ESP_LOGI(TAG, fmt, ...)` | 信息日志 | 同上 | 正常运行信息 |
| `ESP_LOGD(TAG, fmt, ...)` | 调试日志 | 同上 | 需要启用对应日志等级 |
| `ESP_ERROR_CHECK(expr)` | 检查 `esp_err_t` | `expr`：返回 `esp_err_t` 的表达式 | 非 `ESP_OK` 时输出错误并中止程序，适合不可恢复的初始化错误 |
| `esp_err_to_name(err)` | 错误码转文字 | `err`：`esp_err_t` | 便于自定义错误处理和日志输出 |
| `vTaskDelay(pdMS_TO_TICKS(ms))` | 当前任务延时 | `ms`：毫秒数 | 让出 CPU，不是忙等待 |
| `xTaskCreate(fn, name, stack, arg, priority, &handle)` | 创建 FreeRTOS 任务 | 函数、名称、栈字节数、参数、优先级、句柄输出 | ESP-IDF 中栈大小按字节计；成功返回 `pdPASS` |
| `xTaskCreatePinnedToCore(..., core)` | 创建并绑定 CPU 核心 | 前六项同上；`core`：`0`、`1` 或 `tskNO_AFFINITY` | 没有明确需求时优先使用 `xTaskCreate()` |
| `vTaskDelete(handle)` | 删除任务 | `nullptr` 表示删除当前任务 | 任务函数不应直接 `return` |
| `xTaskGetTickCount()` | 获取系统 Tick | 无 | 返回 `TickType_t` |
| `esp_timer_get_time()` | 获取启动后的时间 | 无 | 返回 `int64_t`，单位微秒 |

### FreeRTOS 任务示例

```cpp
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WORKER";

static void worker_task(void *arg)
{
    const int id = *static_cast<int *>(arg);

    while (true) {
        ESP_LOGI(TAG, "worker id=%d", id);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // 如果循环能够退出，应在退出前调用 vTaskDelete(nullptr)。
}

void start_worker(void)
{
    static int worker_id = 1; // 参数必须在任务运行期间保持有效。
    BaseType_t result = xTaskCreate(
        worker_task, "worker", 4096, &worker_id, 5, nullptr);

    if (result != pdPASS) {
        ESP_LOGE(TAG, "create worker task failed");
    }
}
```

## 7. 返回值处理模板

初始化失败后无法继续运行时：

```cpp
ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT));
```

运行期间可能恢复的错误，应自行判断：

```cpp
esp_err_t err = i2c_master_probe(i2c_bus, 0x68, 100);
if (err == ESP_OK) {
    ESP_LOGI("I2C", "device found");
} else {
    ESP_LOGW("I2C", "probe failed: %s", esp_err_to_name(err));
}
```

## 8. 官方参考

- [ESP32-S3 GPIO API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html)
- [ESP32-S3 LEDC PWM API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/ledc.html)
- [ESP32-S3 I2C API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/i2c.html)
- [ESP-IDF FreeRTOS API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/freertos_idf.html)
