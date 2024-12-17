#include  "audio_decode_support.h"


#include "libc.h"
#include "my_malloc.h"
#include "uart.h"


void *aud_memset(void* s, int c, size_t n)
{
	return u_memset(s, c, n);
}

void *aud_memcpy(void *dst, const void *src, size_t n)
{
	return u_memcpy(dst, src, n);
}

void *aud_memmove(void *dst, const void *src, size_t n)
{
	return u_memmove(dst, src, n);
}

size_t aud_strlen(const char *str)
{
	return u_strlen(str);
}

int aud_strcmp(const char *s1, const char *s2)
{
	return u_strcmp(s1, s2);
}

int aud_strncmp(const char *s1, const char *s2, size_t n)
{
	return u_strncmp(s1, s2, n);
}


void *aud_malloc(uint32_t size)
{
	return mem_malloc(size);
}

void aud_free(void *p)
{
	mem_free(p);
}


void aud_printf(const char *fmt, ...)
{
	int len;
	char buff[256];   //不能太大,也不能太小
	va_list ap;
	
	va_start(ap, fmt);
	len = u_vsnprintf(buff, sizeof(buff), fmt, ap);  //len = strlen(buf); 不包括'\0'
	va_end(ap);

	uart1_write(buff, len);
}



