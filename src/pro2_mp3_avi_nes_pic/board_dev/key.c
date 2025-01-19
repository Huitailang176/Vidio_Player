#include "key.h"
#include <stddef.h>
#include "user_task.h"
#include "my_printf.h"
#include "share.h"

//系统按键
key_obj_t key_obj[KEY_NUM];


void key_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	/* PuZhong_STM32F407_T100:  KEY1-PE3  KEY0-PE4  KEY_UP-PA0 */
	GPIO_InitStructure.GPIO_Pin = KEY_SEL_PIN;	   //KEY1-PE3		 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; 	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(KEY_SEL_PORT, &GPIO_InitStructure);
	GPIO_SetBits(KEY_SEL_PORT, KEY_SEL_PIN);
	
	GPIO_InitStructure.GPIO_Pin = KEY_DEC_PIN;     //KEY0-PE4
	GPIO_Init(KEY_DEC_PORT, &GPIO_InitStructure);
	GPIO_SetBits(KEY_DEC_PORT, KEY_DEC_PIN);
	
	GPIO_InitStructure.GPIO_Pin = KEY_INC_PIN;     //KEY_UP-PA0
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(KEY_INC_PORT, &GPIO_InitStructure);
	GPIO_ResetBits(KEY_INC_PORT, KEY_INC_PIN);
}


/** @brief 用户调用此函数可以为按键注册一个按下时的回调函数
  * @param id-按键ID
  *        down_cb-按下时的回调函数
  *        up_cb-抬起时的回调函数
  *        con_cb-连按时的回调函数
  *        tri_time-连按事件的触发时间 单位ms
  *        deta_time-连按事件触发后的执行间隔 单位ms  
  */
int  key_register_cb(int id, void (*down_cb)(void), void (*up_cb)(void), void (*con_cb)(void), uint16_t tri_time, uint16_t deta_time)
{
	if(id<0 || id>=KEY_NUM)
		return -1;

	key_obj[id].key_down_cb = down_cb;
	key_obj[id].key_up_cb = up_cb;
	key_obj[id].key_continuous_cb = con_cb;

	key_obj[id].press_cnt = 0x00;
	key_obj[id].noise_cnt = 0x8000;  //按键切换回调函数(或者说注册)后, 确保按键被释放弹起后, 再按下按键回调函数才能生效
	
	if(tri_time < 10)
		tri_time = 10;
	key_obj[id].con_tri_cnt = tri_time/10;

	if(deta_time < 10)
		deta_time = 10;
	key_obj[id].con_act_cnt = deta_time/10;
	
	return 0;
}

/** @brief 用户调用此函数取消对按键的注册
  * @param id-按键ID
  */
int  key_unregister_cb(int id)
{
	if(id<0 || id>=KEY_NUM)
		return -1;
	
	key_obj[id].key_down_cb = NULL;
	key_obj[id].key_up_cb = NULL;
	key_obj[id].key_continuous_cb = NULL;
	
	return 0;
}

/** @brief 读取按键的状态
  * @param key_id 按键ID
  * @reval 1-按下 0-未按下
  */
int key_state_read(int key_id)
{
	switch(key_id)
	{
		case KEY_SEL:
			return (KEY_SEL_PORT->IDR & KEY_SEL_PIN)? 0 : 1;

		case KEY_DEC:
			return (KEY_DEC_PORT->IDR & KEY_DEC_PIN)? 0 : 1;
		
		case KEY_INC:
//			return (KEY_INC_PORT->IDR & KEY_INC_PIN)? 1 : 0;
		return (KEY_INC_PORT->IDR & KEY_INC_PIN)? 0 : 1;
		
		default:
			return 0;
	}
}

/** @breif 按键10ms扫描任务
  */
void key_10ms_scan(void)
{
	int i, key_state;
	
	for(i=0; i<KEY_NUM; i++)
	{
		key_state = key_state_read(i);

		if(unlikely(key_state == 1))
		{
//			my_printf("KEY%d DOWN, %d %d\n\r", i, key_obj[i].noise_cnt, key_obj[i].press_cnt);  //debug print
			if(key_obj[i].noise_cnt < 1)
			{	//10ms-20ms的去抖时间
				key_obj[i].noise_cnt++;
				continue;
			}

			key_obj[i].press_cnt++;
			if(key_obj[i].press_cnt == 1)
			{	//发生了按下事件
				if(key_obj[i].key_down_cb != NULL)
					(*key_obj[i].key_down_cb)();
			}
			if(key_obj[i].key_continuous_cb != NULL)
			{
				if(key_obj[i].press_cnt < key_obj[i].con_tri_cnt)
					continue;
				else if(key_obj[i].press_cnt == key_obj[i].con_tri_cnt)
					(*key_obj[i].key_continuous_cb)();   //首次执行
				else if((key_obj[i].press_cnt-key_obj[i].con_tri_cnt)%key_obj[i].con_act_cnt == 0)
					(*key_obj[i].key_continuous_cb)();   //间隔con_act_cnt执行
			}
			
		}
		else
		{
			if(unlikely(key_obj[i].noise_cnt != 0))
			{	//之前去过抖动, 但不一定发生过按下事件 
//				my_printf("KEY%d UP, %d %d\n\r", i, key_obj[i].noise_cnt, key_obj[i].press_cnt);  //debug print
				key_obj[i].noise_cnt = 0;
				if(key_obj[i].press_cnt != 0) 
				{	//之前发生过按下事件或者连按事件, 现在发生了弹起事件
					key_obj[i].press_cnt = 0;
					if(key_obj[i].key_up_cb != NULL)
						(*key_obj[i].key_up_cb)();
				}
			}
		}
	}
}






