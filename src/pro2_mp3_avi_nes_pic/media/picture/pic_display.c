#include "pic_display.h"
#include "bmp_codec.h"
#include "jpg_dec.h"

#include "libc.h"
#include "fs_app_v2.h"
#include "key.h"
#include "dir_list.h"
#include "my_printf.h"


static int key_register_mark = 0;

//按键功能
static void pic_browse_exit(void);
static void pic_browse_next(void);
static void pic_browse_prev(void);

//图片显示
int picture_display(char *fname)
{
	int ret;
	int file_type;

	file_type = get_file_obj_type(fname);
	if(file_type == BMP_FILE)
	{
		ret = bmp_decode(fname);
		if(ret != 0)
			my_printf("Error! Bmp decode fail, ret=%d\n\r", ret);
	}
	else if(file_type == JPG_FILE)
	{
		ret = jpeg_decode(fname);
		if(ret != 0)
			my_printf("Error! Jpg decode fail, ret=%d\n\r", ret);
	}
	else
	{
		my_printf("Waring! Unsupport picture file!!\n\r");
	}	

	if(ret==0 && key_register_mark==0)
	{
		key_register_mark = 1;

		//注册所有按键
		key_register_cb(KEY_SEL, pic_browse_exit, NULL, NULL, 0, 0);
		key_register_cb(KEY_DEC, pic_browse_prev, NULL, NULL, 0, 0);
		key_register_cb(KEY_INC, pic_browse_next,  NULL, NULL, 0, 0);
	}

	return ret;
}

void picture_display_close(void)
{
	key_register_mark = 0;
}

//===========================================================
//按键功能
static void pic_browse_exit(void)
{
	key_register_mark = 0;
	dir_list_form_reload();
}

static void pic_browse_next(void)
{
	dir_list_item_auto_run(1, NULL);
}

static void pic_browse_prev(void)
{
	dir_list_item_auto_run(-1, NULL);
}


