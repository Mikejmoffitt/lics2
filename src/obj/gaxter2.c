#include "obj/gaxter2.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "map.h"
#include "lyle.h"
#include "projectile.h"
#include "sfx.h"

#include "cube.h"
#include "palscale.h"

// Constants.

static fix16_t kddy;
static fix16_t kdy_cutoff;
static fix16_t kdx;
static fix16_t kshot_speed;
static int8_t kanim_len;
static int16_t kshot_clock_max;
static int16_t kshot_clock_flicker_time;
static int16_t kshot_flicker_speed;

static bool s_constants_set;

static void set_constants(void)
{
	if (s_constants_set) return;

	kddy = INTTOFIX16(PALSCALE_2ND(0.18));
	kdy_cutoff = INTTOFIX16(PALSCALE_1ST(2.17));
	kdx = INTTOFIX16(PALSCALE_1ST(0.416666666667));
	kshot_speed = INTTOFIX16(PALSCALE_1ST(2.3));
	kanim_len = PALSCALE_DURATION(2.2);
	kshot_clock_max = PALSCALE_DURATION(120);
	kshot_clock_flicker_time = PALSCALE_DURATION(84);
	kshot_flicker_speed = PALSCALE_DURATION(4.4);


	s_constants_set = true;
}

// VRAM.

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_GAXTER);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Main.

static inline void render(O_Gaxter2 *f)
{
	Obj *o = &f->head;
	int16_t sp_x, sp_y;
	obj_render_setup(o, &sp_x, &sp_y, -8, -8,
	                 map_get_x_scroll(), map_get_y_scroll());

	// Flicker the projectile on the end of Gaxter's appendage.
	if (f->shot_clock >= kshot_clock_flicker_time && f->shot_clock % 2)
	{
		const int16_t ball_x = sp_x + 4 +
		                       FIX32TOINT((o->direction == OBJ_DIRECTION_LEFT ?
		                                  o->left : o->right)) / 2;
		const int16_t ball_y = sp_y + 13;
		const int8_t is_flickering = (f->shot_flicker_cnt >= (kshot_flicker_speed / 2)) ? 1 : 0;
		const uint16_t tile_offset = 24 + (is_flickering ? 1 : 0);
		md_spr_put(ball_x, ball_y, SPR_ATTR(s_vram_pos + tile_offset, 0, 0,
		        LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));

	}
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + 12 + (f->anim_frame * 4),
	                    o->direction == OBJ_DIRECTION_LEFT, 0,
	                    ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
}

static void main_func(Obj *o)
{
	O_Gaxter2 *f = (O_Gaxter2 *)o;
	const O_Lyle *l = lyle_get();

	if (o->hurt_stun > 0)
	{
		render(f);
		return;
	}

	f->shot_flicker_cnt++;
	if (f->shot_flicker_cnt >= kshot_flicker_speed) f->shot_flicker_cnt = 0;

	// Horizontal movement.
	if (o->x >= f->x_max) o->dx = -kdx;
	else if (o->x <= f->x_min) o->dx = kdx;

	// Collision and direction.
	if (o->dx > 0 &&
	    map_collision(FIX32TOINT(o->x + o->right), FIX32TOINT(o->y)))
	{
		o->dx = -kdx;
		f->x_min = o->x - INTTOFIX32(50);
		f->x_max = o->x;
	}
	else if (o->dx < 0 &&
	         map_collision(FIX32TOINT(o->x + o->left), FIX32TOINT(o->y)))
	{
		o->dx = kdx;
		f->x_min = o->x;
		f->x_max = o->x + INTTOFIX32(50);
	}

	o->direction = (o->x < l->head.x) ? OBJ_DIRECTION_RIGHT : OBJ_DIRECTION_LEFT;

	// Vertical sinusoidal movement.
	if (f->moving_up)
	{
		o->dy += kddy;
		if (o->dy > kdy_cutoff) f->moving_up = 0;

	}
	else
	{
		o->dy -= kddy;
		if (o->dy < -kdy_cutoff) f->moving_up = 1;
	}

	// Shot clock.
	if (f->shot_clock >= kshot_clock_max)
	{
		f->shot_clock = 0;
		const fix32_t shot_y = o->y + INTTOFIX32(8);
		if (o->direction == OBJ_DIRECTION_LEFT)
		{
			projectile_shoot(o->x + (o->left / 2), shot_y,
			                         PROJECTILE_TYPE_BALL,
			                         -kshot_speed, kshot_speed);
		}
		else
		{
			projectile_shoot(o->x + (o->right / 2), shot_y,
			                         PROJECTILE_TYPE_BALL,
			                         kshot_speed, kshot_speed);
		}
		sfx_play(SFX_GAXTER_SHOT, 14);
	}
	else
	{
		f->shot_clock++;
	}

	obj_mixed_physics_h(o);

	// Animation.
	if (f->anim_cnt == kanim_len)
	{
		f->anim_cnt = 0;
		if (f->anim_frame == 2)
		{
			f->anim_frame = 0;
		}
		else
		{
			f->anim_frame++;
		}
	}
	else
	{
		f->anim_cnt++;
	}
	
	render(f);
}

// Boilerplate.

void o_load_gaxter2(Obj *o, uint16_t data)
{
	O_Gaxter2 *f = (O_Gaxter2 *)o;
	_Static_assert(sizeof(*f) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	vram_load();
	set_constants();

	obj_basic_init(o, "Gaxter 2", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-11), INTTOFIX16(11), INTTOFIX16(-12), 2);
	o->main_func = main_func;
	o->cube_func = NULL;
	o->y += INTTOFIX32(4);
	o->direction = OBJ_DIRECTION_LEFT;
	o->dx = -kdx;

	f->x_min = o->x - INTTOFIX32(50);
	f->x_max = o->x;
	f->moving_up = 1;

}

void o_unload_gaxter2(void)
{
	s_vram_pos = 0;
}
