#include <string.h>

#include "font.h"
#include "i2c.h"

#include "oled.h"

/*
 * Constants.
 */
#define OLED_I2C_DEV	"i2c-0"
#define OLED_I2C_ADDR	0x3C
#define COMMAND		0x80
#define DATA		0x40

/*
 *      buffer(index) <-> display
 * |   0|   1|   2|........| 126| 127|
 * | 128|....                   | 255|
 * | 256|....                   |    |
 * | 384|....                   |    |
 * | 512|....                   |    |
 * | 640|....                   |    |
 * | 768|....                   |    |
 * | 896| 897| 898|........     |1023|
 *
 * every || stands for one byte
 *
 *         |0|
 *         |1|
 *         |2|
 *  || --> |3|
 *         |4|
 *         |5|
 *         |6|
 *         |7|
 *
 * Therefore, there are 8 row, 128 col in total
 *
 * -----------------------------------------------
 *
 *                                  <-8 bits->
 * a 16x8 character -->    /\    | | | | | | | | |
 *                       16 bits | | | | | | | | |
 *                         \/
 * it has 2 row and 8 col
 *
 *                                              <-col->
 * a (8*row) x col character -->    /\    | | | | | | | | | |
 *                                (8*row) | | | | | | | | | |
 *                                  \/    | | | | | | | | | |
 *                                        | | | | | | | | | |
 */
static unsigned char buffer[1024];

/*
 * Initialize SSD1306.
 */
int oled_init()
{
	unsigned char init[] = {
		0xAE,		/* display OFF */
		COMMAND, 0x40,	/* set display start line */
		COMMAND, 0x81,	/* contrast control */
		COMMAND, 0xCF,	/* 128 */
		COMMAND, 0xA1,	/* set segment remap */
		COMMAND, 0xA6,	/* set display normal (not inverse) */
		COMMAND, 0xA8,	/* set multiplex ratio */
		COMMAND, 0x3F,	/* set duty 1/64 */
		COMMAND, 0xC8,	/* set com scan direction */
		COMMAND, 0xD3,	/* set display offset */
		COMMAND, 0x00,
		COMMAND, 0xD5,	/* set osc division */
		COMMAND, 0x80,
		COMMAND, 0xD9,	/* set pre-charge period */
		COMMAND, 0xF1,
		COMMAND, 0xDA,	/* set com pins */
		COMMAND, 0x12,
		COMMAND, 0xDB,	/* set vcomh */
		COMMAND, 0x30,
		COMMAND, 0x8D,	/* set charge pump on */
		COMMAND, 0x14,
		COMMAND, 0x20,	/* set memory addressing mode */
		COMMAND, 0x00,	/* horizontal addressing mode */
		COMMAND, 0xAF	/* display ON */
	};

	return i2c_writen_reg(OLED_I2C_DEV, OLED_I2C_ADDR, COMMAND, init, 47);
}

/*
 * Turn ON/OFF the screen.
 *      1 = ON
 *      0 = OFF
 */
int oled_turn_on_off(int state)
{
	if (state == 1)
		return i2c_write_reg(OLED_I2C_DEV, OLED_I2C_ADDR, COMMAND,
				     0xAF);
	else
		return i2c_write_reg(OLED_I2C_DEV, OLED_I2C_ADDR, COMMAND,
				     0xAE);
}

/*
 * Push buffer to GDDRAM to display it.
 */
int oled_redraw()
{
	return i2c_writen_reg(OLED_I2C_DEV, OLED_I2C_ADDR, DATA, buffer, 1024);
}

/*
 * Clear buffer.
 */
void oled_clear_buffer()
{
	memset(buffer, 0, sizeof(unsigned char) * 1024);
}

/*
 * Draw a pixel in buffer.
 *      0 <= x <= 127
 *      0 <= y <= 63
 *
 *  0------------------127>  X axis
 *  |
 *  |
 *  |
 *  |
 *  63
 *  \/  Y axis
 */
void oled_draw_pixel(int x, int y)
{
	buffer[((y & 0xf8) << 4) + x] |= 1 << (y & 7);
}

/*
 * Draw a character (or image) in buffer.
 * @row number of rows occupied by character,
 *      1 row == 8 bits == 1 byte, 1 <= row <= 8
 * @col number of columns occupied by character,
 *      1 col == 1 bit, 1 <= col <= 128
 * @font font of character
 * @offset begin position (index of buffer), 0 <= offset <= 1023
 */
void oled_draw_char(int row, int col, unsigned char *font, int offset)
{
	int i = 0;

	for (int c = 0; c < col; c++)
		for (int r = 0; r < row; r++)
			buffer[offset + (r << 7) + c] = font[i++];
}

/*
 * Print a string.
 * @str length <= 16, end with \0
 * @offset begin position (index of buffer), 0 <= offset <= 1023
 */
void oled_print(char *str, int offset)
{
	for (char *p = str; *p != 0; p++) {
		oled_draw_char(2, 8, ascii_font_2x8[(*p) - 32], offset);
		offset += 8;
	}
}
