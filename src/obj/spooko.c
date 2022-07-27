#include "obj/spooko.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"


static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_SPOOKO);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	s_constants_set = 1;
}

static void main_func(Obj *o)
{
	O_Spooko *e = (O_Spooko *)o;
	int16_t sp_x, sp_y;

	if (e->data == 0)
	{
		obj_render_setup_simple(o, &sp_x, &sp_y, -8, -16,
		                        map_get_x_scroll(), map_get_y_scroll());
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
		                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
	}
	else
	{
		obj_render_setup_simple(o, &sp_x, &sp_y, -8, -8,
		                        map_get_x_scroll(), map_get_y_scroll());
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + 4, 0, 0,
		                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 1));
	}
}

void o_load_spooko(Obj *o, uint16_t data)
{
	(void)data;
	O_Spooko *e = (O_Spooko *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	set_constants();
	vram_load();

	obj_basic_init(o, "Spooko", 0,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 1);
	o->main_func = main_func;
	o->cube_func = NULL;


	e->data = data;
}

void o_unload_spooko(void)
{
	s_vram_pos = 0;
}
