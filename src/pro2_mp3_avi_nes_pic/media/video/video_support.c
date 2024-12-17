#include "video_support.h"

#include "libc.h"
#include "my_malloc.h"
#include "my_printf.h"
#include "uart.h"
#include "lcd_app_v2.h"
#include "audio_card.h"


int __attribute__((weak)) vid_open_sys_audio(int sample_freq, int channels, int sample_bits, void *pkg_buff, int pkg_size)
{
	//打开声卡
	
	//注册回调函数
	
	//检查音量

	//初始化kfifo

	return 0;
}
int __attribute__((weak)) vid_close_sys_audio(void)
{
	//直接关闭声卡即可

	return 0;
}

int __attribute__((weak)) vid_set_sys_volume(int volume)
{
	return audio_card_chg_volume(volume);
}

int __attribute__((weak)) vid_get_sys_volume(void)
{
	return audio_card_get_volume();
}

void __attribute__((weak)) vid_write_pcm_data(void *pcm_ptr, int size)
{
	//把pcm数据写入fifo
}

/** @brief 输出一行像素
 ** @param line_no 像素行号, 取值[1, 320]
 **        pixel   像素值(RGB565)
 **        pixel_num像素的个数, 320
 **/
void __attribute__((weak)) vid_write_bitmap_line(int line_no, uint16_t *pixel, int pixel_num)
{
	lcd_colorfill_area_16b(0, line_no-1, 319, line_no-1, pixel);
}

/** @breif 播放器定位 音量设置这2个控制状态结束后的回调函数
 **/
void __attribute__((weak)) vid_ctrl_panel_hide(void)
{

}


//==============================================================================================

void *vid_malloc(uint32_t size)
{
	void *ptr = mem_malloc(size);
	if(ptr == NULL)
		ptr = mem2_malloc(size);
	return ptr;
}

void vid_free(void *p)
{
	mem_free(p);
}

void *vid_memset(void* s, int c, size_t n)
{
	return u_memset(s, c, n);
}

void *vid_memcpy (void *dst, const void *src, size_t n)
{
	return u_memcpy(dst, src, n);
}

int  vid_strncmp(const char *s1, const char *s2, size_t n)
{
	return u_strncmp(s1, s2, n);
}

char *vid_strncpy(char *dst, const char *src, size_t n)
{
	return u_strncpy(dst, src, n);
}

int  vid_sscanf(const char * buf, const char * fmt, ...)
{
	va_list args;
	int i;

	va_start(args,fmt);
	i = u_vsscanf(buf,fmt,args);
	va_end(args);
	return i;
}

int  vid_snprintf(char * buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int i;
	
	va_start(args, fmt);
	i = u_vsnprintf(buf,size,fmt,args);
	va_end(args);
	return i;
}

void vid_printf(const char *fmt, ...)
{
	int len;
	char buff[256];   //不能太大,也不能太小
	va_list ap;
	
	va_start(ap, fmt);
	len = u_vsnprintf(buff, sizeof(buff), fmt, ap);  //len = strlen(buf); 不包括'\0'
	va_end(ap);

	uart1_write(buff, len);
}

void vid_printf_arr(const void *buff, int len, uint32_t start_addr)
{
	my_printf_arr(buff, len, start_addr);
}

void vid_fetch_str_matrix(char *str, void *matrix_window, int width, int height)
{
	fetch_str_matrix(str, matrix_window, width, height);	
}


