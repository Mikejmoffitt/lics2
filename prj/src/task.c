#include "task.h"
#include "common.h"
#include "util/text.h"

#include <stdlib.h>

void task_init(Task *task, int array_size)
{
	task[array_size - 1].status = TASK_STATUS_END_MARKER;
	task_purge(task);
}

void task_iterate(Task *task)
{
	do
	{
		if (task->status & (TASK_STATUS_ACTIVE))
		{
			task->func(task);
			task->status &= ~(TASK_STATUS_NEW);
		}
		task++;
	} while (!(task->status & TASK_STATUS_END_MARKER));
}

Task *task_add(Task *task, void *func, TaskStatus flags)
{
	do
	{
		if (!(task->status & (TASK_STATUS_ACTIVE | TASK_STATUS_END_MARKER)))
		{
			task->func = func;
			task->status = flags | TASK_STATUS_ACTIVE | TASK_STATUS_NEW;
			return task;
		}
		task++;
	} while (!(task->status & TASK_STATUS_END_MARKER));
	return NULL;
}

Task *task_set(Task *task, uint16_t index, void *func,
                   TaskStatus flags)
{
	task = &task[index];
	task->func = func;
	task->status = flags | TASK_STATUS_ACTIVE | TASK_STATUS_NEW;
	return task;
}

void task_clear(Task *task)
{
	do
	{
		if (!(task->status & (TASK_STATUS_STICKY | TASK_STATUS_END_MARKER)))
		{
			task->status = TASK_STATUS_NULL;
		}
		task++;
	} while (!(task->status & TASK_STATUS_END_MARKER));
}

void task_purge(Task *task)
{
	do
	{
		if (!(task->status & (TASK_STATUS_END_MARKER)))
		{
			task->status = TASK_STATUS_NULL;
			task->func = NULL;
		}
		task++;
	} while (!(task->status & TASK_STATUS_END_MARKER));
}

Task *task_get(Task *task, uint16_t idx)
{
	return &task[idx];
}
