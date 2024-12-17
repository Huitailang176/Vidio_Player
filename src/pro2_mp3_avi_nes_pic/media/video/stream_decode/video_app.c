#include "video_app.h"

#include "parse_avi.h"
#include "dec_mjpeg.h"
#include "dec_mp3.h"

//依赖的外部函数
#include "video_support.h"
#include "ff.h"


#define  STREAM_SIZE  48*1024        //一帧480x320的jpeg流不会超过36KB
//#define  STREAM_SIZE  56*1024          //800*480

#define  AUDIO_PKG_SIZE   2304*2     //一包音频的大小, 即一帧MP3数据流解码后的大小

#define  DIS_TIME         8          //进度条 播放速度 音量等显示时间 


typedef struct video_obj_s
{
	FIL      fil;             //文件句柄
	int      state;           //播放器的状态

//	char     file_format[4];  //"AVI" "MP4"等, 现在就支持一种格式就没必要设置此变量

	uint8_t  stream_buff[STREAM_SIZE];    //从文件中读取的音频/视频流
	
	avi_info_t avi_info;      //如果还有mp4 flv等其他格式, 就使用union

	uint32_t block_end_ptr;   //一个大的音视频块结束位置
	uint32_t stream_size;	  //流大小,必须是偶数, 如果读取到为奇数,则加1补为偶数.
	uint16_t stream_id;       //流类型ID,'dc'=0X6364 'wb'==0X6277

	int      pcm_size;                    //一包音频解码后的大小, 单位:字节
	uint8_t  pcm_buff[AUDIO_PKG_SIZE];
	
	int      time_cnt;        //秒计数
	player_dis_info_t player_dis_info;
}video_obj_t;

static video_obj_t  *video_obj = NULL;

static void print_avi_info(avi_info_t *avi_info_ptr)
{
	vid_printf("avi_info_ptr->sec_per_frame = %u\n\r", avi_info_ptr->sec_per_frame);
	vid_printf("avi_info_ptr->total_frame = %u\n\r", avi_info_ptr->total_frame);
	vid_printf("avi_info_ptr->width = %u\n\r", avi_info_ptr->width);
	vid_printf("avi_info_ptr->height = %u\n\r", avi_info_ptr->height);
	vid_printf("avi_info_ptr->compression = 0x%X\n\r", avi_info_ptr->compression);
	
	vid_printf("avi_info_ptr->sample_rate = %u\n\r", avi_info_ptr->sample_rate);
	vid_printf("avi_info_ptr->channels = %u\n\r", avi_info_ptr->channels);
	vid_printf("avi_info_ptr->format_tag = %u\n\r", avi_info_ptr->format_tag);
	vid_printf("avi_info_ptr->block_align = %u\n\r", avi_info_ptr->block_align);
	
	vid_printf("avi_info_ptr->stream_start = %u\n\r", avi_info_ptr->stream_start);
	vid_printf("avi_info_ptr->stream_end = %u\n\r", avi_info_ptr->stream_end);
	vid_printf("avi_info_ptr->video_flag = %s\n\r", avi_info_ptr->video_flag);
	vid_printf("avi_info_ptr->audio_flag = %s\n\r", avi_info_ptr->audio_flag);
}

static int _video_get_audio_pkg_size(void);


int video_open_file(char *fname)
{
	int ret, avi_idx_chk_size;
	uint32_t rd_len;

	if(video_obj != NULL)    //说明正在播放视频, 要先关闭
	{
		video_close();
	}

	video_obj = vid_malloc(sizeof(video_obj_t));  //创建视频播放器对象
	if(video_obj == NULL)
	{
		return -1;
	}

	ret = f_open(&video_obj->fil, fname, FA_READ);
	if(ret != FR_OK)
	{
		vid_free(video_obj);
		video_obj = NULL;
		return -2;
	}

	ret = f_read(&video_obj->fil, video_obj->stream_buff, STREAM_SIZE, &rd_len);
	if(ret!=FR_OK || rd_len!=STREAM_SIZE)
	{
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -3;	
	}
	
	//获取avi文件的相关属性信息
	ret = avi_parse(video_obj->stream_buff, STREAM_SIZE, &video_obj->avi_info);
	if(ret < 0)
	{
		vid_printf("avi_parse fail, ret=%d\n\r", ret);
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -4;		
	}
	print_avi_info(&video_obj->avi_info);   //可以打印一下video_obj->avi_info
//	if(video_obj->avi_info.compression!=HANDLER_MJPG || video_obj->avi_info.format_tag!=AUDIO_FORMAT_MP3)
//	{	//仅支持图像JPEG压缩 声音线性PCM
//		f_close(&video_obj->fil);
//		vid_free(video_obj);
//		video_obj = NULL;
//		return -5;
//	}
	if(video_obj->avi_info.compression!=HANDLER_MJPG)
	{
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -5;		
	}

	/******** 处理IndexChunk **********/
	f_lseek(&video_obj->fil, video_obj->avi_info.stream_end);
	ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
	if(ret!=FR_OK || rd_len!=8)
	{
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -6;	
	}
	avi_idx_chk_size = avi_idx_chunk_size(video_obj->stream_buff, 8);
	vid_printf("\nIndexChunk size = %d\n\r", avi_idx_chk_size);
//	my_printf("f_size(&video_obj->fil) = %d\n\r", (uint32_t)f_size(&video_obj->fil));
//	my_printf("video_obj->avi_info.stream_end+avi_idx_chk_size = %d\n\r", video_obj->avi_info.stream_end+avi_idx_chk_size);

	/***** 说明还有第二个大块 ******/
	if(f_size(&video_obj->fil) > video_obj->avi_info.stream_end+avi_idx_chk_size)
	{
		f_lseek(&video_obj->fil, video_obj->avi_info.stream_end+avi_idx_chk_size);
		ret = f_read(&video_obj->fil, video_obj->stream_buff, STREAM_SIZE, &rd_len);
		if(ret != FR_OK)
		{
			f_close(&video_obj->fil);
			vid_free(video_obj);
			video_obj = NULL;
			return -7;
		}
		ret = avi_parse2(video_obj->stream_buff, rd_len, &video_obj->avi_info);   //相对于buff的偏移
		video_obj->avi_info.stream_start2 += video_obj->avi_info.stream_end + avi_idx_chk_size;  //转换为相对于文件的偏移
		video_obj->avi_info.stream_end2 += video_obj->avi_info.stream_end + avi_idx_chk_size;
	}
	else
	{	//标记为0, 说明没有第二个movi
		video_obj->avi_info.stream_start2 = 0;
		video_obj->avi_info.stream_end2 = 0;
	}

	video_obj->time_cnt = DIS_TIME;
	video_obj->player_dis_info.adjust = 0;
	video_obj->player_dis_info.speed = 100;

	ret = jpeg_decode_init();          //jpeg解码初始化
	if(ret != 0)
	{
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -8;
	}

	if(video_obj->avi_info.format_tag == AUDIO_FORMAT_MP3) {
		ret = mp3_decode_init();
		if(ret != 0)
		{
			jpeg_decode_finish();
			f_close(&video_obj->fil);
			vid_free(video_obj);
			video_obj = NULL;
			return -9;
		}
	}

	/************  先播放第一个块 ***********/
	video_obj->block_end_ptr = video_obj->avi_info.stream_end;  

	f_lseek(&video_obj->fil, video_obj->avi_info.stream_start);
	ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
	if(ret != FR_OK) {
		if(video_obj->avi_info.format_tag == AUDIO_FORMAT_MP3)
		{	mp3_decode_finish();	}
		jpeg_decode_finish();
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -10;		
	}
	ret = avi_get_stream_info(&video_obj->stream_buff[0], &video_obj->stream_id, &video_obj->stream_size);
	if(ret != 0)
	{
		if(video_obj->avi_info.format_tag == AUDIO_FORMAT_MP3)
		{	mp3_decode_finish();	}
		jpeg_decode_finish();
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -11;		
	}
	
	if(video_obj->avi_info.format_tag == AUDIO_FORMAT_MP3)
	{
		video_obj->pcm_size = _video_get_audio_pkg_size();
		vid_printf("video_obj->pcm_size = %d\n\r", video_obj->pcm_size);
		if(video_obj->pcm_size < 0)
		{
			mp3_decode_finish();
			jpeg_decode_finish();
			f_close(&video_obj->fil);
			vid_free(video_obj);
			video_obj = NULL;
			return -12;			
		}

		//开启声卡
		ret = vid_open_sys_audio(video_obj->avi_info.sample_rate, video_obj->avi_info.channels, 16, video_obj->pcm_buff, video_obj->pcm_size);
		if(ret < 0)
		{
			mp3_decode_finish();
			jpeg_decode_finish();
			f_close(&video_obj->fil);
			vid_free(video_obj);
			video_obj = NULL;
			return -13;			
		}
	}
	
	if(video_obj->avi_info.format_tag != AUDIO_FORMAT_MP3)
	{
		return video_obj->avi_info.sec_per_frame;
	}
	return 0;
}


static int _video_get_audio_pkg_size(void)
{
	int ret, pkg_size = 0;
	uint32_t rd_len;

decode_stream:
	while(video_obj->stream_size==0 || video_obj->stream_size+8>STREAM_SIZE)
	{	//异常帧(空帧与超大帧)处理, 不处理的话肯定会造成播放终止
		if(video_obj->stream_size == 0)
		{
			vid_printf("Warning! stream size is 0!!\n\r");
			vid_printf("NULL stream attribute data:\n\r");
			vid_printf_arr(video_obj->stream_buff, 8, 0);		
		}
		else
		{
			vid_printf("Warning! Stream too large, abandon it!!\n\r");
			vid_printf("video_obj->stream_size = %uB %uKB\n\r", video_obj->stream_size, video_obj->stream_size>>10);
			f_lseek(&video_obj->fil, video_obj->fil.fptr + video_obj->stream_size);			
		}
		
		ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
		if(ret != FR_OK) {
			return -1;
		}
		if(video_obj->fil.fptr >= video_obj->block_end_ptr)
		{
			vid_printf("video play finish\n\r");		
			return -2;
		}

		ret = avi_get_stream_info(&video_obj->stream_buff[0], &video_obj->stream_id, &video_obj->stream_size);
		if(ret != 0)
		{
			vid_printf("Error2! can't find stream start flag!!\n\r");
			return -4;
		}
	}
	
	/***************** 解码一帧视频流或者音频流 ******************/
	if(video_obj->stream_id == STREAM_VIDS_FLAG)
	{
		ret = f_read(&video_obj->fil, video_obj->stream_buff, video_obj->stream_size+8, &rd_len);
		if(ret != FR_OK) {
			vid_printf("Disk read error!\n\r");
			return -5;
		}

		//解码视频数据  初始化阶段没必要真解
//		ret = jpeg_decode_frame(video_obj->stream_buff, video_obj->stream_size, 0, 0);
//		if(ret != 0)
//		{
//			my_printf("jpeg decode error!\n\r");
//			return -3;
//		}
	}
	else
	{
		ret = f_read(&video_obj->fil, video_obj->stream_buff, video_obj->stream_size+8, &rd_len);
		if(ret != FR_OK) {
			vid_printf("Disk read error!\n\r");
			return -6;
		}
		
		//解码音频数据
		ret = mp3_decode_stream(video_obj->stream_buff, video_obj->stream_size, video_obj->pcm_buff, AUDIO_PKG_SIZE);
		if(ret < 0) {
			vid_printf("[%s] mp3 decode error! ignore!!\n\r", __FUNCTION__);
//			return -10;
		} else {
			pkg_size = ret;
		}
	}
	
	if(video_obj->fil.fptr >= video_obj->block_end_ptr)
	{
		vid_printf("video play finish\n\r");		
		return -7;		
	}
	
	//获取下一个待解码流的属性信息
	ret = avi_get_stream_info(&video_obj->stream_buff[video_obj->stream_size], &video_obj->stream_id, &video_obj->stream_size);
	if(ret != 0)
	{
		vid_printf("Get frame info error!\n\r");
		return -8;
	}
	if(pkg_size > 0)
	{
		return pkg_size;    //已经获取了音频数据包的大小, 直接返回即可
	}

	goto decode_stream;	
}


//获取视频的总时长  单位: s
static double __get_avi_total_time(void)
{
	double avi_total_time = (double)video_obj->avi_info.total_frame * video_obj->avi_info.sec_per_frame / 1000000;  //单位: s
	return avi_total_time;
}

//获取视频已经播放的时间  单位: s
static double __get_avi_played_time(void)
{
	double avi_total_time;
	double avi_palyed_time;
	uint32_t played_data_size;
	uint32_t total_data_size;

	avi_total_time = __get_avi_total_time();
	if(video_obj->avi_info.stream_start2 == 0)
	{
		played_data_size = video_obj->fil.fptr - video_obj->avi_info.stream_start;
		total_data_size = video_obj->avi_info.stream_end - video_obj->avi_info.stream_start;
	}
	else
	{
		if(video_obj->fil.fptr < video_obj->avi_info.stream_end)
			played_data_size = video_obj->fil.fptr - video_obj->avi_info.stream_start;
		else
			played_data_size = video_obj->avi_info.stream_end - video_obj->avi_info.stream_start + video_obj->fil.fptr - video_obj->avi_info.stream_start2;
		
		total_data_size = video_obj->avi_info.stream_end - video_obj->avi_info.stream_start + video_obj->avi_info.stream_end2 - video_obj->avi_info.stream_start2;	
	}
	avi_palyed_time = (double)played_data_size * avi_total_time / total_data_size;

	return avi_palyed_time;
}

int video_decode_stream(void)
{
	int ret, attr_oft;
	uint32_t rd_len;

	if(video_obj == NULL)  //解码结束
		return 1;
	
	while(video_obj->stream_size==0 || video_obj->stream_size+8>STREAM_SIZE)
	{	/************* 异常帧(空帧与超大帧)处理, 不处理的话肯定会造成播放终止 ***************/
		if(video_obj->stream_size == 0)
		{
			vid_printf("Warning! stream size is 0!!\n\r");
		}
		else
		{
			vid_printf("Warning! Stream too large, abandon it!!\n\r");
			vid_printf("video_obj->stream_size = %uB %uKB\n\r", video_obj->stream_size, video_obj->stream_size>>10);		
			f_lseek(&video_obj->fil, video_obj->fil.fptr + video_obj->stream_size);
		}
		
		//读取下一帧(新帧)的属性(帧头)信息
		ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
		if(ret != FR_OK) {
			return -2;
		}

get_stream_attr:
		//判断是否播放结束
		if(video_obj->fil.fptr >= video_obj->block_end_ptr)
		{
			vid_printf("change block or avi finish\n\r");
			vid_printf("fil.fptr = %u\n\r", (uint32_t)video_obj->fil.fptr);
			vid_printf("video_obj->block_end_ptr = %u\n\r", video_obj->block_end_ptr);
			vid_printf("video_obj->avi_info.stream_end = %u\n\r", video_obj->avi_info.stream_end);
			vid_printf("video_obj->avi_info.stream_start2 = %u\n\r", video_obj->avi_info.stream_start2);
			
			if(video_obj->avi_info.stream_end2==0 || video_obj->block_end_ptr==video_obj->avi_info.stream_end2) {
				vid_printf("video play finish\n\r");
				video_close();
				return 1;
			}
			else {
				video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
				f_lseek(&video_obj->fil, video_obj->avi_info.stream_start2);
				ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
				if(ret!=FR_OK || rd_len!=STREAM_SIZE) {
					vid_printf("ret=%d\n\r", ret);
					return -3;
				}
				vid_printf("change to second block ^_^\n\r");
			}
		}

		//获取新帧的属性信息
		video_obj->stream_size = 0;
		ret = avi_get_stream_info(&video_obj->stream_buff[0], &video_obj->stream_id, &video_obj->stream_size);
		if(ret != 0)
		{
			vid_printf("get stream info fail!\n\r");
			vid_printf("fil.fptr = %u\n\r", (uint32_t)video_obj->fil.fptr);
			vid_printf("video_obj->block_end_ptr = %u\n\r", video_obj->block_end_ptr);
			vid_printf("video_obj->avi_info.stream_end = %u\n\r", video_obj->avi_info.stream_end);
			vid_printf("video_obj->avi_info.stream_start2 = %u\n\r", video_obj->avi_info.stream_start2);
			vid_printf("stream attribute data:\n\r");
			vid_printf_arr(&video_obj->stream_buff[0], 8, 0);

			if(0 == vid_strncmp((const char *)&video_obj->stream_buff[0], "ix", 2))
			{
				uint32_t ix00_size = (uint32_t)video_obj->stream_buff[7]<<24 | \
									(uint32_t)video_obj->stream_buff[6]<<16 | \
									(uint32_t)video_obj->stream_buff[5]<<8 | \
									(uint32_t)video_obj->stream_buff[4];
				vid_printf("ix00_size = 0x%08X, %u\n\r", ix00_size, ix00_size);

				f_lseek(&video_obj->fil, video_obj->fil.fptr+ix00_size);
				ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
				if(ret != FR_OK) {
					vid_printf("ret=%d\n\r", ret);
					return -4;
				}
				vid_printf("After skip ix00_size, the next 8 byte:");
				vid_printf_arr(video_obj->stream_buff, 8, 0);
			
				video_obj->stream_size = 0;
				goto get_stream_attr;
			}				
			
			vid_printf("Error! can't find stream start flag!!\n\r");
			return -5;
		}
	}
	
	//======================================================================================
	//解码视频流或音频流
	if(video_obj->stream_id == STREAM_VIDS_FLAG)
	{
		ret = f_read(&video_obj->fil, video_obj->stream_buff, video_obj->stream_size+8, &rd_len);
		if(ret != FR_OK) {
			return -6;
		}

		//解码视频数据
		if(video_obj->time_cnt > 0)
		{
			video_obj->player_dis_info.total_time = __get_avi_total_time();
			video_obj->player_dis_info.played_time = __get_avi_played_time();
			video_obj->player_dis_info.volume = vid_get_sys_volume();
			ret = jpeg_decode_frame(video_obj->stream_buff, video_obj->stream_size, 1, &video_obj->player_dis_info);
		}
		else
		{
			ret = jpeg_decode_frame(video_obj->stream_buff, video_obj->stream_size, 0, 0);
		}
		if(ret != 0)
		{
//			return -2;
			vid_printf("jpeg decode fail, ignore!!\n\r");
		}
	}
	else
	{
		ret = f_read(&video_obj->fil, video_obj->stream_buff, video_obj->stream_size+8, &rd_len);
		if(ret != FR_OK) {
			return -7;
		}
		
		//解码音频数据
		ret = mp3_decode_stream(video_obj->stream_buff, video_obj->stream_size, video_obj->pcm_buff, AUDIO_PKG_SIZE);
		if(ret < 0)
		{
			vid_memset(video_obj->pcm_buff, 0x00, video_obj->pcm_size);
			vid_printf("mp3 decode fail, ignore!!\n\r");
		}
		
		vid_write_pcm_data(video_obj->pcm_buff, video_obj->pcm_size);      //输出音频
	}	
	
get_stream_attr2:
	//判断是否播放结束
	if(video_obj->fil.fptr >= video_obj->block_end_ptr)
	{
		vid_printf("change block or avi finish\n\r");
		vid_printf("fil.fptr = %u\n\r", (uint32_t)video_obj->fil.fptr);
		vid_printf("video_obj->block_end_ptr = %u\n\r", video_obj->block_end_ptr);
		vid_printf("video_obj->avi_info.stream_end = %u\n\r", video_obj->avi_info.stream_end);
		vid_printf("video_obj->avi_info.stream_start2 = %u\n\r", video_obj->avi_info.stream_start2);
		
		if(video_obj->avi_info.stream_end2==0 || video_obj->block_end_ptr==video_obj->avi_info.stream_end2) {
			vid_printf("video play finish\n\r");
			video_close();
			return 1;
		}
		else {
			video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
			f_lseek(&video_obj->fil, video_obj->avi_info.stream_start2);
			ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
			if(ret != FR_OK ) {
				vid_printf("ret=%d\n\r", ret);
				return -8;
			}
			video_obj->stream_size = 0;
			vid_printf("change to second block ^_^\n\r");
		}
	}

	//获取新帧的属性信息
	attr_oft = video_obj->stream_size;
	ret = avi_get_stream_info(&video_obj->stream_buff[attr_oft], &video_obj->stream_id, &video_obj->stream_size);
	if(ret != 0)
	{
		vid_printf("get stream info fail!\n\r");
		vid_printf("fil.fptr = %u\n\r", (uint32_t)video_obj->fil.fptr);
		vid_printf("video_obj->block_end_ptr = %u\n\r", video_obj->block_end_ptr);
		vid_printf("video_obj->avi_info.stream_end = %u\n\r", video_obj->avi_info.stream_end);
		vid_printf("video_obj->avi_info.stream_start2 = %u\n\r", video_obj->avi_info.stream_start2);
		vid_printf("stream attribute data:\n\r");
		vid_printf_arr(&video_obj->stream_buff[attr_oft], 8, 0);

		if(0 == vid_strncmp((const char *)&video_obj->stream_buff[attr_oft], "ix", 2))
		{
			uint32_t ix00_size = (uint32_t)video_obj->stream_buff[attr_oft+7]<<24 | \
								(uint32_t)video_obj->stream_buff[attr_oft+6]<<16 | \
								(uint32_t)video_obj->stream_buff[attr_oft+5]<<8 | \
								(uint32_t)video_obj->stream_buff[attr_oft+4];
			vid_printf("ix00_size = 0x%08X, %u\n\r", ix00_size, ix00_size);

			f_lseek(&video_obj->fil, video_obj->fil.fptr+ix00_size);
			ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
			if(ret != FR_OK) {
				vid_printf("ret=%d\n\r", ret);
				return -9;
			}
			vid_printf("After skip ix00_size, the next 8 byte:");
			vid_printf_arr(video_obj->stream_buff, 8, 0);
		
			video_obj->stream_size = 0;
			goto get_stream_attr2;
		}
		
		vid_printf("Error! can't find stream start flag!!\n\r");
		return -10;
	}	
	
	//video_obj->stream_size的大小是否为0, 是否超过了STREAM_SIZE, 下一次解码时会在前面的while中处理
	
	return 0;
}

int video_close(void)
{
	if(video_obj != NULL)
	{
		vid_close_sys_audio();

		mp3_decode_finish();
		jpeg_decode_finish();
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
	}
	return 0;
}


//===============================================================================
//video播放控制函数

/** @brief 播放器秒任务
 **/
void video_1s_hook(void)
{
	if(video_obj!=NULL && video_obj->time_cnt>0)
	{
		video_obj->time_cnt--;
		if(video_obj->time_cnt == 0)
		{
			vid_ctrl_panel_hide();
		}
	}
}

/** @brief 随机定位视频文件
 ** @param pos-位置[0-9999]
 */
int video_seek(int pos)
{
	if(video_obj == NULL)
		return -1;
	if(pos<0 || pos>9999)
		return -2;
	
	int ret, offset;
	uint32_t new_fil_pos, rd_len;

	new_fil_pos = (uint64_t)pos * (video_obj->avi_info.stream_end - video_obj->avi_info.stream_start + video_obj->avi_info.stream_end2 - video_obj->avi_info.stream_start2) / 10000;
	new_fil_pos += video_obj->avi_info.stream_start;
	if(new_fil_pos >= video_obj->avi_info.stream_end) {
		new_fil_pos += (video_obj->avi_info.stream_start2 - video_obj->avi_info.stream_end);
		video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
	}
	else {
		video_obj->block_end_ptr = video_obj->avi_info.stream_end;
	}
	ret = f_lseek(&video_obj->fil, new_fil_pos);
	if(ret != FR_OK)
		return -3;

	//seek后寻找视频流
find_stream_flag:
	ret = f_read(&video_obj->fil, video_obj->stream_buff, STREAM_SIZE, &rd_len);
	if(ret!=FR_OK || rd_len!=STREAM_SIZE) {
		vid_printf("[%s] Error1! Disk read fail\n\r", __FUNCTION__);
		return -4;
	}
	offset = avi_search_id(video_obj->stream_buff, STREAM_SIZE, video_obj->avi_info.video_flag);
	if(offset < 0)
	{
		vid_printf("[%s] Error2! Search stream ID fail, but seek continue\n\r", __FUNCTION__);
		if(video_obj->fil.fptr < video_obj->block_end_ptr)
		{
			goto find_stream_flag;
		}
		else
		{
			if(video_obj->avi_info.stream_end2==0 || video_obj->block_end_ptr==video_obj->avi_info.stream_end2)
				return 1;           //已经查找到文件的末尾
			else
			{
				video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
				f_lseek(&video_obj->fil, video_obj->avi_info.stream_start2);
				goto find_stream_flag;				
			}	
		}
	}

	video_obj->stream_size = 0;
	ret = avi_get_stream_info(&video_obj->stream_buff[offset], &video_obj->stream_id, &video_obj->stream_size);
	if(ret!=0 || video_obj->stream_size==0 || video_obj->stream_size+8>STREAM_SIZE)
	{
		vid_printf("[%s] Error3! Get stream attr fail, stream_size=%d, but seek continue\n\r", __FUNCTION__, video_obj->stream_size);
		if(video_obj->fil.fptr < video_obj->block_end_ptr)
		{
			goto find_stream_flag;
		}
		else
		{
			if(video_obj->avi_info.stream_end2==0 || video_obj->block_end_ptr==video_obj->avi_info.stream_end2)
				return 1;           //已经查找到文件的末尾
			else
			{
				video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
				f_lseek(&video_obj->fil, video_obj->avi_info.stream_start2);
				goto find_stream_flag;				
			}	
		}
	}
	f_lseek(&video_obj->fil, video_obj->fil.fptr-(STREAM_SIZE-(offset+8)));	

	//寻找到视频流后立即解码一帧视频
	video_obj->time_cnt = DIS_TIME;
	video_obj->player_dis_info.adjust = 1;
	ret = video_decode_stream();
	if(ret != 0) {
		vid_printf("[%s] Error4! Decode fail, ret=%d, but seek continue\n\r", __FUNCTION__, ret);
		if(video_obj->fil.fptr < video_obj->block_end_ptr)
		{
			goto find_stream_flag;
		}
		else
		{
			if(video_obj->avi_info.stream_end2==0 || video_obj->block_end_ptr==video_obj->avi_info.stream_end2)
				return 1;           //已经查找到文件的末尾
			else
			{
				video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
				f_lseek(&video_obj->fil, video_obj->avi_info.stream_start2);
				goto find_stream_flag;				
			}	
		}
	}

	return 0;
}

/** @brief 获取视频文件已播放百分比
 ** @reval 0-9999   5000-表示50.00%
 */
int video_get_pos(void)
{
	if(video_obj == NULL)
		return -1;

	video_obj->time_cnt = DIS_TIME;             //触发屏幕显示
	video_obj->player_dis_info.adjust = 1;	
	return 10000*__get_avi_played_time()/__get_avi_total_time();
}


/**  @brief 设置播放器的音量
 */
int video_set_volume(int volume)
{
	if(video_obj != NULL)
	{
		video_obj->time_cnt = DIS_TIME;
		video_obj->player_dis_info.adjust = 2;
		return vid_set_sys_volume(volume);
	}
	return 0;
}

/**  @brief 读取播放器的音量
 */
int video_get_volume(void)
{
	if(video_obj == NULL)
		return -1;
		
	video_obj->time_cnt = DIS_TIME;          //触发屏幕显示
	video_obj->player_dis_info.adjust = 2;	
	return vid_get_sys_volume();
}

/**  @brief 暂停播放
 */
int video_pause(void)
{
	if(video_obj != NULL)
	{
		video_obj->time_cnt = DIS_TIME;
		video_obj->player_dis_info.adjust = 3;
		
		int i, ret, last_stream;
		for(i=0; i<5; i++)
		{
			last_stream = video_obj->stream_id;
			ret = video_decode_stream();
			if(ret != 0) {	/* 解码失败或者结束 */
				video_close();
				return -1;
			}
			if(last_stream == STREAM_VIDS_FLAG)
				break;
		}
	}
	
	return 0;
}

/** @brief 恢复播放
 */
int video_resume(void)
{
	if(video_obj != NULL)
	{
		video_obj->time_cnt = DIS_TIME;
		video_obj->player_dis_info.adjust = 0;
	}
	return 0;
}











