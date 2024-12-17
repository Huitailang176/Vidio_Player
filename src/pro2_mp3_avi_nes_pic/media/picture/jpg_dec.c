#include "jpg_dec.h"
#include "tjpgd.h"

#include "ff.h"
#include "libc.h"
#include "my_malloc.h"
#include "my_printf.h"
#include "lcd_app_v2.h"

#define  IMAGE_MAX_WIDTH  lcd_dev.xres
#define  IMAGE_MAX_HEIGHT lcd_dev.yres
//#define  IMAGE_MAX_WIDTH  320
//#define  IMAGE_MAX_HEIGHT 240

typedef struct bitmap_2_lcd_s
{
	int draw_width;
	int draw_height;
	int y_oft;
}bitmap_2_lcd_t;

static bitmap_2_lcd_t bitmap_2_lcd;

/* User defined call-back function to input JPEG data */
static unsigned int tjd_input (	/* Returns number of bytes read (zero on error) */
	JDEC* jd,			/* Decompressor object */
	uint8_t* buff,		/* Pointer to the read buffer (null to remove data) */
	unsigned int nd		/* Number of bytes to read/skip from input stream */
)
{
	UINT rb;
	FIL *fp = (FIL*)jd->device;


	if (buff) {	/* Read nd bytes from the input strem */
		f_read(fp, buff, nd, &rb);
		return rb;	/* Returns number of bytes could be read */

	} else {	/* Skip nd bytes on the input stream */
		return (f_lseek(fp, f_tell(fp) + nd) == FR_OK) ? nd : 0;
	}
}

/* User defined call-back function to output RGB bitmap */
static int tjd_output (
	JDEC* jd,		/* Decompressor object of current session */
	void* bitmap,	/* Bitmap data to be output */
	JRECT* rect		/* Rectangular region to output */
)
{
//	jd = jd;	/* Suppress warning (device identifier is not needed in this appication) */

//	my_printf("rect->top = %d\n\r", rect->top);       //debug 发现jpg图片以矩形块的形式解码
//	my_printf("rect->bottom = %d\n\r", rect->bottom);
//	my_printf("rect->left = %d\n\r", rect->left);
//	my_printf("rect->right = %d\n\r", rect->right);
//	my_printf("rectangle width = %d\n\r", rect->right - rect->left + 1);
//	my_printf("rectangle hight = %d\n\n\r", rect->bottom - rect->top + 1);

//	my_printf("scale:%d\n\r", jd->scale);
//	my_printf("width:%d\n\r", jd->width);
//	my_printf("height:%d\n\r", jd->height);

	int x_oft = 0;
	int y_oft = bitmap_2_lcd.y_oft;
	int lcd_sx, lcd_sy, lcd_ex, lcd_ey;
	int img_left, img_right, x_len, y_len, idx, use_len;
	uint16_t *pixel;

	if(bitmap_2_lcd.draw_width <= lcd_dev.xres)
	{	//image居中显示
		x_oft = (lcd_dev.xres - bitmap_2_lcd.draw_width) >> 1;

		lcd_sx = rect->left + x_oft;
		lcd_ex = rect->right + x_oft;
		lcd_sy = rect->top + y_oft;
		lcd_ey = rect->bottom + y_oft;

		lcd_colorfill_area_16b(lcd_sx, lcd_sy, lcd_ex, lcd_ey, bitmap);
	}
	else
	{	//拾取image居中的像素然后全显
		img_left  = (bitmap_2_lcd.draw_width - lcd_dev.xres) >> 1;
		img_right = img_left + lcd_dev.xres - 1;

		lcd_sy = rect->top + y_oft;
		lcd_ey = rect->bottom + y_oft;

		if(rect->left>=img_left && rect->right<=img_right) {
			//直接输出到LCD
			lcd_sx = rect->left - img_left;
			lcd_ex = rect->right - img_left;
			lcd_colorfill_area_16b(lcd_sx, lcd_sy, lcd_ex, lcd_ey, bitmap);
		}
		else if(rect->left<img_left && rect->right>=img_left) {
			//rect右半部分输出到LCD
			pixel = bitmap;
			x_len = rect->right - rect->left + 1;
			y_len = rect->bottom - rect->top + 1;
			use_len = rect->right - img_left + 1;
			idx = x_len - use_len;                //equal img_left - rect->left

			lcd_sx = 0;
			lcd_ex = rect->right - img_left;
			while(y_len > 0)
			{
				lcd_colorfill_area_16b(lcd_sx, lcd_sy, lcd_ex, lcd_sy, &pixel[idx]);
				lcd_sy ++;
				pixel += x_len;
				y_len --;
			}
		}
		else if(rect->left<=img_right && rect->right>img_right) {
			//rect左半部分输出到LCD
			pixel = bitmap;
			x_len = rect->right - rect->left + 1;
			y_len = rect->bottom - rect->top + 1;
			use_len = img_right - rect->left + 1;
			idx = 0;

			lcd_sx = rect->left - img_left;
			lcd_ex = img_right - img_left;
			while(y_len > 0)
			{
				lcd_colorfill_area_16b(lcd_sx, lcd_sy, lcd_ex, lcd_sy, &pixel[idx]);
				lcd_sy ++;
				pixel += x_len;
				y_len --;
			}
		}
		else {
			//直接丢弃
		}
	}

	return 1;               /* Continue decompression */
}


int jpeg_decode(const char *fname)
{
	int ret;
	FIL fil;

	void *jdwork;	/* Pointer to TJpgDec work area */
	JDEC jd;		/* TJpgDec decompression object */
	int scale;      //根据lcd的像素大小,可以自动缩小待显示的图片, 0-原比例显示 1-1/2 2-1/4 3-1/8
	                //(只是解码后改变图片高度与宽度的采样策略, 并不会缩小实际保存在磁盘中的图片)
	const uint32_t sz_work = 4096;	/* Size of working buffer for TJpgDec module */

	jdwork = mem_malloc(sz_work);
	if(jdwork == NULL) {
		my_printf("[%s] error! no memory!!\n\r", __FUNCTION__);
		return -1;
	}
	ret= f_open(&fil, fname, FA_READ);
	if( ret ) {
		mem_free(jdwork);
		my_printf("[%s] file open error!, ret=%d!!\n\r", __FUNCTION__, ret);
		return -2;
	}	
	
	/* 开始解码jpg图像 */
	ret = jd_prepare(&jd, tjd_input, jdwork, sz_work, &fil);  /* Prepare to decompress the file */
	if(ret == JDR_OK)
	{
		/* Determine scale factor */
		for(scale = 0; scale <= 3; scale++)
		{
			if ((jd.width >> scale) <= IMAGE_MAX_WIDTH && (jd.height >> scale) <= IMAGE_MAX_HEIGHT)
				break;
		}

		bitmap_2_lcd.draw_width = jd.width >> scale;
		bitmap_2_lcd.draw_height = jd.height >> scale;
		bitmap_2_lcd.y_oft = 0;
		if(bitmap_2_lcd.draw_width<lcd_dev.xres || bitmap_2_lcd.draw_height<lcd_dev.yres) //LCD显示不满
		{
			lcd_clear(LCD_BLACK);
	
			if(bitmap_2_lcd.draw_height < lcd_dev.yres)
				bitmap_2_lcd.y_oft = (lcd_dev.yres - bitmap_2_lcd.draw_height) >> 1;
		}
//		my_printf("jpg size: %u x %u\n\r", jd.width, jd.height);	/* Image size */
//		my_printf("Sampling ratio: %d:%d:%d\n\r", 4, 4 / jd.msx, (jd.msy == 2) ? 0 : (jd.msx == 1) ? 4 : 2);	/* Sampling ratio */
//		my_printf("jpg decode memory used: %u\n\r", JPG_DECODE_WORKBUFF_SIZE - jd.sz_pool);	/* Get used memory size by rest of memory pool */				
//		my_printf("jpg decode memory aviliable: %u\n\r", jd.sz_pool);                       /* Size of momory pool (bytes available) */
//		my_printf("LCD display scale: 1/%u\n\r", 1<<scale);

		ret = jd_decomp(&jd, tjd_output, scale);	/* Start decompression */
	}

	mem_free(jdwork);
	f_close(&fil);
	return ret;
}









