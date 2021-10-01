#ifndef OBJ_BGTILE_H
#define OBJ_BGTILE_H

#include "obj.h"

typedef struct O_BgTile
{
	Obj head;

	uint16_t attr;
	int16_t px;
	int16_t py;
} O_BgTile;

void o_load_bgtile(Obj *o, uint16_t data);

#endif  // OBJ_BGTILE_H
