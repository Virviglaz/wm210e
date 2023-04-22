#include <free_rtos_h.h>
#include "lcd.h"
#include "esp_buttons.h"
#include "hardware.h"
#include "cutting.h"
#include "cpp_menu.h"
#include <vector>
#include "motor_ctrl.h"

struct menu_item {
	const char *name;
	const uint32_t step;
};

static const std::vector<menu_item> list { {
	.name = "0.05 mm/r",
	.step = 400,
}, {
	.name = "0.10 mm/r",
	.step = 200,
}, {
	.name = "0.25 mm/r",
	.step = 80,
}, {
	.name = "0.50 mm/r",
	.step = 40,
} };

static Menu<menu_item> menu(list);
static SemaphoreHandle_t wait;

static void enc_btn_handler(void *arg)
{
	bool *proceed = (bool *)arg;
	*proceed = true;
	xSemaphoreGive(wait);
}

static void enc_rol_handler(void *arg)
{
	menu.next();
	xSemaphoreGive(wait);
}

static void btn1_handler(void *arg)
{
	bool *is_done = (bool *)arg;
	*is_done = true;
	xSemaphoreGive(wait);
}

int smooth_go_handler(int arg)
{
	bool is_done = false;
	bool proceed = false;
	bool dir = arg == 0;
	wait = xSemaphoreCreateBinary();
	Buttons *btn = new Buttons(10);
	btn->add(ENC_BTN, enc_btn_handler, Buttons::NEGEDGE, &proceed);
	btn->add(ENC_A, enc_rol_handler);
	btn->add(BTN1, btn1_handler, Buttons::NEGEDGE, &is_done);

	while (1) {
		const menu_item *item = menu.get();
		LCD->clear();
		LCD->print(FIRST_ROW, CENTER, "%s %s",
			item->name, arg ? "LEFT" : "RIGHT");
		xSemaphoreTake(wait, portMAX_DELAY);

		if (is_done || proceed)
			break;
	}

	vSemaphoreDelete(wait);
	delete(btn);

	if (proceed) {
		const menu_item *item = menu.get();
		thread_cut(item->name, item->step, dir);
	}

	return 0;
}