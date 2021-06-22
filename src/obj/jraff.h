#ifndef OBJ_JRAFF_H
#define OBJ_JRAFF_H

#include "obj.h"

typedef struct O_Jraff
{
	Obj head;
	int16_t anim_cnt;
	int16_t anim_frame;
} O_Jraff;

void o_load_jraff(Obj *o, uint16_t data);
void o_unload_jraff(void);

#endif  // OBJ_JRAFF_H
