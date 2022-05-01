#include "platform.h"
#include "lcd.h"
#include "menu.h"

extern "C" {
	void app_main();
}

void app_main(void)
{
	/* create LCD */
	LCD = new lcd();

	while (1)
		menu_start();

	return;
}
