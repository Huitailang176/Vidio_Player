#ifndef  __CPU_RELATED_V3_H_
#define  __CPU_RELATED_V3_H_


#ifdef  __cplusplus
    extern "C" {
#endif

#include "stm32f4xx.h"

#define disable_irq()  __asm("cpsid i")
#define enable_irq()   __asm("cpsie i")

#define  ALLOC_CPU_SR()    uint32_t cpu_sr
#define  ENTER_CRITICAL()\
do{\
	cpu_sr = __get_PRIMASK();\
	disable_irq();\
}while(0)
#define  EXIT_CRITICAL()   __set_PRIMASK(cpu_sr)

uint32_t get_cpu_type(const char **type);   //获取CPU内核版本号
void set_align8stk_whenhandler(void);       //使能双字栈,即进入handler前栈帧入栈时确保SP所指内存8字节对齐
//函数的使用方法, 在main()函数初始化时：
//__disable_irq();
//set_align8stk_whenhandler();
//__enable_irq();
		
void     start_timestamp(void);
void     stop_timestamp(void);
//uint32_t get_timestamp(void);

//函数内联是在编译阶段实现的, 而不是链接阶段, 
//若把get_timestamp()函数定义在.c文件中, 别的文件中的函数调用它时根本实现了不了内联
uint32_t get_timestamp(void);
static inline uint32_t get_timestamp(void)
{
	return (*(volatile uint32_t *)0xE0001004);
}


extern uint32_t get_now_PC(void);
extern uint32_t __get_this_PC(void);
extern void __set_PC(uint32_t pc_val);


typedef enum mcu_serie_e
{
	STM32F0 = 0,
	STM32F1,
	STM32F2,
	STM32F3,
	STM32F4,
	STM32F7,
	STM32L0,
	STM32L1,
	STM32L4,
	STM32H7,
}mcu_serie_t;

void mcu_unique_id_flash_size_disp(mcu_serie_t mcu_serie);
void mcu_idcode_disp(void);

#ifdef  __cplusplus
}  
#endif

#endif



