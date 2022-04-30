#include "platform.h"
#include "lcd.h"

extern "C" {
	void app_main();
}

void app_main(void)
{
	lcd *LCD = new lcd();

	LCD->clear();
	LCD->print("Works");
	while (1)
		delay_ms(1000);

	return;
}
