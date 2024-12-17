#include "cpu_related_v3.h"
#include "share.h"
#include <stddef.h>
#include "my_printf.h"


//================================================================================================
/*
在异常/中断的handler函数入口处被CPU内部硬件压入栈中的数据块称为栈帧（栈帧概念）。
栈帧入栈时，如果使能了双字栈对齐特性（SCB->CCR bit9），那么在入栈时若SP所指的内存
地址非双字对齐（地址不能被8整除，后3位不全为0）则硬件会自动插入一个空字用于调整SP，使其双字对齐。
xPSR[9]用于指示SP是否调整过，1-调整过（栈帧入栈前插入了一个空字），0-未调整过。
这样，在handler处理完返回时，栈帧出栈，然后CPU根据xPSR[9]决策是否需要调整SP。
CM3-r2p0及之后版本默认使能双字栈，CM3-r1p0、CM3-r1p1默认不使能双字栈，CM3-r0p0不支持双字栈设置，
强烈推荐使能双字栈, 不过，即使不使能双字栈STM32跑程序也不会有问题。
*/
//Apply for CM3 CM4
typedef struct cpu_type_s
{
	uint32_t cpu_id;
	char *ver;
}cpu_type_t;

const cpu_type_t cpu_type_arr[] =
{
	{0x410CC200,  "CM0-r0p0"},
	{0x410CC600,  "CM0+r0p0"},
	{0x410CC210,  "CM1-r0p0"},
	{0x410CC211,  "CM1-r0p1"},
	{0x411CC210,  "CM1-r1p0"},	
	{0x410FC230,  "CM3-r0p0"},
	{0x410FC231,  "CM3-r1p0"},
	{0x411FC231,  "CM3-r1p1"},
	{0x412FC230,  "CM3-r2p0"},
	{0x412FC231,  "CM3-r2p1"},	
	{0x410FC240,  "CM4-r0p0"},
	{0x410FC241,  "CM4-r0p1"},
};

//获取CPU内核版本号
uint32_t get_cpu_type(const char **type)
{
	int i;
	uint32_t cpu_id = SCB->CPUID;
	
	for(i=0; i<ARRAY_SIZE(cpu_type_arr); i++)
	{
		if(cpu_id == cpu_type_arr[i].cpu_id)
		{
//			my_printf("%s:CPU core version:%s\n\r", __func__, cpu_type_arr[i].ver);
			if(type != NULL) *type=cpu_type_arr[i].ver;
			break;
		}
	}
	return cpu_id;
}

//使能双字栈,即进入handler前栈帧入栈时确保SP所指内存8字节对齐
//强烈推荐使能双字栈, 不过,即使不使能双字栈STM32跑程序也不会有问题
void set_align8stk_whenhandler(void)
{
	if((SCB->CCR&SCB_CCR_STKALIGN_Msk) == 0ul)
	{
		SCB->CCR |= SCB_CCR_STKALIGN_Msk;
	}
}


typedef struct mcu_unique_id_flash_size_s
{
	mcu_serie_t mcu_serie;
	char *mcu_info;
	uint32_t unique_id_addr;
	uint32_t flash_size_addr;
}mcu_unique_id_flash_size_t;

#define mcu_obj(mcu_serie, unique_id_addr, flash_size_addr) \
{mcu_serie, #mcu_serie, unique_id_addr, flash_size_addr}

const mcu_unique_id_flash_size_t mcu_unique_id_flash_size[] =
{
	mcu_obj(STM32F0, 0x1FFFF7AC, 0x1FFFF7CC),
	mcu_obj(STM32F1, 0x1FFFF7E8, 0x1FFFF7E0),
	mcu_obj(STM32F2, 0x1FFF7A10, 0x1FFF7A22),
	mcu_obj(STM32F3, 0x1FFFF7AC, 0x1FFFF7CC),
	mcu_obj(STM32F4, 0x1FFF7A10, 0x1FFF7A22),
	mcu_obj(STM32F7, 0x1FF0F420, 0x1FF0F442),
	mcu_obj(STM32L0, 0x1FF80050, 0x1FF8007C),
	mcu_obj(STM32L1, 0x1FF80050, 0x1FF8004C),
	mcu_obj(STM32L4, 0x1FFF7590, 0x1FFF75E0),
	mcu_obj(STM32H7, 0x1FF0F420, 0x1FF0F442),
};

void mcu_unique_id_flash_size_disp(mcu_serie_t mcu_serie)
{
	int i;
	uint32_t unique_id[3];
	uint16_t flash_size;
	const mcu_unique_id_flash_size_t *mcu_id_size;

	mcu_id_size = NULL;
	for(i=0; i<ARRAY_SIZE(mcu_unique_id_flash_size); i++)
	{
		if(mcu_serie == mcu_unique_id_flash_size[i].mcu_serie)
		{
			mcu_id_size = &mcu_unique_id_flash_size[i];
			break;
		}
	}

	if(mcu_id_size == NULL)
		return;

	unique_id[0] = *(__IO uint32_t *)(mcu_id_size->unique_id_addr + 0);
	unique_id[1] = *(__IO uint32_t *)(mcu_id_size->unique_id_addr + 4);
	unique_id[2] = *(__IO uint32_t *)(mcu_id_size->unique_id_addr + 8);
	flash_size   = *(__IO uint16_t *)mcu_id_size->flash_size_addr;
	my_printf("%s\n\r", mcu_id_size->mcu_info);
	my_printf("Unique ID(96b): 0x%08X 0x%08X 0x%08X\n\r", unique_id[0], unique_id[1], unique_id[2]);
	my_printf("Flash Size(KB): %u\n\r", flash_size);
}


//For STM32F10x and STM32F4xx
#define  DBGMCU_IDCODE_ADDR  0xE0042000
typedef struct rev_id_s
{
	uint16_t id;
	char *describe;
}rev_id_t;

typedef struct mcu_idcode_s
{
	uint16_t dev_id;
	char *describe;
	const rev_id_t *rev_id;
}mcu_idcode_t;

#if defined(STM32F10X_LD) || defined(STM32F10X_MD) || defined(STM32F10X_HD)
const rev_id_t rev_id_small_size[] = 
{
	{0x1000, "Rev A"},
	{0xFFFF, NULL},
};
const rev_id_t rev_id_mediem_size[] = 
{
	{0x0000, "Rev A"},
	{0x2000, "Rev B"},
	{0x2001, "Rev Z"},
	{0x2003, "Rev Y, 1, 2 or X"},
	{0xFFFF, NULL},
};
const rev_id_t rev_id_large_size[] = 
{
	{0x1000, "Rev A or 1"},
	{0x1001, "Rev Z"},
	{0x1003, "Rev Y, 1, 2 or X"},
	{0xFFFF, NULL},
};
const rev_id_t rev_id_connectivity[] = 
{
	{0x1000, "Rev A"},
	{0x1001, "Rev Z"},
	{0xFFFF, NULL},
};

const mcu_idcode_t mcu_idcode[] =
{
	{0x412, "Low-density",    rev_id_small_size},
	{0x410, "Mediem-density", rev_id_mediem_size},
	{0x414, "Large-density",  rev_id_large_size},
	{0x418, "Connectivity",   rev_id_connectivity},
};

#elif defined(STM32F40_41xxx) || defined(STM32F429_439xx) || defined(STM32F401xx)
const rev_id_t rev_id_f40x_41x[] = 
{
	{0x1000, "Rev A"},
	{0x1001, "Rev Z"},
	{0x1003, "Rev 1"},
	{0x1007, "Rev 2"},
	{0x100F, "Rev Y"},
	{0xFFFF, NULL},
};
const rev_id_t rev_id_f42x_43x[] = 
{
	{0x1000, "Rev A"},
	{0x1003, "Rev Y"},
	{0x1007, "Rev 1"},
	{0x2001, "Rev 3"},
	{0xFFFF, NULL},
};

const mcu_idcode_t mcu_idcode[] =
{
	{0x413, "STM32F40x/41x", rev_id_f40x_41x},
	{0x419, "STM32F42x/43x", rev_id_f42x_43x},
};

#else
  #error "Unsupport CPU"
#endif

void mcu_idcode_disp(void)
{
	int i;
	const mcu_idcode_t *mcu_id;

	uint32_t dbgmcu_idcode;
	uint16_t dev_id, rev_id;
	
	dbgmcu_idcode = *(__IO uint32_t *)(DBGMCU_IDCODE_ADDR);
	dev_id = dbgmcu_idcode & 0x0FFF;
	rev_id = (dbgmcu_idcode>>16) & 0xFFFF;
	my_printf("DBGMCU_IDCODE=0x%08X\n\r", dbgmcu_idcode);
	
	mcu_id = NULL;
	for(i=0; i<ARRAY_SIZE(mcu_idcode); i++)
	{
		if(dev_id == mcu_idcode[i].dev_id)
		{
			mcu_id = &mcu_idcode[i];
			break;
		}
	}

	if(mcu_id == NULL)
		return;

	my_printf("%s, ", mcu_id->describe);
	i = 0;
	while(mcu_id->rev_id[i].describe != NULL)
	{
		if(mcu_id->rev_id[i].id == rev_id)
		{
			my_printf("%s\n\r", mcu_id->rev_id[i].describe);
			break;
		}
		i++;
	}
}


//================================================================================================

//================================================================================================
typedef  volatile  unsigned int  CPU_REG32;
typedef  volatile  unsigned int  CPU_INT32U;
#define  BSP_REG_DEM_CR                           (*(CPU_REG32 *)0xE000EDFC)	//DEMCR寄存器
#define  BSP_REG_DWT_CR                           (*(CPU_REG32 *)0xE0001000)  	//DWT控制寄存器
#define  BSP_REG_DWT_CYCCNT                       (*(CPU_REG32 *)0xE0001004)	//DWT时钟计数寄存器	

#define  DEF_BIT_00                                     0x01u
#define  DEF_BIT_24                               0x01000000u
 
//DEMCR寄存器的第24位,如果要使用DWT ETM ITM和TPIU的话DEMCR寄存器的第24位置1
#define  BSP_BIT_DEM_CR_TRCENA                    DEF_BIT_24			

//DWTCR寄存器的第0位,当为1的时候使能CYCCNT计数器,使用CYCCNT之前应当先初始化
#define  BSP_BIT_DWT_CR_CYCCNTENA                 DEF_BIT_00
void     start_timestamp(void)
{
	BSP_REG_DEM_CR     |= (CPU_INT32U)BSP_BIT_DEM_CR_TRCENA;
    BSP_REG_DWT_CYCCNT  = (CPU_INT32U)0u;					       //初始化CYCCNT寄存器
    BSP_REG_DWT_CR     |= (CPU_INT32U)BSP_BIT_DWT_CR_CYCCNTENA;    //开启CYCCNT
}

void     stop_timestamp(void)
{
//	BSP_REG_DWT_CR &= ~((CPU_REG32)BSP_BIT_DWT_CR_CYCCNTENA);
	BSP_REG_DWT_CR = 0;
}

//uint32_t get_timestamp(void)
//{
//	return BSP_REG_DWT_CYCCNT;
//}

//================================================================================================

__asm uint32_t __get_this_PC(void)
{
	MOV R0, LR      //not MOV R0, PC
	SUB R0, #0x04
	BIC R0, #0x01   //clear bit0 
	BX LR
}

__ASM void __set_PC(uint32_t pc_val)
{
	BX R0
	NOP            //can't run here
	NOP
	NOP
	NOP
}

