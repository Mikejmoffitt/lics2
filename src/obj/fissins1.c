#include "obj/fissins1.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"


/*

Fissins 1 is more complicated than it appears on the surface.

Original movement is speed / 5.0, for PAL speed.
Gravity is thus (5/6) / 5.0.

Jump strength is a range between -25 and -35.
That translates to (5/6) * -5 to -7

He waits 20 frames (in PAL) before jumping each time.
So jump delay is 6/5 * 20

*/

static uint16_t s_vram_pos;

static int16_t kanim_delay;
static int16_t kjump_delay;
static fix16_t kgravity;
static fix16_t kjump_dy_table[8];

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_FISSINS1);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	kanim_delay = PALSCALE_DURATION(12);
	kjump_delay = PALSCALE_DURATION(24);
	kgravity = INTTOFIX16(PALSCALE_2ND(0.1666666667));

	const fix16_t kdy_base = INTTOFIX16(PALSCALE_1ST(-4.1667));
	const fix16_t kdy_spread = INTTOFIX16(PALSCALE_1ST(1.6667 /
	                                      ARRAYSIZE(kjump_dy_table)));
	for (uint16_t i = 0; i < ARRAYSIZE(kjump_dy_table); i++)
	{
		kjump_dy_table[i] = kdy_base - (i * kdy_spread);
	}

	s_constants_set = true;
}

static void render(O_Fissins1 *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;

	obj_render_setup(o, &sp_x, &sp_y, -8, -16,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame * 4), 0, o->dy > 0,
	                             LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));
}

static void main_func(Obj *o)
{
	O_Fissins1 *e = (O_Fissins1 *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	if (!e->airborn)
	{
		if (e->jump_countdown == 0)
		{
			o->flags |= OBJ_FLAG_TANGIBLE;
			o->flags |= OBJ_FLAG_HARMFUL;
			e->airborn = 1;
			o->dy = kjump_dy_table[system_rand() % ARRAYSIZE(kjump_dy_table)];
		}
		else
		{
			e->jump_countdown--;
		}
	}
	else
	{
		if (o->dy > 0 && o->y >= e->base_y)
		{
			e->airborn = 0;
			e->jump_countdown = kjump_delay;
			o->y = e->base_y;
			o->flags &= ~(OBJ_FLAG_TANGIBLE);
			o->flags &= ~(OBJ_FLAG_HARMFUL);
		}
		obj_standard_physics(o);
		o->dy += kgravity;
		OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_delay);
		render(e);
	}
}

void o_load_fissins1(Obj *o, uint16_t data)
{
	O_Fissins1 *e = (O_Fissins1 *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Fissins1", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-15), 1);
	o->main_func = main_func;
	o->cube_func = NULL;

	e->base_y = o->y;
}

void o_unload_fissins1(void)
{
	s_vram_pos = 0;
}
