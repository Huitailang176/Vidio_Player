#ifndef  __KEY_H_
#define  __KEY_H_

#ifdef  __cplusplus
    extern "C" {
#endif

#include "stm32f4xx.h"
		
typedef enum
{
	KEY_SEL,
	KEY_DEC,
	KEY_INC,
	KEY_NUM,
}KEY_ID;


/* PuZhong_STM32F407_T100:  KEY1-PE3  KEY0-PE4  KEY_UP-PA0 */

#define  KEY_SEL_PORT  GPIOE
#define  KEY_SEL_PIN   GPIO_Pin_3

#define  KEY_DEC_PORT  GPIOE
#define  KEY_DEC_PIN   GPIO_Pin_4

#define  KEY_INC_PORT  GPIOA
#define  KEY_INC_PIN   GPIO_Pin_0


typedef struct key_obj_s
{
	void (*key_down_cb)(void);         //按下的事件的回调函数
	void (*key_up_cb)(void);           //弹起的事件的回调函数
	void (*key_continuous_cb)(void);   //连续按压不松手事件的回调函数
	uint16_t con_tri_cnt;        //连续按下多长时间认为发生了连按事件
	uint16_t con_act_cnt;        //连按事件的回调函数执行间隔
	uint16_t press_cnt;      //按压计数
	short    noise_cnt;      //消抖计数
}key_obj_t;

//系统调用
void key_init(void);
void key_10ms_scan(void);

//用户调用
int  key_register_cb(int id, void (*down_cb)(void), void (*up_cb)(void), void (*con_cb)(void), uint16_t tri_time, uint16_t deta_time);
int  key_unregister_cb(int id);
		
		
#ifdef  __cplusplus
}  
#endif

#endif


