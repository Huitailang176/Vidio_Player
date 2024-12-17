#include "button_menu.h"
#include "lcd_button.h"
#include "dir_list.h"

#include "libc.h"
#include "share.h"
#include "key.h"
#include "lcd_app_v2.h"

static const button_t start_button[] = 
{
	{"音乐播放器", "/music", "音乐列表", 1, dir_list_form_load},
	{"视频播放器", "/video", "视频列表", 1, dir_list_form_load},
	{"游戏机",     "/nes",   "游戏列表", 1, dir_list_form_load},
	{"图片浏览器", "/picture", "图片列表", 1, dir_list_form_load},
};

static const button_style_t button_style = 
{
	.form_image = "0:/sys/background_horizontal.bmp",
	.form_color = LCD_GREEN,

	.font_color = 0x000000,    //黑色
	.back_color = 0xE1E1E1,    //灰色
	.select_color = 0xFF00FF,  //粉色
	.select_mark = ">",

	.font_size = 24,        //支持12/16/24这三种字体大小
	.font_str_max_len = 10,
	.font_border_fill = 4,
	.obj_space_fill = 12,
};

static lcd_button_t button_menu = 
{
	.button = start_button,
	.style = &button_style,
	.button_cnt = ARRAY_SIZE(start_button),
	.focus_idx = 0,
};

//==========================================================
void work_dir_check_list(void)
{
	int lcd_x, lcd_y;
	char *const tip = "系统正在启动...";

	lcd_clear(lcd_dev.backcolor);
	lcd_x = (lcd_dev.xres - u_strlen(tip)*24/2)/2;
	lcd_y = lcd_dev.yres/2 - 24;
	lcd_show_string(lcd_x, lcd_y, lcd_dev.xres-lcd_x, 24, 24, tip, 0, lcd_dev.forecolor);

	lcd_button_work_dir_scan(button_menu.button, button_menu.button_cnt);
}


static void button_menu_click_action(void);
static void button_menu_move_prev(void);
static void button_menu_move_next(void);

void start_menu_load(void)
{
	//加载界面
	lcd_button_form_load(&button_menu);
	
	//注册按键
	key_register_cb(KEY_SEL, button_menu_click_action, NULL, NULL, 0, 0);
	if(button_menu.button_cnt > 1)
	{
		key_register_cb(KEY_DEC, button_menu_move_prev, NULL, NULL, 0, 0);
		key_register_cb(KEY_INC, button_menu_move_next, NULL, NULL, 0, 0);		
	}
	else
	{
		key_unregister_cb(KEY_DEC);
		key_unregister_cb(KEY_INC);		
	}
}

//==========================================================
//button menu界面下按键的功能
static void button_menu_click_action(void)
{
	lcd_button_focus_click(&button_menu);
}

static void button_menu_move_prev(void)
{
	lcd_button_focus_prev(&button_menu);
}

static void button_menu_move_next(void)
{
	lcd_button_focus_next(&button_menu);
}


