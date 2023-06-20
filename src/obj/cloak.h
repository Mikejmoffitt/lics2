#ifndef OBJ_CLOAK_H
#define OBJ_CLOAK_H

#include "obj.h"

typedef struct O_Cloak
{
	Obj head;
	uint16_t life;
} O_Cloak;

void o_load_cloak(Obj *o, uint16_t data);
void o_unload_cloak(void);

#endif  // OBJ_CLOAK_H
