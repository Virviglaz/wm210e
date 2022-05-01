#include "thread_cut.h"
#include "lcd.h"
#include "platform.h"
#include "hardware.h"
#include "esp_buttons.h"

static const struct thread {
	const char *name;
	const uint32_t step;
	const struct thread *next;
} thread[] = { {
		.name = "M2x0.4",
		.step = 40,
		.next = &thread[1],
	}, {
		.name = "M3x0.5",
		.step = 50,
		.next = &thread[2],
	}, {
		.name = "M4x0.7",
		.step = 70,
		.next = &thread[3],
	}, {
		.name = "M5x0.8",
		.step = 80,
		.next = &thread[4],
	}, {
		.name = "M6x1.0",
		.step = 100,
		.next = &thread[5],
	}, {
		.name = "M8x1.25",
		.step = 125,
		.next = &thread[6],
	}, {
		.name = "M10x1.5",
		.step = 150,
		.next = &thread[7],
	}, {
		.name = "M12x1.75",
		.step = 175,
		.next = &thread[8],
	}, {
		.name = "M14x2.0",
		.step = 200,
		.next = &thread[9],
	}, {
		.name = "M16x2.0",
		.step = 200,
		.next = &thread[0],
	}
};

static struct thread *current_thread = (struct thread *)&thread[0];
static SemaphoreHandle_t wait;

static void btn_handler(void *arg)
{
	current_thread = (struct thread *)current_thread->next;
	xSemaphoreGive(wait);
}

int thread_cut_handler(int arg)
{
	bool is_done = false;
	wait = xSemaphoreCreateBinary();
	buttons *btn = new buttons();
	btn->add(BTN, btn_handler);

	while (1) {
		LCD->clear();
		LCD->print(FIRST_ROW, CENTER, "%s %s",
			current_thread->name, arg ? "RIGHT" : "LEFT");
		xSemaphoreTake(wait, portMAX_DELAY);
		if (is_done)
			break;
	}

	vSemaphoreDelete(wait);
	delete(btn);

	return 0;
}
