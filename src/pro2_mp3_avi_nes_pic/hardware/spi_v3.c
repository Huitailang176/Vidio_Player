#include "spi_v3.h"
#include "share.h"
#include "systick.h"
#include <stddef.h>

/*
 * @brief: spi controller config as master
 */
void spi_controller_init(SPI_TypeDef* SPIx)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
	
	if(SPIx == SPI1) {
//		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); //使能GPIOA时钟
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

		//PA5--SPI1_CLK  PA6-SPI1_MISO  PA7-SPI1_MOSI
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
		
		//SPI1对应引脚复用映射
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_SPI1);
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_SPI1);
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource7,GPIO_AF_SPI1);
		
		/* 把挂接在此总线上的所有从设备的片选拉高(必须, 非常必要) */
//		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;         //LCD_CS
//		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
//		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//		GPIO_Init(GPIOA, &GPIO_InitStructure);
//		GPIO_SetBits(GPIOA, GPIO_Pin_2);
	}
	if(SPIx == SPI3) {
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

		//PB3--SPI3_CLK  PB4-SPI3_MISO  PB5-SPI3_MOSI
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOB, &GPIO_InitStructure);

		//SPI3对应引脚复用映射
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource3, GPIO_AF_SPI3);
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource4, GPIO_AF_SPI3);
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource5, GPIO_AF_SPI3);

		/* 把挂接在此总线上的所有从设备的片选拉高(必须, 非常必要) */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;         //SPI_FLASH_CS
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
		GPIO_SetBits(GPIOB, GPIO_Pin_14);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;         //NRF24L01 CS
		GPIO_Init(GPIOG, &GPIO_InitStructure);
		GPIO_SetBits(GPIOG, GPIO_Pin_7);
	}
	
	SPI_I2S_DeInit(SPIx);  //复位SPIx Controller的所有寄存器
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPIx, &SPI_InitStructure);
	SPI_Cmd(SPIx, ENABLE);

	spi_write_read_byte(SPIx, 0xFF); //dummy send
	if(SPIx == SPI1) {
		SPI1_DMA_Init();
	}
	if(SPIx == SPI3) {
		SPI3_DMA_Init();
	}
}

/*
 * @brief: SPIx数据位宽切换为8bit
 * 使用 SPI_DataSizeConfig(SPI1, SPI_DataSize_8b)
 */
void spi_set_data_8bit(SPI_TypeDef* SPIx)
{
	SPIx->CR1 &= 0xF7BF;            //clear bit11 and bit6
	SPIx->CR1 |= SPI_DataSize_8b;
	SPIx->CR1 |= (uint16_t)0x0040;  //Enable SPIx	
}

/*
 * @brief: SPIx数据位宽切换为16bit
 * 使用 SPI_DataSizeConfig(SPI1, SPI_DataSize_16b)
 */
void spi_set_data_16bit(SPI_TypeDef* SPIx)
{
	SPIx->CR1 &= 0xF7BF;            //clear bit11 and bit6
	SPIx->CR1 |= SPI_DataSize_16b;
	SPIx->CR1 |= (uint16_t)0x0040;  //Enable SPIx
}

/* 工作频率不同、时钟极性不同的SPI设备挂接在同一总线上, 访问不同的设备前要
 * 先切换频率与时钟极性. 这样不仅很恶心, 而且导致效率低, 设计PCB时要尽量避免.
 */

//SPI频率设置, prescaler取值:
//SPI_BaudRatePrescaler_2    (SPI 42M @sys 84M)
//SPI_BaudRatePrescaler_4    (SPI 21M @sys 84M)
//SPI_BaudRatePrescaler_8    (SPI 10.5M @sys 84M)
//SPI_BaudRatePrescaler_16   (SPI 5.25M @sys 84M)
//SPI_BaudRatePrescaler_32   (SPI 2.625M @sys 84M)
//SPI_BaudRatePrescaler_64   (SPI 1.3125M @sys 84M)
//SPI_BaudRatePrescaler_128  (SPI 656.25K @sys 84M)
//SPI_BaudRatePrescaler_256  (SPI 328.125K @sys 84M)
void spi_set_clkpres(SPI_TypeDef* SPIx, uint16_t prescaler)
{
	SPIx->CR1 &= 0xFF87;            //clear bit3-bit6
	SPIx->CR1 |= (uint16_t)prescaler;
	SPIx->CR1 |= (uint16_t)0x0040;  //Enable SPIx
}


//SPI时钟极性与相位设置
//SPI_CPOL: SPI_CPOL_Low, SPI_CPOL_High
//SPI_CPHA: SPI_CPHA_1Edge, SPI_CPHA_2Edge
void spi_set_cpol(SPI_TypeDef* SPIx, uint16_t SPI_CPOL, uint16_t SPI_CPHA)
{
	SPIx->CR1 &= (uint16_t)~(0x0003|0x0040);
	SPIx->CR1 |= (SPI_CPOL|SPI_CPHA);
	SPIx->CR1 |= (uint16_t)0x0040;  //Enable SPI
}

//SPI Controller上会挂接不同的设备, 有时在通信前需要配置Controller的
//频率 数据位宽 时钟极性与相位等信息, 通信结束后要把Controller恢复为
//之前的设置, 于是就设计了这2个获取与恢复Controller配置的函数
int spi_get_config(SPI_TypeDef* SPIx)
{
	return SPIx->CR1;
}

void spi_recover_config(SPI_TypeDef* SPIx, int cr)
{
	SPIx->CR1 = cr;
}

//SPI 收发原理:(对编程非常重要, XX应该是环形移位寄存器, 用户不可操作但Controller硬件可以操作)
//写数据寄存器DR, 会设置SPI_I2S_FLAG_TXE=0(非空), 待Controller硬件把DR中的数据转移到XX, 同时设置SPI_I2S_FLAG_TXE=1(空)
//特别特别特别需要注意的是, SPI_I2S_FLAG_TXE=1(空)并不等于Controller已经把数据已经发送出去了,
//更不等于Controller已经接受到了一个新的数据.
//待Controller硬件把XX中的数据真正发送出去后,也就完成了一个数据的接收, 接收到的数据应该也在XX,
//然后Controller硬件把XX中的数据放DR, 同时设置SPI_I2S_FLAG_RXNE=1(非空), 这时
//读数据寄存器DR, 会拿到接收到的数据, 同时会设置SPI_I2S_FLAG_RXNE=0(空).

//利用DMA发送数据后, 会造成SPI_I2S_FLAG_RXNE=1(非空), 必须通过读DR来清除此标志, 否则利用非DMA
//发送数据会丢失第1字节的数据.

/*
 * @brief: SPIx 收发函数
 *    ret: <0 收发失败;  >=0 收发成功
 */
int spi_write_read_byte(SPI_TypeDef* SPIx, int txdat)
{
	int tryCount = 0;

	SPIx->DR = (uint16_t)txdat;   //发送数据
	while((SPIx->SR & SPI_I2S_FLAG_TXE) == 0)               //等待发送完成
	{                             //0-发送缓冲区非空  1-发送缓冲区为空
		if(++tryCount > 0xFF){
			return -1;            //发送失败 
		}
	}

	while((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET) //等待接收完成
	{                           //0-接收缓冲区为空  1-接收缓冲区非空
		if(++tryCount > 0x1FF) {
			return -2;          //接收失败
		}
	}
	return((uint16_t)SPIx->DR);  //返回接收数据		
}

//把上面一个函数拆成如下2个函数
int spi_write_byte(SPI_TypeDef* SPIx, int txdat)
{
	int recv;

	SPIx->DR = (uint16_t)txdat;   //Write DR register
	while((SPIx->SR & SPI_I2S_FLAG_RXNE) == 0); //不能是SPI_I2S_FLAG_TXE, Wait DR->XX->MOSI_Pin MISO_Pin->XX->DR
	recv = SPIx->DR;             //读数据寄存器, 清除SPI_I2S_FLAG_RXNE标志位(必须)
	return recv;
}

uint16_t spi_read_byte(SPI_TypeDef* SPIx)
{
	uint16_t dummy = 0x0000;
	if((SPIx->CR1 & 0x0002) != 0)   //空闲时CLK为高电平
		dummy = 0xFFFF;
	
	SPIx->DR = dummy;   //发送dummy数据
	while((SPIx->SR & SPI_I2S_FLAG_RXNE) == 0);
	return (uint16_t)SPIx->DR;    //读数据寄存器, 清除SPI_I2S_FLAG_RXNE标志位(必须)
}

/*
 * @brief: SPI连续发送数据
 */
int spi_write_buff(SPI_TypeDef* SPIx, uint8_t *buff, int len)
{
	volatile int dummy;
	int i;
	
	for(i=0; i<len; i++)
	{
		SPIx->DR = buff[i];      //Write DR register
		while((SPIx->SR & SPI_I2S_FLAG_RXNE) == 0); //不能是SPI_I2S_FLAG_TXE
		dummy = SPIx->DR;        //读数据寄存器, 清除SPI_I2S_FLAG_RXNE标志位(必须)
	}
	return i;
}

/*
 * @brief: SPI连续读取数据
 */
int spi_read_buff(SPI_TypeDef* SPIx, uint8_t *buff, int len)
{
	int i;
	uint16_t dummy;
	
	if((SPIx->CR1 & 0x0002) != 0)   //空闲时CLK为高电平
		dummy = 0xFFFF;
	else
		dummy = 0x0000;
	for(i=0; i<len; i++)
	{
		SPIx->DR = dummy;    //发送dummy数据
		while((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
		buff[i] = SPIx->DR;  //返回接收数据		
	}
	return i;
}


//=================================================================================
void SPI1_DMA_Init(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
	
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);//DMA2时钟使能
	
	DMA_DeInit(DMA2_Stream3);
//	DMA_Cmd(DMA2_Stream3, DISABLE);                   //关闭DMA
	while(DMA_GetCmdStatus(DMA2_Stream3) != DISABLE)  //Necessary
	{	}

	DMA_InitStructure.DMA_Channel = DMA_Channel_3;                       //DMA传输触发源 SPI1_TX
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;      //DMA外设地址
	DMA_InitStructure.DMA_Memory0BaseAddr = 0x20000000;                  //DMA存储器0地址   dummy
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;              //memory-->外设
	DMA_InitStructure.DMA_BufferSize = 0;                                //数据传输量        dummy
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;     //外设非增量模式
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;              //存储器增量模式
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//外设数据长度:8位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;        //存储器数据长度:8位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                          //使用普通模式 
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;                     //低优先级
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;          //存储器突发单次传输
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  //外设突发单次传输
	DMA_Init(DMA2_Stream3, &DMA_InitStructure);     //初始化DMA Stream

	DMA_Cmd(DMA2_Stream3, DISABLE);                    //需要时再使能DMA
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);  //需要时再打开   

	/* DMA RX 配置 */
	DMA_DeInit(DMA2_Stream0);
	while(DMA_GetCmdStatus(DMA2_Stream0) != DISABLE)  //Necessary
	{	}
	
	DMA_InitStructure.DMA_Channel = DMA_Channel_3;                       //DMA传输触发源 SPI1_RX
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;              //外设-->memory
	DMA_Init(DMA2_Stream0, &DMA_InitStructure);

	DMA_Cmd(DMA2_Stream0, DISABLE);                    //需要时再使能DMA
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);  //需要时再打开
}

/*
 * @brief 设置SPI1 DMA收发数据时的数据位宽
 * @param 8 -8bits
 *        16-16bits
 *        32-32bits
 */
void spi1_dma_data_size(int size)
{
	uint32_t memory_data_size;
	uint32_t peripheral_data_size;
	
	switch(size)
	{
	case 8:
		memory_data_size = DMA_MemoryDataSize_Byte;
		peripheral_data_size = DMA_PeripheralDataSize_Byte;
		break;
	
	case 16:
		memory_data_size = DMA_MemoryDataSize_HalfWord;
		peripheral_data_size = DMA_PeripheralDataSize_HalfWord;
		break;
	
	default:
		memory_data_size = DMA_MemoryDataSize_Word;
		peripheral_data_size = DMA_PeripheralDataSize_Word;
		break;
	}
	
	int flag;
	//DMA2_Stream3 发送通道
	flag = 0;
	if(DMA2_Stream3->CR & DMA_SxCR_EN)        //设置之前, 必须把EN位写0
	{
		flag = 1;
		DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;
	}
	DMA2_Stream3->CR &= 0xFFFF87FF;     //清除 11 12 13 14位
	DMA2_Stream3->CR |= memory_data_size|peripheral_data_size;
	if(	flag )
		DMA2_Stream3->CR |= DMA_SxCR_EN;
	
	//DMA2_Stream0接收通道
	flag = 0;
	if(DMA2_Stream0->CR & DMA_SxCR_EN)        //设置之前, 必须把EN位写0
	{
		flag = 1;
		DMA2_Stream0->CR &= ~(uint32_t)DMA_SxCR_EN;
	}
	DMA2_Stream0->CR &= 0xFFFF87FF;     //清除 11 12 13 14位
	DMA2_Stream0->CR |= memory_data_size|peripheral_data_size;
	if(	flag )
		DMA2_Stream0->CR |= DMA_SxCR_EN;
}

//mem_inc 0-内存地址不递增  1-内存地址递增
int spi1_write_dma(void *buff, int len, int mem_inc)
{
	int tmp_len;
	int offset = 0;
	int ret = len;

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE); 

	DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;    //EN位为0,该寄存器的其他位才允许设置
	DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_MINC;  //Clear mem_inc Control bit 
	if( mem_inc )
		DMA2_Stream3->CR |= DMA_SxCR_MINC;
	
	/* 数据量超过64K时要循环多次发送 */
	while(len > 0)
	{
		tmp_len = min(len, 0xFFFF);

		DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;    //Stop DMA2_Stream7 transfer 
		DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3|DMA_FLAG_HTIF3|DMA_FLAG_TEIF3);
		
		if( mem_inc ) {
			DMA2_Stream3->M0AR = (uint32_t)buff + offset;
			offset += tmp_len;
		} else {
			DMA2_Stream3->M0AR = (uint32_t)buff;
		}
		DMA2_Stream3->NDTR = tmp_len;

		DMA2_Stream3->CR |= DMA_SxCR_EN;                //Start DMA2_Stream3 transfer

		while(DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_TCIF3) == RESET); //等待传输完成
		DMA_ClearFlag(DMA2_Stream3,DMA_FLAG_TCIF3);                      //清除DMA2_Steam3传输完成标志
		while((SPI1->SR & SPI_I2S_FLAG_BSY) != 0);                       //解决STM32DMA发送最后一字节丢失的问题

		len -= tmp_len;
	}

	DMA_Cmd(DMA2_Stream3, DISABLE);
	DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3|DMA_FLAG_HTIF3|DMA_FLAG_TEIF3);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);

	volatile int dummy = SPI1->DR;      //Clear SPI_I2S_FLAG_RXNE Flag, 解决立即用非DMA方式发送数据会丢失1字节的问题

	return ret;
}

/*
 * @brief: 无法单纯的实现接收, 必须依靠发送来实现接收
 */
int spi1_read_dma(void *buff, int len)
{
	int tmp_len;
	int offset = 0;
	int ret = len; 
	int dummy = 0;

	if((SPI1->CR1 & 0x0002) != 0)   //空闲时CLK为高电平
		dummy = 0xFFFF;

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);  //开启DMA传输触发源开关
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

	DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;    //EN位为0,该寄存器的其他位才允许设置
	DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_MINC;  //Clear mem_inc Control bit
	DMA2_Stream0->CR &= ~(uint32_t)DMA_SxCR_EN;
	DMA2_Stream0->CR |= (uint32_t)DMA_SxCR_MINC;

	/* 数据量超过64K时要循环多次发送 */
	while(len > 0)
	{
		tmp_len = min(len, 0xFFFF);
 
		DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;
		DMA2_Stream0->CR &= ~(uint32_t)DMA_SxCR_EN;
		DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3|DMA_FLAG_HTIF3|DMA_FLAG_TEIF3);
		DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_TCIF0|DMA_FLAG_HTIF0|DMA_FLAG_TEIF0);
		
		DMA2_Stream0->M0AR = (uint32_t)buff + offset;
		DMA2_Stream0->NDTR = tmp_len;
	
		DMA2_Stream3->M0AR = (uint32_t)&dummy;
		DMA2_Stream3->NDTR = tmp_len;

		DMA2_Stream0->CR |= DMA_SxCR_EN;
		DMA2_Stream3->CR |= DMA_SxCR_EN;
		
		while(DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_TCIF3) == RESET); //等待传输完成
		DMA_ClearFlag(DMA2_Stream3,DMA_FLAG_TCIF3);                      //清除DMA2_Steam3传输完成标志
		
		while(DMA_GetFlagStatus(DMA2_Stream0, DMA_FLAG_TCIF0) == RESET); //等待传输完成
		DMA_ClearFlag(DMA2_Stream0,DMA_FLAG_TCIF0);                      //清除DMA2_Steam0传输完成标志

		while((SPI1->SR & SPI_I2S_FLAG_BSY) != 0);        //必须

		len -= tmp_len;
		offset += tmp_len;
	}
	DMA_Cmd(DMA2_Stream3, DISABLE);
	DMA_Cmd(DMA2_Stream0, DISABLE);
	DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3|DMA_FLAG_HTIF3|DMA_FLAG_TEIF3);
	DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_TCIF0|DMA_FLAG_HTIF0|DMA_FLAG_TEIF0);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);  //关闭DMA传输触发源开关
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);

	return ret;	
}

//=================================================================================
void SPI3_DMA_Init(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
	
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);//DMA1时钟使能
	
	DMA_DeInit(DMA1_Stream7);
//	DMA_Cmd(DMA1_Stream7, DISABLE);                   //关闭DMA
	while(DMA_GetCmdStatus(DMA1_Stream7) != DISABLE)  //Necessary
	{	}

	DMA_InitStructure.DMA_Channel = DMA_Channel_0;                       //DMA传输触发源 SPI3_TX
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI3->DR;      //DMA外设地址
	DMA_InitStructure.DMA_Memory0BaseAddr = 0x20000000;                  //DMA存储器0地址   dummy
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;              //memory-->外设
	DMA_InitStructure.DMA_BufferSize = 0;                                //数据传输量        dummy
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;     //外设非增量模式
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;              //存储器增量模式
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//外设数据长度:8位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;        //存储器数据长度:8位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                         //使用普通模式 
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;                    //低优先级
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;          //存储器突发单次传输
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  //外设突发单次传输
	DMA_Init(DMA1_Stream7, &DMA_InitStructure);     //初始化DMA Stream

	DMA_Cmd(DMA1_Stream7, DISABLE);                    //需要时再使能DMA
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, DISABLE);  //需要时再打开   

	/* DMA RX 配置 */
	DMA_DeInit(DMA1_Stream2);
	while(DMA_GetCmdStatus(DMA1_Stream2) != DISABLE)  //Necessary
	{	}
	
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;                       //DMA传输触发源 SPI3_RX
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;              //外设-->memory
	DMA_Init(DMA1_Stream2, &DMA_InitStructure);

	DMA_Cmd(DMA1_Stream2, DISABLE);                    //需要时再使能DMA
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Rx, DISABLE);  //需要时再打开
}

/*
 * @brief 设置SPI3 DMA收发数据时的数据位宽
 * @param 8 -8bits
 *        16-16bits
 *        32-32bits
 */
void spi3_dma_data_size(int size)
{
	uint32_t memory_data_size;
	uint32_t peripheral_data_size;
	
	switch(size)
	{
	case 8:
		memory_data_size = DMA_MemoryDataSize_Byte;
		peripheral_data_size = DMA_PeripheralDataSize_Byte;
		break;
	
	case 16:
		memory_data_size = DMA_MemoryDataSize_HalfWord;
		peripheral_data_size = DMA_PeripheralDataSize_HalfWord;
		break;
	
	default:
		memory_data_size = DMA_MemoryDataSize_Word;
		peripheral_data_size = DMA_PeripheralDataSize_Word;
		break;
	}

	int flag;
	//DMA1_Stream7 发送通道
	flag = 0;
	if(DMA1_Stream7->CR & DMA_SxCR_EN)        //设置之前, 必须把EN位写0
	{
		flag = 1;
		DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;
	}
	DMA1_Stream7->CR &= 0xFFFF87FF;     //清除 11 12 13 14位
	DMA1_Stream7->CR |= memory_data_size|peripheral_data_size;
	if(	flag )
		DMA1_Stream7->CR |= DMA_SxCR_EN;
	
	//DMA1_Stream2接收通道
	flag = 0;
	if(DMA1_Stream2->CR & DMA_SxCR_EN)        //设置之前, 必须把EN位写0
	{
		flag = 1;
		DMA1_Stream2->CR &= ~(uint32_t)DMA_SxCR_EN;
	}
	DMA1_Stream2->CR &= 0xFFFF87FF;     //清除 11 12 13 14位
	DMA1_Stream2->CR |= memory_data_size|peripheral_data_size;
	if(	flag )
		DMA1_Stream2->CR |= DMA_SxCR_EN;
}

//mem_inc 0-内存地址不递增  1-内存地址递增
int spi3_write_dma(void *buff, int len, int mem_inc)
{
	int tmp_len;
	int offset = 0;
	int ret = len;

	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE); 

	DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;    //EN位为0,该寄存器的其他位才允许设置
	DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_MINC;  //Clear mem_inc Control bit 
	if( mem_inc )
		DMA1_Stream7->CR |= DMA_SxCR_MINC;
	
	/* 数据量超过64K时要循环多次发送 */
	while(len > 0)
	{
		tmp_len = min(len, 0xFFFF);

		DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;    //Stop DMA1_Stream7 transfer 
		DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7|DMA_FLAG_HTIF7|DMA_FLAG_TEIF7);
		
		if( mem_inc ) {
			DMA1_Stream7->M0AR = (uint32_t)buff + offset;
			offset += tmp_len;
		} else {
			DMA1_Stream7->M0AR = (uint32_t)buff;
		}
		DMA1_Stream7->NDTR = tmp_len;

		DMA1_Stream7->CR |= DMA_SxCR_EN;                //Start DMA1_Stream7 transfer

		while(DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_TCIF7) == RESET); //等待传输完成
		DMA_ClearFlag(DMA1_Stream7,DMA_FLAG_TCIF7);                      //清除DMA1_Stream7传输完成标志
		while((SPI3->SR & SPI_I2S_FLAG_BSY) != 0);                       //解决STM32DMA发送最后一字节丢失的问题

		len -= tmp_len;
	}

	DMA_Cmd(DMA1_Stream7, DISABLE);
	DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7|DMA_FLAG_HTIF7|DMA_FLAG_TEIF7);
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, DISABLE);

	volatile int dummy = SPI3->DR;      //Clear SPI_I2S_FLAG_RXNE Flag, 解决立即用非DMA方式发送数据会丢失1字节的问题

	return ret;
}

/*
 * @brief: 无法单纯的实现接收, 必须依靠发送来实现接收
 */
int spi3_read_dma(void *buff, int len)
{
	int tmp_len;
	int offset = 0;
	int ret = len; 
	int dummy = 0;

	if((SPI3->CR1 & 0x0002) != 0)   //空闲时CLK为高电平
		dummy = 0xFFFF;

	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Rx, ENABLE);  //开启DMA传输触发源开关
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE);

	DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;    //EN位为0,该寄存器的其他位才允许设置
	DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_MINC;  //Clear mem_inc Control bit
	DMA1_Stream2->CR &= ~(uint32_t)DMA_SxCR_EN;
	DMA1_Stream2->CR |= (uint32_t)DMA_SxCR_MINC;

	/* 数据量超过64K时要循环多次发送 */
	while(len > 0)
	{
		tmp_len = min(len, 0xFFFF);
 
		DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;
		DMA1_Stream2->CR &= ~(uint32_t)DMA_SxCR_EN;
		DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7|DMA_FLAG_HTIF7|DMA_FLAG_TEIF7);
		DMA_ClearFlag(DMA1_Stream2, DMA_FLAG_TCIF2|DMA_FLAG_HTIF2|DMA_FLAG_TEIF2);

		DMA1_Stream2->M0AR = (uint32_t)buff + offset;
		DMA1_Stream2->NDTR = tmp_len;
	
		DMA1_Stream7->M0AR = (uint32_t)&dummy;
		DMA1_Stream7->NDTR = tmp_len;

		DMA1_Stream2->CR |= DMA_SxCR_EN;
		DMA1_Stream7->CR |= DMA_SxCR_EN;
		
		while(DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_TCIF7) == RESET); //等待传输完成
		DMA_ClearFlag(DMA1_Stream7,DMA_FLAG_TCIF7);                      //清除DMA1_Stream7传输完成标志
		
		while(DMA_GetFlagStatus(DMA1_Stream2, DMA_FLAG_TCIF2) == RESET); //等待传输完成
		DMA_ClearFlag(DMA1_Stream2,DMA_FLAG_TCIF2);                      //清除DMA1_Steam3传输完成标志

		while((SPI3->SR & SPI_I2S_FLAG_BSY) != 0);        //必须

		len -= tmp_len;
		offset += tmp_len;
	}
	DMA_Cmd(DMA1_Stream7, DISABLE);
	DMA_Cmd(DMA1_Stream2, DISABLE);
	DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7|DMA_FLAG_HTIF7|DMA_FLAG_TEIF7);
	DMA_ClearFlag(DMA1_Stream2, DMA_FLAG_TCIF2|DMA_FLAG_HTIF2|DMA_FLAG_TEIF2);
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, DISABLE);  //关闭DMA传输触发源开关
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Rx, DISABLE);

	return ret;	
}



