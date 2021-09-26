#ifndef OBJ_BGTILE_H
#define OBJ_BGTILE_H

#include "obj.h"

typedef struct O_BgTile
{
	Obj head;

	uint8_t base_tile_id;
	uint8_t pal;
} O_BgTile;

void o_load_bgtile(Obj *o, uint16_t data);

#endif  // OBJ_BGTILE_H
