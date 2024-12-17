#ifndef  __FLASH_H_
#define  __FLASH_H_



#include "stm32f4xx.h"


//STM32F407ZG Flash 容量(Byte)
#define FLASH_SIZE  1024*1024


/* STM32F407ZG有12个 */
#define  SEC_CNT  12
typedef struct sector_info_s
{
	int size;
	int describe;
}sector_info_t;
	
//int flash_sect_size_of_curr_addr(uint32_t cur_addr);
int flash_get_page_size_by_addr(uint32_t start_addr);
int flash_erase(uint32_t start_addr, int len);
int flash_write(uint32_t start_addr, const void *buff, int len);


#endif



