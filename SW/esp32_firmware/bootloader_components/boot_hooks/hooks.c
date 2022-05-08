#include "hal/gpio_hal.h"

/* 
 * Function used to tell the linker to include this file
 * with all its symbols.
 */
void bootloader_hooks_include(void) {}

void bootloader_before_init(void) {
	/* Keep in my mind that a lot of functions cannot be called from here
	* as system initialization has not been performed yet, including
	* BSS, SPI flash, or memory protection. */

	/* IO12 workaround */
	gpio_ll_output_enable(&GPIO, GPIO_NUM_12);
	gpio_ll_set_level(&GPIO, GPIO_NUM_12, 0);
}

void bootloader_after_init(void) {}
