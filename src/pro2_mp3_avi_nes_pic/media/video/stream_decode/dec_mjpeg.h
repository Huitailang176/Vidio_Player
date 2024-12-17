#ifndef  __DEC_MJPEG_H_
#define  __DEC_MJPEG_H_

#include <stdint.h>

#define  LINE_PIXEL_NUM   320
#define  LINE_CNT         16

typedef struct player_dis_info_s
{
	uint32_t total_time;      //视频总时长
	uint32_t played_time;     //已播放时长
	int adjust;               //0-无调整 1-快进/快退 2-音量增/减
	int volume;               //音量
	int speed;                //倍速 0.75 1.00 1.25
	uint8_t pixel_mark[LINE_CNT][LINE_PIXEL_NUM];  //line buff
}player_dis_info_t;

int jpeg_decode_init(void);
void jpeg_decode_finish(void);
int jpeg_decode_frame(unsigned char *frame_buff, int frame_size, int flag, player_dis_info_t *dis_info_ptr);


#endif



