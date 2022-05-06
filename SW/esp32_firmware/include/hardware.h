#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include <stdint.h>

/* lcd */
#define LCD_I2C_SDA			GPIO_NUM_12
#define LCD_I2C_SCL			GPIO_NUM_14

/* buttons */
#define BTN1				GPIO_NUM_16
#define BTN2				GPIO_NUM_17

/* front panel encoder */
#define ENC_A				GPIO_NUM_5
#define ENC_B				GPIO_NUM_18
#define ENC_BTN				GPIO_NUM_19

/* external encoder */
#define EXT_ENC_A			GPIO_NUM_23
#define EXT_ENC_B			GPIO_NUM_22
#define EXT_ENC_Z			GPIO_NUM_21

/* stepper */
#define STP_CLK_PIN			GPIO_NUM_26
#define STP_DIR_PIN			GPIO_NUM_33
#define STP_ENA_PIN			GPIO_NUM_25
#define STP_DIR_CCW			1

/* Fan */
#define FAN_ENA_PIN			GPIO_NUM_15

int hardware_init(void);
void fan_start();
void fan_stop();
void motor_enable(bool state);

/**
 * @brief Build string
 * 
 *
 * C:\Users\pasha\esp\esp-idf\components\esptool_py\esptool\esptool.py -p COM23 -b 460800 --before usb_reset --after hard_reset --chip esp32 write_flash --flash_mode dout --flash_freq 40m --flash_size detect 0x10000 esp32_firmware.bin 0x1000 bootloader/bootloader.bin 0x8000 partition_table/partition-table.bin 0xd000 ota_data_initial.bin
*/

#endif /* __HARDWARE_H__ */
