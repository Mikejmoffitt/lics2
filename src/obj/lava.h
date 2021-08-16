#ifndef OBJ_LAVA_H
#define OBJ_LAVA_H

#include "obj.h"

typedef struct O_Lava
{
	Obj head;

	Obj *cow;

	int16_t anim_cnt;
	int16_t anim_frame;

	fix32_t max_y;
	int16_t generator_cnt;

} O_Lava;

void o_load_lava(Obj *o, uint16_t data);
void o_unload_lava(void);

#endif  // OBJ_LAVA_H
