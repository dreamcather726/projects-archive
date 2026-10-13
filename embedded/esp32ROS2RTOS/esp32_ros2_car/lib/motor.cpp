#include "motor.hpp"
#include <cstdlib>


void motor_init(void)
{
    gpio_config_t dir_cfg = {};
    dir_cfg.pin_bit_mask =
        (1ULL << MOTOR1_DIR) |
        (1ULL << MOTOR2_DIR);// 设置电机方向引脚DIR1,DIR2为输出模式
    dir_cfg.mode = GPIO_MODE_OUTPUT;// 设置电机方向引脚为输出模式
    dir_cfg.pull_up_en = GPIO_PULLUP_DISABLE;// 禁用上拉电阻
    dir_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;// 禁用下拉电阻
    dir_cfg.intr_type = GPIO_INTR_DISABLE;// 禁用中断
    ESP_ERROR_CHECK(gpio_config(&dir_cfg));// 配置电机方向引脚

    ESP_ERROR_CHECK(gpio_set_level(MOTOR1_DIR, 0));
    ESP_ERROR_CHECK(gpio_set_level(MOTOR2_DIR, 0));

    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode = LEDC_LOW_SPEED_MODE;//定时器模式
    timer_cfg.timer_num = LEDC_TIMER_0;//使用定时器0
    timer_cfg.duty_resolution = LEDC_TIMER_8_BIT;//8位
    timer_cfg.freq_hz = 10000;//频率为10KHz
    timer_cfg.clk_cfg = LEDC_AUTO_CLK;//自动时钟
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));//配置定时器

    ledc_channel_config_t motor1_channel = {};//配置电机1的PWM通道
    motor1_channel.gpio_num = MOTOR1_PWM;// 设置电机1的PWM引脚
    motor1_channel.speed_mode = LEDC_LOW_SPEED_MODE;// 低速模式
    motor1_channel.channel = LEDC_CHANNEL_0;// 设置电机1的PWM通道为通道0
    motor1_channel.timer_sel = LEDC_TIMER_0;// 设置电机1的PWM通道使用定时器0
    motor1_channel.duty = 0;// 设置电机1的PWM通道的占空比为0
    motor1_channel.hpoint = 0;// 设置hpoint为0
    ESP_ERROR_CHECK(ledc_channel_config(&motor1_channel));

    ledc_channel_config_t motor2_channel = {};
    motor2_channel.gpio_num = MOTOR2_PWM;// 设置电机2的PWM引脚
    motor2_channel.speed_mode = LEDC_LOW_SPEED_MODE;// 低速模式
    motor2_channel.channel = LEDC_CHANNEL_1;// 设置电机2的PWM通道为通道1
    motor2_channel.timer_sel = LEDC_TIMER_0;// 设置电机2的PWM通道使用定时器0
    motor2_channel.duty = 0;// 设置电机2的PWM通道的占空比为0
    motor2_channel.hpoint = 0;// 设置hpoint为0
    ESP_ERROR_CHECK(ledc_channel_config(&motor2_channel));// 配置电机2的PWM通道
}

void motor_set_speed(int motor, int speed)
{   if (speed>100)
    {
        speed=100;
    }
    if (speed<-100)
    {
        speed=-100;
    }

    uint32_t duty = (uint32_t)((abs(speed) * 255) / 100); // 将速度值映射到占空比范围 [0, 255]

    
    if (motor == 1)
    {
        if (speed >= 0)
        {
            ESP_ERROR_CHECK(gpio_set_level(MOTOR1_DIR, 0));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));// 设置电机1的PWM占空比
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        }
        else
        {
            ESP_ERROR_CHECK(gpio_set_level(MOTOR1_DIR, 1));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));// 设置电机1的PWM占空比
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        }
   
    }
    if (motor==2)
    {
       if (speed >= 0)
        {
            ESP_ERROR_CHECK(gpio_set_level(MOTOR2_DIR, 0));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty));// 设置电机2的PWM占空比
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
        }
        else
        {
            ESP_ERROR_CHECK(gpio_set_level(MOTOR2_DIR, 1));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
        } /* code */
    }
    
}