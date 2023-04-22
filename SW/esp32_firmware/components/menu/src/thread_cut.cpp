#include "hardware.h"
#include "lcd.h"
#include "esp_buttons.h"
#include "motor_ctrl.h"
#include "thread_cut.h"
#include "driver/gpio.h"
#include "cpp_menu.h"
#include <free_rtos_h.h>
#include <vector>

struct menu_item {
	std::string name;
	const uint32_t step;
};

static const std::vector<menu_item> list { {
	.name = "M2x0.4",
	.step = 50,
}, {
	.name = "M3x0.5",
	.step = 40,
}, {
	.name = "M4x0.7",
	.step = 57 / 2,
}, {
	.name = "M5x0.8",
	.step = 25,
}, {
	.name = "M6x1.0",
	.step = 20,
}, {
	.name = "M8x1.25",
	.step = 16,
}, {
	.name = "M10x1.5",
	.step = 27 / 2,
}, {
	.name = "M12x1.75",
	.step = 23 / 2,
}, {
	.name = "M14x2.0",
	.step = 10,
}, {
	.name = "M16x2.0",
	.step = 10,
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
	if (gpio_get_level(ENC_B))
		menu.next();
	else
		menu.prev();

	xSemaphoreGive(wait);
}

static void enc_rol_dummy(void *arg) {}

static void btn1_handler(void *arg)
{
	bool *is_done = (bool *)arg;
	*is_done = true;
	xSemaphoreGive(wait);
}

int thread_cut_handler(int arg)
{
	bool is_done = false;
	bool proceed = false;
	bool dir = arg == 0;
	wait = xSemaphoreCreateBinary();
	Buttons *btn = new Buttons(10);
	btn->add(ENC_BTN, enc_btn_handler, Buttons::NEGEDGE, &proceed);
	btn->add(ENC_A, enc_rol_handler);
	btn->add(ENC_B, enc_rol_dummy);
	btn->add(BTN1, btn1_handler, Buttons::NEGEDGE, &is_done);

	while (1) {
		const menu_item *item = menu.get();
		LCD->clear();
		LCD->print(FIRST_ROW, CENTER, "%s %s",
			item->name.c_str(), arg ? "LEFT" : "RIGHT");
		xSemaphoreTake(wait, portMAX_DELAY);

		if (is_done || proceed)
			break;
	}

	vSemaphoreDelete(wait);
	delete(btn);

	if (proceed) {
		const menu_item *item = menu.get();
		thread_cut(item->name.c_str(), item->step, dir);
	}

	return 0;
}
