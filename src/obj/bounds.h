#ifndef OBJ_BOUNDS_H
#define OBJ_BOUNDS_H

#include "obj.h"

typedef struct O_Bounds
{
	Obj head;
} O_Bounds;

void o_load_bounds(Obj *o, uint16_t data);
void o_unload_bounds(void);

#endif  // OBJ_BOUNDS_H
