#ifndef TEST_STM32F407XX_H_
#define TEST_STM32F407XX_H_

#include <stdint.h>

static inline uint32_t __get_PRIMASK(void) { return 0U; }
static inline void __disable_irq(void) {}
static inline void __set_PRIMASK(uint32_t value) { (void)value; }

#endif /* TEST_STM32F407XX_H_ */
