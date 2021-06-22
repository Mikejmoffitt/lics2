#ifndef OBJ_GAXTER1_H
#define OBJ_GAXTER1_H

#include "obj.h"
#include <md/megadrive.h>

// Gaxter 1 is the gaxter type that follows the player.

typedef struct O_Gaxter1
{
	Obj head;
	int8_t anim_cnt;
	int8_t anim_frame;
} O_Gaxter1;

void o_load_gaxter1(Obj *o, uint16_t data);
void o_unload_gaxter1(void);

#endif  // OBJ_GAXTER1_H
