#ifndef OBJ_ELEVATOR_H
#define OBJ_ELEVATOR_H

#include "obj.h"

typedef struct O_Elevator
{
	Obj head;

	fix32_t original_y;
	int16_t hidden;

	int16_t anim_cnt;
	int16_t anim_frame;

	int16_t collision_delay_cnt;
} O_Elevator;

void o_load_elevator(Obj *o, uint16_t data);
void o_unload_elevator(void);

#endif  // OBJ_ELEVATOR_H
