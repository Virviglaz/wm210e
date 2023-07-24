#ifndef __LCD_H__
#define __LCD_H__

#include <free_rtos_h.h>
#include <esp32_i2c.h>
#include <stdint.h>
#include <string>

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
	void print(std::string format, ...) {
		print(format.c_str());
	}

	void print(enum row_e row, enum align a, const char *format, ...);
	void print(enum row_e row, enum align a, std::string format, ...) {
		print(row, a, format.c_str());
	}

	void print(enum row_e row, uint8_t col, const char *format, ...);
	void print(enum row_e row, uint8_t col, std::string format, ...) {
		print(row, col, format.c_str());
	}

	void clear();
	void clear(enum row_e row);
private:
	static void handler(void *arg);

	QueueHandle_t queue;
	TaskHandle_t handle = NULL;
};

#endif /* __LCD_H__ */
