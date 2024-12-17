#include "audio_card.h"
#include "i2s.h"

#include <stddef.h>
#include "my_printf.h"
#include "my_malloc.h"
#include "libc.h"


//单声道数据扩充为双声道数据时的两种处理策略
#define  CHANNEL_DEAL_COPY        //复制到另一个声道
//#define  CHANNEL_DEAL_SET_ZERO  //另一个声道设为0


typedef struct dac_audio_obj_s
{
	/* 播放音频的相关属性 */
	int channels;       //声道数 1或者2
	int sample_freq;    //声音的采样频率, 比如8000Hz 16000Hz 44100Hz 48000Hz等
	int sample_bits;    //声音量化的bit数, 比如8bit 16bit
	
	/* 声卡的相关属性 */
	int volume;           //声卡的音量

	//For STM32F407 I2S
	uint8_t *pcm_data[2];    //双PCM数据缓存区
	uint32_t pcm_data_size;  //缓冲区的大小(单位: 字节) 
	uint32_t pkg_size;       //一包数据的大小
	int      cur_use_idx;    //当前所使用的PCM数据缓冲区
	void (*tx_callback)(void);   //I2S Controller发送完一包数据后的回调函数(提醒用户为pcm_data补充数据)
}audio_obj_t;

static audio_obj_t audio_obj;
static void dev_controller_tx_cb(uint32_t cur_buf_idx);


/** @brief 初始化声卡
 */
int audio_card_init(int volume)
{
	i2s_controller_init();

	audio_card_chg_volume(volume);
	return 0;
}

//I2S Controller发送完成后的回调函数
static void dev_controller_tx_cb(uint32_t cur_buf_idx)
{
	audio_obj.cur_use_idx = cur_buf_idx;

	if(audio_obj.tx_callback != NULL)
		(*audio_obj.tx_callback)();	
}


/** @brief 打开声卡
 ** @param sample_freq 声音的采样频率, 单位Hz, 比如8000  16000  22050  44100等
 **        channels    声道数, 1-单声道  2-双声道
 **        sample_bits 声音的采样量化比特数, 单位bit, 比如8 16
 **        pkg_buff    一包音频数据(可以是单声道也可以是双声道)
 **        pkg_size    一包音频数据的大小, 单位byte
 ** @reval 0   声卡开启成功
 **        非0 声卡打开失败
 */
int audio_card_open(int sample_freq, int channels, int sample_bits, void *pkg_buff, int pkg_size)
{
	int ret;

//	my_printf("Audio card start opening\n\r");    //debug
	if(audio_obj.pcm_data[0] != NULL)
	{	//如果声卡未关闭, 则要先关闭
		audio_card_close();
	}

	//=========================先检查传入参数是否合法===========================
	if(sample_freq < 2000)
		return -1;
	if((channels!=1) && (channels!=2))
		return -2;
	if((sample_bits!=8) && (sample_bits!=16))
		return -3;
	if(pkg_size < 16)
		return -4;
	audio_obj.channels = channels;
	audio_obj.sample_freq = sample_freq;
	audio_obj.sample_bits = sample_bits;
	audio_obj.pkg_size = pkg_size;         //一包数据的大小

	//========================分配内存===========================
	if(audio_obj.sample_bits == 16)
	{
		if(audio_obj.channels == 2)
			audio_obj.pcm_data_size = audio_obj.pkg_size;
		else
			audio_obj.pcm_data_size = audio_obj.pkg_size*2;       //单声道时,补充为双声道
	}
	else if(audio_obj.sample_bits == 8)
	{
		//8bit时把数据扩展为16bit
		if(audio_obj.channels == 2)
			audio_obj.pcm_data_size = audio_obj.pkg_size*2;
		else
			audio_obj.pcm_data_size = audio_obj.pkg_size*2*2;     //扩展为16bit后再补充为双声道
	}
	else
	{
		return -3;
	}
	audio_obj.pcm_data[0] = mem_malloc(audio_obj.pcm_data_size);
	audio_obj.pcm_data[1] = mem_malloc(audio_obj.pcm_data_size);
	if(audio_obj.pcm_data[0]==NULL || audio_obj.pcm_data[1]==NULL)
			return -5;	
	u_memset(audio_obj.pcm_data[0], 0x00, audio_obj.pcm_data_size);
	u_memset(audio_obj.pcm_data[1], 0x00, audio_obj.pcm_data_size);

	audio_obj.cur_use_idx = 0;

	//===================填充pcm_buff后面一包数据=================
	ret = audio_card_write(pkg_buff, pkg_size);
	if(ret != 0)
		return -6;

	//===================开始I2S Controller TX=================
	i2s_register_tx_cb(dev_controller_tx_cb);
	ret = i2s_controller_cfg(I2S_Standard_Phillips, I2S_DataFormat_16b, audio_obj.sample_freq);
	if(ret != 0) {
		audio_card_close();
		return -7;
	}

	ret = i2s_start_tx((uint16_t *)audio_obj.pcm_data[0], (uint16_t *)audio_obj.pcm_data[1], audio_obj.pcm_data_size/2);
	if(ret != 0) {
		audio_card_close();
		return -8;
	}

	return 0;
}

/** @breif 获取音频的采样频率
 ** @reval 音频的采样频率
 */
int audio_card_get_freq(void)
{
	return audio_obj.sample_freq;
}

/** @brief 改变(设置)音频的采样率
 ** @param sample_freq 音频采样率
 ** @reval 0   设置成功
 **        非0 设置失败
 */
int audio_card_chg_freq(int sample_freq)
{
	i2s_frequency_adjust(sample_freq);

	audio_obj.sample_freq = sample_freq;
	return 0;		
}

/** @brief 改变(设置)声卡音量
 ** @param volume 音量 范围[0,100]
 ** @reval 0   操作成功
 **        非0 操作失败
 */
int audio_card_chg_volume(int volume)
{
	if(volume < 0)
		volume = 0;

	if(volume > 100)
		volume = 100;

	audio_obj.volume = volume;

	return 0;
}

/** @brief 读取声卡音量
 ** @reval 声卡音量
 */
int audio_card_get_volume(void)
{
	return audio_obj.volume;
}

/** @brief 注册声卡缺pcm数据时的回调函数
 ** @param feeding_func 回调函数指针
 ** @reval 0   操作成功
 **        非0 操作失败
 */
int audio_card_register_feeder(void (*feeding_func)(void))
{
	audio_obj.tx_callback = feeding_func;
	return 0;
}


//===================4个DAC数据提取函数=======================
//8bit分辨率的音频以二进制无符号数存储, 数值范围[0, 255]
static void __pcm_data_extract_8b_1chl(void *src_dat, void *dst_dat, int src_size)
{
	int i, scale, src_dot, tar_dot;
	int t;
	uint8_t *src; 
	short   *dst;

	scale = audio_obj.volume;
	src_dot = src_size;

	src = src_dat;
	dst = dst_dat;

	for(i=0; i<src_dot; i++)
	{
		t = src[i];
//		t = t*65535/255 - 32768;
		t = t*257 - 32768;
		dst[i*2 + 0] = scale * t / 100;
#ifdef  CHANNEL_DEAL_COPY
		dst[i*2 + 1] = dst[i*2 + 0];
#else
		dst[i*2 + 1] = 0;
#endif
	}

	if(src_size < audio_obj.pkg_size)
	{
		tar_dot = audio_obj.pkg_size;
		while(i < tar_dot)
		{
			dst[i*2 + 0] = 0;
			dst[i*2 + 1] = 0;
			i++;
		}
	}
}

static void __pcm_data_extract_8b_2chl(void *src_dat, void *dst_dat, int src_size)
{
	int i, scale, src_dot, tar_dot;
	int t;
	uint8_t *src; 
	short   *dst;

	scale = audio_obj.volume;
	src_dot = src_size;

	src = src_dat;
	dst = dst_dat;

	for(i=0; i<src_dot; i++)
	{
		t = src[i];
//		t = t*65535/255 - 32768;
		t = t*257 - 32768;
		dst[i] = scale * t  / 100;
	}

	if(src_size < audio_obj.pkg_size)
	{
		tar_dot = audio_obj.pkg_size;
		while(i < tar_dot)
		{
			dst[i] = 0;
			i++;
		}
	}
}

//16bit分辨率的音频以二进制补码存储, 数值范围[-32768, 32767]
static void __pcm_data_extract_16b_1chl(void *src_dat, void *dst_dat, int src_size)
{
	int i, scale, src_dot, tar_dot;
	short *src, *dst;

	scale = audio_obj.volume;
	src_dot = src_size/2;

	src = src_dat;
	dst = dst_dat;

	for(i=0; i<src_dot; i++)
	{
		dst[i*2 + 0] = scale * src[i] / 100;  //把单声道扩充为双声道
#ifdef  CHANNEL_DEAL_COPY
		dst[i*2 + 1] = dst[i*2 + 0];      //复制到另一个声道
#else
		dst[i*2 + 1] = 0;                 //0 相当于把右声道静音
#endif
	}

	if(src_size < audio_obj.pkg_size)
	{
		tar_dot = audio_obj.pkg_size/2;
		while(i < tar_dot)
		{
			dst[i*2 + 0] = 0;
			dst[i*2 + 1] = 0;
			i++;
		}
	}
}

static void __pcm_data_extract_16b_2chl(void *src_dat, void *dst_dat, int src_size)
{
	int i, scale, src_dot, tar_dot;
	short *src, *dst;

	scale = audio_obj.volume;
	src_dot = src_size/2;

	src = src_dat;
	dst = dst_dat;

	for(i=0; i<src_dot; i++)
	{
		dst[i] = scale * src[i] / 100;
	}

	if(src_size < audio_obj.pkg_size)
	{
		tar_dot = audio_obj.pkg_size/2;
		while(i < tar_dot)
		{
			dst[i] = 0;
			i++;
		}
	}		
}


/** @brief 丢给声卡一包音频数据
 ** @param pkg_buff 音频数据指针(可以是 单声道/双声道 8b/16)
 **        pkg_size 音频数据的大小, 单位byte
 ** @reval 0   操作成功
 **        非0 操作失败
 */
int audio_card_write(void *pkg_buff, int pkg_size)
{
	if(audio_obj.pcm_data[0] == NULL)
		return -1;

	void *dst_data;
	int pre_idx = (audio_obj.cur_use_idx + 1)%2;  //获取空闲的PCM缓存区
	
	dst_data = audio_obj.pcm_data[pre_idx];
	
	//从pkg_buff提取数据到dst_data
	if(audio_obj.sample_bits == 16)
	{
		if(audio_obj.channels == 2)
			__pcm_data_extract_16b_2chl(pkg_buff, dst_data, pkg_size);
		else
			__pcm_data_extract_16b_1chl(pkg_buff, dst_data, pkg_size);
	}
	else if(audio_obj.sample_bits == 8)
	{
		if(audio_obj.channels == 2)
			__pcm_data_extract_8b_2chl(pkg_buff, dst_data, pkg_size);
		else
			__pcm_data_extract_8b_1chl(pkg_buff, dst_data, pkg_size);
	}
	else
	{
		return -2;
	}

	return 0;	
}


/** @brief 关闭声卡
 */
int audio_card_close(void)
{
//	my_printf("Audio card start closing\n\r");    //debug

	i2s_stop_tx();

	//=====================free memory============================
	if(audio_obj.pcm_data[0] != NULL)
	{
		mem_free(audio_obj.pcm_data[0]);
		audio_obj.pcm_data[0] = NULL;
	}
	if(audio_obj.pcm_data[1] != NULL)
	{
		mem_free(audio_obj.pcm_data[1]);
		audio_obj.pcm_data[1] = NULL;
	}
	
	audio_obj.tx_callback = NULL;
	audio_obj.cur_use_idx = 0;
	
	/* channels sample_freq sample_bits volume pkg_size这5个变量不同释放
	 */
	
//	my_printf("Audio card closed\n\r");   //debug
	return 0;
}





