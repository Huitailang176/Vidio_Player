#ifndef  __SYSTICK_H_
#define  __SYSTICK_H_

#ifdef  __cplusplus
    extern "C" {
#endif

#include "stm32f4xx.h"

//深入挖掘systick的功能,现在用systick共实现了以下4种功能
/* 1.系统节拍 */
#define  TICK_FREQ_HZ  100
extern volatile unsigned long tick_cnt;

void systick_init(void);
		
/* 2.延时函数 */
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

/* 3.超时检测函数 */
void timeout_set(int ms, int us);		
int  is_timeout(void);

/* 4.时间戳功能(需要结合jiffies)实现 */
int64_t get_sys_timestamp(void);  //现在分辨率为1us, 可以修该为 1/systick时钟频率

#ifdef  __cplusplus
}  
#endif


#endif



