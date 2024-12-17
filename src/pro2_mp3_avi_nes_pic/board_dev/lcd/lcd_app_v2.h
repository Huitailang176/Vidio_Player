#ifndef  __LCD_APP_V2_H_
#define  __LCD_APP_V2_H_

#include "lcd_drv_fsmc407_v6.h"


void lcd_draw_circle(uint32_t x0, uint32_t y0, uint32_t r, uint32_t color);
void lcd_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void lcd_draw_rectangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);

void lcd_show_char(uint32_t x, uint32_t y, int num, int size, int mode, uint32_t color);
void lcd_show_string(uint32_t x, uint32_t y, uint32_t width, uint32_t height, int size, char *p, int mode, uint32_t color);

void lcd_show_cross(uint32_t x, uint32_t y, uint32_t r, uint32_t color);

void fetch_str_matrix(char *str, void *matrix_window, int width, int height);

//==================================================================================
typedef struct progress_bar_s
{
	int sx, sy;
	int ex, ey;
	uint32_t pass_color;
	uint32_t undue_color;
}progress_bar_t;

void lcd_draw_progress_bar(const progress_bar_t *bar, int value, int draw_undue);



#endif


