#include "parse_avi.h"

#include "video_support.h"


/** @brief 根据buff的内容, 解析avi文件, 并获取相关属性信息
  * @param buff-指向待解析的avi文件数据
  *        size-buff的大小
  *        avi_info_ptr-avi视频属性输出
  * @reval 0-解析成功 非0-解析失败
  */
int avi_parse(uint8_t *buff, int size, avi_info_t *avi_info_ptr)
{
	int idx;
	
	int header_list_size;
	AVIHeader *pAVIHeader;
	ListHeader *pListHeader;
	MainAVIHeader *pMainAVIHeader;
	AVIStreamHeader *pAVIStreamHeader;
	BITMAPINFO *pBITMAPINFO;
	WAVEFORMAT *pWAVEFORMAT;
	
	//parsing AVI File
	idx = 0;
	pAVIHeader = (AVIHeader *)&buff[idx];
	if(pAVIHeader->RiffID != AVI_RIFF_ID)
	{
		return -AVI_RIFF_ERR;
	}

	if(pAVIHeader->AviID != AVI_AVI_ID)
	{
		return -AVI_AVI_ERR;		
	}
//	my_printf("pAVIHeader->FileSize = %u\n\r", pAVIHeader->FileSize);
	
	//parsing Header List(hdrl)
	idx += sizeof(AVIHeader);
	pListHeader = (ListHeader *)&buff[idx];
	if(pListHeader->ListID != AVI_LIST_ID)
	{
		return -AVI_LIST_ERR;			
	}
	if(pListHeader->ListType != LIST_HDRL_ID)
	{
		return -AVI_LIST_HDRL_ERR;		
	}
	header_list_size = pListHeader->BlockSize;
	
	//parsing AVI Header
	idx += sizeof(ListHeader);
	pMainAVIHeader = (MainAVIHeader *)&buff[idx];
	if(pMainAVIHeader->BlockID != BLOCK_AVIH_ID)
	{
		return -AVI_BLOCK_AVIH_ERR;
	}
	
	avi_info_ptr->sec_per_frame = pMainAVIHeader->SecPerFrame;
	avi_info_ptr->total_frame = pMainAVIHeader->TotalFrame;
//	my_printf("stream type number = %d\n\r", pMainAVIHeader->Streams);    //流的种类数
	
	//parsing AVI Header LIST1(strl)
	idx += 8+pMainAVIHeader->BlockSize;
	pListHeader = (ListHeader *)&buff[idx];
	if(pListHeader->ListID != AVI_LIST_ID)
	{
		return -AVI_LIST_ERR;		
	}
	if(pListHeader->ListType != LIST_HDRL_STRL_ID)
	{
		return -AVI_LIST_HDRL_STRL_ERR;
	}
	
	//parsing Stream Header(strh) and Stream Format(strf)
	pAVIStreamHeader = (AVIStreamHeader *)(&buff[idx]+sizeof(ListHeader));
	if(pAVIStreamHeader->BlockID != BLOCK_STRH_ID)
	{
		return -AVI_BLOCK_STRH_ERR;		
	}
	if(pAVIStreamHeader->StreamType == STREAM_TYPE_VIDS)  //视频帧在前,音频帧在后
	{
		pBITMAPINFO = (BITMAPINFO *)(&buff[idx] + sizeof(ListHeader) + 8+pAVIStreamHeader->BlockSize);
		if(pBITMAPINFO->BlockID != BLOCK_STRF_ID) {
			return -AVI_BLOCK_STRF_ERR;
		}
		avi_info_ptr->width = pBITMAPINFO->bmiHeader.Width;
		avi_info_ptr->height = pBITMAPINFO->bmiHeader.Height;
		avi_info_ptr->compression =  pBITMAPINFO->bmiHeader.Compression;
		avi_info_ptr->video_flag = "00dc";
		
		avi_info_ptr->sample_rate = 0;
		avi_info_ptr->channels = 0;
		avi_info_ptr->audio_flag = NULL;
	}
	else if(pAVIStreamHeader->StreamType == STREAM_TYPE_AUDS)
	{
		pWAVEFORMAT = (WAVEFORMAT *)(&buff[idx] + sizeof(ListHeader) + 8+pAVIStreamHeader->BlockSize);
		if(pWAVEFORMAT->BlockID != BLOCK_STRF_ID) {
			return -AVI_BLOCK_STRF_ERR;
		}
		avi_info_ptr->sample_rate = pWAVEFORMAT->SampleRate;
		avi_info_ptr->channels = pWAVEFORMAT->Channels;
		avi_info_ptr->format_tag = pWAVEFORMAT->FormatTag;
		avi_info_ptr->block_align = pWAVEFORMAT->BlockAlign;
		avi_info_ptr->audio_flag = "00wb";
	}
	else
	{
		return -AVI_STREAM_ERR;		
	}
	
	//parsing AVI Header LIST2(strl),  若只含有视频而没有音频, 则LIST2可能不存在
	idx += 8+pListHeader->BlockSize;
	pListHeader = (ListHeader *)&buff[idx];
	if(pListHeader->ListID==AVI_LIST_ID && pListHeader->ListType==LIST_HDRL_STRL_ID)
	{
		//parsing Stream Header(strh) and Stream Format(strf)
		pAVIStreamHeader = (AVIStreamHeader *)(&buff[idx]+sizeof(ListHeader));
		if(pAVIStreamHeader->BlockID != BLOCK_STRH_ID)
		{
			return -AVI_BLOCK_STRH_ERR;		
		}

		if(pAVIStreamHeader->StreamType == STREAM_TYPE_VIDS)
		{
			pBITMAPINFO = (BITMAPINFO *)(&buff[idx] + sizeof(ListHeader) + 8+pAVIStreamHeader->BlockSize);
			if(pBITMAPINFO->BlockID != BLOCK_STRF_ID) {
				return -AVI_BLOCK_STRF_ERR;
			}
			avi_info_ptr->width = pBITMAPINFO->bmiHeader.Width;
			avi_info_ptr->height = pBITMAPINFO->bmiHeader.Height;
			avi_info_ptr->compression =  pBITMAPINFO->bmiHeader.Compression;
			avi_info_ptr->video_flag = "01dc";
		}
		else if(pAVIStreamHeader->StreamType == STREAM_TYPE_AUDS)
		{
			pWAVEFORMAT = (WAVEFORMAT *)(&buff[idx] + sizeof(ListHeader) + 8+pAVIStreamHeader->BlockSize);
			if(pWAVEFORMAT->BlockID != BLOCK_STRF_ID) {
				return -AVI_BLOCK_STRF_ERR;
			}
			avi_info_ptr->sample_rate = pWAVEFORMAT->SampleRate;
			avi_info_ptr->channels = pWAVEFORMAT->Channels;
			avi_info_ptr->format_tag = pWAVEFORMAT->FormatTag;
			avi_info_ptr->block_align = pWAVEFORMAT->BlockAlign;
			avi_info_ptr->audio_flag = "01wb";
		}
		else
		{
			return -AVI_STREAM_ERR;
		}
	}
//	my_printf("hdrl list size = %d\n\r", header_list_size);
//	my_printf("After parse HeaderList, idx = %d\n\n\r", idx);
//	my_printf_arr(&buff[8+header_list_size], 4096, 0);
	
	int offset;
	
	idx = 8+header_list_size;
	if(idx >= size)
	{
		return -AVI_BUFF_OVER_ERR;
	}

	offset = avi_search_id(&buff[idx], size-idx, "movi");
	if(offset < 0)
	{
		return -AVI_LIST_MOVI_ERR;
	}
	pListHeader = (ListHeader *)&buff[idx+offset-8];
	if(pListHeader->ListID != AVI_LIST_ID)
	{
		return AVI_LIST_ERR;			
	}
	avi_info_ptr->stream_start = idx+offset+4;     //跳过4字节movi,后面的流才是真正的stream流
	avi_info_ptr->stream_end = avi_info_ptr->stream_start-4 + pListHeader->BlockSize;

	return avi_info_ptr->stream_start;
}

//获取IndexChunk的大小
int avi_idx_chunk_size(uint8_t *buff, int size)
{
	if(size < 8)
		return 0;
	
	IndexChunk *pIndexChunk = (IndexChunk *)buff;
	if(pIndexChunk->ChunkID != INDEX_CHUNK_ID)
		return 0;
	
	return (pIndexChunk->ChunkSize + 8);
}

int avi_parse2(uint8_t *buff, int size, avi_info_t *avi_info_ptr)
{
	//当视频很长时(比如 泰坦尼克号.avi 时长超过180min), 会有多个movi,
	//当一个movi结束后, 还要播放第二个movi, 此函数可以获取第二个movi的属性
//	my_printf("AVI Block2 size = %d\n\r", size);
//	my_printf_arr(buff, 1024, 0);
//	AVI Block2 size = 49152
//	00000000  52 49 46 46 8A 68 A6 24 41 56 49 58 4C 49 53 54  |RIFF.h.$AVIXLIST|
//	00000010  7E 68 A6 24 6D 6F 76 69 30 30 64 63 96 3F 00 00  |~h.$movi00dc.?..|
//	00000020  FF D8 FF FE 00 10 4C 61 76 63 35 38 2E 35 34 2E  |......Lavc58.54.|
//	00000030  31 30 30 00 FF FE 00 0C 43 53 3D 49 54 55 36 30  |100.....CS=ITU60|
	
	avi_info_ptr->stream_start2 = 0;   //default mark
	avi_info_ptr->stream_end2 = 0;
	if(size < 8)
	{
		return 0;
	}

	int offset;
	offset = avi_search_id(&buff[0], size, "movi");
	if(offset < 0)
	{
		return 0;
	}
	
	ListHeader *pListHeader;
	pListHeader = (ListHeader *)&buff[offset-8];
	if(pListHeader->ListID != AVI_LIST_ID)
	{
		return 0;			
	}

	avi_info_ptr->stream_start2 = offset+4;        //跳过4字节movi,后面的流才是真正的stream流
	avi_info_ptr->stream_end2 = avi_info_ptr->stream_start2-4 + pListHeader->BlockSize;	
	
	uint32_t block2_frame;
	
	block2_frame = (uint64_t)(avi_info_ptr->stream_end2 - avi_info_ptr->stream_start2) * avi_info_ptr->total_frame / (avi_info_ptr->stream_end - avi_info_ptr->stream_start);
	avi_info_ptr->total_frame += block2_frame;

	vid_printf("block2 total frame = %d\n\r", block2_frame);
	vid_printf("block2 plays time = %ds\n\r", (uint32_t)((uint64_t)block2_frame*avi_info_ptr->sec_per_frame/1000000));
	return avi_info_ptr->stream_start2;
}

int avi_search_id(uint8_t *buff, int size, const char *id)
{
	int i;
	const uint8_t *tar_id = (const uint8_t *)id;
	
	size -= 4;
	for(i=0; i<size; i++)
	{
	   	if(buff[i]==tar_id[0])
			if(buff[i+1]==tar_id[1])
				if(buff[i+2]==tar_id[2])
					if(buff[i+3]==tar_id[3])
						return i;	
	}
	return -1;
}

#define	 MAKEWORD(ptr)	(uint16_t)*((uint8_t *)(ptr)+1)<<8 | *((uint8_t *)(ptr))
#define  MAKEDWORD(ptr)	(uint32_t)*((uint8_t *)(ptr)+3)<<24 | (uint32_t)*((uint8_t *)(ptr)+2)<<16 | \
                        (uint32_t)*((uint8_t *)(ptr)+1)<<8 | *((uint8_t *)(ptr))

int avi_get_stream_info(uint8_t *buff, uint16_t *stream_id, uint32_t *stream_size)
{
	*stream_id = MAKEWORD(buff+2);
	*stream_size = MAKEDWORD(buff+4);
	*stream_size += (*stream_size & 0x01);   //奇数加1, stream_size必须为偶数
	if(*stream_id==STREAM_VIDS_FLAG || *stream_id==STREAM_AUDS_FLAG)
		return AVI_OK;
	else
		return AVI_STRAM_FLAG_ERR;
}




