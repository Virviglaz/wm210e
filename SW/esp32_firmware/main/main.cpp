#include "hardware.h"
#include "platform.h"
#include "menu.h"
#include "ota.h"
#include "esp_ota_ops.h"

extern "C" {
	void app_main();
}

static void ota_confirm(void *args)
{
	delay_s(10);

	INFO("Firmware is valid, confirm image");
	ota_confirm();
	vTaskDelete(NULL);
}

void app_main(void)
{
	const esp_app_desc_t *app_desc = esp_ota_get_app_description();

	xTaskCreate(ota_confirm, NULL, 0x800, 0, 1, 0);

	int res = hardware_init(app_desc->version);
	if (res)
		return;

	while (1)
		menu_start();

	return;
}
