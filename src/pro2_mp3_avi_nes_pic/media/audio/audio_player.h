#ifndef  __AUDIO_PLAYER_H_
#define  __AUDIO_PLAYER_H_

#include <stdint.h>
#include <stddef.h>
#include "ff.h"


enum audio_player_state_e
{
	AUDIO_PLAYER_IDLE = 0,  //表明当前没有在播放音频文件(或者说音频文件播放完毕)
	AUDIO_PLAYER_PLAYING,   //表明当前处于音频播放状态
	AUDIO_PLAYER_PAUSE,     //表明当前处于播放暂停状态
	AUDIO_PLAYER_SEEK,      //表明当前处于快进/快退状态
};

enum player_mode_e
{
	SINGLE,        //单曲播放
	SINGLE_CYCLE,  //单曲循环
	CYCLIC_SONGS,  //循环曲目
};


//解码后pcm数据缓存区的大小
#define  PCM_BUFF_SIZE  2304*2

typedef struct audio_player_obj_s
{
	int     state;           //播放器的状态

	uint8_t __attribute__((aligned(4))) pcm_buff[PCM_BUFF_SIZE];
	int     pcm_pkg_size;    //解码后一包pcm数据的实际大小, pcm_pkg_size<=PCM_BUFF_SIZE

	int     sample_freq;     //音频的采样频率
	int     channels;        //音频的声道数
	int     sample_bits;     //音频的量化比特数(采样分辨率) 8 16 ...

	int     key_set_exit_time;
	int     key_set_identify;
	int     key_set_state_watch;

	int     wave_frame_cnt;
}audio_player_obj_t;



int  audio_player_start(char *fname);
void audio_player_cycle(void);
void audio_player_stop(void);

int audio_player_exec(char *fname);


#endif


