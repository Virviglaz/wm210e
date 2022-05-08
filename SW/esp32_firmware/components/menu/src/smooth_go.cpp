#include "platform.h"
#include "hardware.h"
#include "lcd.h"
#include "esp_buttons.h"
#include "motor_ctrl.h"
#include "smooth_go.h"

static const struct cut {
	const char *name;
	const uint32_t step;
	const struct cut *next;
} cuts[] = { {
		.name = "FINE",
		.step = 1000,
		.next = &cuts[1],
	}, {
		.name = "MEDIUM",
		.step = 600,
		.next = &cuts[2],
	}, {
		.name = "COARSE",
		.step = 100,
		.next = &cuts[0],
	},
};

static struct cut *current_cut = (struct cut *)&cuts[0];
static SemaphoreHandle_t wait;

static void enc_btn_handler(void *arg)
{
	bool *proceed = (bool *)arg;
	*proceed = true;
	xSemaphoreGive(wait);
}

static void enc_rol_handler(void *arg)
{
	current_cut = (struct cut *)current_cut->next;
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
	buttons *btn = new buttons();
	btn->add(ENC_BTN, enc_btn_handler, NEGEDGE, &proceed);
	btn->add(ENC_A, enc_rol_handler);
	btn->add(BTN1, btn1_handler, NEGEDGE, &is_done);

	while (1) {
		LCD->clear();
		LCD->print(FIRST_ROW, CENTER, "%s %s",
			current_cut->name, arg ? "LEFT" : "RIGHT");
		xSemaphoreTake(wait, portMAX_DELAY);

		if (is_done || proceed)
			break;
	}

	vSemaphoreDelete(wait);
	delete(btn);

	if (proceed)
		thread_cut(current_cut->name, current_cut->step, dir);

	return 0;
}