#include "menu.h"
#include "lcd.h"
#include "platform.h"
#include "hardware.h"
#include "esp_buttons.h"
#include "thread_cut.h"
#include "smooth_go.h"
#include "wifi.h"
#include "ota.h"
#include "esp_ota_ops.h"
#include "menu.h"

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

static int start_fw_update(int arg)
{
	const esp_app_desc_t *app_desc = esp_ota_get_app_description();
	ota.version = app_desc->version;

	wifi_init("Tower", "555666777", "WM210E");
	ota_start(&ota);

	INFO("Firmware version: %s", ota.version);

	LCD->clear();
	LCD->print(FIRST_ROW, CENTER, "START FW UPDATE");
	delay_s(3);

	return 0;
}

struct menu_item
{
	std::string first_row;
	std::string second_row;
	int (*handler)(int arg);
	const int arg;
};

static const std::vector<menu_item> list = { {
	.first_row = "METRIC THREAD",
	.second_row = "RIGHT",
	.handler = thread_cut_handler,
	.arg = 1,
}, {
	.first_row = "METRIC THREAD",
	.second_row = "LEFT",
	.handler = thread_cut_handler,
	.arg = 0,
}, {
	.first_row = "SMOOTH GO",
	.second_row = "RIGHT",
	.handler = smooth_go_handler,
	.arg = 0,
}, {
	.first_row = "SMOOTH GO",
	.second_row = "LEFT",
	.handler = smooth_go_handler,
	.arg = 1,
}, {
	.first_row = "START",
	.second_row = "FW UPDATE",
	.handler = start_fw_update,
	.arg = 0,
} };


static SemaphoreHandle_t wait;
static Menu<menu_item> menu(list);

static void enc_btn_handler(void *arg)
{
	bool *proceed = (bool *)arg;
	*proceed = true;
	xSemaphoreGive(wait);
}

static void enc_rol_handler(void *arg)
{
	if (gpio_get_level(ENC_B))
		menu.next();
	else
		menu.prev();

	xSemaphoreGive(wait);
}

static void enc_rol_dummy(void *arg) {}

void menu_start(void)
{
	bool proceed = false;
	wait = xSemaphoreCreateBinary();
	buttons *btn = new buttons(10);
	btn->add(ENC_BTN, enc_btn_handler, NEGEDGE, &proceed);
	btn->add(ENC_A, enc_rol_handler);
	btn->add(ENC_B, enc_rol_dummy);

	while (1) {
		const menu_item *item = menu.get();
		LCD->clear();
		LCD->print(FIRST_ROW, CENTER, "%s", item->first_row.c_str());
		LCD->print(SECOND_ROW, CENTER, "%s", item->second_row.c_str());
		xSemaphoreTake(wait, portMAX_DELAY);
		if (proceed)
			break;
	}

	vSemaphoreDelete(wait);
	delete(btn);

	const menu_item *item = menu.get();
	item->handler(item->arg);
}
