#ifndef  __NES_GAME_SUPPORT_H_
#define  __NES_GAME_SUPPORT_H_

#include <stdint.h>
#include <stddef.h>

/** @biref 本文件中声明的函数在nes_game库中调用, 要在nes_game库外实现.
 */

//=========================================================
//硬件支持
int flash_get_page_size_by_addr(uint32_t start_addr);            //在flash.c文件中实现
int flash_erase(uint32_t start_addr, int len);
int flash_write(uint32_t start_addr, const void *buff, int len);

int nes_read_keypad(void);
int nes_write_bitmap(int line_no, const uint16_t *pixel_buff, int pixel_num);
void nes_show_string(char *str);

int nes_open_sound(int sample_freq, int channels, int sample_bits, int pcm_pkg_size);
int nes_write_pcm(void *pcm_buff_ptr, int pcm_pkg_size);
int nes_get_sys_volume(void);
int nes_set_sys_volume(int volume);
int nes_close_sound(void);


//=========================================================
//软件支持
void *nes_malloc(uint32_t size);
void *nes_malloc_align(uint32_t size, uint32_t addr_align);
void nes_free(void *p);

void *nes_memset(void* s, int c, size_t n);
void *nes_memcpy (void *dst, const void *src, size_t n);
void *nes_memmem(const void* haystack, size_t hl, const void* needle, size_t nl);

size_t nes_strlen(const char *str);
int nes_strcmp(const char *s1, const char *s2);
int nes_strncmp(const char *s1, const char *s2, size_t n);
char *nes_strncpy(char *dst, const char *src, size_t n);

int  nes_snprintf(char * buf, size_t size, const char *fmt, ...);
void nes_printf(const char *fmt, ...);

void nes_fetch_str_matrix(char *str, void *window, int width, int height);

#endif




