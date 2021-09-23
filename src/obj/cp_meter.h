#ifndef OBJ_CP_METER_H
#define OBJ_CP_METER_H

#include "obj.h"

typedef struct O_CpMeter
{
	Obj head;
	int16_t anim_frame;
	int16_t anim_cnt;
} O_CpMeter;

void o_load_cp_meter(Obj *o, uint16_t data);
void o_unload_cp_meter(void);

#endif  // OBJ_CP_METER_H
