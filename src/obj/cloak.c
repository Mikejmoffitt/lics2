#include "obj/cloak.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"


static uint16_t s_vram_pos;

static fix16_t kgravity;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_CLOAK);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kgravity = INTTOFIX16(PALSCALE_1ST((1 / 12.0) * 5.0 / 6.0));

	s_constants_set = true;
}

static void render(O_Cloak *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -16;
	static const int16_t offset_y = -8;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
	                                ENEMY_PAL_LINE, 0), SPR_SIZE(4, 2));
}

static void main_func(Obj *o)
{
	O_Cloak *e = (O_Cloak *)o;
	e->life++;
	o->dy += kgravity;

	obj_mixed_physics_h(o);

	if (e->life < PALSCALE_DURATION(30 * 5.0 / 6.0) || e->life % 2)
	{
		render(e);
	}
	if (e->life >= PALSCALE_DURATION(50 * 5.0 / 6.0)) obj_erase(o);

}

void o_load_cloak(Obj *o, uint16_t data)
{
	O_Cloak *e = (O_Cloak *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Cloak", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-1), INTTOFIX16(1), INTTOFIX16(-2), 1);
	o->main_func = main_func;
	o->cube_func = NULL;

	o->dx = INTTOFIX16(PALSCALE_1ST(1.0 * 5.0 / 6.0));
	o->dy = INTTOFIX16(PALSCALE_1ST(-1.666667 * 5.0 / 6.0));
}

void o_unload_cloak(void)
{
	s_vram_pos = 0;
}
