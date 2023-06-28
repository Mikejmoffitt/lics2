#ifndef OBJ_STAFFROLL_H
#define OBJ_STAFFROLL_H

#include "obj.h"
#include <stdbool.h>

typedef struct O_Staffroll
{
	Obj head;
	bool initialized;
	bool line_ready;
	uint16_t line_idx;
	uint16_t vram_write_addr;

	fix32_t scroll;
	fix16_t scroll_acc;
} O_Staffroll;

void o_load_staffroll(Obj *o, uint16_t data);
void o_unload_staffroll(void);

#endif  // OBJ_STAFFROLL_H
