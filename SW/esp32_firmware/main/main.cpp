#include "hardware.h"
#include "platform.h"
#include "menu.h"
#include "wifi.h"
#include "ota.h"
#include "esp_ota_ops.h"

extern "C" {
	void app_main();
}

static void gpio_ota_workaround(void)
{
	/* IO2 workaround */
	gpio_reset_pin(GPIO_NUM_2);
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_2, 0);
}

static ota_t ota = {
	.server_ip = "192.168.0.108",
	.server_port = 5005,
	.serial_number = 1,
	.check_interval_ms = 0,
	.message_size = 0,
	.uniq_magic_word = 0,
	.version = 0, /* set later */
	.gpio_ota_workaround = gpio_ota_workaround,
	.gpio_ota_cancel_workaround = 0,
};

static void fw_updater(void *args)
{
	delay_s(10);

	INFO("Firmware is valid, confirm image");
	ota_confirm();
	ota_start(&ota);
	vTaskDelete(NULL);
}

void app_main(void)
{
	const esp_app_desc_t *app_desc = esp_ota_get_app_description();
	ota.version = app_desc->version;

	wifi_init("Tower", "555666777", "WM210E");

	xTaskCreate(fw_updater, NULL, 0x800, 0, 1, 0);
	INFO("Firmware version: %s", ota.version);

	int res = hardware_init(ota.version);
	if (res)
		return;

	while (1)
		menu_start();

	return;
}
