#include "hardware.h"
#include "platform.h"
#include "menu.h"
#include "wifi.h"
#include "ota.h"

extern "C" {
	void app_main();
}

static void gpio_ota_workaround(void)
{
	/* IO2 workaround */
	gpio_reset_pin(GPIO_NUM_2);
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_2, 0);

	/* IO12 workaround */
	gpio_reset_pin(GPIO_NUM_12);
	gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_12, 0);
}

static ota_t ota = {
	.server_ip = "192.168.0.108",
	.server_port = 5005,
	.serial_number = 1,
	.check_interval_ms = 5000,
	.uniq_magic_word = 0xDEADBEEF,
	.version = 0,
	.gpio_ota_workaround = gpio_ota_workaround,
};

void app_main(void)
{
	wifi_init("Tower", "555666777", "WM210E");

	INFO("Firmware is valid, confirm image");
	ota_confirm();
	ota_start(&ota);
	INFO("Firmware version: %s", ota.version);

	while (1);
	int res = hardware_init(ota.version);
	if (res)
		return;

	while (1)
		menu_start();

	return;
}
