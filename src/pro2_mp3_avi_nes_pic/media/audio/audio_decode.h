#ifndef  __AUDIO_DECODE_H_
#define  __AUDIO_DECODE_H_


//audio_decode库对外提供的函数, 用户通过调用这些函数即可实现音频文件的播放
int audio_file_open(char *fname);
int audio_file_get_attr_info(int *sample_freq, int *channels, int *sample_bits);
int audio_file_decode_stream(void *pcm_buff, int size);
int audio_file_close(void);

int audio_file_seek(int pos);
int audio_file_get_played_time(void);
int audio_file_get_total_time(void);




#endif



