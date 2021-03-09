#ifndef OBJ_GRASSES_H
#define OBJ_GRASSES_H

#include "obj.h"

typedef struct O_Grasses
{
	Obj head;
} O_Grasses;

void o_load_grasses(Obj *o, uint16_t data);

#endif  // OBJ_GRASSES_H
