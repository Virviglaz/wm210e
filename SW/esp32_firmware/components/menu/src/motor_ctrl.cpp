#include "motor_ctrl.h"
#include "platform.h"
#include "hardware.h"
#include "lcd.h"
#include "esp_buttons.h"

/* ESP32 drivers */
#include "driver/gpio.h"
#include "driver/timer.h"
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
	delayed_action(uint32_t period_us, void (*action)(void),
		timer_group_t group = TIMER_GROUP_0,
		timer_idx_t timer = TIMER_0) : action(action),
			group(group), timer(timer)
	{
		const timer_config_t config = {
			.alarm_en = TIMER_ALARM_EN,
			.counter_en = TIMER_PAUSE,
			.intr_type = TIMER_INTR_LEVEL,
			.counter_dir = TIMER_COUNT_UP,
			.auto_reload = TIMER_AUTORELOAD_EN,
			.divider = TIMER_DIVIDER,
		};
		ESP_ERROR_CHECK(timer_init(group, timer, &config));
		ESP_ERROR_CHECK(timer_set_counter_value(group, timer, 0));
		ESP_ERROR_CHECK(timer_enable_intr(group, timer));
		ESP_ERROR_CHECK(timer_isr_callback_add(group, timer, callback,
			this, 0));
		ESP_ERROR_CHECK(timer_set_alarm_value(group, timer, period_us));
	}

	~delayed_action()
	{
		ESP_ERROR_CHECK(timer_disable_intr(group, timer));
		ESP_ERROR_CHECK(timer_isr_callback_remove(group, timer));
		ESP_ERROR_CHECK(timer_deinit(group, timer));
	}

	void IRAM_ATTR run()
	{
		timer_group_set_counter_enable_in_isr(group, timer, TIMER_START);
	}

private:
	void (*action)(void);
	timer_group_t group;
	timer_idx_t timer;
	static bool IRAM_ATTR callback(void *args)
	{
		delayed_action *m = (delayed_action *)args;
		timer_group_set_counter_enable_in_isr(m->group, m->timer,
			TIMER_PAUSE);
		m->action();
		return true;
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
		ESP_ERROR_CHECK(gpio_reset_pin(EXT_ENC_Z));
		ESP_ERROR_CHECK(gpio_reset_pin(STP_CLK_PIN));
		ESP_ERROR_CHECK(gpio_reset_pin(STP_DIR_PIN));
		ESP_ERROR_CHECK(gpio_reset_pin(STP_ENA_PIN));
	
		ESP_ERROR_CHECK(gpio_set_direction(EXT_ENC_A, GPIO_MODE_INPUT));
		ESP_ERROR_CHECK(gpio_set_direction(EXT_ENC_B, GPIO_MODE_INPUT));
		ESP_ERROR_CHECK(gpio_set_direction(EXT_ENC_Z, GPIO_MODE_INPUT));

		ESP_ERROR_CHECK(gpio_set_direction(STP_CLK_PIN, GPIO_MODE_OUTPUT));
		ESP_ERROR_CHECK(gpio_set_direction(STP_DIR_PIN, GPIO_MODE_OUTPUT));
		ESP_ERROR_CHECK(gpio_set_direction(STP_ENA_PIN, GPIO_MODE_OUTPUT));

		ESP_ERROR_CHECK(gpio_set_intr_type(EXT_ENC_A, GPIO_INTR_ANYEDGE));
		ESP_ERROR_CHECK(gpio_set_intr_type(EXT_ENC_B, GPIO_INTR_ANYEDGE));
		ESP_ERROR_CHECK(gpio_set_intr_type(EXT_ENC_Z, GPIO_INTR_ANYEDGE));

		ESP_ERROR_CHECK(gpio_install_isr_service(0));
		ESP_ERROR_CHECK(gpio_isr_handler_add(
			EXT_ENC_A, stepper_ctrl::isr_a, this));
		ESP_ERROR_CHECK(gpio_isr_handler_add(
			EXT_ENC_B, stepper_ctrl::isr_b, this));
		ESP_ERROR_CHECK(gpio_isr_handler_add(
			EXT_ENC_Z, stepper_ctrl::isr_z, this));

		ESP_ERROR_CHECK(gpio_set_level(STP_DIR_PIN, 0));
		ESP_ERROR_CHECK(gpio_set_level(STP_ENA_PIN, 1));

		fan_start();

		pull_down_clk = new delayed_action(500, pull_down_clk_handler);
	}

	~stepper_ctrl()
	{
		gpio_isr_handler_remove(EXT_ENC_A);
		gpio_isr_handler_remove(EXT_ENC_B);
		gpio_isr_handler_remove(EXT_ENC_Z);
		gpio_reset_pin(EXT_ENC_A);
		gpio_reset_pin(EXT_ENC_B);
		gpio_reset_pin(EXT_ENC_Z);
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
	const int32_t rotation_mult = 1000;
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
		 */
		const int32_t step_deg = s->rotation_mult * (27 * 360) / (2 * 800 * 4 * 60);
		s->cnt++;
		s->position_deg +=
			s->direction == dir::FRONT ? step_deg : - step_deg;

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

	static void IRAM_ATTR isr_z(void *params)
	{
		stepper_ctrl *s = static_cast<stepper_ctrl *>(params);

		s->rotations++;
		//s->position_deg = 0;
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
		if (xSemaphoreTake(wait, pdMS_TO_TICKS(1000)) == pdTRUE)
			break;
		LCD->print(FIRST_ROW, CENTER, "POS: %d   ",
			stepper_thread_cut->get_position_deg());
		LCD->print(SECOND_ROW, CENTER, "ROTATIONS: %u   ",
			stepper_thread_cut->get_rotations_counter());
	}

	vSemaphoreDelete(wait);
	delete(stepper_thread_cut);
	delete(btn);
	INFO("Done thread cut");
}
