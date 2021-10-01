#ifndef OBJ_SMALL_EGG_H
#define OBJ_SMALL_EGG_H

#include "obj.h"

typedef struct O_SmallEgg
{
	Obj head;

	int16_t anim_cnt;
	int16_t anim_frame;

	int16_t generator_cnt;
} O_SmallEgg;

void o_load_small_egg(Obj *o, uint16_t data);
void o_unload_small_egg(void);

#endif  // OBJ_SMALL_EGG_H
