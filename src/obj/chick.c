#include "obj/chick.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "progress.h"
#include "obj/lyle.h"
#include "powerup.h"
#include "sfx.h"

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_CHICK);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static int16_t knormal_anim_speed;
static int16_t kbelching_anim_speed;
static int16_t kflying_anim_speed;
static int16_t kreverse_time;

static fix16_t kflying_accel;
static fix16_t kflying_top_speed;

static fix16_t korb_dx;
static fix16_t korb_dy;

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	knormal_anim_speed = PALSCALE_DURATION(9);
	kbelching_anim_speed = PALSCALE_DURATION(6);
	kflying_anim_speed = PALSCALE_DURATION(7);
	kreverse_time = PALSCALE_DURATION(75);

	kflying_accel = INTTOFIX16(PALSCALE_2ND(0.20833333334));
	kflying_top_speed = INTTOFIX16(PALSCALE_2ND(4.16666666667));

	korb_dx = INTTOFIX16(PALSCALE_1ST(0.8333333334));
	korb_dy = INTTOFIX16(PALSCALE_1ST(-3.333333333));

	s_constants_set = 1;
}

static void render(O_Chick *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_y = -16;
	if (e->metaframe < 8)
	{
		static const int16_t offset_x = -8;
	
		obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
		                 map_get_x_scroll(), map_get_y_scroll());
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (4 * e->metaframe), o->direction == OBJ_DIRECTION_LEFT, 0,
		                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
	}
	else
	{
		static const int16_t offset_x = -12;
		
		obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
		                        map_get_x_scroll(), map_get_y_scroll());
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + 32 + (e->metaframe == 8 ? 0 : 6), o->direction == OBJ_DIRECTION_LEFT, 0,
		                             ENEMY_PAL_LINE, 0), SPR_SIZE(3, 2));
	}
}

static void main_func(Obj *o)
{
	O_Chick *e = (O_Chick *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	static const int16_t knormal_anim[] =
	{
		0, 1, 2, 1,
	};

	static const int16_t kbelching_anim[] =
	{
		3, 3, 4, 5, 6, 7
	};

	static const int16_t kflying_anim[] =
	{
		8, 9
	};

	const ChickState state_prev = e->state;

	switch (e->state)
	{
		case CHICK_STATE_NORMAL:
			if (e->state_elapsed == 0)
			{
				e->anim_cnt = 0;
				e->anim_frame = 0;
				e->reverse_cnt = 0;
				o->direction = OBJ_DIRECTION_RIGHT;
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, knormal_anim_speed);
			if (e->reverse_cnt >= kreverse_time)
			{
				e->reverse_cnt = 0;
				o->direction = (o->direction == OBJ_DIRECTION_RIGHT) ?
				               OBJ_DIRECTION_LEFT : OBJ_DIRECTION_RIGHT;
			}
			else
			{
				e->reverse_cnt++;
			}

			e->metaframe = knormal_anim[e->anim_frame];

			if (o->hp <= 17 && !e->belched && !(progress_get()->cp_orbs & (1 << 3)))
			{
				e->state = CHICK_STATE_BELCHING;
			}
			else if (o->hp >= 127)
			{
				e->state = CHICK_STATE_FLYING;
			}
			break;

		case CHICK_STATE_BELCHING:
			if (e->state_elapsed == 0)
			{
				e->anim_cnt = 0;
				e->anim_frame = 0;
				o->direction = OBJ_DIRECTION_RIGHT;
			}

			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 7, kbelching_anim_speed);
			if (e->anim_frame == 2 && !e->belched)
			{
				e->belched = 1;
				
				sfx_play(SFX_MAGIBEAR_SHOT, 14);
				Powerup *pow = powerup_spawn(o->x + INTTOFIX32(3), o->y - INTTOFIX32(8),
				                                     POWERUP_TYPE_CP_ORB, 3);
				pow->dx = korb_dx;
				pow->dy = korb_dy;
			}
			if (e->anim_frame >= 6)
			{
				e->anim_frame = 5;
				e->state = CHICK_STATE_NORMAL;
			}
			e->metaframe = kbelching_anim[e->anim_frame];
			break;

		case CHICK_STATE_FLYING:
			if (e->state_elapsed == 0)
			{
				e->anim_cnt = 0;
				e->anim_frame = 0;
				o->flags = OBJ_FLAG_HARMFUL | OBJ_FLAG_ALWAYS_ACTIVE;
			}
			else
			{
				const O_Lyle *l = lyle_get();

				// Lazily accelerate towards the player.
				if (o->x < l->head.x && o->dx < kflying_top_speed)
				{
					o->dx += kflying_accel;
					o->direction = OBJ_DIRECTION_RIGHT;
				}
				else if (o->x > l->head.x && o->dx > -kflying_top_speed)
				{
					o->dx -= kflying_accel;
					o->direction = OBJ_DIRECTION_LEFT;
				}
				if (o->y < l->head.y && o->dy < kflying_top_speed)
				{
					o->dy += kflying_accel;
				}
				else if (o->y > l->head.y && o->dy > -kflying_top_speed)
				{
					o->dy -= kflying_accel;
				}
				obj_standard_physics(o);
			}

			e->metaframe = kflying_anim[e->anim_frame];

			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kflying_anim_speed);
			break;
	}

	if (e->state != state_prev) e->state_elapsed = 0;
	else e->state_elapsed++;

	render(e);
}

static void cube_func(Obj *o, Cube *c)
{
	obj_standard_cube_response(o, c);
	if (o->hp <= 0)
	{
		o->hp = 127;
	}
}

void o_load_chick(Obj *o, uint16_t data)
{
	(void)data;
	_Static_assert(sizeof(O_Chick) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");

	if (!progress_get()->egg_dropped)
	{
		obj_erase(o);
		return;
	}

	set_constants();
	vram_load();

	obj_basic_init(o, "Chick", OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 20);
	o->main_func = main_func;
	o->cube_func = cube_func;
}

void o_unload_chick(void)
{
	s_vram_pos = 0;
}
