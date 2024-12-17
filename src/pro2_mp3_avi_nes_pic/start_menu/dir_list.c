#include "dir_list.h"
#include "lcd_list.h"
#include "button_menu.h"
#include "run_app.h"

#include "lcd_app_v2.h"
#include "key.h"

#include "libc.h"
#include "my_printf.h"
#include "my_malloc.h"

//LCD list属性
static const list_style_t style1 = 
{
	.title_font_size = 16, 
	.title_up_skip = 4,
	.title_down_skip = 4,
	.cnt_font_size = 12,
	.title_font_color = LCD_WHITE,
	.title_back_color = 0x0078D7,     //亮蓝

	.icon_size = 16,
	.icon_left_skip = 4,
	.icon_right_skip = 4,

	.item_font_size = 16,
	.item_up_skip = 4,
	.item_down_skip = 4,
	.item_font_color = LCD_BLACK,
	.item_back_color = LCD_WHITE,
	.item_sel_back_color = LCD_LIGHTGRAY,     //0xE5E5E5-浅灰   LCD_LIGHTGRAY
};

static lcd_list_t lcd_list = 
{
	.sy = 0,           //LCD y方向开始坐标
	.title = NULL,
	.item = NULL,      //指向item[], 需要malloc(sizeof(fil_obj_t)*total_item_num)
	.max_item_num = (240-(16+4+4))/(16+4+4), //总列表项, 即item[]数组元素最多个数
	.real_item_num = 0,
	.sel_item_idx = 0, //选中的列表项
	.style = &style1,
};

static dir_obj_t *dir_obj = NULL;

static int dir_item_idx = 0;             //这2个量用于播放结束后恢复dir列表
static int lcd_sel_item_idx = 0;

static int dir_item_idx_sub = 0;
static int lcd_sel_item_idx_sub = 0;

//dir item列表状态下4个按键的回调函数
static void dir_item_close(void);
static void dir_item_select(void);
static void dir_item_slide_up(void);
static void dir_item_slide_down(void);

static void key_func_setting(int dir_item_num)
{
	if(dir_item_num == 0)
	{
		key_register_cb(KEY_SEL, NULL, dir_item_close, dir_item_close, 1000, 50);   //注册3个都为退出按键
		key_register_cb(KEY_DEC, NULL, dir_item_close, dir_item_close, 1000, 50);
		key_register_cb(KEY_INC, NULL, dir_item_close, dir_item_close, 1000, 50);
	}
	else if(dir_item_num == 1)
	{
		key_register_cb(KEY_SEL, NULL, dir_item_select, dir_item_close, 1000, 50); //只需注册1个按键(两个功能)
		key_unregister_cb(KEY_DEC);
		key_unregister_cb(KEY_INC);
	}
	else
	{
		key_register_cb(KEY_SEL, NULL, dir_item_select, dir_item_close, 1000, 50);  //注册所有按键
		key_register_cb(KEY_DEC, dir_item_slide_up, NULL, dir_item_slide_up, 1000, 50);
		key_register_cb(KEY_INC, dir_item_slide_down, NULL, dir_item_slide_down, 1000, 50);
	}
}

/**
 ** @brief 单击 button(按钮) 后的动作函数--在LCD上显示dir目录下的所有文件
 ** @param dir  --目录字符串(比如/music /picture)     
 **        title--dir列表标题
 ** @reval 0  --成功
 **        非0--失败
 **/
int dir_list_form_load(char *dir, char *title)
{
	if(lcd_list.item == NULL)
	{
		lcd_list.item = mem_malloc(sizeof(*lcd_list.item) * lcd_list.max_item_num);
		if(lcd_list.item == NULL) {
			return -1;
		}
	}
	if(title != NULL)
	{	lcd_list.title = title;	}

	if(dir_obj == NULL)
	{
		dir_obj = mem_malloc(sizeof(*dir_obj));
		if(dir_obj == NULL) {
			mem_free(lcd_list.item);
			lcd_list.item = NULL;
			return -2;
		}
		dir_obj->dir_list = NULL;
	}
	if(dir != NULL)
	{	u_strncpy(dir_obj->dir_name, dir, sizeof(dir_obj->dir_name));	}

	if(dir_obj->dir_list == NULL)
	{	dir_obj->dir_list = dir_list_create(dir_obj->dir_name);	}
	if(dir_obj->dir_list == NULL)
	{
		mem_free(dir_obj);
		dir_obj = NULL;
		mem_free(lcd_list.item);
		lcd_list.item = NULL;		
		return -3;
	}
	dir_obj->item_cnt = dir_get_obj_num(dir_obj->dir_list);
	dir_obj->dir_type = DIRECT_DIR;
	
	lcd_dev.backcolor = lcd_list.style->item_back_color;
	lcd_clear(lcd_dev.backcolor);
	lcd_list_load(&lcd_list, dir_obj, lcd_sel_item_idx, dir_item_idx, 1);
	key_func_setting(dir_obj->item_cnt);

	return 0;
}

//dir下子dir目录下文件列表显示到LCD
static int dir_list_form_load_sub(void)
{
	if(dir_obj->dir_list == NULL)
	{
		dir_obj->dir_list = dir_list_create(dir_obj->dir_name);
		if(dir_obj->dir_list == NULL)
			return -1;
	}

	dir_obj->item_cnt = dir_get_obj_num(dir_obj->dir_list);
	dir_obj->dir_type = SUB_DIR;

	lcd_dev.backcolor = lcd_list.style->item_back_color;
	lcd_clear(lcd_dev.backcolor);
	lcd_list_load(&lcd_list, dir_obj, lcd_sel_item_idx_sub, dir_item_idx_sub, 1);	
	key_func_setting(dir_obj->item_cnt);

	return 0;	
}

/**
 ** @brief 重新加载dir列表界面, 音乐/游戏等app退出后调用此函数来返回列表界面
 **/
void dir_list_form_reload(void)
{
	if(dir_obj->dir_type == DIRECT_DIR)
		dir_list_form_load(NULL, NULL);
	else
		dir_list_form_load_sub();
}

//====================================================================================
//dir列表界面下按键功能
//关闭dir
static void dir_item_close(void)
{
	if(dir_obj->dir_list != NULL)
	{
		dir_list_destroy(dir_obj->dir_list);
		dir_obj->dir_list = NULL;
	}

	if(dir_obj->dir_type == DIRECT_DIR)
	{	//从根目录退出
		mem_free(dir_obj);
		dir_obj = NULL;
		mem_free(lcd_list.item);
		lcd_list.item = NULL;

		dir_item_idx = 0;
		lcd_sel_item_idx = 0;

		start_menu_load();    //跳转到开始菜单
	}
	else
	{	//从子目录退出
		dir_item_idx_sub = 0;
		lcd_sel_item_idx_sub = 0;

		dir_back(dir_obj->dir_name);    //目录回退

		dir_list_form_load(NULL, NULL);
	}
}

//打开选中的item
static void dir_item_select(void)
{
	char fname[128];
	int ret;
	fil_obj_t *fil_obj_ptr;

	fil_obj_ptr = lcd_list_get_sel_item(&lcd_list);
	if(fil_obj_ptr->ftype != DIRECTORY)
	{	//用应用程序选中的文件
		if(dir_obj->dir_type == DIRECT_DIR) {
			//在根目录打开选中文件
			dir_item_idx = dir_obj->item_idx;
			lcd_sel_item_idx = lcd_list.sel_item_idx;
		}
		else {
			//在子目录打开选中文件
			dir_item_idx_sub = dir_obj->item_idx;
			lcd_sel_item_idx_sub = lcd_list.sel_item_idx;			
		}

		dir_list_destroy(dir_obj->dir_list);  //销毁文件链表, 释放内存, 否则可能没有足够的内存来运行应用程序
		dir_obj->dir_list = NULL;

		key_unregister_cb(KEY_SEL);           //应用程序会重新注册按键的功能
		key_unregister_cb(KEY_DEC);
		key_unregister_cb(KEY_INC);

		u_snprintf(fname, sizeof(fname), "%s/%s", dir_obj->dir_name, fil_obj_ptr->fname);
		ret = run_file_with_app(fname, fil_obj_ptr->ftype);  //用应用程序打开文件
		if(ret != 0) {
			lcd_dev.backcolor = lcd_list.style->item_back_color;
			lcd_clear(lcd_dev.backcolor);
			u_snprintf(fname, sizeof(fname), "app execute error, ret=%d", ret);
			lcd_show_string(0, 0, lcd_dev.xres, 16, 16, fname, 0, lcd_list.style->item_font_color);		

			key_register_cb(KEY_SEL, dir_list_form_reload, NULL, NULL, 0, 0);  //注册所有按键
			key_register_cb(KEY_DEC, dir_list_form_reload, NULL, NULL, 0, 0);
			key_register_cb(KEY_INC, dir_list_form_reload, NULL, NULL, 0, 0);
		}
	}
	else
	{	//打开选中的子目录
		dir_item_idx = dir_obj->item_idx;
		lcd_sel_item_idx = lcd_list.sel_item_idx;

		dir_increase(dir_obj->dir_name, fil_obj_ptr->fname);    //目录递增

		dir_list_destroy(dir_obj->dir_list);   //销毁根目录list
		dir_obj->dir_list = NULL;

		ret = dir_list_form_load_sub();
		if(ret != 0)
		{
			my_printf("Open child directory fail, ret=%d\n\r", ret);
			dir_list_form_load(NULL, NULL);
		}
	}
}

//上滑与下滑
static void dir_item_slide_up(void)
{
	lcd_list_slide_up(&lcd_list, dir_obj, 1);
}

static void dir_item_slide_down(void)
{
	lcd_list_slide_down(&lcd_list, dir_obj, 1);
}

//====================================================================================
/**
 ** @brief 音乐/图片等app界面下, 根据mode自动运行list item
 ** @param mode  0--运行当前item
 **              1--下滑一个
 **             -1--上滑一个
 **/
int dir_list_item_auto_run(int mode, char **fname_out)
{
	if(dir_obj==NULL || lcd_list.item==NULL)
		return -1;

	if(dir_obj->dir_list == NULL)
	{
		dir_obj->dir_list = dir_list_create(dir_obj->dir_name);
		if(dir_obj->dir_list == NULL)
			return -2;
	}
	dir_obj->item_cnt = dir_get_obj_num(dir_obj->dir_list);

	if(dir_obj->dir_type == DIRECT_DIR)
	{	//在根目录
		lcd_list_load(&lcd_list, dir_obj, lcd_sel_item_idx, dir_item_idx, 0);
	}
	else
	{	//在子目录
		lcd_list_load(&lcd_list, dir_obj, lcd_sel_item_idx_sub, dir_item_idx_sub, 0);		
	}	

	fil_obj_t *fil_obj_ptr;
	if(mode > 0)
		lcd_list_slide_down(&lcd_list, dir_obj, 0);
	else if(mode < 0)
		lcd_list_slide_up(&lcd_list, dir_obj, 0);
	else
		asm("nop");
	fil_obj_ptr = lcd_list_get_sel_item(&lcd_list);
	if(fil_obj_ptr->ftype == DIRECTORY)
	{	//目录特殊处理, 即恢复原样
		if(mode > 0)
			lcd_list_slide_up(&lcd_list, dir_obj, 0);
		else if(mode < 0)
			lcd_list_slide_down(&lcd_list, dir_obj, 0);
		else
			asm("nop");
		fil_obj_ptr = lcd_list_get_sel_item(&lcd_list);
	}

	if(dir_obj->dir_type == DIRECT_DIR)
	{	//在根目录
		dir_item_idx = dir_obj->item_idx;
		lcd_sel_item_idx = lcd_list.sel_item_idx;
	}
	else
	{	//在子目录
		dir_item_idx_sub = dir_obj->item_idx;
		lcd_sel_item_idx_sub = lcd_list.sel_item_idx;			
	}

	char fname[128];	
	int ret;

	dir_list_destroy(dir_obj->dir_list);  //销毁文件链表, 释放内存, 否则可能没有足够的内存来运行应用程序
	dir_obj->dir_list = NULL;

	if(fname_out != NULL)
	{	*fname_out = fil_obj_ptr->fname;	}
	u_snprintf(fname, sizeof(fname), "%s/%s", dir_obj->dir_name, fil_obj_ptr->fname);
	ret = run_file_with_app(fname, fil_obj_ptr->ftype);  //用应用程序打开文件
	if(ret != 0)
	{
		lcd_dev.backcolor = lcd_list.style->item_back_color;
		lcd_clear(lcd_dev.backcolor);
		u_snprintf(fname, sizeof(fname), "app auto execute error, ret=%d", ret);
		lcd_show_string(0, 0, lcd_dev.xres, 16, 16, fname, 0, lcd_list.style->item_font_color);
		return 1;
	}

	return 0;	
}

/**
 ** @brief 获取当前选中的item
 ** @param fname_out  item文件名输出
 ** @reval 0   成功            
 **        非0 失败    
 **/
int dir_list_get_cur_item(char **fname_out)
{
	if(dir_obj==NULL || lcd_list.item==NULL)
		return -1;

	fil_obj_t *fil_obj_ptr = lcd_list_get_sel_item(&lcd_list);
	if(fname_out != NULL)
	{	*fname_out = fil_obj_ptr->fname;	}

	return 0;
}


