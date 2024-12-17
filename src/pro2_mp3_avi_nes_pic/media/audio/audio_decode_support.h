#ifndef  __AUDIO_DECODE_SUPPORT_H_
#define  __AUDIO_DECODE_SUPPORT_H_


#include <stdint.h>
#include <stddef.h>

//audio库所依赖的外部函数
void *aud_memset(void* s, int c, size_t n);
void *aud_memcpy(void *dst, const void *src, size_t n);
void *aud_memmove (void *dst, const void *src, size_t n);

size_t aud_strlen(const char *str);
int aud_strcmp(const char *s1, const char *s2);
int aud_strncmp(const char *s1, const char *s2, size_t n);

void *aud_malloc(uint32_t size);
void aud_free(void *p);

void aud_printf(const char *fmt, ...);

#endif





