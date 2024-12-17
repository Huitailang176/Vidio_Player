#include "lcd_button.h"
#include "libc.h"
#include "bmp_codec.h"

#include "my_printf.h"
#include "lcd_app_v2.h"
#include "fs_app_v2.h"

//显示一个按钮
static void draw_button(lcd_button_t *plb, int idx)
{
	int x, y, t;
	int lcd_sx, lcd_sy, lcd_ex, lcd_ey;
	const button_style_t *style = plb->style;

	x = (lcd_dev.xres - style->font_str_max_len * style->font_size/2) / 2;
	t = style->font_size + style->font_border_fill*2 + style->obj_space_fill*2;
	y = (lcd_dev.yres - plb->button_cnt*t)/2 + t*idx;

	lcd_sx = x - style->font_size;
	lcd_ex = x + (style->font_str_max_len * style->font_size/2) + style->font_size - 1;
	lcd_sy = y + style->obj_space_fill;
	lcd_ey = lcd_sy + style->font_border_fill + style->font_size + style->font_border_fill - 1;
	lcd_fill_area(lcd_sx, lcd_sy, lcd_ex, lcd_ey, style->back_color);

	lcd_sx = (lcd_dev.xres - u_strlen(plb->button[idx].name) * style->font_size/2) / 2;
	lcd_sy = y + style->obj_space_fill + style->font_border_fill;
	if(idx != plb->focus_idx)
	{
		lcd_show_string(lcd_sx, lcd_sy, lcd_dev.xres-lcd_sx, style->font_size, style->font_size, plb->button[idx].name, 0, style->font_color);
	}
	else
	{
		lcd_show_string(lcd_sx, lcd_sy, lcd_dev.xres-lcd_sx, style->font_size, style->font_size, plb->button[idx].name, 0, style->select_color);
		lcd_sx = x - style->font_size + style->font_size/4;
		lcd_show_string(lcd_sx, lcd_sy, lcd_dev.xres-lcd_sx, style->font_size, style->font_size, style->select_mark, 0, style->select_color);
	}
}

/**
 ** @brief 显示(加载)button界面
 ** @param plb-lcd_button界面指针
 **/
void lcd_button_form_load(lcd_button_t *plb)
{
	if(plb == NULL)
		return;

	int i, ret;

	if(plb->style->form_image != NULL)
	{
		ret = bmp_decode(plb->style->form_image);
		if(ret != 0)
		{
			my_printf("bmp_decode fail, ret = %d\n\r", ret);
			lcd_clear(plb->style->form_color);		
		}
	}
	else
	{
		my_printf("Not specify background image, Clear screen color: 0x%06X\n\r", plb->style->form_color);
		lcd_clear(plb->style->form_color);
	}

	lcd_dev.backcolor = plb->style->back_color;

	for(i=0; i<plb->button_cnt; i++)
	{
		draw_button(plb, i);
	}
}

/**
 ** @breif 切换选中的button (依次向上切换)
 ** @param plb-lcd_button界面指针
 **/
void lcd_button_focus_prev(lcd_button_t *plb)
{
	int old_idx = plb->focus_idx;

	plb->focus_idx--;
	if(plb->focus_idx < 0)
		plb->focus_idx = plb->button_cnt - 1;	

	draw_button(plb, old_idx);
	draw_button(plb, plb->focus_idx);	
}

/**
 ** @breif 切换选中的button (依次向下切换)
 ** @param plb-lcd_button界面指针
 **/
void lcd_button_focus_next(lcd_button_t *plb)
{
	int old_idx = plb->focus_idx;

	plb->focus_idx++;
	if(plb->focus_idx >= plb->button_cnt)
		plb->focus_idx = 0;

	draw_button(plb, old_idx);
	draw_button(plb, plb->focus_idx);	
}

/**
 ** @breif 执行当前选中的button命令
 ** @param plb-button界面指针
 **/
void lcd_button_focus_click(lcd_button_t *plb)
{
	int ret;
	int idx = plb->focus_idx;
	char *dir = plb->button[idx].work_dir;
	char *title = plb->button[idx].work_title;

	if(plb->button[idx].click != NULL)
	{
		ret = (*plb->button[idx].click)(dir, title);
		if(ret != 0)
			my_printf("[%s] %s error! ret = %d\n\r", __FUNCTION__, plb->button[idx].name, ret);
	}
}

/**
 ** @breif 返回当前选中的button结构体
 ** @param plb-button界面指针
 ** @reval button_t结构体指针
 **/
const button_t *lcd_button_get_focus(lcd_button_t *plb)
{
	int idx = plb->focus_idx;
	return &plb->button[idx];
}

/**
 ** @breif 扫描button访问目录下的文件, 对文件排序并生成list.txt文件
 ** @param pb-button指针
 **/
void lcd_button_work_dir_scan(const button_t *pb, int n)
{
	int i, order;

	for(i=0; i<n; i++)
	{
		if(pb[i].work_dir!=NULL && pb[i].user_var!=0)
		{
			my_printf("\nScan dir: %s\n\r", pb[i].work_dir);
			if(pb[i].user_var == 1)
				order = ORDER_SEQ;
			else
				order = ORDER_REV;
			dir_check_list(pb[i].work_dir, 1, order);
			dir_check_list_child(pb[i].work_dir, order);
		}
	}
}

