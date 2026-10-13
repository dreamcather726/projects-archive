/**
 * @file stm32f4xx_conf.h
 * @brief STM32F4 标准外设库模块配置。
 *
 * 当前工程只启用串口功能实际依赖的 RCC、GPIO、DMA 和 USART 驱动。
 * 后续增加新外设时，在这里包含对应的标准库头文件，并将对应 .c
 * 文件加入 Keil 工程。
 */

#ifndef STM32F4XX_CONF_H_
#define STM32F4XX_CONF_H_

#include "stm32f4xx_dma.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"

/*
 * 默认关闭标准库参数断言，避免在正式固件中增加额外开销。
 * 调试参数问题时可定义 USE_FULL_ASSERT，并实现 assert_failed()。
 */
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line);
#define assert_param(expression)                                                \
    ((expression) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
#else
#define assert_param(expression) ((void)0U)
#endif

#endif /* STM32F4XX_CONF_H_ */
