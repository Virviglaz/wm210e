#include "menu.h"
#include "lcd.h"
#include "platform.h"
#include "hardware.h"
#include "esp_buttons.h"
#include "thread_cut.h"

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
		.next = &menu[0],
	}
};

static struct menu *current_menu = (struct menu *)&menu[0];
static SemaphoreHandle_t wait;

static void btn_handler(void *arg)
{
	current_menu = (struct menu *)current_menu->next;
	xSemaphoreGive(wait);
}

static void enc_handler(void *arg)
{
	bool *go_next = (bool *)arg;
	*go_next = true;
	xSemaphoreGive(wait);
}

void menu_start(void)
{
	bool go_next = false;
	wait = xSemaphoreCreateBinary();
	buttons *btn = new buttons();
	btn->add(BTN, btn_handler);
	btn->add(ENC_BTN, enc_handler, NEGEDGE, &go_next);

	while (1) {
		LCD->clear();
		LCD->print(FIRST_ROW, CENTER, "%s", current_menu->first_row);
		LCD->print(SECOND_ROW, CENTER, "%s", current_menu->second_row);
		xSemaphoreTake(wait, portMAX_DELAY);
		if (go_next)
			break;
	}

	vSemaphoreDelete(wait);
	delete(btn);

	current_menu->handler(current_menu->arg);
}
