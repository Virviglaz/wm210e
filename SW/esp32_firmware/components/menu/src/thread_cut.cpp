#include "platform.h"
#include "hardware.h"
#include "lcd.h"
#include "esp_buttons.h"
#include "motor_ctrl.h"
#include "thread_cut.h"

static const struct thread {
	const char *name;
	const uint32_t step;
	const struct thread *next;
} thread[] = { {
		.name = "M2x0.4",
		.step = 100,
		.next = &thread[1],
	}, {
		.name = "M3x0.5",
		.step = 80,
		.next = &thread[2],
	}, {
		.name = "M4x0.7",
		.step = 57,
		.next = &thread[3],
	}, {
		.name = "M5x0.8",
		.step = 50,
		.next = &thread[4],
	}, {
		.name = "M6x1.0",
		.step = 40,
		.next = &thread[5],
	}, {
		.name = "M8x1.25",
		.step = 32,
		.next = &thread[6],
	}, {
		.name = "M10x1.5",
		.step = 27,
		.next = &thread[7],
	}, {
		.name = "M12x1.75",
		.step = 23,
		.next = &thread[8],
	}, {
		.name = "M14x2.0",
		.step = 20,
		.next = &thread[9],
	}, {
		.name = "M16x2.0",
		.step = 20,
		.next = &thread[0],
	}
};

static struct thread *current_thread = (struct thread *)&thread[0];
static SemaphoreHandle_t wait;

static void enc_btn_handler(void *arg)
{
	bool *proceed = (bool *)arg;
	*proceed = true;
	xSemaphoreGive(wait);
}

static void enc_rol_handler(void *arg)
{
	current_thread = (struct thread *)current_thread->next;
	xSemaphoreGive(wait);
}

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
	buttons *btn = new buttons();
	btn->add(ENC_BTN, enc_btn_handler, NEGEDGE, &proceed);
	btn->add(ENC_A, enc_rol_handler);
	btn->add(BTN1, btn1_handler, NEGEDGE, &is_done);

	while (1) {
		LCD->clear();
		LCD->print(FIRST_ROW, CENTER, "%s %s",
			current_thread->name, arg ? "LEFT" : "RIGHT");
		xSemaphoreTake(wait, portMAX_DELAY);

		if (is_done || proceed)
			break;
	}

	vSemaphoreDelete(wait);
	delete(btn);

	if (proceed)
		thread_cut(current_thread->name, current_thread->step, dir);

	return 0;
}
