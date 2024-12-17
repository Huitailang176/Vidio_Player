#ifndef  __VIDEO_PLAYER_H_
#define  __VIDEO_PLAYER_H_

#include <stdint.h>

#include "kfifo.h"

#include "video_app.h"

#define  AUDIO_PKG_SIZE   2304*2     //一包音频的大小, 即一帧MP3数据流解码后的大小
#define  AUDIO_BUFF_SIZE  32*1024    //缓存7包, 其实缓存4包即可, 缓存7包只是因为这个数必须为2^n
#define  AUDIO_PKG_NUM    (AUDIO_BUFF_SIZE)/(AUDIO_PKG_SIZE)    //缓存7包音频

enum video_player_state_e
{
	VIDEO_PLAYER_FREE = 0,  //表明当前没有在播放视频文件(或者说音频文件播放完毕)
	VIDEO_PLAYER_PLAYING,   //表明当前处于视频播放状态
	VIDEO_PLAYER_PAUSE,     //表明当前处于播放暂停状态
	VIDEO_PLAYER_FINISH,    //表明刚刚播放结束
};


typedef struct video_player_obj_s
{
	int      state;           //播放器的状态

	uint8_t  audio_buff[AUDIO_BUFF_SIZE]; //音频缓存区
	struct kfifo audio_fifo;
	uint32_t pcm_size;                    //一包音频解码后的大小, 单位:字节
	uint8_t  temp_buff[AUDIO_PKG_SIZE];
}video_player_obj_t;


int  video_player_start(char *fname);
void video_player_cycle(void);
void video_player_stop(void);

int video_player_seek(int pos);
int video_player_set_volume(int volume);
int video_player_pause(void);
int video_player_resume(void);

int video_player_exec(char *fname);


#endif


