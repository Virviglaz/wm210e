#include "lcd.h"
#include <HD44780.h>
#include <esp32_i2c.h>
#include <free_rtos_h.h>
#include <string.h>
#include <log.h>
#include "hardware.h"

#define LCD_I2C_ADDR			0x27
#define LCD_TASK_SIZE			0x1000
#define LCD_QUEUE_SIZE			4
#define MAX_MESSAGE_SIZE		36
#define LCD_ROW_LENGHT			16

typedef struct {
	uint8_t cmd;
	uint8_t row, col;
	char text[MAX_MESSAGE_SIZE];
} *msg_t;

static i2c *i2c_bus;
lcd *LCD;

static void write(uint8_t data)
{
	i2c_bus->write_reg(LCD_I2C_ADDR, data, 0, 0);
}

void delay_func(uint16_t us)
{
	delay_ms(us / 1000 + 1);
}

static struct hd44780_conn conn;
static struct hd44780_lcd hd44780 = {
	.write = write,
	.write16 = 0,
	.delay_us = delay_func,
	.is_backlight_enabled = 1,
	.type = HD44780_TYPE_LCD,
	.font = HD44780_ENGLISH_RUSSIAN_FONT,
	.conn = &conn,
	.ext_con = 0,
};

lcd::lcd()
{
	i2c_bus = new i2c(LCD_I2C_SDA, LCD_I2C_SCL);
	queue = xQueueCreate(LCD_QUEUE_SIZE, sizeof(msg_t));
	xTaskCreate(lcd::handler, "lcd", LCD_TASK_SIZE, this, 1, &handle);
}

lcd::~lcd()
{
	vTaskDelete(handle);
	vQueueDelete(queue);
	delete(i2c_bus);
}

static msg_t prepare(char *dest, const char *format, va_list arg)
{
	msg_t msg = (msg_t)malloc(sizeof(*msg));

	vsnprintf(msg->text, MAX_MESSAGE_SIZE, format, arg);

	return msg;
}

void lcd::print(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	msg_t msg = prepare(msg->text, format, args);
	va_end(args);

	msg->row = 0;
	msg->col = 0;
	msg->cmd = 0;

	xQueueSend(queue, &msg, portMAX_DELAY);
}

void lcd::print(enum row_e row, uint8_t col, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	msg_t msg = prepare(msg->text, format, args);
	va_end(args);

	msg->row = (uint8_t)row;
	msg->col = col < LCD_ROW_LENGHT ? col : 0;
	msg->cmd = 0;

	xQueueSend(queue, &msg, portMAX_DELAY);
}

void lcd::print(enum row_e row, enum align a, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	msg_t msg = prepare(msg->text, format, args);
	va_end(args);

	uint8_t col = 0;

	switch (a) {
	case LEFT:
		break;
	case RIGHT:
		col = LCD_ROW_LENGHT - strlen(msg->text);
		break;
	case CENTER:
		col = LCD_ROW_LENGHT - strlen(msg->text);
		col /= 2;
	}

	msg->row = row < 2 ? row : 1;
	msg->col = col < LCD_ROW_LENGHT ? col : LCD_ROW_LENGHT - col;
	msg->cmd = 0;

	xQueueSend(queue, &msg, portMAX_DELAY);
}

void lcd::clear()
{
	msg_t msg = (msg_t)malloc(sizeof(*msg));

	msg->text[0] = 0;
	msg->row = 0;
	msg->col = 0;
	msg->cmd = 1;

	xQueueSend(queue, &msg, portMAX_DELAY);
}

void lcd::clear(enum row_e row)
{
	msg_t msg = (msg_t)malloc(sizeof(*msg));

	memset(msg->text, ' ', LCD_ROW_LENGHT);
	msg->text[LCD_ROW_LENGHT] = 0;
	msg->row = 0;
	msg->col = 0;
	msg->cmd = 0;

	xQueueSend(queue, &msg, portMAX_DELAY);
}

void lcd::handler(void *arg)
{
	lcd *l = (lcd *)arg;

	hd44780_pcf8574_con_init(&hd44780);
	hd44780_init(&hd44780);

	while (1) {
		msg_t msg;

		xQueueReceive(l->queue, &msg, portMAX_DELAY);

		if (msg->cmd)
			hd44780_send_cmd(msg->cmd);
		hd44780_set_pos(msg->row, msg->col);
		if (msg->text[0]) {
			hd44780_print(msg->text);
			INFO("%s", msg->text);
		}

		free(msg);
	}
}
