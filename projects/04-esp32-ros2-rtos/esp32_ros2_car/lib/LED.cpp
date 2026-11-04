#include "LED.hpp"
#include "driver/gpio.h"
#include "esp_log.h"


void LED_init(int pin)
{
    gpio_config_t LED_init={};
    LED_init.pin_bit_mask =1ULL<<pin;//配置的引脚是GPIO17
    LED_init.mode = GPIO_MODE_OUTPUT;//配置GPIO17为输出模式
    LED_init.pull_up_en = GPIO_PULLUP_ENABLE;//配置GPIO17上拉,使得GPIO17默认输出高电平
    LED_init.pull_down_en = GPIO_PULLDOWN_DISABLE;//配置GPIO17不上拉,使得GPIO17默认输出高电平
    LED_init.intr_type = GPIO_INTR_DISABLE;//配置GPIO17不产生中断
    gpio_config(&LED_init);//初始化GPIO17
    esp_log_level_set("LED_init", ESP_LOG_INFO);//设置日志级别为INFO
    ESP_LOGI("LED_init", "LED_init success");//打印日志
}
