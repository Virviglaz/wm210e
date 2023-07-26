#ifndef __MOTOR_CTRL_H__
#define __MOTOR_CTRL_H__

#include <stdint.h>
#include "lcd.h"
#include <esp_buttons.h>

enum dir { CW, CCW };

void thread_cut(lcd& lcd,
		Buttons& btns,
		const char *name,
		float step_mm,
		bool dir,
		int limit10);

#endif /* __MOTOR_CTRL_H__ */
