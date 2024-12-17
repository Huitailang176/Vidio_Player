//user 
#include "user_task.h"
#include "share.h"
#include "libc.h"
#include "my_printf.h"
#include "my_malloc.h"
#include "time.h"
#include "myfat.h"
#include "button_menu.h"

//board device
#include "sdio_sdcard_v5.h"
#include "spi_flash_v4.h"
#include "led.h"
#include "lcd_app_v2.h"
#include "key.h"

//hardware controller
#include "cpu_related_v3.h"
#include "systick.h"
#include "uart.h"
#include "spi_v3.h"
#include "sdio_v3.h"
#include "rtc_v2.h"
#include "fsmc.h"
#include "audio_card.h"

static unsigned int speed_cnt=0;
/*
 * 每次调用task_scheduler()函数都会执行此函数
 */
static void task_idle(void)
{//可以在此函数中喂一下看门狗
	speed_cnt++;
}

void task_10ms_tick(void)
{// user 10ms tasks
	sf_10ms_tick();

	key_10ms_scan();
}

//100ms定时器
void task_100ms_timer(void)
{// user 100ms tasks
	LED0Toggle();
	LED1Toggle();
}

int print_switch = 0;
//1s定时器
void task_1s_timer(void)
{// user 1s tasks

	speed_cnt=0;
}

static task_control_block_t tcb_idle = 
{
	.name = "idle",
	.state = TS_STOPPED,
	.is_always_alive = 1,
	.action = task_idle,
	.mark = 0,
};

task_control_block_t tcb_10ms =
{
	.name = "10ms tick",
	.state = TS_STOPPED,
	.is_always_alive = 0,
	.action = task_10ms_tick,
	.mark = 0,
};

task_control_block_t tcb_100ms =
{
	.name = "100ms timer",
	.state = TS_STOPPED,
	.is_always_alive = 0,
	.action = task_100ms_timer,
	.mark = 0,
};

task_control_block_t tcb_1s =
{
	.name = "1s timer",
	.state = TS_STOPPED,
	.is_always_alive = 0,
	.action = task_1s_timer,
	.mark = CPU_USAGE_CAL_MARK,
};


void hardware_init(void)
{
	int ret, try_cnt;

	disable_irq();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
	set_align8stk_whenhandler();

	fsmc_controller_init();         //有些外设初始化时需要申请内存
	mem_init();                     //外设初始化时可能会申请内存
	systick_init();                 //有些外设初始化时需要延时

	uart1_init(115200);
//	uart3_init();         //debug uart

	key_init();
	led_init();
	lcd_init();
	rtc_init();
	spi_controller_init(SPI3);
//	spi_set_clkpres(SPI3, SPI_BaudRatePrescaler_4);  //Not Necessary
	ret = sf_probe();
	try_cnt = 0;
	while(ret != 0)
	{
		my_printf("Detecting SPI Flash...\r\n");
		try_cnt++;
		if(try_cnt > 10)
		{
			my_printf("SPI Flash detect fail!\n\r");
			my_printf("Program died!\n\r");
			while(1);    //die here
		}
		delay_ms(10);
		ret = sf_probe();	
	}

	ret = fatfs_init();
	if(ret) {
		my_printf("FatFs mount fail, ret=%d\n\r", ret);
		my_printf("Program died\n\r");
		while(1);
	}

	enable_irq();

	char buff[128];
	u_snprintf(buff, sizeof(buff), "APP Build time: %s-%s", __DATE__, __TIME__);
	my_printf("%s\n\r", buff);
	lcd_show_string(0, 0, lcd_dev.xres, 16, 16, buff, 0, lcd_dev.forecolor);
	delay_ms(100);

	audio_card_init(30);
	delay_ms(100);

//	sd_init();
	if(0 == mount_sd()) {  //挂载SD
		change_drv(1);     //设置SD为当前默认的磁盘
	}

	work_dir_check_list();
	delay_ms(800);
	start_menu_load();
}


void task_init(void)
{
	task_create(&tcb_idle);
	task_create(&tcb_10ms);
	task_create(&tcb_100ms);
	task_create(&tcb_1s);
}




