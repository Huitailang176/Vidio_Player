#ifndef  __AUDIO_PLAYER_UI_H_
#define  __AUDIO_PLAYER_UI_H_

#include <stdint.h>

#include "lcd_app_v2.h"

//Flash常量
typedef struct wave_disp_s
{
	uint16_t wave_center_y;    //波形中心y坐标
	uint16_t wave_max_hight;

	uint16_t pick_dot_skip;    //每多少个点取一个
	uint32_t wave_draw_color;
	uint32_t wave_border_color;
	uint32_t wave_center_color;
}wave_disp_t;

//Flash常量
typedef struct audio_player_page_s
{
	uint32_t back_color;
	uint32_t font_color;
	uint16_t font_size;

	uint16_t song_y;

	progress_bar_t progress_bar;

	uint16_t time_x;
	uint16_t time_y;

	uint16_t volume_x;
	uint16_t volume_y;

	uint16_t mode_x;
	uint16_t mode_y;

	uint32_t pause_tip_color;
	uint16_t pause_tip_x;
	uint16_t pause_tip_y;

	uint32_t set_tip_color;
	uint16_t set_tip_x;
	uint16_t set_tip_y;	
}audio_player_page_t;


void lcd_draw_sound_wave(const wave_disp_t *wave_attr, uint8_t *pcm_buff, int pcm_pkg_size, int channels, int sample_bits);
void lcd_clear_sound_wave(const wave_disp_t *wave_attr);

void audio_player_disp_song_name(const audio_player_page_t *page, char *name, int disp_width);
void audio_player_disp_play_time(const audio_player_page_t *page, int total_time, int play_time);
void audio_player_disp_volume(const audio_player_page_t *page, char *volume_prefix, int volume);
void audio_player_disp_play_mode(const audio_player_page_t *page, char *mode);
void audio_player_disp_pause_tip(const audio_player_page_t *page, char *pause_tip_str);
void audio_player_disp_set_tip(const audio_player_page_t *page, char *set_tip_str);


#endif

