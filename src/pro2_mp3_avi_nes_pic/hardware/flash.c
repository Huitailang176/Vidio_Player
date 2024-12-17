#include "flash.h"
#include <stddef.h>
#include "my_printf.h"
#include "cpu_related_v3.h"

/* STM32F407ZG有12个,每个扇区的大小如下 */
static const sector_info_t sector_arr[SEC_CNT] = {
	{16*1024,  FLASH_Sector_0},
	{16*1024,  FLASH_Sector_1},
	{16*1024,  FLASH_Sector_2},
	{16*1024,  FLASH_Sector_3},
	{64*1024,  FLASH_Sector_4},
	{128*1024, FLASH_Sector_5},
	{128*1024, FLASH_Sector_6},
	{128*1024, FLASH_Sector_7},
	{128*1024, FLASH_Sector_8},
	{128*1024, FLASH_Sector_9},
	{128*1024, FLASH_Sector_10},
	{128*1024, FLASH_Sector_11}
};


//获取当前地址所在扇区的大小
static const sector_info_t *get_sector_info(uint32_t cur_addr)
{
	if(cur_addr<FLASH_BASE || cur_addr>=FLASH_BASE+FLASH_SIZE) {
		return NULL;
	}
	
	uint32_t tmp_addr = FLASH_BASE;
	int i = 0;
	while(i < SEC_CNT)
	{
		if(tmp_addr >= cur_addr)
			return &sector_arr[i];
		tmp_addr += sector_arr[i].size;
		i++;
	}
	
	return NULL;
}

/**
  * @brief 获取当前地址所在扇区的扇区大小
  * @param start_addr-当前flash地址
  * @reval 当前地址所在扇区的扇区大小
  */
int flash_get_page_size_by_addr(uint32_t start_addr)
{
	if(start_addr<FLASH_BASE || start_addr>=FLASH_BASE+FLASH_SIZE) {
		return -1;
	}
	
	uint32_t tmp_addr = FLASH_BASE;
	int i = 0;
	while(i < SEC_CNT)
	{
		if(tmp_addr >= start_addr)
			return sector_arr[i].size;
		tmp_addr += sector_arr[i].size;
		i++;
	}
	
	return -1;
}

/*
 * @brief: STM32F407片内flash擦除函数
 * @param: start_addr-待擦除flash的开始地址
 *         len-擦除的长度
 * @ret:   实际擦除的长度
 */
int flash_erase(uint32_t start_addr, int len)
{
	//地址不能超出FLASH的范围
	if(start_addr<FLASH_BASE || start_addr+len>=FLASH_BASE+FLASH_SIZE) {
		return -1;
	}
	
//	d_printf("[%s] erase addr = 0x%X, len = %d\n\r", __FUNCTION__, start_addr, len);  //debug print
	
	FLASH_Status status;
	const sector_info_t *p_sector;
	int offset = 0;
	ALLOC_CPU_SR();
	ENTER_CRITICAL();
	FLASH_Unlock();              //STM32 Flash写与擦之前都要先解锁
	FLASH_DataCacheCmd(DISABLE); //FLASH 擦与写期间,必须禁止数据缓存
	while(offset < len)
	{
		p_sector = get_sector_info(start_addr+offset);
		if(p_sector == NULL) {
			offset = -2;
			break;
		}
		FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_OPERR|FLASH_FLAG_WRPERR| \
                        FLASH_FLAG_PGAERR|FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
		status = FLASH_EraseSector(p_sector->describe, VoltageRange_3);
		if(FLASH_COMPLETE != status) {
			offset = -3;
			break;
		}
		offset += p_sector->size;
	}
	FLASH_DataCacheCmd(ENABLE);  //FLASH擦写结束,开启数据缓存
	FLASH_Lock();
	EXIT_CRITICAL();

	return offset;
}

/*
 * @breif: STM32F4407片内flash写函数\
 * @param: start_addr-待写flash的开始地址
 *         buff-待写数据的开始地址
 *         len-待写数据的长度
 * @ret:   实际写入flash的数据量, 负数表示写入失败
 */ 
int flash_write(uint32_t start_addr, const void *buff, int len)
{
	//写入地址也不能超出FLASH的范围
	if(start_addr<FLASH_BASE || start_addr+len>=FLASH_BASE+FLASH_SIZE) {
		return 0;
	}
	
	int ret = len;
	unsigned long datp = (unsigned long)buff;
	
	FLASH_Unlock();              //STM32 Flash写与擦之前都要先解锁
	FLASH_DataCacheCmd(DISABLE); //FLASH 擦与写期间,必须禁止数据缓存

	if(len > 16) {
		/* 先让flash地址4字节对齐 */
		while(start_addr%4 != 0)
		{
			if(FLASH_ProgramByte(start_addr, *((uint8_t *)datp)) != FLASH_COMPLETE) {
				ret = -1;
				len = 0;
				break;
			}
			start_addr++;
			datp++;
			len--;
		}
	
		/* flash地址4字节对齐后就可以按字写flash了 */
		uint32_t word;
		int loop_cnt = len/4;
		len -= 4*loop_cnt;
		while(loop_cnt--)
		{
			if((datp&0x03) == 0) //数据地址也4字节对齐了, 可以直接取数据
			{	word = *((uint32_t *)datp);	}
			else {               //数据地址没有4字节对齐, 需要拼装
				uint8_t *byte = (uint8_t *)datp;
				word = ((uint32_t)byte[3]<<24) | ((uint32_t)byte[2]<<16) | ((uint32_t)byte[1]<<8) | byte[0];
			}
			if(FLASH_ProgramWord(start_addr, word) != FLASH_COMPLETE) {
				ret = -2;
				len = 0;
				break;
			}
			start_addr += 4;
			datp += 4;
		}
	}

	while(len--)
	{
		if(FLASH_ProgramByte(start_addr, *((uint8_t *)datp)) != FLASH_COMPLETE) {
			ret = -3;
			break;
		}
		start_addr++;
		datp++;		
	}
	
	FLASH_DataCacheCmd(ENABLE);  //FLASH擦写结束,开启数据缓存
	FLASH_Lock();

	return ret;
}

//没必要提供这个函数,因为flash可以像SRAM一样读.
//int flash_read(uint32_t start_addr, char *buff, int len)
//{
//	return len;
//}




