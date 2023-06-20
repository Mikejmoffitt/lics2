#include "obj/vyle1.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"

#include "lyle.h"
#include "projectile.h"
#include "game.h"
#include "obj/rockman_door.h"
#include "sfx.h"
#include "music.h"

static fix16_t kintro_dx;
static int16_t kintro_anim_speed;
static int16_t kintro_rockman_door_activate_time;
static int16_t kintro_walk_stop_time;
static int16_t kintro_reach_frame_time;
static int16_t kintro_toss_time;
static int16_t kintro_seek_start_time;
static int16_t kintro_found_time;
static int16_t kintro_finished_time;

static fix16_t kdx;
static fix16_t kjump_dy[8];
static fix16_t kgravity;

static int16_t kjump_cnt_initial;
static int16_t kjump_time[8];
static int16_t kdirection_flip_time[8];
static int16_t kpre_shot_time[8];

static int16_t kshot_anim_speed;
static fix16_t kshot_speed;

static int16_t kwalk_anim_speed;

static fix16_t krecoil_dx;
static fix16_t krecoil_dy;
static fix16_t krecoil_gravity;

static int16_t kreatreat_flashing_stop_time;
static int16_t kretreat_rockman_door_activate_time;

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_VYLE1);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;

	kintro_dx = INTTOFIX16(PALSCALE_1ST(-0.2777777773));
	kintro_anim_speed = PALSCALE_DURATION(14);  // Cribbed from title.c
	kintro_rockman_door_activate_time = PALSCALE_DURATION(216);
	kintro_walk_stop_time = PALSCALE_DURATION(420);
	kintro_reach_frame_time = PALSCALE_DURATION(444);
	kintro_toss_time = PALSCALE_DURATION(480);
	kintro_seek_start_time = PALSCALE_DURATION(540);
	kintro_found_time = PALSCALE_DURATION(672);
	kintro_finished_time = PALSCALE_DURATION(696);

	kdx = INTTOFIX16(PALSCALE_1ST(0.833333333333));
	kgravity = INTTOFIX16(PALSCALE_2ND(0.138888888));

	// The jump counter is biased a tiny bit at first to simulate the original
	// MMF behavior.
	kjump_cnt_initial = PALSCALE_DURATION(125);

	const float jump_dy_base = -4.1666666667;
	const int16_t jump_base = 198;
	const float jump_spread = 12.0f / (ARRAYSIZE(kjump_time) - 1);
	const int16_t direction_flip_base = 240;
	const float jump_dy_spread = -1.6666666667 / (ARRAYSIZE(kjump_dy) - 1);
	const float direction_flip_spread = 24.0f / (ARRAYSIZE(kdirection_flip_time) - 1);
	const int16_t pre_shot_base = 42;
	const float pre_shot_spread = 18.0f / (ARRAYSIZE(kpre_shot_time) - 1);
	for (uint16_t i = 0; i < ARRAYSIZE(kjump_time); i++)
	{
		kjump_dy[i] = INTTOFIX16(PALSCALE_1ST(jump_dy_base + (i * jump_dy_spread)));
		kjump_time[i] = PALSCALE_DURATION(jump_base + (i * jump_spread));
		kdirection_flip_time[i] = PALSCALE_DURATION(direction_flip_base + (i * direction_flip_spread));
		kpre_shot_time[i] = PALSCALE_DURATION(pre_shot_base + (i * pre_shot_spread));
	}

	kshot_anim_speed = PALSCALE_DURATION(5);  // TODO: refine
	kshot_speed = INTTOFIX16(PALSCALE_1ST(3));

	kwalk_anim_speed = PALSCALE_DURATION(5);

	krecoil_dx = kdx;
	krecoil_dy = INTTOFIX16(PALSCALE_1ST(-3.57142847));
	krecoil_gravity = INTTOFIX16(PALSCALE_2ND(0.099206349206));

	kreatreat_flashing_stop_time = PALSCALE_DURATION(60);
	kretreat_rockman_door_activate_time = PALSCALE_DURATION(120);

	s_constants_set = true;
}

static void render(O_Vyle1 *e)
{
	if (e->metaframe == -1) return;
	Obj *o = &e->head;
	int16_t sp_x, sp_y;

	obj_render_setup(o, &sp_x, &sp_y, -12, -24,
	                 map_get_x_scroll(), map_get_y_scroll());

	typedef struct SprDef
	{
		int16_t x;
		int16_t y;
		uint16_t attr;  // Minus base VRAM pos
		int16_t size;
	} SprDef;

	static const int16_t pal = ENEMY_PAL_LINE;
	static const SprDef frames[] =
	{
		{4, 0, SPR_ATTR(0x00, 0, 0, pal, 0), SPR_SIZE(2, 3)},  // 00 cloak walk 1
		{0, 0, SPR_ATTR(0x06, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 01 cloak walk 2
		{4, 0, SPR_ATTR(0x0F, 0, 0, pal, 0), SPR_SIZE(2, 3)},  // 02 cloak reach
		{0, 0, SPR_ATTR(0x15, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 03 tossing
		{0, 0, SPR_ATTR(0x1E, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 04 stand, no gun
		{0, 0, SPR_ATTR(0x27, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 05 search 1
		{0, 0, SPR_ATTR(0x30, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 06 search 2
		{0, 0, SPR_ATTR(0x39, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 07 gun found
		{0, 0, SPR_ATTR(0x42, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 08 aim up diag.
		{0, 0, SPR_ATTR(0x4B, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 09 stand w/ gun
		{0, 0, SPR_ATTR(0x54, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 10 walk 1
		{0, 0, SPR_ATTR(0x5D, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 11 walk 2
		{0, 0, SPR_ATTR(0x66, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 12 walk 3
		{0, 0, SPR_ATTR(0x6F, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 13 shoot 1
		{0, 0, SPR_ATTR(0x78, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 14 shoot 2
		{0, 0, SPR_ATTR(0x81, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 15 jump fwd
		{0, 0, SPR_ATTR(0x8A, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 16 jump aim down
		{0, 0, SPR_ATTR(0x93, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 17 shoot air 1
		{0, 0, SPR_ATTR(0x9C, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 18 shoot air 2
		{0, 0, SPR_ATTR(0xA5, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 19 recoil
		{0, 2, SPR_ATTR(0xAE, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 20 on ground
		{0, 0, SPR_ATTR(0x54, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 21 run 1
		{0, 0, SPR_ATTR(0x5D, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 22 run 2
		{0, 0, SPR_ATTR(0x66, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // 23 run 3
	};

	const SprDef *def = &frames[e->metaframe];
	const int16_t flipped = (o->direction == OBJ_DIRECTION_RIGHT);
	const uint16_t attr_flip = SPR_ATTR(0, flipped, 0, 0, 0);
	md_spr_put(sp_x + def->x, sp_y + def->y,
	        (s_vram_pos + (def->attr | attr_flip)), def->size);
}

static void main_func(Obj *o)
{
	O_Vyle1 *e = (O_Vyle1 *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	const Vyle1State state_prev = e->state;

	O_Lyle *l = lyle_get();

	switch (e->state)
	{
		case VYLE1_STATE_INIT:
			o->x = INTTOFIX32(GAME_SCREEN_W_PIXELS + 32);
			e->metaframe = 0;

			if (l->head.x >= INTTOFIX32(64) && l->grounded)
			{
				o->direction = OBJ_DIRECTION_LEFT;
				e->state = VYLE1_STATE_INTRO;
				l->head.direction = OBJ_DIRECTION_RIGHT;
				lyle_set_control_en(0);
				l->anim_frame = 0;
			}
			if (l->head.x >= INTTOFIX32(160))
			{
				l->head.x = INTTOFIX32(160);
			}
			break;

		case VYLE1_STATE_INTRO:
			if (e->state_elapsed == 0)
			{
				o->dx = kintro_dx;
			}

			if (e->state_elapsed < kintro_walk_stop_time)
			{
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame,
				                2, kintro_anim_speed);
				e->metaframe = e->anim_frame;
			}

			if (e->state_elapsed == kintro_rockman_door_activate_time)
			{
				rockman_door_set_closed(1);
			}
			else if (e->state_elapsed == kintro_walk_stop_time)
			{
				o->dx = 0;
				e->metaframe = 0;
			}
			else if (e->state_elapsed == kintro_reach_frame_time)
			{
				e->metaframe = 2;
			}
			else if (e->state_elapsed == kintro_toss_time)
			{
				sfx_play(SFX_SAND, 1);
				e->metaframe = 3;
				obj_spawn(FIX32TOINT(o->x) + 16, FIX32TOINT(o->y) - 12, OBJ_CLOAK, 0);
			}
			else if (e->state_elapsed == kintro_seek_start_time)
			{
				e->metaframe = 3;
				e->anim_cnt = 0;
				e->anim_frame = 0;
			}
			else if (e->state_elapsed > kintro_seek_start_time &&
			         e->state_elapsed < kintro_found_time)
			{
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 6, kintro_anim_speed);
				if (e->anim_frame > 4) e->anim_frame = 3;

				static const int16_t metaframes[] =
				{
					3, 4, 4, 5, 6,
				};
				e->metaframe = metaframes[e->anim_frame];
			}
			else if (e->state_elapsed == kintro_found_time)
			{
				e->metaframe = 7;
			}
			else if (e->state_elapsed == kintro_finished_time)
			{
				e->state = VYLE1_STATE_ACTIVE;
			}

			obj_mixed_physics_h(o);

			break;

		case VYLE1_STATE_ACTIVE:
			if (e->state_elapsed == 0)
			{
				lyle_set_control_en(1);
				music_play(10);
				o->flags = OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE;
			}

			o->direction = (l->head.x < o->x) ? OBJ_DIRECTION_LEFT : OBJ_DIRECTION_RIGHT;

			if (e->airborn)
			{
				if (e->shot_anim_active)
				{
					OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 5, kshot_anim_speed);
					if (e->anim_frame > 3)
					{
						e->anim_frame = 3;
						e->shot_anim_active = 0;
					}

					static const int16_t metaframes[] =
					{
						15, 17, 18, 18
					};
					e->metaframe = metaframes[e->anim_frame];
				}
				else
				{
					// If he's close to lyle, aim the gun downwards.
					const int16_t y_delta = FIX32TOINT((l->head.y > o->y) ?
					                        (l->head.y - o->y) :
					                        (o->y - l->head.y));
					const int16_t x_delta = FIX32TOINT((l->head.x > o->x) ?
					                        (l->head.x - o->x) :
					                        (o->x - l->head.x));
					e->metaframe = (x_delta < y_delta) ? 16 : 15;
				}

				o->dy += kgravity;
				obj_mixed_physics_h(o);

				if (o->dy > 0 && o->y >= e->original_y)
				{
					o->dy = 0;
					o->y = e->original_y;
					e->airborn = 0;
				}
			}
			else
			{
				if (e->shot_anim_active)
				{
					OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 5, kshot_anim_speed);
					if (e->anim_frame > 3)
					{
						e->anim_frame = 3;
						e->shot_anim_active = 0;
					}

					static const int16_t metaframes[] =
					{
						9, 13, 14, 14
					};
					e->metaframe = metaframes[e->anim_frame];
				}
				else
				{
					OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kwalk_anim_speed);

					static const int16_t metaframes[] =
					{
						10, 11, 12, 11
					};
					e->metaframe = metaframes[e->anim_frame];
					obj_mixed_physics_h(o);
				}

				// "Direction" refers to his movement, not which way he faces.
				if (e->direction_flip_cnt > 0)
				{
					e->direction_flip_cnt--;
				}
				else
				{
					if (o->dx > 0) o->dx = -kdx;
					else o->dx = kdx;
					e->direction_flip_cnt = kdirection_flip_time[system_rand() % ARRAYSIZE(kdirection_flip_time)];
				}
			}

			// Horizontal boundaries
			if (o->dx > 0 && o->x + o->right >= INTTOFIX32(GAME_SCREEN_W_PIXELS - 16))
			{
				o->dx = -kdx;
			}
			else if (o->dx < 0 && o->x + o->left <= INTTOFIX32(16))
			{
				o->dx = kdx;
			}

			if (e->jump_cnt > 0)
			{
				e->jump_cnt--;
			}
			else
			{
				e->jump_cnt = kjump_time[system_rand() % ARRAYSIZE(kjump_time)];
				if (!e->airborn)
				{
					o->dy = kjump_dy[system_rand() % ARRAYSIZE(kjump_dy)];
					sfx_play(SFX_JUMP, 17);
					e->airborn = 1;
				}
			}

			if (e->pre_shot_cnt > 0)
			{
				e->pre_shot_cnt--;
			}
			else
			{
				e->pre_shot_cnt = kpre_shot_time[system_rand() % ARRAYSIZE(kpre_shot_time)];
				e->shot_anim_active = 1;
				e->anim_frame = 0;
				e->anim_cnt = 0;
				projectile_shoot_at(o->x, o->y - INTTOFIX32(9), PROJECTILE_TYPE_BALL2,
				                            l->head.x, l->head.y - INTTOFIX32(10), kshot_speed);
			}

			if (o->hp >= 127)
			{
				sfx_play(SFX_OBJ_BURST, 3);
				sfx_play(SFX_OBJ_BURST_HI, 3);
				music_stop();
				e->state = VYLE1_STATE_RECOIL;
				o->dx = (o->direction == OBJ_DIRECTION_LEFT) ? krecoil_dx : -krecoil_dx;
				o->dy = krecoil_dy;
			}

			break;

		case VYLE1_STATE_RECOIL:
			o->flags = 0;
			o->dy += krecoil_gravity;
			e->metaframe = (e->state_elapsed % 2) ? 19 : -1;

			if (o->dy > 0 && o->y >= e->original_y)
			{
				o->dx = 0;
				o->dy = 0;
				o->y = e->original_y;
				e->state = VYLE1_STATE_RETREAT;
			}

			// Horizontal boundaries
			if (o->dx > 0 && o->x + o->right >= INTTOFIX32(GAME_SCREEN_W_PIXELS - 16))
			{
				o->dx = 0;
				o->direction = OBJ_DIRECTION_LEFT;
			}
			else if (o->dx < 0 && o->x + o->left <= INTTOFIX32(16))
			{
				o->dx = 0;
				o->direction = OBJ_DIRECTION_RIGHT;
			}

			obj_mixed_physics_h(o);
			break;

		case VYLE1_STATE_RETREAT:
			if (e->state_elapsed < kreatreat_flashing_stop_time)
			{
				e->metaframe = (e->state_elapsed % 2) ? 20 : -1;
			}
			else if (e->state_elapsed == kreatreat_flashing_stop_time)
			{
				e->metaframe = 20;
			}

			if (e->state_elapsed == kretreat_rockman_door_activate_time)
			{
				rockman_door_set_single_closed(1, 0);
			}
			else if (e->state_elapsed > kretreat_rockman_door_activate_time)
			{
				o->direction = OBJ_DIRECTION_RIGHT;
				o->dx = kdx * 2;
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kwalk_anim_speed);
				static const int16_t metaframes[] =
				{
					21, 22, 23, 22
				};
				e->metaframe = metaframes[e->anim_frame];
			}

			if (o->x >= INTTOFIX32(GAME_SCREEN_W_PIXELS) + o->right)
			{
				obj_erase(o);
			}

			obj_mixed_physics_h(o);

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

void o_load_vyle1(Obj *o, uint16_t data)
{
	O_Vyle1 *e = (O_Vyle1 *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Vyle 1", OBJ_FLAG_HARMFUL | OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-12), INTTOFIX16(12), INTTOFIX16(-24), 20);
	o->main_func = main_func;
	o->cube_func = cube_func;

	e->original_y = o->y;
}

void o_unload_vyle1(void)
{
	s_vram_pos = 0;
}
