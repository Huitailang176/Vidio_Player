#ifndef  __VIDEO_SUPPORT_H_
#define  __VIDEO_SUPPORT_H_


#include <stddef.h>
#include <stdint.h>

/** libvideo.lib库中依赖的外部函数
 **/

//====================================================================
//硬件支持
int vid_open_sys_audio(int sample_freq, int channels, int sample_bits, void *pkg_buff, int pkg_size);
int vid_close_sys_audio(void);
int vid_set_sys_volume(int volume);
int vid_get_sys_volume(void);
void vid_write_pcm_data(void *pcm_ptr, int size);
void vid_write_bitmap_line(int line_no, uint16_t *pixel, int pixel_num);

void vid_ctrl_panel_hide(void);

//=====================================================================
//软函数支持
void *vid_malloc(uint32_t size);
void vid_free(void *p);

void *vid_memset(void* s, int c, size_t n);
void *vid_memcpy (void *dst, const void *src, size_t n);
int  vid_strncmp(const char *s1, const char *s2, size_t n);
char *vid_strncpy(char *dst, const char *src, size_t n);

int  vid_sscanf(const char * buf, const char * fmt, ...);
int  vid_snprintf(char * buf, size_t size, const char *fmt, ...);
void vid_printf(const char *fmt, ...);
void vid_printf_arr(const void *buff, int len, uint32_t start_addr);

void vid_fetch_str_matrix(char *str, void *matrix_window, int width, int height);

#endif


