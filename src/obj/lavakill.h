#ifndef OBJ_LAVAKILL_H
#define OBJ_LAVAKILL_H

#include "obj.h"

typedef struct O_Lavakill
{
	Obj head;
} O_Lavakill;

void o_load_lavakill(Obj *o, uint16_t data);

#endif  // OBJ_LAVAKILL_H
