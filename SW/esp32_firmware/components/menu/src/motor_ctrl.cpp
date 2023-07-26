#include "motor_ctrl.h"
#include "hardware.h"
#include "log.h"
#include <free_rtos_h.h>
#include <errno.h>
#include <string.h>
#include <esp32_timer.h>

/* ESP32 drivers */
#include "driver/gpio.h"
#include "esp_timer.h"
#include "hal/gpio_ll.h"

#include <atomic>

/* Hardware timer clock divider */
#define TIMER_DIVIDER		(16)
/* convert counter value to seconds */
#define TIMER_SCALE		(TIMER_BASE_CLK / (TIMER_DIVIDER * 1000000UL))
#define GPIO_SET(pin, state)	gpio_ll_set_level(&GPIO, pin, state)
#define GPIO_GET(pin)		gpio_ll_get_level(&GPIO, pin)

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
 *
 * enc pulses / 20 = 1mm support movement
 */

class stepper_ctrl
{
public:
	stepper_ctrl(uint32_t ratio, bool dir_invert) :
		ratio(ratio), dir_invert(dir_invert)
	{
		ESP_ERROR_CHECK(gpio_reset_pin(EXT_ENC_A));
		ESP_ERROR_CHECK(gpio_reset_pin(EXT_ENC_B));
		ESP_ERROR_CHECK(gpio_reset_pin(STP_CLK_PIN));
		ESP_ERROR_CHECK(gpio_reset_pin(STP_DIR_PIN));
		ESP_ERROR_CHECK(gpio_reset_pin(STP_ENA_PIN));
	
		ESP_ERROR_CHECK(gpio_set_direction(EXT_ENC_A, GPIO_MODE_INPUT));
		ESP_ERROR_CHECK(gpio_set_direction(EXT_ENC_B, GPIO_MODE_INPUT));

		ESP_ERROR_CHECK(gpio_set_direction(STP_CLK_PIN, GPIO_MODE_OUTPUT));
		ESP_ERROR_CHECK(gpio_set_direction(STP_DIR_PIN, GPIO_MODE_OUTPUT));
		ESP_ERROR_CHECK(gpio_set_direction(STP_ENA_PIN, GPIO_MODE_OUTPUT));

		ESP_ERROR_CHECK(gpio_set_intr_type(EXT_ENC_A, GPIO_INTR_ANYEDGE));
		ESP_ERROR_CHECK(gpio_set_intr_type(EXT_ENC_B, GPIO_INTR_ANYEDGE));

		gpio_install_isr_service(0);
		ESP_ERROR_CHECK(gpio_isr_handler_add(
			EXT_ENC_A, stepper_ctrl::isr_a, this));
		ESP_ERROR_CHECK(gpio_isr_handler_add(
			EXT_ENC_B, stepper_ctrl::isr_b, this));

		ESP_ERROR_CHECK(gpio_set_level(STP_DIR_PIN, 0));
		ESP_ERROR_CHECK(gpio_set_level(STP_ENA_PIN, STP_ENA_POL));

		fan_start();

		pull_down_clk = new delayed_action(MOTOR_CLK_PULSE_US,
			pull_down_clk_handler);
	}

	~stepper_ctrl()
	{
		gpio_isr_handler_remove(EXT_ENC_A);
		gpio_isr_handler_remove(EXT_ENC_B);
		gpio_reset_pin(EXT_ENC_A);
		gpio_reset_pin(EXT_ENC_B);
		ESP_ERROR_CHECK(gpio_set_level(STP_ENA_PIN, !STP_ENA_POL));

		fan_stop();

		delete(pull_down_clk);
	}

	float get_abs_position()
	{
		return (float)position / (float)ratio /
			MOTOR_PULSES_TO_SUPPORT_MM;
	}

private:
	uint32_t ratio;
	uint32_t cnt = 0;
	std::atomic<int32_t> position { 0 };
	bool dir_invert = false;
	enum prev_p { ENC_P_A, ENC_P_B } prev_p = ENC_P_A;
	enum dir { FRONT, REVERS } direction;
	delayed_action *pull_down_clk;

	static void pull_down_clk_handler(void *args)
	{
		GPIO_SET(STP_CLK_PIN, !STP_CLK_POL);
	}

	static void motor_step(stepper_ctrl *s)
	{
		s->cnt++;

		if (s->direction == dir::FRONT)
			s->position++;
		else
			s->position--;

		if (s->cnt == s->ratio) {
			s->cnt = 0;
			GPIO_SET(STP_CLK_PIN, STP_CLK_POL);
			delayed_action *d = s->pull_down_clk;
			d->run();
		}
	}

	static void isr_a(void *params)
	{
		stepper_ctrl *s = static_cast<stepper_ctrl *>(params);
		if (s->prev_p == ENC_P_A)
			return;

		s->prev_p = ENC_P_A;

		if (GPIO_GET(EXT_ENC_A)) {
			if (GPIO_GET(EXT_ENC_B)) {
				GPIO_SET(STP_DIR_PIN, s->dir_invert);
				s->direction = dir::FRONT;
			} else {
				GPIO_SET(STP_DIR_PIN, !s->dir_invert);
				s->direction = dir::REVERS;
			}
		}

		motor_step(s);
	}

	static void isr_b(void *params)
	{
		stepper_ctrl *s = static_cast<stepper_ctrl *>(params);
		if (s->prev_p == ENC_P_B)
			return;

		s->prev_p = ENC_P_B;

		motor_step(s);
	}
};

void thread_cut(lcd& lcd,
		Buttons& btns,
		const char *name,
		uint32_t step,
		bool dir)
{
	freq_meter meter(EXT_ENC_Z, false);

	stepper_ctrl stepper_thread_cut(step, dir);

	while (1) {
		uint32_t freq =
			SPEED_RATIO_CALC(meter.get_frequency<uint32_t>());

		lcd.print(FIRST_ROW, LEFT, "FREQ: %-10u", freq);
		lcd.print(SECOND_ROW, LEFT, "TOOL POS: %-6.2f",
			stepper_thread_cut.get_abs_position());

		int press = btns.wait(200);
		if (press == BUTTON_RETURN)
			break;
	}

}
