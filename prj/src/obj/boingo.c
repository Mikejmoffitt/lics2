#include "obj/boingo.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "obj/map.h"
#include "obj/lyle.h"
#include "common.h"
#include "sfx.h"
#include "obj/particle_manager.h"

#include "cube.h"
#include "palscale.h"

#define BOINGO_CUBE_ACTIVATION_DISTANCE INTTOFIX16(30)

// Constants.

static fix16_t kjump_str_table[8];
static fix16_t kgravity;
static int8_t kjump_delay;
static int8_t kjump_delay_angry;
static fix16_t kdx;
static fix16_t kcube_bounce_dy;
static int8_t kanim_speed;
static fix16_t kceiling_dy;

static void set_constants(void)
{
	static uint16_t constants_set;
	if (constants_set) return;

	kgravity = INTTOFIX16(PALSCALE_2ND(0.167));

	kjump_str_table[0] = INTTOFIX16(PALSCALE_1ST(0.83));
	kjump_str_table[1] = INTTOFIX16(PALSCALE_1ST(0.83 + 0.6));
	kjump_str_table[2] = INTTOFIX16(PALSCALE_1ST(0.83 + 1.2));
	kjump_str_table[3] = INTTOFIX16(PALSCALE_1ST(0.83 + 1.8));
	kjump_str_table[4] = INTTOFIX16(PALSCALE_1ST(0.83 + 2.4));
	kjump_str_table[5] = INTTOFIX16(PALSCALE_1ST(0.83 + 3.0));
	kjump_str_table[6] = INTTOFIX16(PALSCALE_1ST(0.83 + 3.6));
	kjump_str_table[7] = INTTOFIX16(PALSCALE_1ST(0.83 + 4.2));

	kjump_delay = PALSCALE_DURATION(24);
	kjump_delay_angry = PALSCALE_DURATION(6.3);
	kdx = INTTOFIX16(PALSCALE_1ST(1));
	kcube_bounce_dy = INTTOFIX16(PALSCALE_1ST(-1.8333334));
	kanim_speed = PALSCALE_DURATION(11);
	kceiling_dy = INTTOFIX16(PALSCALE_1ST(2.0));

	constants_set = 1;
}

// VRAM.

static uint16_t vram_pos;

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g = gfx_get(GFX_BOINGO);
	vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Main.

static inline void render(O_Boingo *b)
{
	Obj *o = &b->head;
	int16_t sp_x, sp_y;

	if (b->jumping)
	{
		if (b->boingo_type == BOINGO_TYPE_CUBE_ACTIVE)
		{
			// The cube.
			obj_render_setup(o, &sp_x, &sp_y, -8, -18,
			                 map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(vram_pos + 56, 0, 0,
			                             LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));

			// The legs.
			obj_render_setup(o, &sp_x, &sp_y, -8, -2,
			                 map_get_x_scroll(), map_get_y_scroll());
			const uint16_t tile = vram_pos + (b->anim_frame ? 30 : 28);
			spr_put(sp_x, sp_y, SPR_ATTR(tile, 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 1));
		}
		else if (b->boingo_type == BOINGO_TYPE_CUBE)
		{
			obj_render_setup(o, &sp_x, &sp_y, -8, -15,
			                 map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(vram_pos + 56, 0, 0,
			                             LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));
		}
		else
		{
			const uint16_t tile = vram_pos + 12 + (b->anim_frame ? 6 : 0) +
			                      (b->boingo_type == BOINGO_TYPE_ANGRY ? 32 : 0);
			obj_render_setup(o, &sp_x, &sp_y, -8, -18,
			                 map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(tile, 0, 0,
			                    ENEMY_PAL_LINE, 0), SPR_SIZE(2, 3));
		}
	}
	else
	{
		if (b->boingo_type == BOINGO_TYPE_CUBE_ACTIVE)
		{
			// Cube
			obj_render_setup(o, &sp_x, &sp_y, -8, -18,
			                 map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y + b->anim_frame, SPR_ATTR(vram_pos + 56, 0, 0,
			                             LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));
			// The legs.
			obj_render_setup(o, &sp_x, &sp_y, -8, -2,
			                 map_get_x_scroll(), map_get_y_scroll());
			const uint16_t tile = vram_pos + (b->anim_frame ? 26 : 24);
			spr_put(sp_x, sp_y, SPR_ATTR(tile, 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 1));

		}
		else if (b->boingo_type == BOINGO_TYPE_CUBE)
		{
			obj_render_setup(o, &sp_x, &sp_y, -8, -15,
			                 map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(vram_pos + 56, 0, 0,
			                             LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));
		}
		else
		{
			const uint16_t tile = vram_pos + (b->anim_frame ? 6 : 0) +
			                      (b->boingo_type == BOINGO_TYPE_ANGRY ? 32 : 0);
			obj_render_setup(o, &sp_x, &sp_y, -12, -13,
			                 map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(tile, 0, 0,
			                    ENEMY_PAL_LINE, 0), SPR_SIZE(3, 2));
		}
	}
}

static inline void bg_collisions(O_Boingo *b)
{
	Obj *o = &b->head;
	const int16_t x_center = FIX32TOINT(b->head.x);
	const int16_t left = FIX32TOINT(b->head.x) - 7;
	const int16_t right = FIX32TOINT(b->head.x) + 7;
	const int16_t top = FIX32TOINT(b->head.y) - 23;
	const int16_t mid = FIX32TOINT(b->head.y) - 11;
	const int16_t bottom = FIX32TOINT(b->head.y) + 1;
	
	if (o->dy < 0 && map_collision(x_center, top))
	{
		o->dy = kceiling_dy;
	}
	else if (o->dy > 0 && map_collision(x_center, bottom))
	{
		o->dy = 0;
		o->y = INTTOFIX32(8 * ((bottom) / 8) - 1);
		b->jumping = 0;
		return;
	}

	if (o->dx < 0 && map_collision(left, mid))
	{
		o->direction = OBJ_DIRECTION_RIGHT;
		o->dx = kdx;
		o->x += INTTOFIX32(4);
	}
	else if (o->dx > 0 && map_collision(right, mid))
	{
		o->direction = OBJ_DIRECTION_LEFT;
		o->dx = -kdx;
		o->x -= INTTOFIX32(4);
	}
}

static void main_func(Obj *o)
{
	O_Boingo *b = (O_Boingo *)o;
	const O_Lyle *l = lyle_get();

	if (o->hurt_stun > 0)
	{
		render(b);
		return;
	}

	if (b->transition_to_normal)
	{
		b->boingo_type = BOINGO_TYPE_NORMAL;
		b->transition_to_normal = 0;
		particle_manager_spawn(o->x, o->y, PARTICLE_TYPE_FIZZLERED);
		particle_manager_spawn(o->x + o->right, o->y, PARTICLE_TYPE_FIZZLERED);
		particle_manager_spawn(o->x + o->left, o->y, PARTICLE_TYPE_FIZZLERED);
		particle_manager_spawn(o->x, o->y + o->top, PARTICLE_TYPE_FIZZLERED);
		particle_manager_spawn(o->x + o->right, o->y + o->top, PARTICLE_TYPE_FIZZLERED);
		particle_manager_spawn(o->x + o->left, o->y + o->top, PARTICLE_TYPE_FIZZLERED);
		sfx_play(SFX_OBJ_BURST, 3);
	}

	if (b->boingo_type == BOINGO_TYPE_CUBE)
	{
		// Activate cube-boingo if the player is active.
		if (o->x < l->head.x + BOINGO_CUBE_ACTIVATION_DISTANCE &&
		    o->x > l->head.x - BOINGO_CUBE_ACTIVATION_DISTANCE)
		{
			b->boingo_type = BOINGO_TYPE_CUBE_ACTIVE;
		}
	}
	else
	{
		if (b->jumping)
		{
			obj_standard_physics(o);
			o->dy += kgravity;
			bg_collisions(b);
		}
		else
		{
			if (b->jump_cnt >= (b->boingo_type == BOINGO_TYPE_ANGRY ?
			                   kjump_delay_angry : kjump_delay))
			{
				// Do a jump.
				if (l->head.x < o->x)
				{
					o->direction = OBJ_DIRECTION_LEFT;
					o->dx = -kdx;
				}
				else
				{
					o->direction = OBJ_DIRECTION_RIGHT;
					o->dx = kdx;
				}

				o->dy = -kjump_str_table[system_rand() % ARRAYSIZE(kjump_str_table)];
				b->jumping = 1;
				b->jump_cnt = 0;
			}
			else
			{
				b->jump_cnt++;
			}
		}
	}

	// Animation.
	if (b->anim_cnt >= ((b->jumping) ? 2 : kanim_speed))
	{
		b->anim_cnt = 0;
		b->anim_frame++;
		if (b->anim_frame > 1) b->anim_frame = 0;
	}
	else
	{
		b->anim_cnt++;
	}

	if (o->hurt_stun > 0)
	{
		render(b);
		return;
	}

	render(b);
}

static void cube_func(Obj *o, Cube *c)
{
	O_Boingo *b = (O_Boingo *)o;

	if (b->boingo_type == BOINGO_TYPE_CUBE)
	{
		if (c->status == CUBE_STATUS_AIR)
		{
			// Just setting some random high dx to give the cube some direction
			// since the clamp will put it within a reasonable range.
			if (c->dx == 0) c->dx = INTTOFIX32(system_rand() % 2 ? 5 : -5);
			cube_clamp_dx(c);
			c->dy = kcube_bounce_dy;
		}
		sfx_play(SFX_CUBE_BOUNCE, 15);
		return;
	}
	else if (b->boingo_type == BOINGO_TYPE_CUBE_ACTIVE)
	{
		b->transition_to_normal = 1;
		o->cube_func = NULL;
		obj_standard_cube_response(o, c);
		o->hp = 1;
	}
	else
	{
		obj_standard_cube_response(o, c);
	}
}

// Boilerplate.

void o_load_boingo(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Boingo) <= sizeof(ObjSlot));
	O_Boingo *b = (O_Boingo *)o;
	vram_load();
	set_constants();

	obj_basic_init(o, OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-9), INTTOFIX16(9), INTTOFIX16(-15), 1);
	o->main_func = main_func;
	b->boingo_type = (BoingoType)data;
	if (b->boingo_type == BOINGO_TYPE_CUBE)
	{
		o->cube_func = cube_func;
		o->hp = 3;
	}

	o->x -= INTTOFIX32(1);


}

void o_unload_boingo(void)
{
	vram_pos = 0;
}


