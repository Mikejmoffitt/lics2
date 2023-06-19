#ifndef OBJ_POINTYSIGN_H
#define OBJ_POINTYSIGN_H

#include "obj.h"

typedef struct O_PointySign
{
	Obj head;
} O_PointySign;

void o_load_pointysign(Obj *o, uint16_t data);
void o_unload_pointysign(void);

#endif  // OBJ_POINTYSIGN_H
