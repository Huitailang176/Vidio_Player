#include "nes_emulator.h"
#include "nes_game.h"

#include "my_printf.h"
#include "user_task.h"
#include "lcd_app_v2.h"
#include "gamepad.h"
#include "flash.h"
#include "media_timer_v2.h"

#include "dir_list.h"
#include "key.h"
#include <stddef.h>

//NES文件在STM32Flash中的偏移(注意:偏移量必须按页对齐, 且不能覆盖用户程序)
#define  NES_FLASH_OFFSET  384*1024

#define  NES_LOAD_FLASH_ADDR  (FLASH_BASE + (NES_FLASH_OFFSET))
#define  NES_MAX_FLASH_SIZE   (FLASH_SIZE - NES_FLASH_OFFSET)

//nes游戏Task Control Block
static task_control_block_t tcb_nes_emulator;

static void nes_timer_start(void);

/** @brief 加载nes游戏
 **/
int nes_emulator_load_game(char *fname)
{
	int ret;

	lcd_dev.backcolor = LCD_BLACK;
	lcd_dev.forecolor = LCD_MAGENTA;
	lcd_clear(lcd_dev.backcolor);

	ret = nes_game_load(fname, NES_LOAD_FLASH_ADDR, NES_MAX_FLASH_SIZE);
	if(ret != 0)
	{
		lcd_show_string(64, 120, lcd_dev.xres-64, 16, 16, "Error!!!", 0, lcd_dev.forecolor);
		return ret;
	}

	gamepad_init();

	//开始运行游戏
	tcb_nes_emulator.name = "nes_emulator";
	tcb_nes_emulator.state = TS_STOPPED;
	tcb_nes_emulator.is_always_alive = 0;
	tcb_nes_emulator.action = nes_emulator_cycle;
	tcb_nes_emulator.mark = 0;
	task_create(&tcb_nes_emulator);  //创建nes任务
	task_wakeup(&tcb_nes_emulator);  //唤醒nes任务

	nes_timer_start();

	return 0;
}

static int timer_cycle_cnt = 0;
static void nes_timer_isr(void);

static void nes_timer_start(void)
{
	//启动一个16.64ms的定时器来统计帧率
	//100KHz即10us, 10us*1664=16.64ms
	media_timer_start(100000, 1664, nes_timer_isr);
	
	timer_cycle_cnt = 0;
}

static void nes_timer_stop(void)
{
	media_timer_stop();
}

//Timer定时周期 16.64ms
static void nes_timer_isr(void)
{	
	timer_cycle_cnt++;
	if(timer_cycle_cnt % 3 == 0)
	{
		task_wakeup(&tcb_nes_emulator);

		if(timer_cycle_cnt % 60 == 0)
		{
			tcb_nes_emulator.mark = 0x01;
		}
	}
}

void nes_emulator_cycle(void)
{
	nes_game_run();

	if(unlikely(tcb_nes_emulator.mark == 0x01))
	{
		nes_game_1s_hook();
		tcb_nes_emulator.mark = 0;
	}
}


void nes_emulator_stop(void)
{
	nes_timer_stop();
	task_delete(&tcb_nes_emulator);  //删除nes任务
	nes_game_stop();

	gamepad_deinit();
	lcd_dma_trans_release();
	lcd_clear(lcd_dev.backcolor);

	key_unregister_cb(KEY_DEC);      //只保留退出按键
	key_unregister_cb(KEY_INC);
}

//================================================================================================
//按键控制

static void close_game(void);
static void inc_game_volume(void);
static void dec_game_volume(void);


int nes_emulator_exec(char *fname)
{
	int ret;
//	my_printf("nes emulator execute: %s\n\r", fname);

	key_register_cb(KEY_SEL, close_game, NULL, NULL, 0, 0);   //注册所有按键
	key_register_cb(KEY_DEC, dec_game_volume, NULL, dec_game_volume, 200, 20);
	key_register_cb(KEY_INC, inc_game_volume, NULL, inc_game_volume, 200, 20);

	ret = nes_emulator_load_game(fname);
	return ret;
}


static void close_game(void)
{
	my_printf("Close game\n\r");
	nes_emulator_stop();

	dir_list_form_reload();
}

static void inc_game_volume(void)
{
	int volume = nes_game_get_volume();
	if(volume < 100)
	{
		volume++;
		nes_game_set_volume(volume);
	}
}

static void dec_game_volume(void)
{
	int volume = nes_game_get_volume();
	if(volume > 0)
	{
		volume--;
		nes_game_set_volume(volume);
	}	
}



