#include "user_tcb_v3.h"
#include "cpu_related_v3.h"
#include "my_printf.h"
#include <stddef.h>

//定义任务链表
static struct list_head tcb_head = LIST_HEAD_INIT(tcb_head);


//创建任务
void task_create(task_control_block_t *tcb)
{
	struct list_head *slider;
	struct task_control_block_s *task;

	//先遍历任务链表, 查看任务是否已经添加到链表中(防止用户反复添加一个任务)
	list_for_each(slider, &tcb_head)
	{
		task = list_entry(slider, struct task_control_block_s, node);
		if(task == tcb)
			return;
	}

	list_add_tail(&tcb->node, &tcb_head);
}

//删除任务
void task_delete(task_control_block_t *tcb)
{
	struct list_head *slider;
	struct task_control_block_s *task;

	if(tcb == NULL)
		return;

	//遍历任务链表, 只有已经添加到链表中的任务才能删除
	list_for_each(slider, &tcb_head)
	{
		task = list_entry(slider, struct task_control_block_s, node);
		if(task == tcb)
		{
			list_del(&tcb->node);
			return;
		}
	}
}

//唤醒任务
//inline void __attribute__((always_inline)) task_wakeup(task_control_block_t *tcb)
//{
//	tcb->state = TS_RUNNING;
//}

//休眠任务
inline void __attribute__((always_inline)) task_sleep(task_control_block_t *tcb)
{
	tcb->state = TS_STOPPED;
}

//任务调度器
void task_scheduler(void)
{
	struct list_head *slider;
	struct list_head *temp;
	struct task_control_block_s *task;

	uint32_t cpu_clk_cnt1, cpu_clk_cnt2;
	int    refresh_cpu_usage_mark = 0;
	static uint32_t total_time = 0;

	//遍历任务链表, 并执行已经唤醒的任务
//	list_for_each(slider, &tcb_head)              //(*task->action)中删除节点会导致致命BUG
	list_for_each_safe(slider, temp, &tcb_head)   //(*task->action)中允许删除节点  变更为v2
	{
		task = list_entry(slider, struct task_control_block_s, node);
		if(task->state == TS_RUNNING || task->is_always_alive)
		{
			cpu_clk_cnt1 = get_timestamp();
			task->state = TS_STOPPED;
			(*task->action)();
			cpu_clk_cnt2 = get_timestamp();
			
			task->time_spend += cpu_clk_cnt2 - cpu_clk_cnt1;
			total_time += cpu_clk_cnt2 - cpu_clk_cnt1;

			if(unlikely(task->mark == CPU_USAGE_CAL_MARK)) {
				refresh_cpu_usage_mark = 1;
			}

			//2023-12-21添加下面2个if 变更为v3
			if(unlikely(slider->next == NULL)) {   //task->action把自己给删除了
//				my_printf("%s delete itself\n\r", task->name);
				return;   //删除自己的同时可能会添加或删除别的任务结点, 直接结束本轮遍历.
			}
			if(unlikely(slider->next != temp)) {   //task->action后面(紧邻)添加或删除了一个任务结点
//				my_printf("%s add or delete next task(s)\n\r", task->name);
				temp = slider->next;
			}
		}
	}
	
	if(likely(refresh_cpu_usage_mark == 0))
		return;
	
	//遍历任务链表, 统计各个任务的CPU使用率
	stop_timestamp();
	list_for_each(slider, &tcb_head)
	{
		task = list_entry(slider, struct task_control_block_s, node);
		task->cpu_usage = ((uint64_t)10000 * task->time_spend) / total_time;
		task->time_spend = 0;
	}
	start_timestamp();
	total_time = 0;
}

void task_cpu_usage(void)
{
	struct list_head *slider;
	struct task_control_block_s *task;

	my_printf("\n");
	my_printf("%-12s %s\n\r", "task", "used%");
	list_for_each(slider, &tcb_head)
	{
		task = list_entry(slider, struct task_control_block_s, node);
		my_printf("%-12s %d.%02d%%\n\r", task->name, task->cpu_usage/100, task->cpu_usage%100);
	}
}













