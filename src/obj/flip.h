#ifndef OBJ_FLIP_H
#define OBJ_FLIP_H

#include "obj.h"
#include <md/megadrive.h>

typedef struct O_Flip
{
	Obj head;
	fix32_t x_max;
	fix32_t x_min;
	int16_t moving_up;
	int16_t anim_cnt;
	int16_t anim_frame;
} O_Flip;

void o_load_flip(Obj *o, uint16_t data);
void o_unload_flip(void);

#endif  // OBJ_FLIP_H
