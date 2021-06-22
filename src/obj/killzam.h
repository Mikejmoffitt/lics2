#ifndef OBJ_KILLZAM_H
#define OBJ_KILLZAM_H

#include "obj.h"

typedef struct O_Killzam
{
	Obj head;

	int16_t moving_down;

	int16_t anim_cnt;
	int16_t anim_frame;
	int16_t timer;

	int16_t flicker;
} O_Killzam;

void o_load_killzam(Obj *o, uint16_t data);
void o_unload_killzam(void);

#endif  // OBJ_KILLZAM_H
