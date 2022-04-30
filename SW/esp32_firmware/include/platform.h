#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)			(sizeof(a) / sizeof(a[0]))
#endif /* ARRAY_SIZE */

#ifndef BIT
#define BIT(x)				(1 << x)
#endif

/* FreeRTOS */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

/* STD */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* ESP32 drivers */
#include "driver/gpio.h"
#include "driver/gptimer.h"

#include "sdkconfig.h"
#include "esp_system.h"

/* Helpers */
#define delay(x)			vTaskDelay(pdMS_TO_TICKS(x))
#define delay_ms(x)			delay(x)
#define delay_s(x)			delay_ms(x * 1000)

#define task_create(c, n, s, p, pr, h)	xTaskCreate(c, n, s, p, pr, h)
#define task_delete(t)			vTaskDelete(t)

#define queue_create(l, s) 		xQueueCreate(l, s)
#define queue_send(q, i, t)		xQueueSend(q, i, pdMS_TO_TICKS(t))
#define queue_receive(q, b, t)		xQueueReceive(q, b, pdMS_TO_TICKS(t))

#define semaphore_create_binary()	xSemaphoreCreateBinary()
#define semaphore_create_mutex()	xSemaphoreCreateMutex()
#define semaphore_take(s,t) 		xSemaphoreTake(s, pdMS_TO_TICKS(t))
#define semaphore_give(s)		xSemaphoreGive(s)
#define semaphore_delete(s)		vSemaphoreDelete(s)

/* Logs */
#include "esp_log.h"
#define DEBUG(format, ...)	ESP_LOGD(__func__, format, ##__VA_ARGS__)
#define INFO(format, ...)	ESP_LOGI(__func__, format, ##__VA_ARGS__)
#define WARN(format, ...)	ESP_LOGW(__func__, format, ##__VA_ARGS__)
#define ERROR(format, ...)	ESP_LOGE(__func__, format, ##__VA_ARGS__)

#define errlog			ERROR
#define ERR_NO_MEMORY()		ERROR("No memory!")
#define ERR_RTN(x)		res = (x); if (res) return res;

static inline void delay_us(uint32_t micros)
{
	uint32_t v = esp_timer_get_time() + micros;

	while (esp_timer_get_time() < v) { }
}

#endif /* __PLATFORM_H__ */
