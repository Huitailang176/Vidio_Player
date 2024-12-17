#ifndef  __HELIX_SUPPORT_H_
#define  __HELIX_SUPPORT_H_

#include <stddef.h>

/** @brief 本文件中声明的函数是helix库中调用的函数, 需要在库外实现这些函数
 **/

extern void *helix_memcpy (void *dst, const void *src, size_t n);
extern void *helix_memmove(void *dst, const void *src, size_t count);
extern void *helix_malloc(unsigned int size);
extern void helix_free(void *mem_ptr);


#endif



