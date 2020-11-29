#ifndef OBJ_FISSINS1_H
#define OBJ_FISSINS1_H

#include "obj.h"

typedef struct O_Fissins1
{
	Obj head;
} O_Fissins1;

void o_load_fissins1(Obj *o, uint16_t data);
void o_unload_fissins1(void);

#endif  // OBJ_FISSINS1_H
