#ifndef __MOTOR_CTRL_H__
#define __MOTOR_CTRL_H__

#include <stdint.h>
#include "lcd.h"

enum dir { CW, CCW };

void thread_cut(lcd& lcd, const char *name, uint32_t step, bool dir);

#endif /* __MOTOR_CTRL_H__ */
