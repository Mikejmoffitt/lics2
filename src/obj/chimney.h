#ifndef OBJ_CHIMNEY_H
#define OBJ_CHIMNEY_H

#include "obj.h"

typedef struct O_Chimney
{
	Obj head;
	uint16_t data;

	int16_t anim_cnt;
	int16_t anim_frame;
} O_Chimney;

void o_load_chimney(Obj *o, uint16_t data);
void o_unload_chimney(void);

#endif  // OBJ_CHIMNEY_H
