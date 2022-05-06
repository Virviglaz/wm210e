#ifndef __MOTOR_CTRL_H__
#define __MOTOR_CTRL_H__

#include <stdint.h>

void thread_cut(const char *name, uint32_t step, bool dir);

#endif /* __MOTOR_CTRL_H__ */
