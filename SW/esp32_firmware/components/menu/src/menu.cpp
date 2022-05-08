#include "menu.h"
#include "lcd.h"
#include "platform.h"
#include "hardware.h"
#include "esp_buttons.h"
#include "thread_cut.h"
#include "smooth_go.h"

static const struct menu {
	const char *first_row;
	const char *second_row;
	int (*handler)(int arg);
	const int arg;
	const struct menu *next;
} menu[] = {
	[0] = {
		.first_row = "METRIC THREAD",
		.second_row = "RIGHT",
		.handler = thread_cut_handler,
		.arg = 0,
		.next = &menu[1],
	},
	[1] = {
		.first_row = "METRIC THREAD",
		.second_row = "LEFT",
		.handler = thread_cut_handler,
		.arg = 1,
		.next = &menu[2],
	},
	[2] = {
		.first_row = "SMOOTH GO",
		.second_row = "RIGHT",
		.handler = smooth_go_handler,
		.arg = 0,
		.next = &menu[3],
	},
	[3] = {
		.first_row = "SMOOTH GO",
		.second_row = "LEFT",
		.handler = smooth_go_handler,
		.arg = 1,
		.next = &menu[0],
	},
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
	current_menu = (struct menu *)current_menu->next;
	xSemaphoreGive(wait);
}

void menu_start(void)
{
	bool proceed = false;
	wait = xSemaphoreCreateBinary();
	buttons *btn = new buttons();
	btn->add(ENC_BTN, enc_btn_handler, NEGEDGE, &proceed);
	btn->add(ENC_A, enc_rol_handler);

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
