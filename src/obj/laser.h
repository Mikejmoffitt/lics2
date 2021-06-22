#ifndef OBJ_LASER_H
#define OBJ_LASER_H

#include "obj.h"

typedef struct O_Laser
{
	Obj head;

	int16_t height;  // Height in 16px increments.
	int16_t timer;
	int16_t phase;
	int16_t anim_cnt;
	int16_t anim_frame;
} O_Laser;

void o_load_laser(Obj *o, uint16_t data);
void o_unload_laser(void);

#endif  // OBJ_LASER_H
