#ifndef  __USER_TASK_H_
#define  __USER_TASK_H_

#include "user_tcb_v3.h"



extern task_control_block_t tcb_10ms;
extern task_control_block_t tcb_100ms;
extern task_control_block_t tcb_1s;
extern task_control_block_t tcb_usb_device;


void hardware_init(void);
void task_init(void);


#endif

