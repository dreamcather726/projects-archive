#ifndef TEST_FREERTOS_H_
#define TEST_FREERTOS_H_

#include <stdint.h>

typedef uint32_t TickType_t;

#define pdMS_TO_TICKS(milliseconds) ((TickType_t)(milliseconds))

#endif /* TEST_FREERTOS_H_ */
