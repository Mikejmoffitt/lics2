#ifndef OBJ_FLARGY_H
#define OBJ_FLARGY_H

#include "obj.h"

typedef struct O_Flargy
{
	Obj head;

	int16_t punch_cnt;  // When non-zero, he is punching.
	fix32_t min_x;
	fix32_t max_x;

	int16_t anim_cnt;
	int16_t anim_frame;
} O_Flargy;

void o_load_flargy(Obj *o, uint16_t data);
void o_unload_flargy(void);

#endif  // OBJ_FLARGY_H
