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
	const int16_t sp_x = FIX32TOINT(o->x) - map_get_x_scroll();
	const int16_t sp_y = FIX32TOINT(o->y) - map_get_y_scroll();

	uint16_t attr = SPR_ATTR(e->base_tile_id, 0, 0, MAP_PAL_LINE, 0);

	spr_put(sp_x, sp_y, attr, SPR_SIZE(2, 1));
	spr_put(sp_x, sp_y + 8, attr + 0x10, SPR_SIZE(2, 1));
}

void o_load_bgtile(Obj *o, uint16_t data)
{
	O_BgTile *e = (O_BgTile *)o;
	SYSTEM_ASSERT(sizeof(O_BgTile) <= sizeof(ObjSlot));
	e->base_tile_id = data;

	obj_basic_init(o, 0, INTTOFIX16(-8), INTTOFIX16(8),
	               INTTOFIX16(-16), 127);
	o->x -= INTTOFIX32(8);
	o->y -= INTTOFIX32(16);
	o->main_func = main_func;
	o->cube_func = NULL;
}
