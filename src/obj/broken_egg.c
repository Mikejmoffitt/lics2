#include "obj/broken_egg.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "progress.h"

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_BROKEN_EGG);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static void main_func(Obj *o)
{
	O_BrokenEgg *e = (O_BrokenEgg *)o;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -12;
	static const int16_t offset_y = -16;

	obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
	                        map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR((s_vram_pos + (e->type ? 6 : 0)), 0, 0,
	                             ENEMY_PAL_LINE, 0),
	        (e->type ? SPR_SIZE(3, 1) : SPR_SIZE(3, 2)));
}

void o_load_broken_egg(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_BrokenEgg) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");


	if (!progress_get()->egg_dropped)
	{
		obj_erase(o);
		return;
	}

	vram_load();

	obj_basic_init(o, "BroknEgg", 0,
	               INTTOFIX16(-12), INTTOFIX16(12), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
	O_BrokenEgg *e = (O_BrokenEgg *)o;
	e->type = data ? 1 : 0;
}

void o_unload_broken_egg(void)
{
	s_vram_pos = 0;
}
