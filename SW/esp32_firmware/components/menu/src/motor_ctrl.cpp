#include "motor_ctrl.h"
#include "platform.h"
#include "hardware.h"
#include "lcd.h"
#include "esp_buttons.h"

/* ESP32 drivers */
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_timer.h"

class delayed_action
{
public:
	delayed_action(uint32_t period_us, void (*action)(void)) :
		action(action)
	{
		gptimer_config_t timer_config;
		timer_config.clk_src = GPTIMER_CLK_SRC_APB;
		timer_config.direction = GPTIMER_COUNT_UP;
		timer_config.resolution_hz = 1u * 1000u * 1000u;
		ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

		gptimer_event_callbacks_t event;
		event.on_alarm = timer_event;
		ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &event, this));

		gptimer_alarm_config_t alarm_config;
		alarm_config.alarm_count = period_us;
		ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
	}

	~delayed_action()
	{
		ESP_ERROR_CHECK(gptimer_del_timer(gptimer));
	}

	IRAM_ATTR void run()
	{
		gptimer_set_raw_count(gptimer, 0);
		gptimer_start(gptimer);
	}

private:
	gptimer_handle_t gptimer;
	void (*action)(void);
	IRAM_ATTR static bool timer_event(gptimer_handle_t timer,
		const gptimer_alarm_event_data_t *edata, void *user_data)
	{
		delayed_action *m = (delayed_action *)user_data;
		gptimer_stop(timer);
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

		//pull_down_clk = new delayed_action(1, pull_down_clk_handler);
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

		//delete(pull_down_clk);
	}

	uint32_t get_rp_cnt()
	{
		return cnt;
	}
private:
	uint32_t ratio;
	uint32_t cnt = 0;
	uint32_t rpm_cnt = 0;
	bool dir_invert = false;
	enum prev_p { ENC_P_A, ENC_P_B } prev_p = ENC_P_A;
	//delayed_action *pull_down_clk;

	static void pull_down_clk_handler()
	{
		gpio_set_level(STP_CLK_PIN, 0);
	}

	static void motor_step(stepper_ctrl *s)
	{
		gpio_set_level(STP_CLK_PIN, 0);

		s->cnt++;
		if (s->cnt == s->ratio) {
			s->cnt = 0;
			gpio_set_level(STP_CLK_PIN, 1);

			//delayed_action *d = s->pull_down_clk;
			//d->run();
		}
	}

	static void isr_a(void *params)
	{
		stepper_ctrl *s = static_cast<stepper_ctrl *>(params);
		if (s->prev_p == ENC_P_A)
			return;

		s->prev_p = ENC_P_A;

		if (gpio_get_level(EXT_ENC_A)) {
			if (gpio_get_level(EXT_ENC_B))
				gpio_set_level(STP_DIR_PIN, s->dir_invert);
			else
				gpio_set_level(STP_DIR_PIN, !s->dir_invert);
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

	static void isr_z(void *params)
	{
		stepper_ctrl *s = static_cast<stepper_ctrl *>(params);

		s->rpm_cnt++;
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
		LCD->print(SECOND_ROW, CENTER, "%u", stepper_thread_cut->get_rp_cnt());
	}

	vSemaphoreDelete(wait);
	delete(stepper_thread_cut);
	delete(btn);
	INFO("Done thread cut");
}
