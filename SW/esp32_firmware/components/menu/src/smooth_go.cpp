#include "smooth_go.h"
#include "hardware.h"
#include "lcd.h"
#include "platform.h"
#include "esp_stepper.h"

const int SPEED_IN_STEPS_PER_SECOND = 100;
const int ACCELERATION_IN_STEPS_PER_SECOND = 100;
const int DECELERATION_IN_STEPS_PER_SECOND = 100;

static SemaphoreHandle_t wait;
int smooth_go_handler(int arg)
{
	bool proceed = false;
	wait = xSemaphoreCreateBinary();
	/*buttons *btn = new buttons();
	btn->add(ENC_BTN, enc_btn_handler, NEGEDGE, &proceed);
	btn->add(ENC_A, enc_rol_handler);*/
	stepper *motor = new stepper(STP_CLK_PIN, STP_DIR_PIN,
		SPEED_IN_STEPS_PER_SECOND,
		ACCELERATION_IN_STEPS_PER_SECOND,
		DECELERATION_IN_STEPS_PER_SECOND, 1, 10);
	motor_enable(true);
	INFO("Run 1!");
	motor->move(800);
	motor->wait_for_stop();
	INFO("Done!");

	delay_ms(1000);
	INFO("Run 2!");
	motor->move(-800);
	motor->wait_for_stop();
	INFO("Done!");

	delete(motor);
	motor_enable(false);

	while (1) {
		LCD->clear();
		xSemaphoreTake(wait, portMAX_DELAY);
		if (proceed)
			break;
	}

	vSemaphoreDelete(wait);
	//delete(btn);
	return 0;
}