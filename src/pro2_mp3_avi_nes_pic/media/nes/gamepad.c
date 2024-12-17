#include "gamepad.h"

#include "stm32f4xx.h"

//===================================KeyPad=======================================
//Platform: PuZhong_STM32F407_T200

//用2个IO管脚给手柄供电(不推荐使用这种方法)
#define  PAD_VCC_PORT  GPIOE
#define  PAD_VCC_PIN   GPIO_Pin_6

#define  PAD_GND_PORT  GPIOF
#define  PAD_GND_PIN   GPIO_Pin_0


//keypad1 keypad2共用Clock与Latch信号
//PB10--LATCH  PC0--CLK
#define  LATCH_PORT  GPIOF
#define  LATCH_PIN   GPIO_Pin_2
#define  CLOCK_PORT  GPIOF
#define  CLOCK_PIN   GPIO_Pin_4

//keypad1 PF6--DATA1 
#define  DATA1_PORT  GPIOF
#define  DATA1_PIN   GPIO_Pin_6
//keypad2 PB11--DATA2
#define  DATA2_PORT  GPIOF
#define  DATA2_PIN   GPIO_Pin_8

//======================================================================
//keypad控制信号的输出与输入
#define  LATCH_H()   LATCH_PORT->BSRRL = LATCH_PIN
#define  LATCH_L()   LATCH_PORT->BSRRH = LATCH_PIN
#define  CLOCK_H()   CLOCK_PORT->BSRRL = CLOCK_PIN
#define  CLOCK_L()   CLOCK_PORT->BSRRH = CLOCK_PIN

#define  PAD_DATA1() (DATA1_PORT->IDR & DATA1_PIN)
#define  PAD_DATA2() (DATA2_PORT->IDR & DATA2_PIN)

/** @brief keypad控制管脚初始化
  * @tip   LATCH信号--空闲状态下是为低电平
  *        CLOCK信号--空闲状态下是为低电平
  *        DATA信号 --空闲状态下是为高电平
  */
int gamepad_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
 
	//GamePad Power supply
	GPIO_InitStructure.GPIO_Pin = PAD_VCC_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(PAD_VCC_PORT, &GPIO_InitStructure);
	GPIO_SetBits(PAD_VCC_PORT, PAD_VCC_PIN);
	
	GPIO_InitStructure.GPIO_Pin = PAD_GND_PIN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(PAD_GND_PORT, &GPIO_InitStructure);
	GPIO_ResetBits(PAD_GND_PORT, PAD_GND_PIN);	
	
	//初始化4个信号线管脚
	GPIO_InitStructure.GPIO_Pin = LATCH_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(LATCH_PORT, &GPIO_InitStructure);
	LATCH_L();
	
	GPIO_InitStructure.GPIO_Pin = CLOCK_PIN;
	GPIO_Init(CLOCK_PORT, &GPIO_InitStructure);
	CLOCK_L();
	
	GPIO_InitStructure.GPIO_Pin = DATA1_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   //input模式时不用配置这2个参数
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(DATA1_PORT, &GPIO_InitStructure);	      
	GPIO_SetBits(DATA1_PORT, DATA1_PIN);      //数据线要拉高
	
	GPIO_InitStructure.GPIO_Pin = DATA2_PIN;
	GPIO_Init(DATA2_PORT, &GPIO_InitStructure);	
	GPIO_SetBits(DATA2_PORT, DATA2_PIN);      //数据线要拉高

	return 0;
}

void gamepad_deinit(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	//Release power pin
	GPIO_InitStructure.GPIO_Pin = PAD_VCC_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(PAD_VCC_PORT, &GPIO_InitStructure);	      
	GPIO_ResetBits(PAD_VCC_PORT, PAD_VCC_PIN);
	
	GPIO_InitStructure.GPIO_Pin = PAD_GND_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(PAD_GND_PORT, &GPIO_InitStructure);	      
	GPIO_ResetBits(PAD_GND_PORT, PAD_GND_PIN);
	
	//释放4个信号线
	GPIO_InitStructure.GPIO_Pin = LATCH_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   //input模式时不用配置这2个参数
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(LATCH_PORT, &GPIO_InitStructure);	      
	GPIO_ResetBits(LATCH_PORT, LATCH_PIN);
	
	GPIO_InitStructure.GPIO_Pin = CLOCK_PIN;
	GPIO_Init(CLOCK_PORT, &GPIO_InitStructure);
	GPIO_ResetBits(LATCH_PORT, CLOCK_PIN);
	
	GPIO_InitStructure.GPIO_Pin = DATA1_PIN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(DATA1_PORT, &GPIO_InitStructure);	      
	GPIO_SetBits(DATA1_PORT, DATA1_PIN);
	
	GPIO_InitStructure.GPIO_Pin = DATA2_PIN;
	GPIO_Init(DATA2_PORT, &GPIO_InitStructure);	      
	GPIO_SetBits(DATA2_PORT, DATA2_PIN);
}


//CD4021 3-15V供电  5V供电时时钟频率最大3MHz

/** @brief 读取8个按键值
  * @tip   DATA低电平表示按键按下
  * @reval bit15-bit8 手柄1的键值 bit7-bit0手柄2的键值
  */
#define  DELAY_CNT  8      //F407 168MHz下时钟频率约1.2MHz
//#define  DELAY_CNT  4        //F103 128MHz下时钟频率约1.2MHz
int gamepad_read(void)
{
	int i;
	volatile int cnt;
	uint32_t pad1_val = 0;
	uint32_t pad2_val = 0;
	
	LATCH_H();         //锁存8个按键的状态到移位寄存器
	for(cnt=0; cnt<DELAY_CNT; cnt++);
	LATCH_L();         //改用串行方式从CD4021的8个移位寄存器中读数
	for(cnt=0; cnt<DELAY_CNT; cnt++);

	if(PAD_DATA1()==0)
		pad1_val |= 0x80;
	if(PAD_DATA2()==0)
		pad2_val |= 0x80;

	for(i=0; i<7; i++)
	{
		pad1_val >>= 1;
		pad2_val >>= 1;
		
		CLOCK_H();
		for(cnt=0; cnt<DELAY_CNT; cnt++);	
		CLOCK_L();
		for(cnt=0; cnt<DELAY_CNT; cnt++);
		if(PAD_DATA1()==0)
			pad1_val |= 0x80;
		if(PAD_DATA2()==0)
			pad2_val |= 0x80;
	}
	
	//7个脉冲已经把8个按键的状态全都读出来了, 但必须再加一个脉冲,
	//否则下一次读数时, 可能会出错. 这可能是CD4021的问题, 这个BUG坑了我很久.
	CLOCK_H();
	for(cnt=0; cnt<DELAY_CNT; cnt++);
	CLOCK_L();
	for(cnt=0; cnt<DELAY_CNT; cnt++);
	
	return pad1_val<<8 | pad2_val;
}







