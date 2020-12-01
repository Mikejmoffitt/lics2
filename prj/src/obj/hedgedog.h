#ifndef OBJ_HEDGEDOG_H
#define OBJ_HEDGEDOG_H

#include "obj.h"

typedef struct O_Hedgedog
{
	Obj head;

	fix32_t original_y;
	int16_t jump_timer;
	int16_t anim_cnt;
	int16_t anim_frame;
	int16_t phase;  // 0 = walking; 1 = moving up; 2 = moving down
} O_Hedgedog;

void o_load_hedgedog(Obj *o, uint16_t data);
void o_unload_hedgedog(void);

#endif  // OBJ_HEDGEDOG_H
