#ifndef OBJ_BALL_H
#define OBJ_BALL_H

#include "obj.h"

typedef struct O_Ball
{
	Obj head;
	uint16_t data;

	int16_t anim_cnt;
	int16_t anim_frame;
} O_Ball;

void o_load_ball(Obj *o, uint16_t data);
void o_unload_ball(void);

#endif  // OBJ_BALL_H
