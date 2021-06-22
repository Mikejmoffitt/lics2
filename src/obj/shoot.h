#ifndef OBJ_SHOOT_H
#define OBJ_SHOOT_H

#include "obj.h"

typedef struct O_Shoot
{
	Obj head;
	fix32_t x_max;
	fix32_t x_min;
	int16_t anim_cnt;
	int16_t anim_frame;

	int16_t moving_up;
	int16_t swoop_en;

} O_Shoot;

void o_load_shoot(Obj *o, uint16_t data);
void o_unload_shoot(void);

#endif  // OBJ_SHOOT_H
