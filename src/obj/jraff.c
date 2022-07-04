#include "obj/jraff.h"
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

	const Gfx *g = gfx_get(GFX_JRAFF);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.
static fix16_t kdx;
static fix16_t kgravity;
static fix16_t kdy_max;
static int16_t kanim_speed;

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kdx = INTTOFIX16(PALSCALE_1ST(0.416666666667));
	kdy_max = INTTOFIX16(PALSCALE_1ST(3.333333333));
	kgravity = INTTOFIX16(PALSCALE_2ND(0.1388888888892));
	kanim_speed = PALSCALE_DURATION(10);

	s_constants_set = 1;
}

static inline void render(O_Jraff *f)
{
	Obj *o = (Obj *)f;
	// Top half.
	int16_t sp_x, sp_y;
	obj_render_setup(o, &sp_x, &sp_y, -12, -63,
	                 map_get_x_scroll(), map_get_y_scroll());

	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + 36 + (12 * f->anim_frame),
	        o->direction == OBJ_DIRECTION_LEFT,
	        0, ENEMY_PAL_LINE, 0), SPR_SIZE(3, 4));

	// Bottom half.
	sp_y += 32;
	if (f->anim_frame == 1)
	{
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos,
		        o->direction == OBJ_DIRECTION_LEFT,
		        0, ENEMY_PAL_LINE, 0), SPR_SIZE(3, 4));
	}
	else if (f->anim_frame == 3)
	{
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + 12,
		        o->direction == OBJ_DIRECTION_LEFT,
		        0, ENEMY_PAL_LINE, 0), SPR_SIZE(3, 4));
	}
	else
	{
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + 24,
		        o->direction == OBJ_DIRECTION_LEFT,
		        0, ENEMY_PAL_LINE, 0), SPR_SIZE(3, 4));
	}
}

static void main_func(Obj *o)
{
	O_Jraff *f = (O_Jraff *)o;

	if (o->hurt_stun > 0)
	{
		render(f);
		return;
	}

	obj_standard_physics(o);

	// Left/right movement, and collision.
	const int16_t left = FIX32TOINT(o->x) - 10;
	const int16_t right = FIX32TOINT(o->x) + 10;
	const int16_t bottom = FIX32TOINT(o->y);

	// In the original MMF game, Jraff moves back 4 pixels from the wall after
	// a collision, so that quirk has been replicated.
	if (o->dx > 0 && map_collision(right + 1, bottom - 5))
	{
		o->dx = -kdx;
		o->x -= INTTOFIX32(4);
		o->direction = OBJ_DIRECTION_LEFT;
	}
	else if (o->dx < 0 && map_collision(left - 1, bottom - 5))
	{
		o->dx = kdx;
		o->x += INTTOFIX32(4);
		o->direction = OBJ_DIRECTION_RIGHT;
	}

	// Vertical movement.
	if (!map_collision(left, bottom + 1) && !map_collision(right, bottom + 1))
	{
		o->dy += kgravity;
	}
	else
	{
		o->dy = 0;
		o->y = INTTOFIX32((8 * ((bottom + 1) / 8)) - 1);
	}


	// Animation.
	f->anim_cnt++;
	if (f->anim_cnt >= kanim_speed)
	{
		f->anim_cnt = 0;
		f->anim_frame++;
		if (f->anim_frame >= 4)
		{
			f->anim_frame = 0;
		}
	}
	render(f);
}

void o_load_jraff(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Jraff) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Jraff", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-7), INTTOFIX16(7), INTTOFIX16(-63), 2);
	o->main_func = main_func;
	o->cube_func = NULL;
	o->dx = kdx;
}

void o_unload_jraff(void)
{
	s_vram_pos = 0;
}
