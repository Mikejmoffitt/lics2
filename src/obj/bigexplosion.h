#ifndef OBJ_BIGEXPLOSION_H
#define OBJ_BIGEXPLOSION_H

#include "obj.h"

typedef struct O_BigExplosion
{
	Obj head;
	uint16_t anim_cnt;
	uint16_t anim_frame;
} O_BigExplosion;

void o_load_bigexplosion(Obj *o, uint16_t data);
void o_unload_bigexplosion(void);

#endif  // OBJ_BIGEXPLOSION_H
