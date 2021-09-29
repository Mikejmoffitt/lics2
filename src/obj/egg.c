#include "obj/egg.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"
#include "progress.h"

static uint16_t s_vram_pos;

static fix16_t kgravity;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_EGG);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kgravity = INTTOFIX16(PALSCALE_1ST(0.1666666667));
	s_constants_set = 1;
}

static void render(O_Egg *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -12;
	static const int16_t offset_y = -32;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(3, 4));
}

static void main_func(Obj *o)
{
	O_Egg *e = (O_Egg *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	if (o->dy > 0)
	{
		obj_standard_physics(o);
		o->dy += kgravity;

		if (o->y + o->top > map_get_bottom())
		{
			o->status = OBJ_STATUS_NULL;
			progress_get()->egg_dropped = 1;
		}
	}

	render(e);
}

static void cube_func(Obj *o, Cube *c)
{
	obj_standard_cube_response(o, c);
	if (o->hp <= 0)
	{
		o->hp = 127;
		o->dy = kgravity;
	}
}

void o_load_egg(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Egg) <= sizeof(ObjSlot));
	(void)data;

	if (progress_get()->egg_dropped)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}

	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_TANGIBLE | OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-12), INTTOFIX16(12), INTTOFIX16(-32), 3);
	o->left = INTTOFIX16(-10);
	o->right = INTTOFIX16(10);
	o->top = INTTOFIX16(-26);
	o->main_func = main_func;
	o->cube_func = cube_func;
}

void o_unload_egg(void)
{
	s_vram_pos = 0;
}
