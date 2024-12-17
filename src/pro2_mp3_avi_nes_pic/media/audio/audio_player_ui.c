#include "audio_player_ui.h"
#include "libc.h"

//#define  WAVE_CENTER_Y  180         //pixel
//#define  WAVE_MAX_HIGH  50          //pixel

//#define  PCM_DOT_SKIP      4        //每4个点取一个 (1152/320)上取整为4
//#define  PCM_DOT_COLOR     LCD_CYAN
//#define  PCM_BORDER_COLOR  LCD_LIGHTGRAY
//#define  PCM_CENTER_COLOR  LCD_LIGHTCYAN

static void filling_dot_8b(const wave_disp_t *wave_attr, uint32_t pcm_value, uint16_t *color_buff)
{
	uint16_t color_t;
	int scale_vale;
	int i, start, end;
	scale_vale = (pcm_value*(wave_attr->wave_max_hight*2-1))/255;   //[0, 255]-->[0, WAVE_MAX_HIGH*2-1]

	color_buff[0] = color24to16(wave_attr->wave_border_color);
	color_buff[wave_attr->wave_max_hight*2 - 1] = color24to16(wave_attr->wave_center_color);

	if(scale_vale == 0)
	{
		color_t = color24to16(lcd_dev.backcolor);
		end = wave_attr->wave_max_hight*2-1;
		for(i=1; i<end; i+=1) {
			color_buff[i] = color_t;
		}
	}
	else
	{
		color_t = color24to16(wave_attr->wave_draw_color);
		start = wave_attr->wave_max_hight*2-1-1;
		end   = wave_attr->wave_max_hight*2-scale_vale-1;
		for(i=start; i>=end; i-=1) {
			color_buff[i] = color_t;
		}

		color_t = color24to16(lcd_dev.backcolor);
		start = wave_attr->wave_max_hight*2-scale_vale-1-1;
		for(i=start; i>0; i-=1) {
			color_buff[i] = color_t;
		}
	}
}

static void filling_dot_16b(const wave_disp_t *wave_attr, int pcm_value, uint16_t *color_buff)
{
	uint16_t color_t;
	int scale_vale;
	int i, start, end;

	if(pcm_value >= 0)
		scale_vale = (pcm_value*(wave_attr->wave_max_hight-1))/32767;
	else
		scale_vale = (pcm_value*(wave_attr->wave_max_hight))/32768;

	color_buff[0] = color24to16(wave_attr->wave_border_color);
	color_buff[wave_attr->wave_max_hight - 1]  = color24to16(wave_attr->wave_center_color);
	color_buff[wave_attr->wave_max_hight*2 - 1] = color24to16(wave_attr->wave_border_color);

	if(scale_vale == 0)
	{
		color_t = color24to16(lcd_dev.backcolor);
		end = wave_attr->wave_max_hight - 1;
		for(i=1; i<end; i+=1) {
			color_buff[i] = color_t;
		}

		start = wave_attr->wave_max_hight;
		end   = wave_attr->wave_max_hight*2 - 1;
		for(i=start; i<end; i+=1) {
			color_buff[i] = color_t;
		}
	}
	else if(scale_vale > 0)
	{
		color_t = color24to16(wave_attr->wave_draw_color);
		start = wave_attr->wave_max_hight-1-1;
		end   = wave_attr->wave_max_hight-scale_vale-1;
		for(i=start; i>=end; i-=1) {
			color_buff[i] = color_t;
		}
		color_t = color24to16(lcd_dev.backcolor);
		start = wave_attr->wave_max_hight-scale_vale-1-1;
		for(i=start; i>0; i-=1) {
			color_buff[i] = color_t;
		}

		color_t = color24to16(lcd_dev.backcolor);
		start = wave_attr->wave_max_hight;
		end   = wave_attr->wave_max_hight*2 - 1;
		for(i=start; i<end; i+=1) {
			color_buff[i] = color_t;
		}
	}
	else
	{
		scale_vale = -scale_vale;
		
		color_t = color24to16(lcd_dev.backcolor);
		end = wave_attr->wave_max_hight-1;
		for(i=1; i<end; i+=1) {
			color_buff[i] = color_t;
		}		

		color_t = color24to16(wave_attr->wave_draw_color);
		start = wave_attr->wave_max_hight;
		end   = wave_attr->wave_max_hight+scale_vale;
		for(i=start; i<end; i+=1) {
			color_buff[i] = color_t;
		}
		color_t = color24to16(lcd_dev.backcolor);
		start = wave_attr->wave_max_hight+scale_vale;
		end   = wave_attr->wave_max_hight*2 - 1;
		for(i=start; i<end; i+=1) {
			color_buff[i] = color_t;
		}
	}
}


//显示声音的时域波形
void lcd_draw_sound_wave(const wave_disp_t *wave_attr, uint8_t *pcm_buff, int pcm_pkg_size, int channels, int sample_bits)
{
	int16_t *s16_ptr;                     //16bits 采样点的数据
	uint8_t *u8_ptr;                      //8bits 采样点的数据
	uint16_t *wave_buff[2];                //指针数组
	int     sample_dot_cnt;                //本包数据包含的采样点个数(双声道一次采样算2个点)
    int     pick_dot_deta;                 //遍历采样点时的坐标增量
	int     i, lcd_sx, lcd_sy, lcd_ex, lcd_ey;

	//wave_attr->wave_max_hight太大时, 小心栈溢出
	wave_buff[0] = alloca(sizeof(uint16_t) * (wave_attr->wave_max_hight*2));  //alloca()编译器内置函数,用于申请栈内存, 不需要释放
	wave_buff[1] = alloca(sizeof(uint16_t) * (wave_attr->wave_max_hight*2));  //alloca()申请的栈内存地址8字节对齐(keil平台)

	if(sample_bits == 8)
	{	//==== 8bit sample ===
		sample_dot_cnt = pcm_pkg_size;         //本包数据采样点的个数
		pick_dot_deta = wave_attr->pick_dot_skip*2 * channels;  //双声道时只拾取一个声道的波形
		u8_ptr = (uint8_t *)pcm_buff;
	}
	else if(sample_bits == 16)
	{	//==== 16bit sample ===
		sample_dot_cnt = pcm_pkg_size / 2;    //本包数据采样点的个数
		pick_dot_deta  = wave_attr->pick_dot_skip * channels;
		s16_ptr = (int16_t *)pcm_buff;
	}
	else
	{
		//Unsupport
		return;
	}

	lcd_sx = (lcd_dev.xres - sample_dot_cnt/pick_dot_deta) / 2;
	lcd_ex = lcd_sx + sample_dot_cnt/pick_dot_deta - 1;
	lcd_sy = wave_attr->wave_center_y - wave_attr->wave_max_hight + 1;
	lcd_ey = wave_attr->wave_center_y + wave_attr->wave_max_hight;

	if(sample_bits == 8)
	{	//==== 8bit sample ===
		for(i=lcd_sx; i<=lcd_ex; i++)
		{
			filling_dot_8b(wave_attr, *u8_ptr, wave_buff[i&0x01]);
			lcd_dma_trans_area(i, lcd_sy, i, lcd_ey, wave_buff[i&0x01]);
			u8_ptr += pick_dot_deta;
		}
		lcd_dma_trans_release();          //等待DMA传输完成
	}
	else if(sample_bits == 16)
	{	//==== 16bit sample ===
		for(i=lcd_sx; i<=lcd_ex; i++)
		{
			filling_dot_16b(wave_attr, *s16_ptr, wave_buff[i&0x01]);
			lcd_dma_trans_area(i, lcd_sy, i, lcd_ey, wave_buff[i&0x01]);
			s16_ptr += pick_dot_deta;
		}
		lcd_dma_trans_release();          //等待DMA传输完成
	}
}

//清除声音的时域波形
void lcd_clear_sound_wave(const wave_disp_t *wave_attr)
{
	int lcd_sx, lcd_sy, lcd_ex, lcd_ey;

	lcd_sx = 0;
	lcd_ex = lcd_dev.xres - 1;
	lcd_sy = wave_attr->wave_center_y - wave_attr->wave_max_hight + 1;
	lcd_ey = wave_attr->wave_center_y + wave_attr->wave_max_hight;

	lcd_fill_area(lcd_sx, lcd_sy, lcd_ex, lcd_ey, lcd_dev.backcolor);
}


//====================================================================================================
//显示歌曲名
void audio_player_disp_song_name(const audio_player_page_t *page, char *name, int disp_width)
{
	int lcd_x, lcd_y, width, height;

	width = u_strlen(name) * page->font_size/2;
	height = page->font_size;
	lcd_x = (disp_width - width) / 2;
	lcd_y = page->song_y;
	lcd_show_string(lcd_x, lcd_y, width, height, page->font_size, name, 0, page->font_color);
}

//显示歌曲播放时间
void audio_player_disp_play_time(const audio_player_page_t *page, int total_time, int play_time)
{
	char buff[32];
	int lcd_x, lcd_y, width, height;

	u_snprintf(buff, sizeof(buff), "%02d:%02d/%02d:%02d", total_time/60, total_time%60, play_time/60, play_time%60);

	lcd_x = page->time_x;
	lcd_y = page->time_y;
	width = u_strlen(buff) * page->font_size/2;
	height = page->font_size;
	lcd_show_string(lcd_x, lcd_y, width, height, page->font_size, buff, 0, page->font_color);
}

//显示音量
#define  VOLUME_PREFIX_STR_LEN  5
void audio_player_disp_volume(const audio_player_page_t *page, char *volume_prefix, int volume)
{
	char buff[32];
	int lcd_x, lcd_y, width, height;

	lcd_y = page->volume_y;
	height = page->font_size;

	if(volume_prefix != NULL)
	{
		lcd_x = page->volume_x;
		width = u_strlen(volume_prefix) * page->font_size/2;
		lcd_show_string(lcd_x, lcd_y, width, height, page->font_size, volume_prefix, 0, page->font_color);
	}

	u_snprintf(buff, sizeof(buff), "%3d", volume);
	lcd_x = page->volume_x + VOLUME_PREFIX_STR_LEN * page->font_size/2;
	width = u_strlen(buff) * page->font_size/2;
	lcd_show_string(lcd_x, lcd_y, width, height, page->font_size, buff, 0, page->font_color);
}

//显示播放模式
void audio_player_disp_play_mode(const audio_player_page_t *page, char *mode)
{
	int lcd_x, lcd_y, width, height;

	lcd_x = page->mode_x;
	lcd_y = page->mode_y;
	width = u_strlen(mode) * page->font_size/2;
	height = page->font_size;
	lcd_show_string(lcd_x, lcd_y, width, height, page->font_size, mode, 0, page->font_color);	
}

//显示/清除暂停提示
#define  PAUSE_TIP_STR_LEN  4
void audio_player_disp_pause_tip(const audio_player_page_t *page, char *pause_tip_str)
{
	int str_len;
	int lcd_x, lcd_y, width, height;

	lcd_x = page->pause_tip_x;
	lcd_y = page->pause_tip_y;

	if(pause_tip_str == NULL)
	{
		width  = page->font_size/2 * PAUSE_TIP_STR_LEN;
		height = page->font_size;
		lcd_fill_area(lcd_x, lcd_y, lcd_x+width-1, lcd_y+height-1, page->back_color);
	}
	else
	{
		str_len = u_strlen(pause_tip_str);
		width =  page->font_size/2 * str_len;
		height = page->font_size;
		lcd_show_string(lcd_x, lcd_y, width, height, page->font_size, pause_tip_str, 0, page->pause_tip_color);	
	}
}

//显示/清除功能设置状态信息
#define  SET_TIP_STR_MAX_LEN  9
void audio_player_disp_set_tip(const audio_player_page_t *page, char *set_tip_str)
{
	int str_len;
	int lcd_x, lcd_y, width, height;

	lcd_x = page->set_tip_x;
	lcd_y = page->set_tip_y;

	if(set_tip_str == NULL)
	{
		width  = page->font_size/2 * SET_TIP_STR_MAX_LEN;
		height = page->font_size;
		lcd_fill_area(lcd_x, lcd_y, lcd_x+width-1, lcd_y+height-1, page->back_color);
	}
	else
	{
		str_len = u_strlen(set_tip_str);
		width =  page->font_size/2 * str_len;
		height = page->font_size;
		lcd_show_string(lcd_x, lcd_y, width, height, page->font_size, set_tip_str, 0, page->set_tip_color);
		if(str_len < SET_TIP_STR_MAX_LEN) {
			lcd_x += width;
			width = (SET_TIP_STR_MAX_LEN - str_len) * page->font_size/2;
			lcd_fill_area(lcd_x, lcd_y, lcd_x+width-1, lcd_y+height-1, page->back_color);
		}
	}	
}

//加载播放器界面
//void audio_player_disp_form_load(const audio_player_page_t *page)
//{
//	lcd_dev.backcolor = (*page).back_color;
//	lcd_clear(lcd_dev.backcolor);
//}




