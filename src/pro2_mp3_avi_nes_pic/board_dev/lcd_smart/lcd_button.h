#ifndef  __LCD_BUTTON_H_
#define  __LCD_BUTTON_H_

#include <stdint.h>

//flash常量
typedef struct button_s
{
	char *name;
	char *work_dir;
	char *work_title;
	int  user_var;            //0-不扫描工作目录  1-正向扫描  2-逆向扫描
	int (*click)(char *dir, char *title);
}button_t;

//flash常量
typedef struct button_style_s
{
	char *form_image;
	uint32_t form_color;

	uint32_t font_color;
	uint32_t back_color;
	uint32_t select_color;
	char *select_mark;

	uint16_t font_size;
	uint16_t font_str_max_len;
	uint16_t font_border_fill;
	uint16_t obj_space_fill;
}button_style_t;

//RAM变量
typedef struct lcd_button_s
{
	const button_t *button;
	const button_style_t *style;
	int button_cnt;
	int focus_idx;
}lcd_button_t;


void lcd_button_form_load(lcd_button_t *plb);
void lcd_button_focus_prev(lcd_button_t *plb);
void lcd_button_focus_next(lcd_button_t *plb);
void lcd_button_focus_click(lcd_button_t *plb);

const button_t *lcd_button_get_focus(lcd_button_t *plb);
void lcd_button_work_dir_scan(const button_t *pb, int n);

#endif



