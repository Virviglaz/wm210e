#include "motor_ctrl.h"
#include "hardware.h"
#include "log.h"
#include <free_rtos_h.h>
#include <errno.h>
#include <string.h>
#include <esp32_timer.h>
#include <step_motor.h>
#include <esp_encoder.h>

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

class stepper_ctrl
{
public:
	stepper_ctrl(uint32_t ratio,
		     bool dir_invert,
		     int32_t limit = 0) :
		ratio(ratio),
		dir_invert(dir_invert) {
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
		enable();

		fan_start();

		pull_down_clk = new delayed_action(MOTOR_CLK_PULSE_US,
			pull_down_clk_handler);
	}

	~stepper_ctrl() {
		gpio_isr_handler_remove(EXT_ENC_A);
		gpio_isr_handler_remove(EXT_ENC_B);
		gpio_reset_pin(EXT_ENC_A);
		gpio_reset_pin(EXT_ENC_B);
		disable();

		fan_stop();
		delete(pull_down_clk);
	}

	float get_abs_position() {
		return (float)position * ENC_PULSES_TO_SUPPORT_MM /
			(float)ratio / MOTOR_PULSES_TO_SUPPORT_MM;
	}

	void clear_abs_position() {
		position = 0;
		cnt = 0;
		check_limit();
	}

	void enable() {
		GPIO_SET(STP_ENA_PIN, STP_ENA_POL);
		is_enabled = true;
		INFO("Stepper enabled");
	}

	void disable() {
		GPIO_SET(STP_ENA_PIN, !STP_ENA_POL);
		is_enabled = false;
		INFO("Stepper disabled");
	}

	void set_limit(float lim) {
		limit_reached = false;
		max = lim  / ENC_PULSES_TO_SUPPORT_MM *
			(float)ratio * MOTOR_PULSES_TO_SUPPORT_MM;
	}

	bool check_limit() {
		bool ret = limit_reached;
		if (ret)
			INFO("LIMIT REACHED! (pos = %ld, lim: %ld)",
				position, max);
		limit_reached = false;
		return ret;
	}

	void reset() {
		clear_abs_position();
		disable();
		delay_ms(STP_DELAY_MS);
		enable();
	}

private:
	bool is_enabled = true;
	uint32_t ratio;
	uint32_t cnt = 0;
	int32_t max = 0;
	bool limit_reached = false;
	int32_t position = 0;
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

		if (s->direction == dir::REVERS) {
			s->position++;
			if (s->max && s->position > s->max) {
				s->limit_reached = true;
				return;
			}
		} else {
			s->position--;
			if (s->max && s->position < -s->max) {
				s->limit_reached = true;
				return;
			}
		}

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

		if (s->is_enabled == false)
			return;

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

		if (s->is_enabled == false)
			return;

		motor_step(s);
	}
};

void thread_cut(lcd& lcd,		/* LCD driver */
		Buttons& btns,		/* Buttons driver */
		const char *name,	/* Title */
		float step_mm,		/* Support steps per rotation */
		enum dir dir,		/* Support movement direction */
		int32_t limit10,	/* Support movement limit x10 mm */
		bool sup_return)	/* Automatic support return */
{
	uint32_t step = (uint32_t)((float)ENC_PULSES_TO_SUPPORT_MM / step_mm);
	int32_t step_dir = dir == CW ? 1 : -1;
	float limit = (float)(limit10 * step_dir) / 10;
	freq_meter meter(EXT_ENC_Z, false);
	stepper_ctrl stepper_thread_cut(step, dir);
	Encoder<int32_t> *enc = nullptr;
	int32_t enc_prev = 0;

	lcd.clear();
	if (limit10) {
		lcd.print(FIRST_ROW, RIGHT, "L:%-5.1f", limit);
		stepper_thread_cut.set_limit(limit);
		INFO("Setting limit: %.2f [mm]", limit);
		enc = new Encoder<int32_t>(ENC_A, ENC_B, Encoder<int32_t>::NONE);
		enc->set_value(limit10);
		enc->invert();
		enc_prev = limit10;
	}
	if (sup_return)
		lcd.print(SECOND_ROW, RIGHT, "+RTN");
	
	while (1) {
		float freq =
			SPEED_RATIO_CALC(meter.get_frequency<float>());
		float abs_pos = stepper_thread_cut.get_abs_position();

		lcd.print(FIRST_ROW,  LEFT, "FRQ:%-4.0f", freq);
		lcd.print(SECOND_ROW, LEFT, "POS:%-6.2f", abs_pos);

		int press = btns.wait(200);
		if (press == BUTTON_RETURN)
			break;
		else if (press == BUTTON_ENTER)
			stepper_thread_cut.reset();

		/* Semiautomatic support return */
		if (sup_return && stepper_thread_cut.check_limit()) {
			int32_t step_to_run = (limit + STP_BACKLASH_MM) * \
				step_mm  * \
				MOTOR_PULSES_TO_SUPPORT_MM * step_dir / 10;
			int32_t backlash = STP_BACKLASH_MM * step_mm * \
				MOTOR_PULSES_TO_SUPPORT_MM * step_dir / 10;
			stepper_thread_cut.disable();
			INFO("steps: %ld, backlash: %ld", step_to_run, backlash);
			Step_motor m(STP_ENA_PIN,
				     STP_CLK_PIN,
				     STP_DIR_PIN,
				     STP_ACC, 50,
				     nullptr,
				     STP_CLK_INVERT,
				     STP_ENA_INVERT);
			m.init(true);
			m.add_segment(step_to_run, STP_SPEED);
			m.add_segment(-backlash, STP_SPEED);
			m.run();
			m.wait();
			stepper_thread_cut.reset();
		}

		/* Update support limit using rotary encoder */
		if (enc && enc->get_value() != enc_prev) {
			enc_prev = enc->get_value();
			limit = (float)enc_prev / 10;
			lcd.print(FIRST_ROW, RIGHT, "L:%-5.1f", limit);
			stepper_thread_cut.set_limit(limit);
		}
	}

	delete(enc);
}
