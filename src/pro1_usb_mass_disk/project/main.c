#include "user_task.h"

int main(void)
{
	hardware_init();
	task_init();

	for( ; ; )
	{
		task_scheduler();
	}
}


