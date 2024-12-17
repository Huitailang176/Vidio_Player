#include "nes_game_support.h"
#include "lcd_app_v2.h"
#include "gamepad.h"
#include "audio_card.h"
#include "uart.h"

#include "share.h"
#include "my_malloc.h"
#include "my_printf.h"
#include "libc.h"
#include "kfifo.h"


//===================================NESPad========================================
//读取2个手柄的按键值
int nes_read_keypad(void)
{	
	return gamepad_read();
}


//======================================LCD========================================
/** @brief nes模拟器输出一行像素
 ** @param line_no = 行号, 0-239
 **        pixel_buff = 像素值, RGB565
 **        pixel_num  = 一行像素的个数, 固定为320
 **/
int nes_write_bitmap(int line_no, const uint16_t *pixel_buff, int pixel_num)
{
	if(line_no == 0)              //每一帧开始前重新设置窗口
	{
		//使DMA让出LCD控制权(DMA与CPU不能同时控制LCD)
		lcd_dma_trans_release();

		//320*300的图像显示在LCD(320*480竖屏)的中央
//		LCD_Set_Window(0, (lcd_dev.yres-300)/2, lcd_dev.xres-1, (lcd_dev.yres-300)/2 +300-1);

		//341*320的图像显示在LCD(480*320横屏)的中央
//		LCD_Set_Window((lcd_dev.xres-341)/2, 0, (lcd_dev.xres-341)/2 +341-1, lcd_dev.yres-1);

		LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1);		

		LCD_WriteRAM_Prepare();
	}

	lcd_dma_trans_line((uint16_t *)pixel_buff, pixel_num);   //DMA方式往LCD发送像素

	return 0;
}

void nes_show_string(char *str)
{
//	lcd_fill_area(0, lcd_dev.yres/2-16, lcd_dev.xres-1, lcd_dev.yres/2 -16+16-1, lcd_dev.backcolor);
	lcd_show_string(32, lcd_dev.yres/2-24, lcd_dev.xres-32, 24, 24, str, 0, lcd_dev.forecolor);
}

//======================================Audio========================================
static void *nes_pcm_buff;
static struct kfifo nes_pcm_fifo;
static void *nes_pcm_pkg_buff;
static int   nes_pcm_pkg_size;

static void nes_feeding_sound_card(void);

/** @brief nes模拟器配置声卡属性
 ** @param sample_freq = nes模拟器输出声音的频率
 **        channels    = nes模拟器输出声音的声道数
 **        sample_bits = nes模拟器输出声音的位数
 **        pcm_pkg_size= nes模拟器输出一包音频数据的大小
 **/
int nes_open_sound(int sample_freq, int channels, int sample_bits, int pcm_pkg_size)
{
	int ret;
	uint32_t buff_len;
	
	nes_pcm_pkg_size = pcm_pkg_size;
	
	buff_len = pcm_pkg_size*4;     //缓存4包即可
	buff_len = roundup_pow_of_two(buff_len);
	if(nes_pcm_buff == NULL)
	{
		nes_pcm_buff = mem_malloc(buff_len);
		if(nes_pcm_buff == NULL)
			return -1;
	}
	if(nes_pcm_pkg_buff == NULL)
	{
		nes_pcm_pkg_buff = mem_malloc(nes_pcm_pkg_size);
		if(nes_pcm_pkg_buff == NULL)
			return -2;
	}
	
	u_memset(nes_pcm_buff, 0x00, buff_len); 
	ret = kfifo_init(&nes_pcm_fifo, nes_pcm_buff, buff_len);
	if(ret != 0)
		return -3;

	u_memset(nes_pcm_pkg_buff, 0x00, nes_pcm_pkg_size);   //开启声卡
	ret = audio_card_open(sample_freq, channels, sample_bits, nes_pcm_pkg_buff, nes_pcm_pkg_size);
	if(ret != 0)
		return -4;

	audio_card_register_feeder(nes_feeding_sound_card);   //注册声卡喂食函数

	my_printf("[%s] sample_freq=%u\n\r", __FUNCTION__, sample_freq);

	return 0;
}

//声卡喂食函数
static void nes_feeding_sound_card(void)
{
	uint32_t ret = kfifo_len(&nes_pcm_fifo);
	
	if(ret == 0)
	{
		u_memset(nes_pcm_pkg_buff, 0x00, nes_pcm_pkg_size);
//		my_printf("L\n\r");
	}
	else
	{
		ret = kfifo_out(&nes_pcm_fifo, nes_pcm_pkg_buff, nes_pcm_pkg_size);
		if(ret == 0)
		{
			u_memset(nes_pcm_pkg_buff, 0x00, nes_pcm_pkg_size);
			my_printf("nes_pcm_fifo out error!!!\n\r");
		}
	}
	
	audio_card_write(nes_pcm_pkg_buff, nes_pcm_pkg_size);
}

/** @brief nes模拟器输出一包音频数据
 ** @param pcm_buff_ptr = 音频数据指针
 **        pcm_pkg_size = 音频数据的大小(此变量是个固定值)
 **/
int nes_write_pcm(void *pcm_buff_ptr, int pcm_pkg_size)
{
	uint32_t ret = kfifo_avail(&nes_pcm_fifo);
	if(ret < pcm_pkg_size)         //fifo已满
	{
		my_printf("O\n\r");
	}
	else
	{
		ret = kfifo_in(&nes_pcm_fifo, pcm_buff_ptr, pcm_pkg_size);
		if(ret != pcm_pkg_size)
			my_printf("audio_fifo in error!!!\n\r");
	}

	return 0;
}

//获取系统音量
int nes_get_sys_volume(void)
{
	return audio_card_get_volume();
}

//设置系统音量
int nes_set_sys_volume(int volume)
{
	return audio_card_chg_volume(volume);
}

//关闭系统声卡
int nes_close_sound(void)
{
	audio_card_close();
	
	mem_free(nes_pcm_buff);
	nes_pcm_buff = NULL;
	mem_free(nes_pcm_pkg_buff);
	nes_pcm_pkg_buff = NULL;
	
	return 0;
}



//====================================软件支持=======================================
void *nes_malloc(uint32_t size)
{
	return mem_malloc(size);
}

void *nes_malloc_align(uint32_t size, uint32_t addr_align)
{
	return mem_malloc_align(size, addr_align);
}

void nes_free(void *p)
{
	mem_free(p);
}

void *nes_memset(void* s, int c, size_t n)
{
	return u_memset(s, c, n);
}

void *nes_memcpy (void *dst, const void *src, size_t n)
{
	return u_memcpy(dst, src, n);
}

void *nes_memmem(const void* haystack, size_t hl, const void* needle, size_t nl)
{
	return u_memmem(haystack, hl, needle, nl);
}

size_t nes_strlen(const char *str)
{
	return u_strlen(str);
}

int nes_strcmp(const char *s1, const char *s2)
{
	return u_strcmp(s1, s2);
}

int nes_strncmp(const char *s1, const char *s2, size_t n)
{
	return u_strncmp(s1, s2, n);
}

char *nes_strncpy(char *dst, const char *src, size_t n)
{
	return u_strncpy(dst, src, n);
}

int  nes_snprintf(char * buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int i;
	
	va_start(args, fmt);
	i = u_vsnprintf(buf,size,fmt,args);
	va_end(args);
	return i;
}

void nes_printf(const char *fmt, ...)
{
	int len;
	char buff[256];   //不能太大,也不能太小
	va_list ap;
	
	va_start(ap, fmt);
	len = u_vsnprintf(buff, sizeof(buff), fmt, ap);  //len = strlen(buf); 不包括'\0'
	va_end(ap);

	uart1_write(buff, len);
}


void nes_fetch_str_matrix(char *str, void *window, int width, int height)
{
	fetch_str_matrix(str, window, width, height);
}


