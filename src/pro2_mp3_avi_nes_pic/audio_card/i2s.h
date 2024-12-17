#ifndef  __I2S_H_
#define  __I2S_H_

#include "stm32f4xx.h"

//============================================================
//I2S2 Controller
#define  i2s_tx_dma_irq_disable()  NVIC_DisableIRQ(DMA1_Stream4_IRQn)
#define  i2s_tx_dma_irq_enable()   NVIC_EnableIRQ(DMA1_Stream4_IRQn)

#define  i2s_rx_dma_irq_disable()  NVIC_DisableIRQ(DMA1_Stream3_IRQn)
#define  i2s_rx_dma_irq_enable()   NVIC_EnableIRQ(DMA1_Stream3_IRQn)

void i2s_controller_init(void);
int i2s_controller_cfg(uint16_t I2S_Standard, uint16_t I2S_DataFormat, uint32_t sample_rate);
int i2s_frequency_adjust(uint32_t new_freq);

void i2s_register_tx_cb(void (*tx_cb_func)(uint32_t));
void i2s_register_rx_cb(void (*rx_cb_func)(uint32_t));
int  i2s_start_tx(uint16_t *tx_buf, uint16_t *tx_buf2, uint16_t tx_len);
int  i2s_start_rx(uint16_t *rx_buf, uint16_t *rx_buf2, uint16_t rx_len);
int  i2s_stop_tx(void);
int  i2s_stop_rx(void); 

//void i2s_register_tx_rx_cb(void (*tx_cb_func)(uint32_t), void (*rx_cb_func)(uint32_t));
//int i2s_start_tx_rx(uint16_t *tx_buf, uint16_t *tx_buf2, uint16_t tx_len, uint16_t *rx_buf, uint16_t *rx_buf2, uint16_t rx_len);
//int i2s_tx_release(void);
//int i2s_rx_release(void);


#endif



