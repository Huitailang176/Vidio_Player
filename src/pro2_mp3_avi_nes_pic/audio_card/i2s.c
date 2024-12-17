#include "i2s.h"
#include <stddef.h>

//==========================================================================================
//I2S Controoler 

void i2s_controller_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	DMA_InitTypeDef   DMA_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	
	//=========================管脚配置===============================
	//PB12--I2S2_LRCK  PB13--I2S2_SCLK
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
		
	//PC2--I2S2_SDIN  PC3--I2S2_SDOUT  PC6--I2S2_MCLK
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_6;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
		
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource12,GPIO_AF_SPI2);  //PB12,AF5  I2S_LRCK
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource13,GPIO_AF_SPI2);	//PB13,AF5  I2S_SCLK 
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource3,GPIO_AF_SPI2);	//PC3, AF5  I2S_DACDATA 
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource6,GPIO_AF_SPI2);	//PC6, AF5  I2S_MCK
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource2,GPIO_AF6_SPI2);	//PC2, AF6  I2S_ADCDATA  I2S2ext_SD是AF6!!!

	//为什么有2个数据管脚? 2024-09-03
	//个人猜想:  PC3_I2S2_SD 一个管脚可以实现半双工收发
	//           PC3_I2S2_SD + PC2_I2S2ext_SD 2个管脚可以实现全双工收发  (待实验验证)

	//===========================================================
	//采用循环方式发送音频数据测试(要进行发送缓冲区非空检查), Not Necessary
//	i2s_controller_cfg(I2S_Mode_MasterTx, I2S_Standard_Phillips, I2S_DataFormat_16b, 8000);
//	SPI_I2S_SendData(SPI2, 0x1122);
//	while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);  //不进行发送缓冲区非空检查会发送失败
//	SPI_I2S_SendData(SPI2, 0x3344);
//	while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);	
//	SPI_I2S_SendData(SPI2, 0x5566);
//	while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);	
//	SPI_I2S_SendData(SPI2, 0x7788);
//	while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);

	//=========================DMA配置===============================
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);  //DMA1时钟使能
	
	//I2S发送通道
	DMA_DeInit(DMA1_Stream4);
	while(DMA_GetCmdStatus(DMA1_Stream4) != DISABLE)
	{	}

	DMA_InitStructure.DMA_Channel = DMA_Channel_0;                       //DMA传输触发源 SPI2_TX
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR;      //DMA外设地址
	DMA_InitStructure.DMA_Memory0BaseAddr = 0x20000000;                  //DMA存储器0地址   dummy
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;              //memory-->外设
	DMA_InitStructure.DMA_BufferSize = 0;                                //数据传输量        dummy
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;    //外设非增量模式
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;             //存储器增量模式
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//外设数据长度:16位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;        //存储器数据长度:16位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;                       //DMA_Mode_Normal
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;               // 非常高优先级
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;          //存储器突发单次传输
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  //外设突发单次传输
	DMA_Init(DMA1_Stream4, &DMA_InitStructure);     //初始化DMA Stream
	
	DMA_DoubleBufferModeConfig(DMA1_Stream4,0x20008000, DMA_Memory_0);   //双缓冲模式配置
	DMA_DoubleBufferModeCmd(DMA1_Stream4, ENABLE);                       //双缓冲模式开启

	DMA_Cmd(DMA1_Stream4, DISABLE);                    //需要时再使能DMA
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);  //需要时再打开
	
	//I2S接收通道
	DMA_DeInit(DMA1_Stream3);
	while(DMA_GetCmdStatus(DMA1_Stream3) != DISABLE)
	{	}
	
	DMA_InitStructure.DMA_Channel = DMA_Channel_3;                       //DMA传输触发源 I2S2_EXT_RX
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&I2S2ext->DR;   //DMA外设地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;              //外设-->memory
	DMA_Init(DMA1_Stream3, &DMA_InitStructure);     //初始化DMA Stream
	
	DMA_DoubleBufferModeConfig(DMA1_Stream3,0x2000C000, DMA_Memory_0);   //双缓冲模式配置
	DMA_DoubleBufferModeCmd(DMA1_Stream3, ENABLE);                       //双缓冲模式开启
	
	DMA_Cmd(DMA1_Stream3, DISABLE);                      //需要时再使能DMA
	SPI_I2S_DMACmd(I2S2ext, SPI_I2S_DMAReq_Rx, DISABLE); //需要时再打开 
	
	//=========================DMA中断配置===============================
	//DMA发送完成中断
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream4_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        //响应优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);	

	DMA_ITConfig(DMA1_Stream4, DMA_IT_TC, ENABLE);    //开启DMA传输完成与传输错误中断
	DMA_ITConfig(DMA1_Stream4, DMA_IT_TE, ENABLE);
	
	//DMA接收完成中断
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream3_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;        //响应优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);	

	DMA_ITConfig(DMA1_Stream3, DMA_IT_TC, ENABLE);    //开启DMA传输完成与传输错误中断
	DMA_ITConfig(DMA1_Stream3, DMA_IT_TE, ENABLE);
}


//音频采样频率与I2S总线接口几个时钟频率的关系: 
//I2S_LRCK = 音频的采样频率
//I2S_MCK  = 256*I2S_LRCK (固定是这样, 有的硬件是384*I2S_LRCK)
//I2S_SCLK = 2*I2S_LRCK*采样分辨率 (音频数据无扩展时是这样)
//I2S_SCLK = 2*I2S_LRCK*采样分辨率*2 16位或者24位音频数据扩展成32位传输)

/** 采样率计算公式:Fs =I2SxCLK/[256*(2*I2SDIV+ODD)]
 ** I2SxCLK = (HSE/pllm)* PLLI2SN / PLLI2SR 
 ** HSE=8Mhz, pllm=8, 在system_stm32f4xx.c文件中配置系统时钟时确定
 ** PLLI2SN∈[192, 432]  PLLI2SR∈[2, 7]
 ** I2SDIV∈[2, 255]   ODD 0/1
 ** 表格式:采样率/10,PLLI2SN,PLLI2SR,I2SDIV,ODD
 **/
/*
static const uint16_t i2s_clk_tab[][5]=
{
	{800 ,256,5,12,1},		//8Khz采样率
	{1102,429,4,19,0},		//11.025Khz采样率 
	{1600,213,2,13,0},		//16Khz采样率
	{2205,429,4, 9,1},		//22.05Khz采样率
	{3200,213,2, 6,1},		//32Khz采样率
	{4410,271,2, 6,0},		//44.1Khz采样率
	{4800,258,3, 3,1},		//48Khz采样率
	{8820,316,2, 3,1},		//88.2Khz采样率
	{9600,344,2, 3,1},  	//96Khz采样率
	{17640,361,2,2,0},  	//176.4Khz采样率 
	{19200,393,2,2,0},  	//192Khz采样率
};
*/

//取代上面的表格, 根据目标频率自动计算4个参数
static int auto_cal_reg_param(uint32_t tar_freq, uint32_t *reg_param, uint32_t param_size)
{
	if(param_size < 4*sizeof(reg_param[0]))
		return -1;
	
	uint32_t record_deta = 0xFFFFFFFF;
	uint32_t pll_n, pll_r, div;
	uint32_t tmp_freq, tmp_deta;
	uint32_t cnt;

	//执行时间不超过800us
	for(pll_n=192; pll_n<=432; pll_n++)
	{
		for(pll_r=2; pll_r<=7; pll_r++)
		{
			div = pll_n*1000000/pll_r/256/tar_freq;
			cnt = 2;       //误差最小的值可能在div处, 也可能在div+1处, 所以此处要循环2次
			do
			{
				if(div>=4 && div<=511)
				{
					tmp_freq = pll_n*1000000/pll_r/256/div;
					if(tmp_freq > tar_freq)
						tmp_deta = tmp_freq - tar_freq;
					else
						tmp_deta = tar_freq - tmp_freq;
					
					if(tmp_deta < record_deta)
					{
						record_deta = tmp_deta;
						reg_param[0] = pll_n;
						reg_param[1] = pll_r;
						reg_param[2] = div/2;
						reg_param[3] = div%2;
						if(record_deta == 0)
							return 0;
					}
				}
				div++;
				cnt--;
			} while(cnt > 0);			
		}
	}

	if(record_deta == 0xFFFFFFFF)
		return -2;
	
	return record_deta;
}

/** @brief 配置I2S Controller的相关属性
 **
 **/
int i2s_controller_cfg(uint16_t I2S_Standard, uint16_t I2S_DataFormat, uint32_t sample_rate)
{
	I2S_InitTypeDef I2S_InitStructure;
	
	I2S_Cmd(SPI2, DISABLE);
	I2S_Cmd(I2S2ext, DISABLE);
	SPI_I2S_DeInit(SPI2);

	//====================配置Controller的属性====================
	I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;
	I2S_InitStructure.I2S_Standard = I2S_Standard;
	I2S_InitStructure.I2S_DataFormat = I2S_DataFormat;
	I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;  //在下面通过查表配置
	I2S_InitStructure.I2S_AudioFreq = I2S_AudioFreq_Default;    //在下面通过查表配置
	I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;                  //SCLK在空闲时为低电平
	I2S_Init(SPI2, &I2S_InitStructure); 
	
//	I2S_InitStructure.I2S_Mode = I2S_Mode_SlaveTx;     //不使用全双工
//	I2S_FullDuplexConfig(I2S2ext, &I2S_InitStructure);
	
	//========配置PLL输出的I2SCLK & Controller的分频系数==========
	int ret = i2s_frequency_adjust(sample_rate);;
	if(ret < 0)
		return -1;

	/* 最后使能I2S2 COntroller */
	I2S_Cmd(SPI2, ENABLE);
//	I2S_Cmd(I2S2ext, ENABLE);    //不使用全双工

	return 0;
}

/** @brief 改变I2S 音频的采样频率
  * @param new_freq 新的频率
  * @reval 0-成功 非零-失败
  */
int i2s_frequency_adjust(uint32_t new_freq)
{
	//====================配置PLL输出的I2SCLK====================
	uint32_t reg_param[4];
	int ret = auto_cal_reg_param(new_freq, reg_param, sizeof(reg_param));
	if(ret < 0)
		return -1;

	/* 先关闭PLLI2S */
	RCC->CR &= ~(uint32_t)RCC_CR_PLLI2SON;  //RCC_PLLI2SCmd(DISABLE);	

	/* 设置PLL的N R */
	//PLLI2SN∈[192, 432]  PLLI2SR∈[2, 7]
	RCC_PLLI2SConfig(reg_param[0], reg_param[1]);
	
	/* 开启PLLI2S */
	RCC->CR |= RCC_CR_PLLI2SON;  //RCC_PLLI2SCmd(ENABLE);
	
	/* Wait till the main PLL is ready */
    while((RCC->CR & RCC_CR_PLLI2SRDY) == 0)
    {
    }
	
	//====================配置Controller的分频系数====================
	SPI2->I2SPR = I2S_MCLKOutput_Enable| (reg_param[3]<<8) | reg_param[2];	
	
	return 0;
}


//DMA发送完成后的回调函数
static void (*i2s_tx_callback)(uint32_t) = NULL;

//DMA接收完成后的回调函数
static void (*i2s_rx_callback)(uint32_t) = NULL;


/**
 ** @brief: 注册获取DMA发送完成后的回调函数
 */
void i2s_register_tx_cb(void (*tx_cb_func)(uint32_t))
{
	i2s_tx_callback = tx_cb_func;
}

/**
 ** @brief: 注册获取DMA接收完成后的回调函数
 */
void i2s_register_rx_cb(void (*rx_cb_func)(uint32_t))
{
	i2s_rx_callback = rx_cb_func;
}


/** @brief 启动I2S数据发送
 ** @param buff--数据所在的内存地址
 **        len--以uint16_t为单位, 数据的大小
 ** @reval 0-启动成功  非0-启动失败
 */
int  i2s_start_tx(uint16_t *tx_buf, uint16_t *tx_buf2, uint16_t tx_len)
{
	if(tx_buf==NULL || tx_buf2==NULL || tx_len==0)
	{
		return -1;
	}

	/* I2S发送配置 */
	DMA_Cmd(DMA1_Stream4, DISABLE);                      //先关闭
	while(DMA_GetCmdStatus(DMA1_Stream4) != DISABLE)
	{	}
	DMA_ClearFlag(DMA1_Stream4, DMA_IT_TCIF4|DMA_IT_TEIF4);
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);

	DMA1_Stream4->M0AR = (uint32_t)tx_buf;
	DMA1_Stream4->NDTR = tx_len;
	DMA_DoubleBufferModeConfig(DMA1_Stream4, (uint32_t)tx_buf2, DMA_Memory_0);

	i2s_tx_dma_irq_enable();
	DMA_Cmd(DMA1_Stream4, ENABLE);                      //这2个开关打开后将启动DMA传输
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);

	return 0;	
}

int i2s_start_rx(uint16_t *rx_buf, uint16_t *rx_buf2, uint16_t rx_len)
{
	if(rx_buf==NULL || rx_buf2==NULL || rx_len==0)
	{
		return -1;
	}
	
	/* I2S接收配置 */
	DMA_Cmd(DMA1_Stream3, DISABLE);
	while(DMA_GetCmdStatus(DMA1_Stream3) != DISABLE)
	{	}
	DMA_ClearFlag(DMA1_Stream3, DMA_IT_TCIF3|DMA_IT_TEIF3);
	SPI_I2S_DMACmd(I2S2ext, SPI_I2S_DMAReq_Rx, DISABLE);

	DMA1_Stream3->M0AR = (uint32_t)rx_buf;
	DMA1_Stream3->NDTR = rx_len;
	DMA_DoubleBufferModeConfig(DMA1_Stream3, (uint32_t)rx_buf2, DMA_Memory_0);	
	
	i2s_rx_dma_irq_enable();
	DMA_Cmd(DMA1_Stream3, ENABLE);
	SPI_I2S_DMACmd(I2S2ext, SPI_I2S_DMAReq_Rx, ENABLE);
	
	return 0;
}

/*
 * @brief: 停止音频播放, 调用此函数
 */
int  i2s_stop_tx(void)
{
	i2s_tx_dma_irq_disable();
	DMA_Cmd(DMA1_Stream4, DISABLE);
	DMA_ClearFlag(DMA1_Stream4, DMA_IT_TCIF4|DMA_IT_TEIF4);  //F407 DMA使能之后,单独运行关闭使能语句, 会改写中断传输标志位,
	                                                         //从而触发一次中断.所以此处有必要清除一次
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);
	I2S_Cmd(SPI2, DISABLE);

	i2s_tx_callback = NULL;

	return 0;
}

/*
 * @brief: 停止录音, 调用此函数
 */
int i2s_stop_rx(void)
{
	int ret = DMA_GetCurrDataCounter(DMA1_Stream3);
	
//	i2s_tx_release();          //接收是靠发送驱动的, 有接收一定有发送

	i2s_rx_dma_irq_disable();
	DMA_Cmd(DMA1_Stream3, DISABLE);
	DMA_ClearFlag(DMA1_Stream3, DMA_IT_TCIF3|DMA_IT_TEIF3);  //F407 DMA使能之后,单独运行关闭使能语句, 会改写中断传输标志位,
	                                                         //从而触发一次中断.所以此处有必要清除一次
	SPI_I2S_DMACmd(I2S2ext, SPI_I2S_DMAReq_Rx, DISABLE);
	I2S_Cmd(I2S2ext, DISABLE);

	i2s_rx_callback = NULL;

	return ret;
}

//DMA发送完成中断服务函数
void DMA1_Stream4_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_Stream4, DMA_IT_TCIF4) == SET)
	{
		DMA_ClearITPendingBit(DMA1_Stream4, DMA_IT_TCIF4);

		if(i2s_tx_callback != NULL)
		{
			uint32_t cur_buff_idx = DMA_GetCurrentMemoryTarget(DMA1_Stream4);
			(*i2s_tx_callback)(cur_buff_idx);
		}
		else
		{	/* 没有注册回调函数, 中断没有意义,直接关闭中断 */
			i2s_tx_dma_irq_disable();
		}
	}
	else
	{
		DMA_ClearITPendingBit(DMA1_Stream4, DMA_IT_TEIF4);
	}
}

//DMA接收完成中断服务函数
void DMA1_Stream3_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_Stream3, DMA_IT_TCIF3) == SET)
	{
		DMA_ClearITPendingBit(DMA1_Stream3, DMA_IT_TCIF3);

		if(i2s_rx_callback != NULL)
		{
			uint32_t cur_buff_idx = DMA_GetCurrentMemoryTarget(DMA1_Stream3);
			(*i2s_rx_callback)(cur_buff_idx);
		}
		else
		{	/* 没有注册回调函数, 中断没有意义,直接关闭中断 */
			i2s_rx_dma_irq_disable();
		}
	}
	else
	{
		DMA_ClearITPendingBit(DMA1_Stream3, DMA_IT_TEIF3);
	}	
}













