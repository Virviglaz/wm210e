#include "hardware.h"
#include "menu.h"
#include "wifi.h"

extern "C" {
	void app_main();
}

void app_main(void)
{
	wifi_init("Tower", "555666777", "WM210E");

	int res = hardware_init();
	if (res)
		return;

	while (1)
		menu_start();

	return;
}
