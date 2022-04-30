#ifndef __HARDWARE_H__
#define __HARDWARE_H__

/* lcd */
#define LCD_I2C_SDA			GPIO_NUM_32
#define LCD_I2C_SCL			GPIO_NUM_33

/* buttons */
#define BTN1				GPIO_NUM_21

/* encoder */
#define ENC_A				GPIO_NUM_2
#define ENC_B				GPIO_NUM_15
#define ENC_BTN				GPIO_NUM_19

/* stepper */
#define STP_CLK_PIN			GPIO_NUM_17
#define STP_DIR_PIN			GPIO_NUM_13
#define STP_ENA_PIN			GPIO_NUM_16
#define STP_TQ1_PIN			GPIO_NUM_5
#define STP_TQ2_PIN			GPIO_NUM_18

#endif /* __HARDWARE_H__ */
