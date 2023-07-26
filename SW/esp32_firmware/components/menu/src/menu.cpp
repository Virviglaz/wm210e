#include "menu.h"
#include <free_rtos_h.h>
#include "hardware.h"
#include "feedrate.h"
#include "motor_ctrl.h"
#include <wifi.h>
#include <ota.h>
#include <log.h>
#include "esp_ota_ops.h"
#include "cpp_menu.h"

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

static int start_fw_update()
{
	const esp_app_desc_t *app_desc = esp_app_get_description();
	ota.version = app_desc->version;

	wifi_init("Tower", "555666777");
	ota_start(&ota);

	INFO("Firmware version: %s", ota.version);

	//LCD->clear();
	//LCD->print(FIRST_ROW, CENTER, "START FW UPDATE");
	delay_s(3);

	return 0;
}

typedef std::vector<MenuItem *> menu_t;

static FeedRateMenu thread_right("RIGHT", CW, thread_list);
static FeedRateMenu thread_left("LEFT", CCW, thread_list);
static FeedRateMenu feed_right("RIGHT", CW, feedrate_list);
static FeedRateMenu feed_left("LEFT", CCW, feedrate_list);

static MenuItem metric_thread("METRIC THREAD", menu_t {
	&thread_right,
	&thread_left
});

static MenuItem manual_feed("MANUAL FEED", menu_t {
	&feed_right,
	&feed_left
});

static MenuExe fw_update("RUN FW UPDATE", start_fw_update);
static MenuItem top("E-GEAR LATHE", menu_t {
	&metric_thread,
	&manual_feed,
	&fw_update
});

void menu_start(const char *version)
{
	lcd lcd;
	lcd.clear();
	lcd.print(FIRST_ROW, CENTER, "VERSION");
	lcd.print(SECOND_ROW, CENTER, "%s", version);
	delay_s(3);
	lcd.clear();

	Buttons btns;
	btns.add(ENC_BTN);
	btns.add(BTN1);
	btns.add(BTN2);

	MenuItem *current = &top;

	while (1) {
		current->update_lcd(lcd);

		switch (btns.wait())
		{
		case BUTTON_ENTER:
			current = current->enter(lcd, btns);
			break;
		case BUTTON_NEXT:
			current->next();
			break;
		case BUTTON_RETURN:
			current = current->back();
			break;
		/*case 3:
			current->prev();
			break;*/
		
		default:
			break;
		}
	}
}
