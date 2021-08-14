#ifndef OBJ_TOSSMUFFIN_H
#define OBJ_TOSSMUFFIN_H

#include "obj.h"
#include "cube.h"

typedef struct O_Tossmuffin
{
	Obj head;

	Cube *holding_cube;

	int16_t anim_frame;
	int16_t anim_cnt;
	int16_t lift_cnt;
	int16_t toss_cnt;
	int16_t saw_player;
} O_Tossmuffin;

void o_load_tossmuffin(Obj *o, uint16_t data);
void o_unload_tossmuffin(void);

#endif  // OBJ_TOSSMUFFIN_H
