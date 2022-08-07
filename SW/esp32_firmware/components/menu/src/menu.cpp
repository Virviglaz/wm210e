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

static const struct menu {
	const char *first_row;
	const char *second_row;
	int (*handler)(int arg);
	const int arg;
	const struct menu *next;
	const struct menu *prev;
} menu[] = {
	[0] = {
		.first_row = "METRIC THREAD",
		.second_row = "RIGHT",
		.handler = thread_cut_handler,
		.arg = 1,
		.next = &menu[1],
		.prev = &menu[4],
	},
	[1] = {
		.first_row = "METRIC THREAD",
		.second_row = "LEFT",
		.handler = thread_cut_handler,
		.arg = 0,
		.next = &menu[2],
		.prev = &menu[0],
	},
	[2] = {
		.first_row = "SMOOTH GO",
		.second_row = "RIGHT",
		.handler = smooth_go_handler,
		.arg = 0,
		.next = &menu[3],
		.prev = &menu[1],
	},
	[3] = {
		.first_row = "SMOOTH GO",
		.second_row = "LEFT",
		.handler = smooth_go_handler,
		.arg = 1,
		.next = &menu[4],
		.prev = &menu[2],
	},
	[4] = {
		.first_row = "START",
		.second_row = "FW UPDATE",
		.handler = start_fw_update,
		.arg = 0,
		.next = &menu[0],
		.prev = &menu[3],
	}
};

static struct menu *current_menu = (struct menu *)&menu[0];
static SemaphoreHandle_t wait;

static void enc_btn_handler(void *arg)
{
	bool *proceed = (bool *)arg;
	*proceed = true;
	xSemaphoreGive(wait);
}

static void enc_rol_handler(void *arg)
{
	if (gpio_get_level(ENC_B))
		current_menu = (struct menu *)current_menu->next;
	else
		current_menu = (struct menu *)current_menu->prev;

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
		LCD->clear();
		LCD->print(FIRST_ROW, CENTER, "%s", current_menu->first_row);
		LCD->print(SECOND_ROW, CENTER, "%s", current_menu->second_row);
		xSemaphoreTake(wait, portMAX_DELAY);
		if (proceed)
			break;
	}

	vSemaphoreDelete(wait);
	delete(btn);

	current_menu->handler(current_menu->arg);
}
