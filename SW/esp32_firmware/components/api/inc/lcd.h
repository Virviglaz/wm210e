#ifndef __LCD_H__
#define __LCD_H__

#include <HD44780.h>
#include <stdint.h>
#include "platform.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

enum row_e {
	FIRST_ROW,
	SECOND_ROW
};

enum align {
	LEFT,
	CENTER,
	RIGHT,
};

class lcd {
public:
	lcd();
	~lcd();
	void print(const char *format, ...);
	void print(enum row_e row, enum align a, const char *format, ...);
	void print(enum row_e row, uint8_t col, const char *format, ...);
	void clear();
	void clear(enum row_e row);
private:
	static void handler(void *arg);

	QueueHandle_t queue;
	TaskHandle_t handle = NULL;
};

extern lcd *LCD;

#endif /* __LCD_H__ */
