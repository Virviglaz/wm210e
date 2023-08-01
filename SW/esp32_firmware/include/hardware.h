#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include <stdint.h>
#include "driver/gpio.h"

#define BUTTON_RETURN			0
#define BUTTON_NEXT			1
#define BUTTON_ENTER			2

/* lcd */
#define LCD_I2C_SDA			12
#define LCD_I2C_SCL			14

/* buttons */
#define BTN1				GPIO_NUM_16
#define BTN2				GPIO_NUM_17

/* front panel encoder */
#define ENC_A				GPIO_NUM_5
#define ENC_B				GPIO_NUM_18
#define ENC_BTN				GPIO_NUM_19

/* external encoder */
#define EXT_ENC_A			GPIO_NUM_23
#define EXT_ENC_B			GPIO_NUM_22
#define EXT_ENC_Z			GPIO_NUM_21

/* stepper */
#define STP_CLK_PIN			GPIO_NUM_26
#define STP_CLK_POL			1
#define STP_DIR_PIN			GPIO_NUM_33
#define STP_ENA_PIN			GPIO_NUM_25
#define STP_DIR_CCW			1
#define STP_ENA_POL			0
#define STP_ACC				1000
#define STP_SPEED			1000
#define STP_BACKLASH_MM			5
#define STP_CLK_INVERT			false
#define STP_ENA_INVERT			true
#define STP_DELAY_MS			1000

/* Fan */
#define FAN_ENA_PIN			GPIO_NUM_15

/* Timings */
#define MOTOR_CLK_PULSE_US		50
#define SPEED_RATIO_CALC(x)		(200 * (x) / 11500)
#define MOTOR_PULSES_TO_SUPPORT_MM	(float)(2 * 800 * 4 * 60 / 27)
#define ENC_PULSES_TO_SUPPORT_MM	(float)(20) /* 1r spindle == enc / 20 */
#define MOTOR_BACKLASH_MM		5
#define STP_ACC_TIME_MS			500

int hardware_init();
void fan_start();
void fan_stop();
void motor_enable(bool state);

/*
 * 27T o 800 PPM encoder (generates 800 x 4 = 3200 interrupts / revolution)
 *     |					o NEMA23 Stepper motor (400 s/r)
 * 60T O-o 20T BLDC motor			|
 *       |				    12T	o-O 32T  helical spiral gear
 *       O 40T spindle				  |
 *						  o M16x1.5 drive thread
 *
 * 1mm (support move) =
 *	1/1,5(thread mm) x 32/12 (gear ratio) x 400 (p/r stepper) =
 *
 * 1 spindle rot = 2 (bldc) x 60/27 (enc gear) x 800 (enc ppm) x 4 interrupts
 *
 * 4 x 96000 x 2 x 18 / (27 x 25600) = 20
 * =======> [enc pulses] / [20] = 1mm support movement
 */

/**
 * @brief Build string
 * 
 * scp.exe wm210e_esp32.bin pavel@192.168.0.108:/tmp
 * ssh.exe pavel@192.168.0.108 "ota /tmp/wm210e_esp32.bin 5005 0xDEADBEEF new"
*/

#endif /* __HARDWARE_H__ */
