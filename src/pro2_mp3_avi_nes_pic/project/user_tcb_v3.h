#ifndef  __USER_TCB_V3_H_
#define  __USER_TCB_V3_H_

#include <stdint.h>
#include "klist.h"

#define  CPU_USAGE_CAL_MARK  0x8000

enum task_state
{
    TS_STOPPED,
    TS_RUNNING,
};

typedef struct task_control_block_s
{
	struct list_head node;

	char *name;
	enum task_state state;
	int  is_always_alive;
	void (*action)(void);
	int  mark;

	uint32_t time_spend;
	uint32_t cpu_usage;
}task_control_block_t;


void task_create(task_control_block_t *tcb);
void task_delete(task_control_block_t *tcb);
void task_wakeup(task_control_block_t *tcb);
void task_sleep(task_control_block_t *tcb);
void task_scheduler(void);
void task_cpu_usage(void);

//函数内联是在编译阶段, 而不是链接阶段
static inline void __attribute__((always_inline)) task_wakeup(task_control_block_t *tcb)
{
	tcb->state = TS_RUNNING;
}


#endif



