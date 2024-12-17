#include "lcd_list.h"
#include "lcd_app_v2.h"
#include "libc.h"
#include "my_printf.h"
#include "ico_decode.h"

typedef struct item_ico_s
{
	const int type;
	char *const ico;
}item_ico_t;

static const item_ico_t item_ico_tab[] =
{
	{DIRECTORY, "0:/sys\\icon//folder.ico"},
	{MP3_FILE, "0://sys//icon//music.ico"},
	{WAV_FILE, "0:\\sys\\icon\\music.ico"},
	{AAC_FILE, "0:sys/icon/music.ico"},
	{AVI_FILE, "0:/sys/icon/video.ico"},
	{MP4_FILE, "0:/sys/icon/video.ico"},
	{NES_FILE, "0:/sys/icon/pad.ico"},
	{JPG_FILE, "0:/sys/icon/picture.ico"},
	{BMP_FILE, "0:/sys/icon/picture.ico"},
	{UNKNOW_FILE, "0:/sys/icon/unknown.ico"},
};

static char *get_ico_name_by_type(int ftype)
{
	int i;
	for(i=0; i<sizeof(item_ico_tab)/sizeof(item_ico_tab[0]); i++)
	{
		if(ftype == item_ico_tab[i].type)
			return item_ico_tab[i].ico;
	}
	return item_ico_tab[i-1].ico;
}

static void draw_title(lcd_list_t *list)
{
	int lcd_sx, lcd_sy, lcd_ex, lcd_ey;
	const list_style_t *style = list->style;

	lcd_sx = 0;
	lcd_sy = list->sy;
	lcd_ex = lcd_dev.xres-1;
	lcd_ey = lcd_sy + style->title_up_skip + style->title_font_size + style->title_down_skip - 1;
	lcd_fill_area(lcd_sx, lcd_sy, lcd_ex, lcd_ey, lcd_dev.backcolor);

	lcd_sx = (lcd_dev.xres - u_strlen(list->title) * style->title_font_size/2) / 2;
	lcd_sy = list->sy + style->title_up_skip;
	lcd_show_string(lcd_sx, lcd_sy, lcd_dev.xres - lcd_sx, style->title_font_size, style->title_font_size, list->title, 0, style->title_font_color);	
}

static void draw_cnt(lcd_list_t *list, int total_cnt, int cur_cnt)
{
	int t, lcd_sx, lcd_sy;
	char buff[32];
	const list_style_t *style = list->style;

	u_snprintf(buff, sizeof(buff), "%3d/%-3d", total_cnt, cur_cnt);
//	lcd_sx = lcd_dev.xres/2 + (u_strlen(list->title)*style->title_font_size/2)/2;          //策略1-紧挨着title显示
	t = lcd_dev.xres/2 + u_strlen(list->title)*style->title_font_size/2/2;
	lcd_sx = t + ((lcd_dev.xres-t) -  u_strlen(buff)*style->cnt_font_size/2)/2;            //策略2-title后面居中显示 
	lcd_sy = list->sy + style->title_up_skip + style->title_font_size - style->cnt_font_size;
	lcd_show_string(lcd_sx, lcd_sy, lcd_dev.xres-lcd_sx, style->cnt_font_size, style->cnt_font_size, buff, 0, style->title_font_color);	
}

static void draw_item(lcd_list_t *list, int idx)
{
	int y;
	int lcd_sx, lcd_sy, lcd_ex, lcd_ey;
	const list_style_t *style = list->style;

	y = list->sy + style->title_up_skip + style->title_font_size + style->title_down_skip;
	y += (style->item_up_skip + style->item_font_size + style->item_down_skip) * idx;

	lcd_sx = 0;
	lcd_ex = lcd_sx + style->icon_left_skip - 1;
	lcd_sy = y + style->item_up_skip;
	lcd_ey = lcd_sy + style->item_font_size - 1;
	lcd_fill_area(lcd_sx, lcd_sy, lcd_ex, lcd_ey, lcd_dev.backcolor);

	lcd_sx = style->icon_left_skip + style->icon_size;
	lcd_ex = lcd_sx + style->icon_right_skip - 1;
	lcd_fill_area(lcd_sx, lcd_sy, lcd_ex, lcd_ey, lcd_dev.backcolor);

	lcd_sx = 0;
	lcd_ex = lcd_dev.xres - 1;
	lcd_sy = y;
	lcd_ey = lcd_sy + style->item_up_skip - 1;
	lcd_fill_area(lcd_sx, lcd_sy, lcd_ex, lcd_ey, lcd_dev.backcolor);

	lcd_sy = y + style->item_up_skip + style->item_font_size;
	lcd_ey = lcd_sy + style->item_down_skip - 1;
	lcd_fill_area(lcd_sx, lcd_sy, lcd_ex, lcd_ey, lcd_dev.backcolor);

	lcd_sx = style->icon_left_skip + style->icon_size + style->icon_right_skip + u_strlen(list->item[idx].fname)*style->item_font_size/2;
	if(lcd_sx < lcd_dev.xres) {
		lcd_ex = lcd_dev.xres - 1;
		lcd_sy = y + style->item_up_skip;
		lcd_ey = lcd_sy + style->item_font_size - 1;
		lcd_fill_area(lcd_sx, lcd_sy, lcd_ex, lcd_ey, lcd_dev.backcolor);
	}

	lcd_sx = style->icon_left_skip;
	lcd_sy = y + (style->item_up_skip + style->item_font_size + style->item_down_skip - style->icon_size)/2;
	icon_decode(get_ico_name_by_type(list->item[idx].ftype), lcd_sx, lcd_sy);
	lcd_sx = style->icon_left_skip + style->icon_size + style->icon_right_skip;
	lcd_sy = y + style->item_up_skip;
	lcd_show_string(lcd_sx, lcd_sy, lcd_dev.xres-lcd_sx, style->item_font_size, style->item_font_size, list->item[idx].fname, 0, style->item_font_color);
}


//更新list->item[]后, 刷新整个屏幕
//flag 0-不刷新title
//     1-刷新title
int lcd_list_refresh_all_item(lcd_list_t *list, int total_cnt, int cur_cnt, int flag)
{
	int i;

	lcd_dev.backcolor = list->style->title_back_color;
	if( flag )
	{
		draw_title(list);
	}
	draw_cnt(list, total_cnt, cur_cnt);

	for(i=0; i<list->real_item_num; i++)
	{
		if(i == list->sel_item_idx)
		{	lcd_dev.backcolor = list->style->item_sel_back_color;	}
		else
		{	lcd_dev.backcolor = list->style->item_back_color;	}

		draw_item(list, i);
	}

	return 0;
}

//更新list->sel_item_idx后, 刷新2个条目
int lcd_list_refresh_sel_item(lcd_list_t *list, int total_cnt, int cur_cnt, int old_sel_item_idx)
{
	lcd_dev.backcolor = list->style->title_back_color;
	draw_cnt(list, total_cnt, cur_cnt);

	//取消之前选中的item
	lcd_dev.backcolor = list->style->item_back_color;
	draw_item(list, old_sel_item_idx);

	//设置现在选中的item
	lcd_dev.backcolor = list->style->item_sel_back_color;
	draw_item(list, list->sel_item_idx);

	return 0;
}


void lcd_list_load(lcd_list_t *list, dir_obj_t *dir, int lcd_sel_item_idx, int dir_pick_item_idx, int refresh_lcd)
{
	if(dir->item_cnt == 0)
	{
		lcd_show_string(0, 0, lcd_dev.xres, 24, 24, "NULL", 0, list->style->item_font_color);
	}
	else if(dir->item_cnt == 1)
	{
		//从dir->dir_list中拉取fil_obj到list->item, 并根据refresh_lcd判决是否显示到LCD
		dir->item_idx = 0;
		list->sel_item_idx = 0;
		list->real_item_num = 1;
		dir_read_obj_info(dir->dir_list, dir->item_idx, list->item, list->real_item_num);
		if( refresh_lcd )
		{	lcd_list_refresh_all_item(list, dir->item_cnt, dir->item_idx+list->sel_item_idx+1, 1);	}
	}
	else
	{
		//从dir->dir_list中拉取fil_obj到list->item, 并根据refresh_lcd判决是否显示到LCD
		dir->item_idx = dir_pick_item_idx;
		list->sel_item_idx = lcd_sel_item_idx;
		if(dir->item_cnt - dir->item_idx >= list->max_item_num)
			list->real_item_num = list->max_item_num;
		else
			list->real_item_num = dir->item_cnt - dir->item_idx;
		dir_read_obj_info(dir->dir_list, dir->item_idx, list->item, list->real_item_num);
		if( refresh_lcd )
		{	lcd_list_refresh_all_item(list, dir->item_cnt, dir->item_idx+list->sel_item_idx+1, 1);	}
	}
}

void lcd_list_slide_up(lcd_list_t *list, dir_obj_t *dir, int refresh_lcd)
{
	int old_sel_item_idx= list->sel_item_idx;

	list->sel_item_idx--;

	if(list->sel_item_idx < 0)
	{
		if(dir->item_idx > 0)
		{
			/* 还可以上滑 */
			dir->item_idx--;
			dir_read_obj_info(dir->dir_list, dir->item_idx, list->item, list->real_item_num);
			list->sel_item_idx = 0;
			if( refresh_lcd )
				lcd_list_refresh_all_item(list, dir->item_cnt, dir->item_idx+list->sel_item_idx+1, 0);		
		}
		else
		{	/* 已经上滑到了最顶部 */
			list->sel_item_idx = old_sel_item_idx;
		}
	}
	else
	{
		if( refresh_lcd )
			lcd_list_refresh_sel_item(list, dir->item_cnt, dir->item_idx+list->sel_item_idx+1, old_sel_item_idx);
	}	
}

void lcd_list_slide_down(lcd_list_t *list, dir_obj_t *dir, int refresh_lcd)
{
	int old_sel_item_idx = list->sel_item_idx;

	list->sel_item_idx++;

	if(list->sel_item_idx >= list->real_item_num)
	{
		if(dir->item_idx + list->real_item_num < dir->item_cnt)
		{	/* 还可以从list中拉取新的item */
			dir->item_idx++;
			dir_read_obj_info(dir->dir_list, dir->item_idx, list->item, list->real_item_num);
			list->sel_item_idx = list->real_item_num - 1;
			if( refresh_lcd )
				lcd_list_refresh_all_item(list, dir->item_cnt, dir->item_idx+list->sel_item_idx+1, 0);
		}
		else
		{	/* 无法再从list中拉取新的item了(已经访问到最底部了) */
			list->sel_item_idx = old_sel_item_idx;
		}
	}
	else
	{
		if( refresh_lcd )
			lcd_list_refresh_sel_item(list, dir->item_cnt, dir->item_idx+list->sel_item_idx+1, old_sel_item_idx);
	}
}

//获取当前选择的item
fil_obj_t *lcd_list_get_sel_item(lcd_list_t *list)
{
	int idx = list->sel_item_idx;
	return &list->item[idx];
}









