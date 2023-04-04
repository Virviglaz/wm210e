#include "motor_ctrl.h"
#include <free_rtos_h.h>
#include "hardware.h"
#include "lcd.h"
#include "esp_buttons.h"
#include "log.h"

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

class delayed_action
{
public:
	delayed_action(uint32_t timeout_us, void (*action)(void))
	{
		const esp_timer_create_args_t config = {
			.callback = callback,
			.arg = this,
			.dispatch_method = ESP_TIMER_ISR,
			.name = __func__,
			.skip_unhandled_events = true,
		};
		ESP_ERROR_CHECK(esp_timer_create(&config, &handle));
		_action = action;
		_timeout_us = timeout_us;
	}

	~delayed_action()
	{
		ESP_ERROR_CHECK(esp_timer_delete(handle));
	}

	void IRAM_ATTR run()
	{
		esp_timer_start_once(handle, _timeout_us);
	}

private:
	void (*_action)(void);
	esp_timer_handle_t handle;
	uint32_t _timeout_us;
	static void IRAM_ATTR callback(void *args)
	{
		delayed_action *m = (delayed_action *)args;
		m->_action();
	}
};

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

		ESP_ERROR_CHECK(gpio_install_isr_service(0));
		ESP_ERROR_CHECK(gpio_isr_handler_add(
			EXT_ENC_A, stepper_ctrl::isr_a, this));
		ESP_ERROR_CHECK(gpio_isr_handler_add(
			EXT_ENC_B, stepper_ctrl::isr_b, this));

		ESP_ERROR_CHECK(gpio_set_level(STP_DIR_PIN, 0));
		ESP_ERROR_CHECK(gpio_set_level(STP_ENA_PIN, 1));

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
		gpio_uninstall_isr_service();
		ESP_ERROR_CHECK(gpio_set_level(STP_ENA_PIN, 0));

		fan_stop();

		delete(pull_down_clk);
	}

	uint32_t get_rotations_counter()
	{
		return rotations;
	}

	int32_t get_position_deg()
	{
		return (position_deg / rotation_mult) % 360;
	}

	uint32_t err_cnt = 0;
private:
	uint32_t ratio;
	const int32_t rotation_mult = 1000000;
	std::atomic<uint32_t> cnt { 0 };
	std::atomic<uint32_t> rotations { 0 };
	std::atomic<int32_t> position_deg { 0 };
	bool dir_invert = false;
	enum prev_p { ENC_P_A, ENC_P_B } prev_p = ENC_P_A;
	enum dir { FRONT, REVERS } direction;
	delayed_action *pull_down_clk;

	static void pull_down_clk_handler()
	{
		GPIO_SET(STP_CLK_PIN, 0);
	}

	static void motor_step(stepper_ctrl *s)
	{
		/**
		 * @brief Calcluation
		 *
		 * (m) 60T->27T (60:27), 20T->40T (2:1), 800ppm * 4 (2x2 isr)
		 * Encoder gives 2x2x800 = 3200 isrs/rev
		 * 
		 */
		const int32_t step_deg =
			s->rotation_mult / 2 * 27 * 3200 / 60 / 360;
		s->cnt++;
		s->position_deg +=
			s->direction == dir::FRONT ? step_deg : - step_deg;

		if (s->direction == dir::FRONT &&
			s->position_deg >= s->rotation_mult * 360) {
			s->rotations++;
			s->position_deg = 0;
		}

		if (s->direction == dir::REVERS &&
			s->position_deg <= 0) {
			s->rotations++;
			s->position_deg = 0;
		}

		if (s->cnt == s->ratio) {
			s->cnt = 0;
			if (GPIO_GET(STP_CLK_PIN))
				s->err_cnt++;
			GPIO_SET(STP_CLK_PIN, 1);

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

static void btn1_handler(void *arg)
{
	xSemaphoreGive((SemaphoreHandle_t)arg);
}

void thread_cut(const char *name, uint32_t step, bool dir)
{
	SemaphoreHandle_t wait = xSemaphoreCreateBinary();
	buttons *btn = new buttons();

	btn->add(BTN1, btn1_handler, NEGEDGE, (void *)wait);

	LCD->clear();
	LCD->print(FIRST_ROW, CENTER, "%s", name);

	stepper_ctrl *stepper_thread_cut = new stepper_ctrl(step, dir);

	while (1) {
		if (xSemaphoreTake(wait, pdMS_TO_TICKS(250)) == pdTRUE)
			break;
		LCD->print(FIRST_ROW, CENTER, "DEG: %d   ",
			stepper_thread_cut->get_position_deg());
		LCD->print(SECOND_ROW, CENTER, "ROTATIONS: %u   ",
			stepper_thread_cut->get_rotations_counter());
	}

	vSemaphoreDelete(wait);
	delete(stepper_thread_cut);
	delete(btn);
	INFO("Done thread cut");
}
