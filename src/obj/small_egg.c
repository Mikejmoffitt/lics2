#include "obj/small_egg.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "lyle.h"
#include "particle.h"
#include "progress.h"

static uint16_t s_vram_pos;

static fix16_t kgravity;
static fix16_t kbounce_dy;
static fix16_t kcube_bounce_dy;
static fix16_t kbounce_dx;
static int16_t kgenerator_cnt_max;
static int16_t kanim_speed;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_SMALL_EGG);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kgravity = INTTOFIX16(PALSCALE_1ST(0.16666666667));
	kbounce_dy = INTTOFIX16(PALSCALE_1ST(1.6666666667));
	kcube_bounce_dy = INTTOFIX16(PALSCALE_1ST(-0.16666666667));
	kbounce_dx = INTTOFIX16(PALSCALE_1ST(-0.8333333333));
	kgenerator_cnt_max = PALSCALE_DURATION(180);
	kanim_speed = PALSCALE_DURATION(7);

	s_constants_set = true;
}

static void render(O_SmallEgg *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -16;

	obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
	                        map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (4 * e->anim_frame), 0, 0,
	                             LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));
}

static inline void check_collision_with_lyle_cube(Obj *o)
{
	const O_Lyle *l = lyle_get();

	// Egg half-width plus cube half-width.
	if (o->dy <= 0) return;
	const fix32_t adj_x = INTTOFIX32(4 + 8);
	if (o->x + adj_x < l->head.x) return;
	if (o->x - adj_x > l->head.x) return;
	if (o->y < l->head.y - INTTOFIX32(35)) return;
	if (o->y + o->top > l->head.y - INTTOFIX32(35 - 16)) return;

	o->dx = kbounce_dx;
	o->dy = (-o->dy / 2) - kbounce_dy;

}

static void main_func(Obj *o)
{
	O_SmallEgg *e = (O_SmallEgg *)o;
	// Bouncing on Lyle's cube
	if (lyle_get()->holding_cube)
	{
		check_collision_with_lyle_cube(o);
	}

	o->dy += kgravity;
	obj_standard_physics(o);
	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kanim_speed);

	// Destruction upon hitting the ground
	int16_t y_px = FIX32TOINT(o->y);
	const int16_t x_center = FIX32TOINT(o->x);
	if (o->dy > 0 && map_collision(x_center, y_px))
	{
		obj_erase(o);
		particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
		particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
		particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
		particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
		particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
	}


	render(e);
}

static void generator_func(Obj *o)
{
	O_SmallEgg *e = (O_SmallEgg *)o;
	if (e->generator_cnt >= kgenerator_cnt_max)
	{
		e->generator_cnt = 0;
		obj_spawn(FIX32TOINT(o->x) - 8, FIX32TOINT(o->y) - 10, OBJ_SMALL_EGG, 0);
	}
	else
	{
		e->generator_cnt++;
	}
}

static void cube_func(Obj *o, Cube *c)
{
	(void)c;
	if (o->dy > kcube_bounce_dy) o->dy = kcube_bounce_dy;
}

void o_load_small_egg(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_SmallEgg) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");

	obj_basic_init(o, "SmallEgg", OBJ_FLAG_TANGIBLE, INTTOFIX16(-8), INTTOFIX16(8),
	               INTTOFIX16(-16), 127);

	// Generator does not spawn once CP orb 6 has been collected.
	if (data && (progress_get()->cp_orbs & (1 << 6)))
	{
		obj_erase(o);
		return;
	}
	o->left = INTTOFIX16(-4);
	o->left = INTTOFIX16(4);
	o->top = INTTOFIX16(-9);

	o->main_func = data ? generator_func : main_func;
	o->cube_func = cube_func;

	set_constants();
	vram_load();
}

void o_unload_small_egg(void)
{
	s_vram_pos = 0;
}
