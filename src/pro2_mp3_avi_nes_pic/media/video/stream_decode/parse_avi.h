#ifndef  __PARSE_AVI_H_
#define  __PARSE_AVI_H_

#include <stdint.h>

#define AVI_RIFF_ID			0X46464952  //'RIFF'
#define AVI_AVI_ID			0X20495641  //'AVI '
#define AVI_LIST_ID			0X5453494C  //'LIST'

#define LIST_HDRL_ID			0X6C726468		//'hdrl', 信息块标志
#define LIST_MOVI_ID			0X69766F6D 		//'movi', 数据块标志
#define LIST_HDRL_STRL_ID		0X6C727473		//'strl', LIST(类型为hdrl)中的LIST(类型为strl)的标志

#define BLOCK_AVIH_ID			0X68697661 		//'avih', LIST(类型为hdrl)的子块标志
#define BLOCK_STRH_ID           0X68727473      //'strh', LIST(类型为hdrl)中的LIST(类型为strl)的strh子块标志
#define BLOCK_STRF_ID           0X66727473      //'strf', LIST(类型为hdrl)中的LIST(类型为strl)的strf子块标志

#define INDEX_CHUNK_ID          0X31786469      //'idx1', IndexChunk块的标志

//AVIStreamHeader结构体相关成员的取值
#define STREAM_TYPE_VIDS		0X73646976		//视频流
#define STREAM_TYPE_AUDS		0X73647561 		//音频流

#define HANDLER_MJPG		    0X47504A4D      //'MJPG', 视频编码方式为MJPG
#define HANDLER_H264            0x34363248      //'H264', 视频编码方式为H264
#define HANDLER_AUDIO           0x00000001

#define AUDIO_FORMAT_PCM        0x0001          //音频采用PCM编码
#define AUDIO_FORMAT_MP3        0x0055          //音频采用MP3编码


//解码MovieList时的音频流与视频流标志
#define STREAM_VIDS_FLAG   0X6364		        //'dc', 视频流标志
#define STREAM_AUDS_FLAG   0X6277               //'wb', 音频流标志


//错误类型
typedef enum {
	AVI_OK = 0,
	AVI_RIFF_ERR,
	AVI_AVI_ERR,
	AVI_LIST_ERR,

	AVI_LIST_HDRL_ERR,
	AVI_LIST_MOVI_ERR,
	AVI_LIST_HDRL_STRL_ERR,

	AVI_BLOCK_AVIH_ERR,
	AVI_BLOCK_STRH_ERR,
	AVI_BLOCK_STRF_ERR,

	AVI_STREAM_ERR,
	AVI_HANDLER_ERR,            //不支持的音视频编码
	AVI_STRAM_FLAG_ERR,
	AVI_STRAM_LARGE_ERR,        //stream流太大, buff放不下

	AVI_BUFF_OVER_ERR,
}AVISTATUS;


//AVI文件头部信息
typedef struct
{	
	uint32_t RiffID;			//必须为'RIFF', 即0X46464952
	uint32_t FileSize;			//AVI文件大小(不包含最初的8字节, 也就是RiffID和FileSize不计算在内)
	uint32_t AviID;				//必须为'AVI ', 即0X41564920 
}AVIHeader;


//LIST块信息, 主要包括三种LIST块, hdrl(信息块)/movi(数据块)/idxl(索引块, 非必须, 是可选的)
typedef struct
{	
	uint32_t ListID;            //必须为'LIST', 即0X5453494C
	uint32_t BlockSize;			//块大小(不包含最初的8字节,也ListID和BlockSize不计算在内)
	uint32_t ListType;			//LIST子块类型:hdrl(信息块)/movi(数据块)/idxl(索引块, 非必须, 是可选的)
}ListHeader;


//LIST(类型为hdrl)的avih子块信息
typedef struct
{	
	uint32_t BlockID;			//必须为'avih', 即0X68697661
	uint32_t BlockSize;			//块大小(不包含最初的8字节,也就是BlockID和BlockSize不计算在内)
	uint32_t SecPerFrame;		//视频帧间隔时间(单位为us)
	uint32_t MaxByteSec; 		//最大数据传输率, 字节/秒
	uint32_t PaddingGranularity; //数据填充的粒度
	uint32_t Flags;				//AVI文件的全局标记, 比如是否含有索引块等
	uint32_t TotalFrame;		//文件总帧数
	uint32_t InitFrames;  		//为交互格式指定初始帧数(非交互格式应该指定为0)
	uint32_t Streams;			//包含的数据流种类个数, 通常为2
	uint32_t RefBufSize;		//建议读取本文件的缓存大小(应能容纳最大的块), 默认可能是1M字节!
	uint32_t Width;				//图像宽
	uint32_t Height;			//图像高
	uint32_t Reserved[4];		//保留
}MainAVIHeader;


//LIST(类型为hdrl)中的LIST(类型为strl)的strh子块信息
typedef struct
{	
	uint32_t BlockID;			//必须为'strh', 即0X68727473
	uint32_t BlockSize;			//块大小(不包含最初的8字节, 也就是BlockID和BlockSize不计算在内)
	uint32_t StreamType;		//数据流种类, vids(0X73646976)-视频流, auds(0X73647561)-音频流
	uint32_t Handler;			//指定流的处理者, 对于音视频来说就是解码器, 视频:H264/MJPG/MPEG 音频:0x00000001
	uint32_t Flags;  			//标记: 是否允许这个流输出? 调色板是否变化?
	uint16_t Priority;			//流的优先级(当有多个相同类型的流时优先级最高的为默认流)
	uint16_t Language;			//音频的语言代号
	uint32_t InitFrames;  		//为交互格式指定初始帧数
	uint32_t Scale;				//数据量, 视频每桢的大小或者音频的采样大小
	uint32_t Rate; 				//Scale/Rate=每秒采样数
	uint32_t Start;				//数据流开始播放的位置, 单位为Scale
	uint32_t Length;			//数据流的数据量, 单位为Scale
 	uint32_t SuggestBuffSize;   //建议使用的缓冲区大小
    uint32_t Quality;			//解压缩质量参数,值越大,质量越好
	uint32_t SampleSize;		//音频的样本大小
	struct					    //视频帧所占的矩形 
	{				
	   	short Left;
		short Top;
		short Right;
		short Bottom;
	}Frame;				
}AVIStreamHeader;


//LIST(类型为hdrl)中的LIST(类型为strl)的strf子块信息(前提:strh子块已经表明该LIST为Video)
typedef struct 
{
	uint32_t BlockID;			//必须为'strf', 即0X66727473
	uint32_t BlockSize;			//块大小(不包含最初的8字节,也就是BlockID和本BlockSize不计算在内)

	/* 位图信息头 */
	struct {
		uint32_t BmpSize;			//bmp结构体大小,包含(BmpSize在内)
		int32_t Width;				//图像宽, 以象素为单位
		int32_t Height;				//图像高, 以象素为单位. 正数, 说明图像是倒向的; 负数, 则说明图像是正向的.
		uint16_t Planes;			//平面数, 必须为1
		uint16_t BitCount;			//像素比特数, 其值为1 4 8 16 24或32
		uint32_t Compression;		//压缩类型, MJPG/H264等
		uint32_t SizeImage;			//图像大小, 以字节为单位
		int32_t XpixPerMeter;		//水平分辨率, 用象素/米表示
		int32_t YpixPerMeter;		//垂直分辨率, 用象素/米表示
		uint32_t ClrUsed;			//实际使用了调色板中的颜色数, 压缩格式中不使用
		uint32_t ClrImportant;		//重要的颜色		
	} bmiHeader;
	
	/* 颜色表, 可有可无 */
	struct {
		uint8_t  rgbBlue;			//蓝色的亮度(值范围为0-255)
		uint8_t  rgbGreen; 			//绿色的亮度(值范围为0-255)
		uint8_t  rgbRed; 			//红色的亮度(值范围为0-255)
		uint8_t  rgbReserved;		//保留, 必须为0		
	} bmColors[0];
}BITMAPINFO;

//LIST(类型为hdrl)中的LIST(类型为strl)的strf子块信息(前提:strh子块已经表明该LIST为Audio)
typedef struct 
{
	uint32_t BlockID;			//必须为'strf', 即0X66727473
	uint32_t BlockSize;			//块大小(不包含最初的8字节,也就是BlockID和本BlockSize不计算在内)
   	uint16_t FormatTag;			//格式标志:0X0001=PCM, 0X0055=MP3...
	uint16_t Channels;	  		//声道数, 一般为2,表示立体声
	uint32_t SampleRate; 		//音频采样率
	uint32_t BaudRate;   		//波特率 
	uint16_t BlockAlign; 		//数据块对齐标志
	uint16_t Size;				//该结构大小
}WAVEFORMAT; 

//Chunk块信息
typedef struct
{	
	uint32_t ChunkID;         //必须为'idx1', 即0X31786469
	uint32_t ChunkSize;		  //Chunk块大小(不包含最初的8字节,也ChunkID和ChunkSize不计算在内)
}IndexChunk;

//用户关心的avi信息
typedef struct avi_info_s
{
	uint32_t sec_per_frame;    //视频帧间隔时间(单位为us)
	uint32_t total_frame;      //视频总帧数
	uint32_t width;            //图像的宽度
	uint32_t height;           //图像的高度
	uint32_t compression;      //图像压缩编码方式
	
	uint32_t sample_rate;     //音频采样率
	uint16_t channels;        //声道数, 一般为2,表示立体声
	uint16_t format_tag;      //音频格式标志, 0X0001=PCM, 0X0055=MP3...
	uint16_t block_align;     //数据块对齐标志

	uint32_t stream_start;      //movi流开始的位置
	uint32_t stream_end;        //movi流结束的位置
	uint32_t stream_start2;     //movi流开始的位置 这2个数为0时说明没有第二个movi
	uint32_t stream_end2;       //movi流结束的位置
	const char *video_flag;     //视频帧标记, VideoFLAG="00dc"/"01dc"
	const char *audio_flag;     //音频帧标记, AudioFLAG="00wb"/"01wb"	
}avi_info_t;


int avi_parse(uint8_t *buff, int size, avi_info_t *avi_info_ptr);
int avi_idx_chunk_size(uint8_t *buff, int size);
int avi_parse2(uint8_t *buff, int size, avi_info_t *avi_info_ptr);
int avi_search_id(uint8_t *buff, int size, const char *id);
int avi_get_stream_info(uint8_t *buff, uint16_t *stream_id, uint32_t *stream_size);


#endif



