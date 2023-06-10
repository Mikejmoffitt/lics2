#ifndef OBJ_FISSINS2_H
#define OBJ_FISSINS2_H

#include "obj.h"

typedef struct O_Fissins2
{
	Obj head;
	fix32_t base_y;
	int16_t cooldown;

	int16_t anim_cnt;
	int16_t anim_frame;
	bool airborn;
} O_Fissins2;

void o_load_fissins2(Obj *o, uint16_t data);
void o_unload_fissins2(void);

#endif  // OBJ_FISSINS2_H
