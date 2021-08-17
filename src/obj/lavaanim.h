#ifndef OBJ_LAVAANIM_H
#define OBJ_LAVAANIM_H

#include "obj.h"

typedef struct O_LavaAnim
{
	Obj head;

	int16_t anim_frame;
	int16_t anim_cnt;

	uint16_t variant;
} O_LavaAnim;

void o_load_lavaanim(Obj *o, uint16_t data);
void o_unload_lavaanim(void);

#endif  // OBJ_LAVAANIM_H
