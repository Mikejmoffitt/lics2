#include "obj/buggo2.h"
// Buggo that spins.

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "obj/map.h"
#include "lyle.h"
#include "projectile.h"
#include "sfx.h"

#include "cube.h"
#include "palscale.h"

// Constants.
static int16_t kanim_speed;

static fix16_t kdx;

static int16_t kbuggo1_shot_test;
static int16_t kbuggo1_shot_fire;
static fix16_t kbuggo1_shot_dy;

static int16_t kbuggo2_spark_time;
static fix16_t kbuggo2_spark_dx;

static fix16_t kcube_bounce_dy;

static void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kbuggo1_shot_test = PALSCALE_DURATION(48);
	kbuggo1_shot_fire = PALSCALE_DURATION(84);
	kbuggo1_shot_dy = INTTOFIX16(PALSCALE_1ST(5.416666666667));

	kbuggo2_spark_time = PALSCALE_DURATION(144);
	kbuggo2_spark_dx = INTTOFIX16(PALSCALE_1ST(2.5));

	kanim_speed = PALSCALE_DURATION(7);
	kcube_bounce_dy = INTTOFIX16(PALSCALE_1ST(-1.8333334));

	kdx = INTTOFIX16(PALSCALE_1ST(.277777777777778));
	s_constants_set = 1;
}

// VRAM.

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_BUGGO);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Main.

static inline void render(O_Buggo2 *f)
{
	uint16_t tile_offset;

	Obj *o = &f->head;
	int16_t sp_x, sp_y;
	obj_render_setup(o, &sp_x, &sp_y, -8, -16,
	                 map_get_x_scroll(), map_get_y_scroll());

	if (f->spin_cnt == 0)
	{
		// Normal walking animation.
		tile_offset = 16 + (f->anim_frame * 4);
	}
	else
	{
		tile_offset = 32 + (f->spin_anim_frame * 4);
		// Spinning animation.
	}

	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + tile_offset,
	                    o->direction == OBJ_DIRECTION_LEFT, 0,
	                    ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
}

static void buggo2_cube_func(Obj *o, Cube *c)
{
	if (o->hurt_stun > 0) return;
	if (c->status == CUBE_STATUS_KICKED)
	{
		obj_standard_cube_response(o, c);
	}
	else if (c->type == CUBE_TYPE_GREEN)
	{
		// Just setting some random high dx to give the cube some direction
		// since the clamp will put it within a reasonable range.
		if (c->dx == 0) c->dx = INTTOFIX32(system_rand() % 2 ? 5 : -5);
		cube_clamp_dx(c);
		c->dy = kcube_bounce_dy;
		sfx_play(SFX_CUBE_BOUNCE, 15);
	}
	else
	{
		cube_destroy(c);
	}
}

static void main_func(Obj *o)
{
	O_Buggo2 *f = (O_Buggo2 *)o;

	if (o->hurt_stun > 0)
	{
		render(f);
		return;
	}

	f->shot_clock++;

	// Movement.
	if (o->direction == OBJ_DIRECTION_LEFT &&
	    o->x < f->x_min)
	{
		o->direction = OBJ_DIRECTION_RIGHT;
		o->dx = kdx;
	}
	else if (o->direction == OBJ_DIRECTION_RIGHT &&
	         o->x > f->x_max)
	{
		o->direction = OBJ_DIRECTION_LEFT;
		o->dx = -kdx;
	}

	// Collision.
	if (o->dx > 0 &&
	    map_collision(FIX32TOINT(o->x + o->right), FIX32TOINT(o->y) - 1))
	{
		o->direction = OBJ_DIRECTION_LEFT;
		o->dx = -kdx;
		f->x_min = o->x - INTTOFIX32(50);
		f->x_max = o->x;
	}
	else if (o->dx < 0 &&
	         map_collision(FIX32TOINT(o->x + o->left), FIX32TOINT(o->y) - 1))
	{
		o->direction = OBJ_DIRECTION_RIGHT;
		o->dx = kdx;
		f->x_min = o->x;
		f->x_max = o->x + INTTOFIX32(50);
	}
	// Shooting.
	if (f->spin_cnt > 0)
	{
		f->spin_cnt--;
	}
	if (f->shot_clock == kbuggo2_spark_time)
	{
		f->shot_clock = 0;
		f->spin_cnt = kbuggo2_spark_time / 2;
		projectile_shoot(o->x, o->y - INTTOFIX32(1), PROJECTILE_TYPE_SPARK,
		                         kbuggo2_spark_dx, 0);
		projectile_shoot(o->x, o->y - INTTOFIX32(1), PROJECTILE_TYPE_SPARK,
		                         -kbuggo2_spark_dx, 0);
		sfx_play(SFX_GAXTER_SHOT, 15);

	}

	if (f->spin_cnt == 0) obj_accurate_physics(o);

	// Animate.
	f->anim_cnt++;
	if (f->anim_cnt >= kanim_speed)
	{
		f->anim_cnt = 0;
		f->anim_frame++;
		if (f->anim_frame > 2) f->anim_frame = 0;
		f->spin_anim_frame++;
		if (f->spin_anim_frame > 3) f->spin_anim_frame = 0;
	}

	render(f);
}

// Boilerplate.

void o_load_buggo2(Obj *o, uint16_t data)
{
	O_Buggo2 *f = (O_Buggo2 *)o;
	_Static_assert(sizeof(*f) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	vram_load();
	set_constants();

	obj_basic_init(o, "Buggo 2", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16),
	               1);
	o->main_func = main_func;
	f->x_min = o->x - INTTOFIX32(50);
	f->x_max = o->x;
	o->dx = -kdx;

	o->left = INTTOFIX16(-7);
	o->right = INTTOFIX16(7);
	o->cube_func = buggo2_cube_func;
	o->top = INTTOFIX16(-11);
	o->direction = OBJ_DIRECTION_LEFT;
}

void o_unload_buggo2(void)
{
	s_vram_pos = 0;
}
