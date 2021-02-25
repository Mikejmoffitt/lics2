#ifndef OBJ_BOGOLOGO_H
#define OBJ_BOGOLOGO_H

#include "obj.h"

typedef struct O_Bogologo
{
	Obj head;
	int16_t appear_cnt;
	int16_t anim_frame;
	int16_t anim_cnt;
} O_Bogologo;

void o_load_bogologo(Obj *o, uint16_t data);
void o_unload_bogologo(void);

#endif  // OBJ_BOGOLOGO_H
