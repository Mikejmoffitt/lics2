#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define TASK_LIST_MAX 64
#define TASK_RAW_MEM_WORDS 16

typedef enum TaskStatus
{
	TASK_STATUS_ACTIVE     = 0x8000,  // Set when the object slot is occupied.
	TASK_STATUS_END_MARKER = 0x4000,  // Marks the end of the object list.
	TASK_STATUS_STICKY     = 0x0002,  // Don't clear this object.
	TASK_STATUS_NEW        = 0x0001,  // Set on the first frame of execution.

	TASK_STATUS_NULL       = 0x0000,
} TaskStatus;

typedef struct Task
{
	void (*func)(void *);
	TaskStatus status;
	uint16_t raw_mem[TASK_RAW_MEM_WORDS];
} Task;

// Clear all object slots, and place the end marker in the last slot.
void task_init(Task *obj, int array_size);

// Runs through the object list, executing object code.
void task_iterate(Task *obj);

// It is not necessary to set the ACTIVE flag. Please don't set the END MARKER flag.
Task *task_add(Task *obj, void *main_func, TaskStatus flags);

// Places a task at the specified index.
Task *task_set(Task *task, uint16_t index, void *func, TaskStatus flags);

// Remove all non-sticky objects.
void task_clear(Task *obj);

// Remove all objects, even sticky ones.
void task_purge(Task *obj);

Task *task_get(Task *obj, uint16_t idx);


#endif  // TASK_H
