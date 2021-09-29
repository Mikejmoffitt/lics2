#ifndef OBJ_EGG_H
#define OBJ_EGG_H

#include "obj.h"

typedef struct O_Egg
{
	Obj head;
} O_Egg;

void o_load_egg(Obj *o, uint16_t data);
void o_unload_egg(void);

#endif  // OBJ_EGG_H
