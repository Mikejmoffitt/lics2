#ifndef OBJTILE_H
#define OBJTILE_H

// Really simple list for sprites used to draw background tiles.
// Drawn underneath objects.

#include "obj.h"

#include <stdlib.h>
#include "md/megadrive.h"

enum
{
	OBJTILE_FLAG_ACTIVE = 0x8000
};

typedef struct ObjTile ObjTile;
struct ObjTile
{
	uint16_t flags;
	SprParam spr;
	int16_t px, py;
} __attribute__((aligned(16)));

void objtile_clear(void);
void objtile_poll(void);

ObjTile *objtile_place(fix32_t x, fix32_t y, uint16_t attr);

void objtile_set_hibernate(uint16_t en);

#endif  // OBJTILE_H
