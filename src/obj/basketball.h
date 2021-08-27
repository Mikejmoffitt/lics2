#ifndef OBJ_BASKETBALL_H
#define OBJ_BASKETBALL_H

#include "obj.h"

typedef struct O_Basketball
{
	Obj head;
} O_Basketball;

void o_load_basketball(Obj *o, uint16_t data);
void o_unload_basketball(void);

#endif  // OBJ_BASKETBALL_H
