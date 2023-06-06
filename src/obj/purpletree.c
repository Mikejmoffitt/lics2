#include "obj/purpletree.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"


static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_PURPLETREE);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static void render(O_PurpleTree *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -16;

	obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
	                        map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (4 * e->tile), 0, 0,
	                             MAP_PAL_LINE, 0), SPR_SIZE(2, 2));
}

static void main_func(Obj *o)
{
	O_PurpleTree *e = (O_PurpleTree *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}
	render(e);
}

void o_load_purpletree(Obj *o, uint16_t data)
{
	O_PurpleTree *e = (O_PurpleTree *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	e->tile = data;
	vram_load();

	obj_basic_init(o, "PrplTree", 0, INTTOFIX16(-8), INTTOFIX16(8),
	               INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_purpletree(void)
{
	s_vram_pos = 0;
}
