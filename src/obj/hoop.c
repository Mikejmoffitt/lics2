#include "obj/hoop.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "obj/exploder.h"

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_HOOP);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static int16_t kdestroy_cnt_max;

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kdestroy_cnt_max = PALSCALE_DURATION(20);  // TODO: Tune, this is guesswork.

	s_constants_set = 1;
}

static void main_func(Obj *o)
{
	O_Hoop *e = (O_Hoop *)o;
	if (e->destroy_cnt > 0)
	{
		e->destroy_cnt++;
		if (e->destroy_cnt > kdestroy_cnt_max)
		{
			const fix16_t kspawn_rate = INTTOFIX16(PALSCALE_1ST(1.0));
			// TODO: Is it fizzle red, or blue?
			exploder_spawn(o->x, o->y + (o->top / 2), o->dx, o->dy, PARTICLE_TYPE_FIZZLERED, 6, kspawn_rate);
			obj_erase(o);
			// TODO: Tsss sand sound (same as fissins2)
		}
	}
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -18;
	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
	                             MAP_PAL_LINE, 0), SPR_SIZE(2, 2));
}

void o_load_hoop(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Hoop) <= sizeof(ObjSlot));
	(void)data;
	vram_load();
	set_constants();

	// TODO: Should be OBJ_TANGIBLE if cubes can collide?
	obj_basic_init(o, "Hoop", 0,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_hoop(void)
{
	s_vram_pos = 0;
}

void hoop_mark_for_destruction(O_Hoop *e)
{
	e->destroy_cnt = 1;
}
