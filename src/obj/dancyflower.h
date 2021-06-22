#ifndef OBJ_DANCYFLOWER_H
#define OBJ_DANCYFLOWER_H

#include "obj.h"

typedef struct O_Dancyflower
{
	Obj head;
	int16_t anim_cnt;
	int16_t anim_frame;
} O_Dancyflower;

void o_load_dancyflower(Obj *o, uint16_t data);
void o_unload_dancyflower(void);

#endif  // OBJ_DANCYFLOWER_H
