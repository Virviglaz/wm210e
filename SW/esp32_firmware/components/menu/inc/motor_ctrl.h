#ifndef __MOTOR_CTRL_H__
#define __MOTOR_CTRL_H__

#include <stdint.h>
#include "lcd.h"
#include <esp_buttons.h>

enum dir { CW, CCW };

void thread_cut(lcd& lcd,		/* LCD driver */
		Buttons& btns,		/* Buttons driver */
		const char *name,	/* Title */
		float step_mm,		/* Support steps per rotation */
		enum dir dir,		/* Support movement direction */
		int32_t limit10,	/* Support movement limit x10 mm */
		bool sup_return);	/* Automatic support return */

#endif /* __MOTOR_CTRL_H__ */
