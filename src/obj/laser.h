#ifndef OBJ_LASER_H
#define OBJ_LASER_H

#include "obj.h"

typedef enum LaserMode
{
	LASER_MODE_PERIODIC,
	LASER_MODE_ON,
	LASER_MODE_OFF,
} LaserMode;

typedef struct O_Laser
{
	Obj head;

	bool timer_master;

	int16_t height;  // Height in 16px increments.
	int16_t phase;
	int16_t anim_cnt;
	int16_t anim_frame;

	LaserMode mode;
} O_Laser;

void o_load_laser(Obj *o, uint16_t data);
void o_unload_laser(void);

void laser_set_mode(LaserMode mode);

#endif  // OBJ_LASER_H
