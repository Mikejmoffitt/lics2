#ifndef OBJ_GAXTER2_H
#define OBJ_GAXTER2_H

#include "obj.h"
#include <md/megadrive.h>

// Gaxter 1 is the gaxter type that follows the player.

typedef struct O_Gaxter2
{
	Obj head;
	int8_t anim_cnt;
	int8_t anim_frame;
} O_Gaxter2;

void o_load_gaxter2(Obj *o, uint16_t data);
void o_unload_gaxter2(void);

#endif  // OBJ_GAXTER2_H
