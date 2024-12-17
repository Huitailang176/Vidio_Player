#include "systick.h"
#include "cpu_related_v3.h"

volatile unsigned long tick_cnt = 0;

//static uint32_t fac_us=0;        //us延时倍乘数
static uint32_t systick_freq = 0;  //systick定时器的时钟频率       

void systick_init(void)
{
	uint32_t reload;

	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//选择外部时钟  HCLK/8
//	fac_us=SystemCoreClock/8000000;				//为系统时钟的1/8
	systick_freq = SystemCoreClock/8;
 
	reload=SystemCoreClock/8000000;				//每秒钟的计数次数 单位为K	   
	reload*=1000000/TICK_FREQ_HZ;		        //根据HZ设定溢出时间
												//reload为24位寄存器,最大值:16777216,在72M下,约合1.86s左右   

	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	//开启SYSTICK中断
	SysTick->LOAD=reload-1; 				    //每1/delay_ostickspersec秒中断一次	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	//开启SYSTICK

	//把Systick异常的优先级设置为最低
	uint32_t priority = NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 3, 3);
	NVIC_SetPriority(SysTick_IRQn, priority);
}

//===========================================================================
void delay_us(uint32_t us)
{
	uint32_t ticks;
	uint32_t told,tnow,tcnt;
	
	told=SysTick->VAL;        					//刚进入时的计数器值
	ticks=us*(systick_freq/1000000); 			    //需要的节拍数	  		 
	tcnt=0;

	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow != told)
		{	    
			if(tnow < told)
				tcnt += told-tnow;		//这里注意一下SYSTICK是一个递减的计数器就可以了.
			else
				tcnt += SysTick->LOAD - tnow + told +1;	    
			if(tcnt>=ticks)
				break;				//时间超过/等于要延迟的时间,则退出.
			told = tnow;
		}  
	}		
}

void delay_ms(uint32_t ms)
{
	delay_us(1000*ms);
}


//===========================================================================
long sum_ticks = 0;
long old_ticks = 0;

//设置超时时间, 参数范围 ms*1000+us < 2^31,  us<1000
void timeout_set(int ms, int us)    //需要在is_timeout_ms()函数前面调用一次该函数
{
	sum_ticks = (long)(systick_freq/1000000) * (ms*1000 + us);
	old_ticks = SysTick->VAL & SysTick_LOAD_RELOAD_Msk;
}

//超时检测函数  ret:0-没有超时  1-已经超时
int is_timeout(void)
{
	int deta;
	int now  = SysTick->VAL & SysTick_LOAD_RELOAD_Msk;
	
	if(now < old_ticks)
		deta = old_ticks-now;
	else
		deta = SysTick->LOAD - now + old_ticks;
	old_ticks = now;
	sum_ticks -= deta;
	if(sum_ticks <= 0)
		return 1;
	else
		return 0;
}
/* 
使用举例:
timeout_clear();
while(1)
{
	...

	if(is_timeout(600, 0))   //600ms后会自动退出while(1)死循环
		break;
}

*/

//===========================================================================
/* 时间戳功能(需要结合tick_cnt)实现 */
int64_t get_sys_timestamp(void)  //单位: us
{
	int64_t timestamp;
	
	ALLOC_CPU_SR();
	ENTER_CRITICAL();   //现在分辨率为1us, 可以修该为 1/systick时钟频率
	timestamp = (int64_t)tick_cnt*(1000000/TICK_FREQ_HZ)+ (SysTick->LOAD - SysTick->VAL + 1)/(systick_freq/1000000);
	EXIT_CRITICAL();
	return timestamp;
}


