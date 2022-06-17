#ifndef OBJ_BUGGO2_H
#define OBJ_BUGGO2_H

#include "obj.h"
#include <md/megadrive.h>

typedef struct O_Buggo2
{
	Obj head;

	fix32_t x_min;
	fix32_t x_max;

	int16_t anim_frame;
	int16_t anim_cnt;

	int16_t spin_anim_frame;

	int16_t shot_clock;  // Countdown until the next shot.
	int16_t spin_cnt;  // Counts upwards for the floor buggo.
} O_Buggo2;

void o_load_buggo2(Obj *o, uint16_t data);
void o_unload_buggo2(void);

#endif  // OBJ_BUGGO2_H
