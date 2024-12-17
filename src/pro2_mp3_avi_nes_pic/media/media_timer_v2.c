#include "media_timer_v2.h"
#include "stm32f4xx.h"

#include <stddef.h>

static void (*media_timer_isr)(void);

//启动一个定时器来周期性的回调beat_callback函数, 
//回调周期 (1/beat_freq)*beat_count
void media_timer_start(int beat_freq, int beat_count, void (*beat_callback)(void))
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	RCC_ClocksTypeDef RCC_ClocksStruct;
	uint16_t prescaler;
	
	TIM_Cmd(TIM7, DISABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);  //使能TIM7时钟
	TIM_DeInit(TIM7);
	TIM_InternalClockConfig(TIM7);                        //使用内部时钟	
	
	//TIM7 Time Base config
	RCC_GetClocksFreq(&RCC_ClocksStruct);
	prescaler = RCC_ClocksStruct.PCLK1_Frequency*2/beat_freq;   //分频后得到一个beat_freq(Hz)的时钟
	TIM_TimeBaseStructure.TIM_Prescaler = prescaler-1; //预分频值  分频后得到一个beat_freq(Hz)的信号
	TIM_TimeBaseStructure.TIM_Period = beat_count-1;   //计数器自动重载值
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);
	
	TIM_ARRPreloadConfig(TIM7, DISABLE);          //更新ARR后立即生效

	NVIC_InitTypeDef  NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);

	TIM_ClearITPendingBit(TIM7,TIM_IT_Update);
	TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);

	TIM_Cmd(TIM7, ENABLE);	

	media_timer_isr = beat_callback;
}

void media_timer_stop(void)
{
	TIM_Cmd(TIM7, DISABLE);
	TIM_DeInit(TIM7);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, DISABLE);

	media_timer_isr = NULL;
}

//TIM7更新中断
void TIM7_IRQHandler(void)
{
	TIM_ClearITPendingBit(TIM7, TIM_IT_Update);

	if(media_timer_isr != NULL)
		(*media_timer_isr)();
}

