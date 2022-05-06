#include "hardware.h"
#include "lcd.h"

int hardware_init(void)
{
	/* create LCD */
	LCD = new lcd();

	ESP_ERROR_CHECK(gpio_reset_pin(FAN_ENA_PIN));
	ESP_ERROR_CHECK(gpio_set_direction(FAN_ENA_PIN, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(FAN_ENA_PIN, 0));

	ESP_ERROR_CHECK(gpio_reset_pin(STP_ENA_PIN));
	ESP_ERROR_CHECK(gpio_set_direction(STP_ENA_PIN, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(STP_ENA_PIN, 0));

	return 0;
}

void fan_start()
{
	ESP_ERROR_CHECK(gpio_set_level(FAN_ENA_PIN, 1));
}

void fan_stop()
{
	ESP_ERROR_CHECK(gpio_set_level(FAN_ENA_PIN, 0));
}

void motor_enable(bool state)
{
	ESP_ERROR_CHECK(gpio_set_level(STP_ENA_PIN, state));
	ESP_ERROR_CHECK(gpio_set_level(FAN_ENA_PIN, state));
}
