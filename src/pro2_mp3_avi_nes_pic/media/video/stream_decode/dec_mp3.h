#ifndef  __DEC_MP3_H_
#define  __DEC_MP3_H_


int mp3_decode_init(void);
void mp3_decode_finish(void);
int mp3_decode_stream(unsigned char *stream_buff, int stream_size, void *pcm_buff, int pcm_size);


#endif


