#ifndef  __LCD_LIST_H_
#define  __LCD_LIST_H_

#include <stdint.h>
#include "fs_app_v2.h"

//flash常量
typedef struct list_style_s
{
	uint8_t title_font_size;
	uint8_t title_up_skip;
	uint8_t title_down_skip;
	uint8_t cnt_font_size;      //计数信息(总数/当前), 比如 24/2 
	uint32_t title_font_color;
	uint32_t title_back_color;

	uint8_t icon_size;
	uint8_t icon_left_skip;
	uint8_t icon_right_skip;

	uint8_t item_font_size;
	uint8_t item_up_skip;
	uint8_t item_down_skip;
	uint32_t item_font_color;
	uint32_t item_back_color;
	uint32_t item_sel_back_color;
}list_style_t;


//RAM变量
typedef struct lcd_list_s
{
	uint16_t  sy;                   //LCD y方向开始坐标
	char *title;
	fil_obj_t *item;                //指向item[], 需要malloc(sizeof(fil_obj_t)*total_item_num)
	int max_item_num;               //列表项最大数目, 即item[]数组元素最大个数
	int real_item_num;              //列表项实际数据, 即item[]数组元素实际个数
	int sel_item_idx;               //选中的列表项
	const list_style_t *style;
}lcd_list_t;

int lcd_list_refresh_all_item(lcd_list_t *list, int total_cnt, int cur_cnt, int flag);
int lcd_list_refresh_sel_item(lcd_list_t *list, int total_cnt, int cur_cnt, int old_sel_item_idx);

void lcd_list_load(lcd_list_t *list, dir_obj_t *dir, int lcd_sel_item_idx, int dir_pick_item_idx, int refresh_lcd);
void lcd_list_slide_up(lcd_list_t *list, dir_obj_t *dir, int refresh_lcd);
void lcd_list_slide_down(lcd_list_t *list, dir_obj_t *dir, int refresh_lcd);

fil_obj_t *lcd_list_get_sel_item(lcd_list_t *list);

#endif


