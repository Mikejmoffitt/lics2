#ifndef OBJ_SHOOT_H
#define OBJ_SHOOT_H

#include "obj.h"

typedef struct O_Shoot
{
	Obj head;
	uint16_t h_flip_cnt;

} O_Shoot;

void o_load_shoot(Obj *o, uint16_t data);
void o_unload_shoot(void);

#endif  // OBJ_SHOOT_H
