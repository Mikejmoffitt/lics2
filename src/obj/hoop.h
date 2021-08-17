#ifndef OBJ_HOOP_H
#define OBJ_HOOP_H

#include "obj.h"

typedef struct O_Hoop
{
	Obj head;

	// TODO: Track O_Ball.
} O_Hoop;

void o_load_hoop(Obj *o, uint16_t data);
void o_unload_hoop(void);

#endif  // OBJ_HOOP_H
