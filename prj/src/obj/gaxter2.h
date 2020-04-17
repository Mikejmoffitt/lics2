#ifndef OBJ_GAXTER2_H
#define OBJ_GAXTER2_H

#include "obj.h"
#include <md/megadrive.h>

typedef struct O_Gaxter2
{
	Obj head;
	fix32_t x_max;
	fix32_t x_min;
	int16_t shot_clock;
	int8_t moving_up;
	int8_t tile_offset;
	int8_t anim_cnt;
	int8_t anim_frame;

} O_Gaxter2;

void o_load_gaxter2(Obj *o, uint16_t data);
void o_unload_gaxter2(void);

#endif  // OBJ_GAXTER2_H
