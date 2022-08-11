#include "obj/fissins2.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "obj/lyle.h"
#include "particle.h"
#include "game.h"

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
static int16_t kcooldown;
static fix16_t kdx;
static fix16_t kgravity;
static fix16_t kjump_dy_table[8];

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_FISSINS2);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kanim_delay = PALSCALE_DURATION(12);
	kcooldown = PALSCALE_DURATION(20);
	kdx = INTTOFIX16(PALSCALE_1ST(1.666667));
	kgravity = INTTOFIX16(PALSCALE_2ND(0.1666666667));

	const fix16_t kdy_base = INTTOFIX16(PALSCALE_1ST(-2.5));
	const fix16_t kdy_spread = INTTOFIX16(PALSCALE_1ST(-1.66667 / ARRAYSIZE(kjump_dy_table)));
	for (uint16_t i = 0; i < ARRAYSIZE(kjump_dy_table); i++)
	{
		kjump_dy_table[i] = kdy_base + (i * kdy_spread);
	}

	s_constants_set = 1;
}

static void render(O_Fissins2 *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;

	obj_render_setup(o, &sp_x, &sp_y, -8, -16,
	                 map_get_x_scroll(), map_get_y_scroll());

	if (e->airborn)
	{
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame * 4), o->direction == OBJ_DIRECTION_LEFT, 0,
		                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
	}
	else
	{
		md_spr_put(sp_x, sp_y + 16, SPR_ATTR(s_vram_pos + 8 + (e->anim_frame * 4), 0, 0,
		                                  MAP_PAL_LINE, 1), SPR_SIZE(2, 2));
	}
}

static void main_func(Obj *o)
{
	O_Fissins2 *e = (O_Fissins2 *)o;
	O_Lyle *l = lyle_get();
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}
	// Reverse course if at the screen boundary.
	const int16_t obj_x_px = FIX32TOINT(o->x);
	const int16_t cam_left = map_get_x_scroll();
	const int16_t cam_right = map_get_x_scroll() + GAME_SCREEN_W_PIXELS;

	if (o->direction == OBJ_DIRECTION_LEFT && obj_x_px <= cam_left + 8)
	{
		o->direction = OBJ_DIRECTION_RIGHT;
	}
	else if (o->direction == OBJ_DIRECTION_RIGHT && obj_x_px >= cam_right - 8)
	{
		o->direction = OBJ_DIRECTION_LEFT;
	}

	// scan for boundaries and use them to reverse course.
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *other = &g_objects[i].obj;
		if (other->status == OBJ_STATUS_NULL) continue;
		if (other->type != OBJ_BOUNDS) continue;
		if (!obj_touching_obj(o, other)) continue;

		if (o->x < other->x)
		{
			o->direction = OBJ_DIRECTION_LEFT;
		}
		else
		{
			o->direction = OBJ_DIRECTION_RIGHT;
		}
	}


	// Set dx based on direction.
	o->dx = (o->direction == OBJ_DIRECTION_RIGHT) ? kdx : -kdx;

	if (!e->airborn)  // Internal flag 0 in MMF2.
	{
		static const fix32_t distance_margin = INTTOFIX32(80);
		if (e->cooldown == 0 &&
		    o->x > (l->head.x - distance_margin) &&
		    o->x < (l->head.x + distance_margin))
		{
			// TODO: Play sand sound
			o->flags |= OBJ_FLAG_TANGIBLE;
			o->flags |= OBJ_FLAG_HARMFUL;
			e->airborn = 1;
			o->dy = kjump_dy_table[system_rand() % ARRAYSIZE(kjump_dy_table)];
			e->anim_frame = 0;
			particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
			particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
			particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
			particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
			particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
		}
		else
		{
			if (e->cooldown > 0) e->cooldown--;
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 3, kanim_delay / 2);
		}
	}
	else
	{
		if (o->dy > 0 && o->y >= e->base_y)
		{
			// TODO: Play sand sound
			e->airborn = 0;
			o->y = e->base_y;
			o->dy = 0;
			o->flags &= ~(OBJ_FLAG_TANGIBLE);
			o->flags &= ~(OBJ_FLAG_HARMFUL);
			e->cooldown = kcooldown;
			particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
			particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
			particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
			particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
			particle_spawn(o->x, o->y, PARTICLE_TYPE_SAND);
		}
		else
		{
			o->dy += kgravity;
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_delay);
		}
	}
	obj_standard_physics(o);
	render(e);
}

void o_load_fissins2(Obj *o, uint16_t data)
{
	O_Fissins2 *e = (O_Fissins2 *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Fissins2", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-15), 1);
	o->main_func = main_func;
	o->cube_func = NULL;

	e->base_y = o->y;  // Variable G in MMF2 source.
}

void o_unload_fissins2(void)
{
	s_vram_pos = 0;
}
