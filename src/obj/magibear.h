#ifndef OBJ_MAGIBEAR_H
#define OBJ_MAGIBEAR_H

#include "obj.h"

typedef struct O_Magibear
{
	Obj head;
	int16_t shot_cnt;  // Counts down until he fires.
	int16_t mouth_cnt;  // Counts down; keeps him immobile and mouth ajar.
	int16_t anim_cnt;
	int16_t anim_frame;
} O_Magibear;

void o_load_magibear(Obj *o, uint16_t data);
void o_unload_magibear(void);

#endif  // OBJ_MAGIBEAR_H
