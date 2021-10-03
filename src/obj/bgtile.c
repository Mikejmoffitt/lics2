#include "obj/bgtile.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"
#include "obj/lyle.h"
#include "obj/exploder.h"

static void main_func(Obj *o)
{
	O_BgTile *e = (O_BgTile *)o;
	const int16_t sp_x = e->px - map_get_x_scroll();
	const int16_t sp_y = e->py - map_get_y_scroll();

	spr_put(sp_x, sp_y, e->attr, SPR_SIZE(2, 1));
	spr_put(sp_x, sp_y + 8, e->attr + 0x10, SPR_SIZE(2, 1));
}

void o_load_bgtile(Obj *o, uint16_t data)
{
	O_BgTile *e = (O_BgTile *)o;
	SYSTEM_ASSERT(sizeof(O_BgTile) <= sizeof(ObjSlot));

	obj_basic_init(o, 0, INTTOFIX16(-8), INTTOFIX16(8),
	               INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
	e->attr = SPR_ATTR(data & 0x00FF, 0, 0, (data & 0x0300) >> 8, 0);
	e->px = FIX32TOINT(o->x) - 8;
	e->py = FIX32TOINT(o->y) - 16;
}
