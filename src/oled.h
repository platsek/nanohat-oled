#pragma once

/*
 * Commonly used offsets.
 */
#define LINE1	  0
#define LINE2	256
#define LINE3	512
#define LINE4	768

/*
 * Functions.
 */
int oled_init();
int oled_turn_on_off(int state);
int oled_redraw();
void oled_clear_buffer();
void oled_draw_pixel(int x, int y);
void oled_draw_char(int row, int col, unsigned char *font, int offset);
void oled_print(char *str, int offset);
