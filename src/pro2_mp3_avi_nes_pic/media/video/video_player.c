#include "video_player.h"
#include "video_support.h"

#include "audio_card.h"
#include "lcd_app_v2.h"
#include "my_printf.h"
#include "my_malloc.h"
#include "user_task.h"
#include "libc.h"

#include "media_timer_v2.h"
#include "cpu_related_v3.h"
#include "dir_list.h"

video_player_obj_t  *video_player_obj = NULL;

//视频播放器Task Control Block
static task_control_block_t tcb_video_player;
static void video_timer_start(void);


int  video_player_start(char *fname)
{
	int ret;
	
	if(video_player_obj != NULL)   //先关闭播放器
	{
		video_player_stop();
	}
	
	video_player_obj = mem_malloc(sizeof(video_player_obj_t));
	if(video_player_obj == NULL)
	{
		my_printf("maloc video_player_obj fail\n\r");
		return -1;
	}

//	ret = lcd_dma_init();
//	if(ret != 0)
//	{
//		mem_free(video_player_obj);
//		my_printf("lcd_dma_init fail, ret=%d\n\r", ret);
//		return -2;
//	}

	ret = video_open_file(fname);
	if(ret != 0)
	{
		mem_free(video_player_obj);
//		lcd_dma_deinit();
		my_printf("video_open_file() fail, ret=%d\n\r", ret);
		return -3;
	}

	video_player_obj->state = VIDEO_PLAYER_PLAYING;
	
	//整个屏幕刷为黑色
	lcd_clear(LCD_BLACK);

	//======================
	tcb_video_player.name = "video_player";
	tcb_video_player.state = TS_STOPPED;
	tcb_video_player.is_always_alive = 0;
	tcb_video_player.action = video_player_cycle;
	tcb_video_player.mark = 0;
	task_create(&tcb_video_player);

	task_wakeup(&tcb_video_player);

	video_timer_start();

	return 0;
}

static void video_feeding_sound_card(void);

int  vid_open_sys_audio(int sample_freq, int channels, int sample_bits, void *pkg_buff, int pkg_size)
{
	int ret;

	video_player_obj->pcm_size = pkg_size;

	//打开声卡
	ret = audio_card_open(sample_freq, channels, sample_bits, pkg_buff, pkg_size);
	if(ret != 0)
	{
		my_printf("[%s] dac_sound_start fail, ret=%d\n\r", __FUNCTION__, ret);
		return -1;
	}

	//注册回调函数
	audio_card_register_feeder(video_feeding_sound_card);
	
	//检查音量
//	ret = audio_card_get_volume();
//	if(ret < 28)
//	{
//		audio_card_chg_volume(28);
//	}

	//初始化kfifo
	ret = kfifo_init(&video_player_obj->audio_fifo, video_player_obj->audio_buff, AUDIO_BUFF_SIZE);
	if(ret != 0)
	{
		audio_card_close();
		return -2;
	}
	return 0;
}


int vid_close_sys_audio(void)
{
	audio_card_close();    //直接关闭声卡即可
	return 0;
}

//从fifo中取出pcm数据扔给声卡
static void video_feeding_sound_card(void)
{
	uint32_t ret = kfifo_len(&video_player_obj->audio_fifo);
	
	if(ret == 0)
	{
		u_memset(video_player_obj->temp_buff, 0x00, video_player_obj->pcm_size);
		my_printf("L\n\r");
	}
	else
	{
		ret = kfifo_out(&video_player_obj->audio_fifo, video_player_obj->temp_buff, video_player_obj->pcm_size);
		if(ret == 0)
		{
			u_memset(video_player_obj->temp_buff, 0x00, video_player_obj->pcm_size);
			my_printf("audio_fifo out error!!!\n\r");
		}
	}

	audio_card_write(video_player_obj->temp_buff, video_player_obj->pcm_size);

	tcb_video_player.mark |= 0x01;
	task_wakeup(&tcb_video_player);	
}


//把pcm数据写入fifo
void  vid_write_pcm_data(void *pcm_ptr, int size)
{
	int ret;
	
	ret = kfifo_avail(&video_player_obj->audio_fifo);
	if(ret > size)
	{
		if(video_player_obj->state != VIDEO_PLAYER_PLAYING)
			u_memset(pcm_ptr, 0x00, size);
		ret = kfifo_in(&video_player_obj->audio_fifo, pcm_ptr, size);
		if(ret != size)
		{
			my_printf("ret = %d\n\r", ret);
			my_printf("audio_fifo write error!\n\r");
		}
	}
	else
	{
		my_printf("O\n\r");
	}
}

/** @brief 输出一行像素
 ** @param line_no 像素行号, 取值[1, 240]
 **        pixel   像素值(RGB565)
 **        pixel_num像素的个数, 320
 **/
void vid_write_bitmap_line(int line_no, uint16_t *pixel, int pixel_num)
{
//	lcd_colorfill_area_16b(0, line_no-1, pixel_num-1,line_no-1, pixel);

	if(line_no == 1)
	{
		LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1);
		LCD_WriteRAM_Prepare();		
	}

	lcd_dma_trans_line(pixel, pixel_num);

	if(line_no == 240)
	{
		lcd_dma_trans_release();      //等待最后一行输出完成
	}
}

static void video_timer_isr(void);

static void video_timer_start(void)
{
	//启动一个1s的定时器
	//10KHz即100us, 100us*10000=1000ms=1s
	media_timer_start(10000, 10000, video_timer_isr);
}

static void video_timer_stop(void)
{
	media_timer_stop();	
}

//Timer定时周期 1000ms
static void video_timer_isr(void)
{
	tcb_video_player.mark |= 0x02;
	task_wakeup(&tcb_video_player);
}


void video_player_cycle(void)
{
	int ret, task_mark;
	int decode_cnt = 0;

	if(video_player_obj == NULL)
		return;	

	ALLOC_CPU_SR();
	ENTER_CRITICAL();
	task_mark = tcb_video_player.mark;
	tcb_video_player.mark = 0;
	EXIT_CRITICAL();

	if(likely(task_mark & 0x01))
	{
decode_loop:
		if(video_player_obj->state == VIDEO_PLAYER_PLAYING)
		{		
			ret = video_decode_stream();
			if(ret == 0)
			{	/* 解码成功 */
				if(kfifo_len(&video_player_obj->audio_fifo) < video_player_obj->pcm_size*6)   //缓存4包左右
				{
					decode_cnt++;
					if(decode_cnt > 8) {   //强行退出, 防止别的任务饿死
	//					my_printf("Giving up CPU\n\r");
						tcb_video_player.mark |= 0x01;
						task_wakeup(&tcb_video_player);
					} else {
						goto decode_loop;
					}
				}
			}
			else if(ret < 0)
			{	/* 解码失败 */
				my_printf("decode stream error, ret=%d\n\r", ret);
				vid_ctrl_panel_hide();
				video_player_stop();
			}
			else
			{	/* 解码结束 */
				vid_ctrl_panel_hide();
				video_player_stop();
//				task_mark = 0;
			}
		}
		else
		{
			u_memset(video_player_obj->temp_buff, 0x00, video_player_obj->pcm_size);
			vid_write_pcm_data(video_player_obj->temp_buff, video_player_obj->pcm_size);
		}
	}

	if(unlikely(task_mark & 0x02))
	{
		video_1s_hook();
	}
}

void video_player_stop(void)
{
	video_timer_stop();
	task_delete(&tcb_video_player);    //删除audio任务

	if(video_player_obj != NULL)
	{
		video_close();
//		lcd_dma_deinit();
		mem_free(video_player_obj);
		video_player_obj = NULL;
	}
}


//====================================================================
//播放器控制

/** @brief 随机定位视频文件
 ** @param pos-位置[0-99]
 */
int video_player_seek(int pos)
{
	int ret = 0;
	if(video_player_obj != NULL)
	{
		ret = video_seek(pos);
	}
	return ret;
}

/** @brief 设置播放器的音量
 ** @param volume-音量 [0,100]
 */
int video_player_set_volume(int volume)
{
	int ret = 0;
	if(video_player_obj != NULL)
	{
		ret = video_set_volume(volume);
	}
	return ret;
}

/**  @brief 暂停播放
 */
int video_player_pause(void)
{
	int ret = 0;
	if(video_player_obj!=NULL && video_player_obj->state==VIDEO_PLAYER_PLAYING)
	{
		ret = video_pause();
		if(ret == 0)
			video_player_obj->state = VIDEO_PLAYER_PAUSE;
	}
	return ret;
}

/** @brief 恢复播放
 */
int video_player_resume(void)
{
	int ret = 0;
	if(video_player_obj!=NULL && video_player_obj->state==VIDEO_PLAYER_PAUSE)
	{
		ret = video_resume();
		if(ret == 0)
			video_player_obj->state = VIDEO_PLAYER_PLAYING;
	}
	return 0;
}

//===========================================================================
//视频播放状态下按键调整
#include "key.h"

//video play状态下4个按键的回调函数
static void close_video(void);
static void set_state(void);
static void pause_resume_video(void);

int video_player_exec(char *fname)
{
	int ret;
	my_printf("video player execute: %s\n\r", fname);
	
	//注册所有按键
	key_register_cb(KEY_SEL, set_state, NULL, NULL, 0, 0);
	key_register_cb(KEY_DEC, pause_resume_video, NULL, NULL, 0, 0);
	key_register_cb(KEY_INC, close_video, NULL, NULL, 0, 0);

	ret = video_player_start(fname);
	return ret;
}

//视频播放状态下按键的功能
static void close_video(void)
{
	my_printf("Close video\n\r");
	video_player_stop();
	
	dir_list_form_reload();
}

//此按键用来改变其他2个按键的功能
static int key_func_flag = 0;

static void seek_video_inc(void);
static void seek_video_dec(void);
static void seek_video_end_cb(void);
static void set_video_volume_inc(void);
static void set_video_volume_dec(void);

static void set_state(void)
{	
	//非播放状态下设置音量后LCD上显示不出来
	if(video_player_obj==NULL || video_player_obj->state!=VIDEO_PLAYER_PLAYING)
		return;

	key_func_flag++;
	
	if(key_func_flag == 1)
	{	//播放器进入播放定位状态
		video_get_pos();
		key_register_cb(KEY_DEC, seek_video_dec, seek_video_end_cb, seek_video_dec, 600, 10);
		key_register_cb(KEY_INC, seek_video_inc, seek_video_end_cb, seek_video_inc, 600, 10);
	}
	else if(key_func_flag == 2)
	{	//播放器进入音量设置状态
		video_get_volume();
		key_register_cb(KEY_DEC, set_video_volume_dec, NULL, set_video_volume_dec, 500, 10);
		key_register_cb(KEY_INC, set_video_volume_inc, NULL, set_video_volume_inc, 500, 10);
	}
	else
	{	//结束对播放器的控制, 并回复2个按键的功能
		key_func_flag = 0;
		key_register_cb(KEY_DEC, pause_resume_video, NULL, NULL, 0, 0);
		key_register_cb(KEY_INC, close_video, NULL, NULL, 0, 0);
		video_resume();
	}
}

static void pause_resume_video(void)
{
	int ret = 0;
	
	if(video_player_obj != NULL)
	{
		if(video_player_obj->state == VIDEO_PLAYER_PLAYING)
		{
			ret = video_pause();
			if(ret == 0)
				video_player_obj->state = VIDEO_PLAYER_PAUSE;			
		}
		else
		{
			ret = video_resume();
			if(ret == 0)
				video_player_obj->state = VIDEO_PLAYER_PLAYING;		
		}
		
	}
}

//========================================================
static void seek_video_inc(void)
{
	int ret = 0;
	if(video_player_obj != NULL)
	{
		video_player_obj->state = VIDEO_PLAYER_PAUSE;
		int pos = video_get_pos();
		if(pos < 9800)
		{
			pos+=200;      //前进2%
			ret = video_seek(pos);
			if(ret != 0)
			{
				video_player_stop();
			}
		}
	}
}

static void seek_video_dec(void)
{
	int ret = 0;
	if(video_player_obj!= NULL)
	{
		video_player_obj->state = VIDEO_PLAYER_PAUSE;
		int pos = video_get_pos();
		if(pos > 200)
		{
			pos-=200;       //回退2%
			ret = video_seek(pos);
			if(ret != 0)
			{
				video_player_stop();
			}
		}
	}	
}

static void seek_video_end_cb(void)
{
	if(video_player_obj != NULL)
	{
		video_player_obj->state = VIDEO_PLAYER_PLAYING;
	}
}


static void set_video_volume_inc(void)
{
	if(video_player_obj!=NULL && video_player_obj->state==VIDEO_PLAYER_PLAYING)
	{
		int volume = video_get_volume();
		if(volume < 100)
		{
			volume++;
			video_set_volume(volume);
		}
	}	
}

static void set_video_volume_dec(void)
{
	if(video_player_obj!=NULL && video_player_obj->state==VIDEO_PLAYER_PLAYING)
	{
		int volume = video_get_volume();
		if(volume > 0)
		{
			volume--;
			video_set_volume(volume);
		}
	}	
}


/** @breif 播放定位 音量设置这2个控制状态结束后的回调函数
 **/
void vid_ctrl_panel_hide(void)
{
	//非播放状态下设置音量后LCD上显示不出来
	if(video_player_obj==NULL || video_player_obj->state!=VIDEO_PLAYER_PLAYING)
		return;

	if(key_func_flag != 0)
	{
		key_func_flag = 0;
		key_register_cb(KEY_DEC, pause_resume_video, NULL, NULL, 0, 0);
		key_register_cb(KEY_INC, close_video, NULL, NULL, 0, 0);
	}
}





