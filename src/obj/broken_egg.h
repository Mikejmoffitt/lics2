#ifndef OBJ_BROKEN_EGG_H
#define OBJ_BROKEN_EGG_H

#include "obj.h"

typedef struct O_BrokenEgg
{
	Obj head;
	int16_t type;
} O_BrokenEgg;

void o_load_broken_egg(Obj *o, uint16_t data);
void o_unload_broken_egg(void);

#endif  // OBJ_BROKEN_EGG_H
