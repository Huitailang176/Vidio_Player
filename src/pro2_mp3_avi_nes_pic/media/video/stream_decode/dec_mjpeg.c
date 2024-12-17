#include "dec_mjpeg.h"
#include "cdjpeg.h"
#include <setjmp.h>

#include "video_support.h"

#include "lcd_app_v2.h"
#include "my_printf.h"
//#include "my_malloc.h"


//快速内存管理, 以提高解码速度
#define  MJPEG_MAX_MALLOC_SIZE  38*1024  //320x240 比特率1000
static uint8_t *jmem_buf;	             //mjpeg解码的内存池
static uint32_t jmem_pos;			     //内存池指针

//mjpeg申请内存, jmemnobs.c中调用
void *mjpeg_malloc(unsigned int num)
{
	void *rp;

	num = (num+ 3) & ~3;			/* Align block size to the word boundary */
	rp = &jmem_buf[jmem_pos];
	jmem_pos += num;
	if(jmem_pos > MJPEG_MAX_MALLOC_SIZE)
	{
		vid_printf("[%s] malloc fail, cur_idx=%u need_len=%u\n\r", __FUNCTION__, jmem_pos-num, num);
		return NULL;
	}
	return rp;
}

struct my_error_mgr {
  struct jpeg_error_mgr pub;	
  jmp_buf env_buff;
}; 
typedef struct my_error_mgr *my_error_ptr;

struct jpeg_decompress_struct *cinfo;
struct my_error_mgr *jerr;

static uint8_t *jpeg_buf;			//指向待解码的jpeg数据
static uint32_t jbuf_size;			//jpeg_buf的大小

//jpeg解码异常退出, 在jerror.c中调用 
void exception_exit(int no)
{
	longjmp(jerr->env_buff, no);
}


static void init_source (j_decompress_ptr cinfo)
{
  /* no work necessary here */
}

//填充输入缓冲区, 一次性读取整帧数据
static boolean fill_input_buffer(j_decompress_ptr cinfo)
{  
	if(jbuf_size==0)
	{
		vid_printf("jd read off\r\n");
        //填充结束符
        jpeg_buf[0] = 0xFF;
        jpeg_buf[1] = JPEG_EOI;
  		cinfo->src->next_input_byte =jpeg_buf;
		cinfo->src->bytes_in_buffer = 2; 
	}
	else
	{
		cinfo->src->next_input_byte =jpeg_buf;
		cinfo->src->bytes_in_buffer = jbuf_size;
		jbuf_size -= jbuf_size;
	}
    return TRUE;
}

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */
static void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  struct jpeg_source_mgr * src = cinfo->src;

  /* Just a dumb implementation for now.  Could use fseek() except
   * it doesn't work on pipes.  Not clear that being smart is worth
   * any trouble anyway --- large skips are infrequent.
   */
  if (num_bytes > 0) {
    while (num_bytes > (long) src->bytes_in_buffer)
	{
      num_bytes -= (long) src->bytes_in_buffer;
      (void) (*src->fill_input_buffer) (cinfo);
      /* note we assume that fill_input_buffer will never return FALSE,
       * so suspension need not be handled.
       */
    }
    src->next_input_byte += (size_t) num_bytes;
    src->bytes_in_buffer -= (size_t) num_bytes;
  }
}

/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */
static void term_source (j_decompress_ptr cinfo)
{
  /* no work necessary here */
}


/*
 * Prepare for input from a supplied memory buffer.
 * The buffer must contain the whole JPEG data.
 */

static void jpeg_frame_src(j_decompress_ptr cinfo)
{
  struct jpeg_source_mgr * src;

  /* The source object is made permanent so that a series of JPEG images
   * can be read from the same buffer by calling jpeg_mem_src only before
   * the first one.
   */
  if (cinfo->src == NULL) {	/* first time for this JPEG object? */
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  SIZEOF(struct jpeg_source_mgr));
  }

  src = cinfo->src;
  src->init_source = init_source;
  src->fill_input_buffer = fill_input_buffer;
  src->skip_input_data = skip_input_data;
  src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->term_source = term_source;
  src->bytes_in_buffer = 0;
  src->next_input_byte = NULL;
}


int jpeg_decode_init(void)
{
	int ret = 0;
	cinfo = vid_malloc(sizeof(struct jpeg_decompress_struct));
	jerr  = vid_malloc(sizeof(struct my_error_mgr));
	jmem_buf = vid_malloc(MJPEG_MAX_MALLOC_SIZE);
	if(cinfo==NULL || jerr==NULL || jmem_buf==NULL)
	{
		jpeg_decode_finish();
		return -1;
	}

//	my_printf("[%s] Clear LCD\n\r", __FUNCTION__);
	lcd_clear(0x00101010);

//	cpu_over_clock_run();
//	ret = lcd_dma_init(lcd_dev.xres);
//	if(ret != 0)
//	{
//		mem_free(cinfo);
//		mem_free(jerr);
//		mem_free(jmem_buf);
//		cinfo = NULL;
//		jerr = NULL;
//		jmem_buf = NULL;	
//	}

	return ret;
}

void jpeg_decode_finish(void)
{
	vid_free(cinfo);
	vid_free(jerr);
	vid_free(jmem_buf);
	cinfo = NULL;
	jerr = NULL;
	jmem_buf = NULL;
	
//	lcd_dma_deinit();
//	cpu_normal_clock_run();
}


//源地址与目的地址都4字节对齐时的高效拷贝函数
static __attribute__((noinline)) void __memcpy_align4(void *dst, void *src, size_t size)
{
	uint32_t *dstp;
	uint32_t *srcp;
	uint32_t i, cnt;
	uint32_t dat0, dat1, dat2, dat3;
	
	dstp = dst;
	srcp = src;
	cnt = size>>2;
	while(cnt >= 4)
	{
		dat0 = srcp[0];
		dat1 = srcp[1];
		dat2 = srcp[2];
		dat3 = srcp[3];
		srcp += 4;

		dstp[0] = dat0;
		dstp[1] = dat1;
		dstp[2] = dat2;
		dstp[3] = dat3;
		dstp += 4;
	
		cnt -= 4;
	}

	cnt = cnt*4 + (size&0x03);  //注意优先级 +高于&
	i = 0;
	while(i < cnt)
	{
		((uint8_t *)dstp)[i] = ((uint8_t *)srcp)[i];
		i++;
	}
}


static void __pixel_line_add_progress_bar(uint16_t *pixel_line_buff, player_dis_info_t *dis_info_ptr, int pixel_cnt);
static void __pixel_line_add_play_info(uint16_t *pixel_line_buff, player_dis_info_t *dis_info_ptr, int pixel_cnt, int line_no);

#define  color24to16(color)  (uint16_t)(((color)&0xF80000)>>8 | ((color)&0x00FC00)>>5 | ((color)&0x0000F8)>>3)
#define  LCD_MAGENTA 	  0x00FF00FF
#define  LCD_WHITE		  0x00FFFFFF
#define  LCD_GRAY		  0x00808080
#define  LCD_FILL         0x00101010

//图像的最大宽度与高度
#define  WIDTH_MAX  320
#define  HEIGHT_MAX 240
//#define  WIDTH_MAX  240
//#define  HEIGHT_MAX 200


int jpeg_decode_frame(unsigned char *frame_buff, int frame_size, int flag, player_dis_info_t *dis_info_ptr)
{
	int ret;
	uint16_t __attribute__((aligned(4))) pixel_line_buff[WIDTH_MAX*2];  //视频的宽度为320(cinfo->output_width), 这个地方写死, 不写死就得用malloc(会降低解码效率)
	uint16_t __attribute__((aligned(4))) lcd_line_buff[2][WIDTH_MAX];      //此函数栈很深, 注意栈溢出
	uint8_t  *pixel_ptr;

	int idx;
	int lcd_sx, lcd_sy, lcd_ex, lcd_ey;

	if(frame_size == 0)
	{
		vid_printf("[%s] Error! frame size is 0!!\n\r", __FUNCTION__);
		return -1;
	}

	jpeg_buf = frame_buff;
	jbuf_size = frame_size;   
	jmem_pos = 0;

	cinfo->err = jpeg_std_error(&jerr->pub); 
	
	ret = setjmp(jerr->env_buff);
	if(ret == 0)
	{	//相当于try, 当{}中的代码调用longjmp相当于throw抛出异常
		jpeg_create_decompress(cinfo);
		jpeg_frame_src(cinfo);
		jpeg_read_header(cinfo, TRUE); 
		cinfo->dct_method = JDCT_IFAST;
		cinfo->do_fancy_upsampling = 0; 
		jpeg_start_decompress(cinfo);

		if(cinfo->output_width > WIDTH_MAX || cinfo->output_height > HEIGHT_MAX) {
			longjmp(jerr->env_buff, 1234);
		}
//		my_printf("%d %d %d\n\r", cinfo->output_width, cinfo->output_height, cinfo->output_scanline);

		//LCD开窗
		lcd_sx = (lcd_dev.xres - cinfo->output_width) / 2;
		lcd_ex = lcd_sx + cinfo->output_width - 1;
		lcd_sy = (lcd_dev.yres - cinfo->output_height) / 2;
		lcd_ey = lcd_sy + cinfo->output_height - 1;
		LCD_Set_Window(lcd_sx, lcd_sy, lcd_ex, lcd_ey);
		LCD_WriteRAM_Prepare();

		//图像的提取与显示
		idx = 0;
		pixel_ptr = (uint8_t *)pixel_line_buff;
		while(cinfo->output_scanline < cinfo->output_height)
		{
			jpeg_read_scanlines(cinfo, &pixel_ptr, 1);
			__memcpy_align4(lcd_line_buff[idx&0x01], pixel_line_buff, cinfo->output_width*sizeof(uint16_t));

			//添加进度条, 时长, 音量等信息
			if(flag!=0 && cinfo->output_height>100) {
				int prog_bar_start_line, prog_bar_end_line;
				int play_info_start_line, play_info_end_line;

				prog_bar_start_line = cinfo->output_height - 36; //bar and info height is 6+6+16=28
				prog_bar_end_line = prog_bar_start_line + 5;     //progress bar height is 6 pixel

				play_info_start_line = prog_bar_end_line + 1 + 6;   //deta is 6 pixel
				play_info_end_line = play_info_start_line + LINE_CNT - 1;

				if(idx>=prog_bar_start_line && idx<=prog_bar_end_line)
					__pixel_line_add_progress_bar(lcd_line_buff[idx&0x01], dis_info_ptr, cinfo->output_width);
				else if(idx>=play_info_start_line && idx<=play_info_end_line)
					__pixel_line_add_play_info(lcd_line_buff[idx&0x01], dis_info_ptr, cinfo->output_width, idx-play_info_start_line);
			}

			lcd_dma_trans_line(lcd_line_buff[idx&0x01], cinfo->output_width);
			idx++;
		}
		lcd_dma_trans_release();

		jpeg_finish_decompress(cinfo); 
		jpeg_destroy_decompress(cinfo);
	}
	else
	{	//相当于catch
		vid_printf("[%s] catch exception!\n\r", __FUNCTION__);
		jpeg_abort_decompress(cinfo);
		jpeg_destroy_decompress(cinfo);
		return -2;
	}

	//无异常, 正常返回
	return 0;
}

//添加进度条
#define  BAR_BLANK_SPACE  10
static void __pixel_line_add_progress_bar(uint16_t *pixel_line_buff, player_dis_info_t *dis_info_ptr, int pixel_cnt)
{
	int i, t;
	uint16_t fore_color, back_color;   //进度条颜色

	fore_color = color24to16(LCD_MAGENTA);  //LCD_LIGHTCYAN  LCD_MAGENTA
	back_color = color24to16(LCD_GRAY);

	t = (pixel_cnt-BAR_BLANK_SPACE*2)*dis_info_ptr->played_time/dis_info_ptr->total_time;
	for(i=BAR_BLANK_SPACE; i<BAR_BLANK_SPACE+t; i++)
		pixel_line_buff[i] = fore_color;
	for(i=BAR_BLANK_SPACE+t; i<pixel_cnt-BAR_BLANK_SPACE; i++)
		pixel_line_buff[i] = back_color;
}

//添加时间 音量等相关信息
#define  BAR_LEFT_SPACE  10
static void __pixel_line_add_play_info(uint16_t *pixel_line_buff, player_dis_info_t *dis_info_ptr, int pixel_cnt, int line_no)
{
	int i;
	char     info_buff[64];
	uint16_t font_color;               //字体颜色

	if(line_no == 0)
	{
		if(dis_info_ptr->adjust == 1)
			vid_snprintf(info_buff, sizeof(info_buff), "%02u:%02u/%02u:%02u  Volume:%02u  PlaySeek", \
						 dis_info_ptr->total_time/60,  dis_info_ptr->total_time%60, \
						 dis_info_ptr->played_time/60, dis_info_ptr->played_time%60, dis_info_ptr->volume);
		else if(dis_info_ptr->adjust == 2)
			vid_snprintf(info_buff, sizeof(info_buff), "%02u:%02u/%02u:%02u  Volume:%02u  SetVolume", \
						 dis_info_ptr->total_time/60,  dis_info_ptr->total_time%60, \
						 dis_info_ptr->played_time/60, dis_info_ptr->played_time%60, dis_info_ptr->volume);
		else if(dis_info_ptr->adjust == 3)
			vid_snprintf(info_buff, sizeof(info_buff), "%02u:%02u/%02u:%02u  Volume:%02u  Pause", \
						 dis_info_ptr->total_time/60,  dis_info_ptr->total_time%60, \
						 dis_info_ptr->played_time/60, dis_info_ptr->played_time%60, dis_info_ptr->volume);
		else
			vid_snprintf(info_buff, sizeof(info_buff), "%02u:%02u/%02u:%02u  Volume:%02u", \
						 dis_info_ptr->total_time/60, dis_info_ptr->total_time%60, \
						 dis_info_ptr->played_time/60, dis_info_ptr->played_time%60, dis_info_ptr->volume);
		vid_fetch_str_matrix(info_buff, dis_info_ptr->pixel_mark, LINE_PIXEL_NUM, LINE_CNT);
	}

	font_color = color24to16(LCD_WHITE);
	for(i=BAR_LEFT_SPACE; i<pixel_cnt; i++)
	{
		if(dis_info_ptr->pixel_mark[line_no][i-BAR_LEFT_SPACE] != 0)
			pixel_line_buff[i] = font_color;
	}
}



