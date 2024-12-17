#include "dec_mp3.h"

//需要MP3解码库的支持
#include "mp3dec.h"

#include <stddef.h>

static HMP3Decoder mp3_decoder;

//创建MP3解码对象
int mp3_decode_init(void)
{
	mp3_decoder = MP3InitDecoder();
	if(mp3_decoder == NULL)
		return -1;
	return 0;
}

//释放MP3解码对象
void mp3_decode_finish(void)
{
	MP3FreeDecoder(mp3_decoder);
	mp3_decoder = NULL;
}


/*
 * @brief: 解码MP3音频流,得到PCM数据
 * @reval: 负数-解码失败 非负数-解码得到数据量(单位-字节)
 */
int mp3_decode_stream(unsigned char *stream_buff, int stream_size, void *pcm_buff, int pcm_size)
{
	if(mp3_decoder == NULL)
		return -1;
	
	if(pcm_size < 2304*2)
		return -2;
	
	int offset = MP3FindSyncWord(stream_buff, stream_size);
	if(offset < 0)
		return -3;
	
	stream_buff += offset;
	stream_size -= offset;
	int ret = MP3Decode(mp3_decoder, &stream_buff, &stream_size, (short*)pcm_buff, 0);
	if(ret != 0)
		return -4;
	
	MP3FrameInfo mp3_frameinfo;
	MP3GetLastFrameInfo(mp3_decoder, &mp3_frameinfo);	
	return mp3_frameinfo.outputSamps*(mp3_frameinfo.bitsPerSample/8);
}





