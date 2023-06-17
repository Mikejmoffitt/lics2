#include "obj/cp_giver.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"

#include "lyle.h"
#include "pause.h"
#include "sfx.h"
#include "cube_manager.h"

static uint16_t s_vram_pos;

static int16_t kanim_speed;  // 50 in MMF2 terms
static int16_t ktake_anim_speed;  // 30 in MMF2 terms

static int16_t ktake_anim_start_time;  // when taking anim begins and first sound is played
static int16_t ktake_orb_rise_time;  // when orb comes out of lyle and sound plays again
static fix16_t korb_ddy;
static int16_t korb_solid_time;

static int16_t kgive_anim_start_time;  // When CP orb is emitted.
static int16_t kgive_anim_stop_time;

static fix16_t klyle_center_dx;

static fix16_t kpowerup_ddy;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_CP_GIVER);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;

	kanim_speed = PALSCALE_DURATION(6);
	ktake_anim_speed = PALSCALE_DURATION(9);

	ktake_anim_start_time = PALSCALE_DURATION(30);
	ktake_orb_rise_time = PALSCALE_DURATION(42);
	korb_ddy = INTTOFIX16(PALSCALE_2ND((-1.0 / 8.0) * (5.0 / 6.0) * (5.0 / 6.0)));
	korb_solid_time = PALSCALE_DURATION(24);

	kgive_anim_start_time = PALSCALE_DURATION(24);
	kgive_anim_stop_time = PALSCALE_DURATION(36);

	kpowerup_ddy = INTTOFIX16(PALSCALE_2ND(0.16666666667));

	klyle_center_dx = INTTOFIX16(PALSCALE_1ST(0.83333333334));

	s_constants_set = true;
}

typedef struct CpGiverFrame
{
	int16_t tile;
	int16_t xoff;
	int16_t yoff;
} CpGiverFrame;

static const CpGiverFrame giver_frames[] =
{
	{4, 0, 0},
	{4, 1, 0},
	{4, -1, 0},
	// Take anim
	{8, 0, -2},
	{8, 0, -2},
	{8, 0, 1},
	{8, 0, -1},
	// Give anim
	{4, 0, -2},
	{4, 0, -3},
	{4, 0, 1},
	{4, 0, -1},
};

static void render(O_CpGiver *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -16;

	const int16_t yscroll = map_get_y_scroll();

	obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
	                        map_get_x_scroll(), yscroll);
	md_spr_put(sp_x + giver_frames[e->metaframe].xoff,
	        sp_y + giver_frames[e->metaframe].yoff,
	        SPR_ATTR(s_vram_pos + giver_frames[e->metaframe].tile, 0, 0, ENEMY_PAL_LINE, 0),
	        SPR_SIZE(2, 2));

	if (e->orb_y >= o->y)
	{
		if (e->orb_flicker_cnt >= korb_solid_time || !e->orb_anim_frame)
		{
			const uint16_t frame_offs = (e->orb_anim_frame) ? 4 : 0;
			const uint8_t size = SPR_SIZE(2, 2);
			const int16_t ty = FIX32TOINT(e->orb_y) + offset_y - yscroll;
			// Second overlay
			md_spr_put(sp_x + 1, ty, SPR_ATTR(POWERUP_VRAM_POSITION/32 + 48 + frame_offs, 0, 0, BG_PAL_LINE, 0), size);
			md_spr_put(sp_x + 1, ty, SPR_ATTR(POWERUP_VRAM_POSITION/32 + 8 + frame_offs, 0, 0, ENEMY_PAL_LINE, 0), size);
		}
	}

	if (e->powerup_y >= o->y)
	{
		md_spr_put(sp_x + 1,
		        FIX32TOINT(e->powerup_y) + offset_y - yscroll,
		        SPR_ATTR(s_vram_pos + 20, 0, 0, ENEMY_PAL_LINE, 0),
		        SPR_SIZE(2, 2));
	}
}

static void main_func(Obj *o)
{
	O_CpGiver *e = (O_CpGiver *)o;
	O_Lyle *l = lyle_get();
	ProgressSlot *prog = progress_get();

	const CpGiverState state_prev = e->state;

	static const fix32_t kactivation_margin = INTTOFIX32(24);

	switch (e->state)
	{
		case CP_GIVER_STATE_IDLE:
			if (e->state_elapsed == 0)
			{
				e->metaframe = 0;
				lyle_set_control_en(1);
			}

			// If there are orbs to deposit and lyle is under the giver, begin.
			if (prog->collected_cp_orbs > prog->registered_cp_orbs &&
			    l->head.x > o->x - kactivation_margin &&
			    l->head.x < o->x + kactivation_margin &&
			    l->grounded)
			{
				e->state = CP_GIVER_STATE_PRETAKE;
				l->head.dx = 0;
				l->head.dy = 0;
				lyle_set_control_en(0);
				l->anim_frame = 0;
				if (l->holding_cube != CUBE_TYPE_NULL)
				{
					// Destroy the cube that Lyle is holding.
					Cube *c = cube_manager_spawn(l->head.x, l->head.y - INTTOFIX32(32),
					                             l->holding_cube, CUBE_STATUS_AIR,
					                             0, 0);
					if (c->type != CUBE_TYPE_GREEN)
					{
						cube_destroy(c);
					}
					l->holding_cube = CUBE_TYPE_NULL;
				}
			}
			break;

		case CP_GIVER_STATE_PRETAKE:
			if (e->state_elapsed == 0)
			{
				e->metaframe = 0;
				e->orb_y = INTTOFIX32(-1024);
				e->powerup_y = e->orb_y;
				e->powerup_dy = 0;
				e->powerup_ability = 0;
			}
			if (l->head.x > o->x + klyle_center_dx) l->head.x -= klyle_center_dx;
			else if (l->head.x < o->x - klyle_center_dx) l->head.x += klyle_center_dx;
			if (e->state_elapsed >= ktake_orb_rise_time) e->state = CP_GIVER_STATE_TAKING;
			break;

		case CP_GIVER_STATE_TAKING:
			if (e->state_elapsed == 0)
			{
				sfx_play(SFX_GIVER, 5);
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_speed);
			e->metaframe = 1 + e->anim_frame;

			if (e->state_elapsed == ktake_orb_rise_time)
			{
				e->orb_y = l->head.y - INTTOFIX32(12);
				e->orb_dy = 0;
				e->orb_flicker_cnt = 0;
				e->orb_anim_frame = 0;
				sfx_play(SFX_GIVER, 1);
			}
			else if (e->state_elapsed > ktake_orb_rise_time)
			{
				e->orb_dy += korb_ddy;
				e->orb_y += e->orb_dy;
				e->orb_flicker_cnt++;
				e->orb_anim_frame = (e->orb_anim_frame) ? 0 : 1;

				if (e->orb_y <= o->y)
				{
					e->state = CP_GIVER_STATE_TOOK;
					prog->registered_cp_orbs++;
					e->orb_y = INTTOFIX32(-1024);
					sfx_play(SFX_CUBE_LIFT, 0);
				}
			}
			break;

		case CP_GIVER_STATE_TOOK:
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 5, ktake_anim_speed);
			if (e->anim_frame >= 4)
			{
				// Hack because OBJ_SIMPLE_ANIM always loops.
				e->anim_frame = 3;
			}
			e->metaframe = 3 + e->anim_frame;
			if (e->state_elapsed >= kgive_anim_start_time)
			{
				static const struct PhantomReward
				{
					uint16_t level;
					ProgressAbility mask;
				} rewards[] =
				{
					{3, ABILITY_2X_DAMAGE_PHANTOM},
					{7, ABILITY_FAST_PHANTOM},
					{10, ABILITY_CHEAP_PHANTOM}
				};

				// If special thresholds were passed, prepare to give the CP orb.
				bool reward_given = false;
				for (uint16_t i = 0; i < ARRAYSIZE(rewards); i++)
				{
					if (prog->registered_cp_orbs >= rewards[i].level &&
					    !(prog->abilities & rewards[i].mask))
					{
						e->powerup_ability = rewards[i].mask;
						e->state = CP_GIVER_STATE_GIVING;
						reward_given = true;
						break;
					}
				}
				if (reward_given) break;
			}
			if (e->state_elapsed >= ktake_orb_rise_time)
			{
				e->state = (prog->registered_cp_orbs < prog->collected_cp_orbs) ?
				           CP_GIVER_STATE_TAKING : CP_GIVER_STATE_IDLE;
			}
			break;

		case CP_GIVER_STATE_GIVING:
			if (e->state_elapsed == 0)
			{
				e->powerup_y = o->y;
				e->powerup_dy = 0;
				sfx_play(SFX_MAGIBEAR_SHOT, 0);
			}

			e->powerup_dy += kpowerup_ddy;
			e->powerup_y += physics_trunc_fix16(e->powerup_dy);

			if (e->powerup_y >= l->head.y - INTTOFIX32(12))
			{
				e->powerup_y = INTTOFIX32(-1024);
				if (e->powerup_ability & ABILITY_FAST_PHANTOM)
				{
					pause_set_screen(PAUSE_SCREEN_GET_PHANTOM_HALF_TIME);
				}
				else if (e->powerup_ability & ABILITY_CHEAP_PHANTOM)
				{
					pause_set_screen(PAUSE_SCREEN_GET_PHANTOM_CHEAP);
				}
				else if (e->powerup_ability & ABILITY_2X_DAMAGE_PHANTOM)
				{
					pause_set_screen(PAUSE_SCREEN_GET_PHANTOM_DOUBLE_DAMAGE);
				}
				prog->abilities |= e->powerup_ability;
				e->state = (prog->registered_cp_orbs < prog->collected_cp_orbs) ?
				           CP_GIVER_STATE_TAKING : CP_GIVER_STATE_IDLE;
				break;
			}

			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kanim_speed);
			if (e->anim_frame >= 3)
			{
				// Hack because OBJ_SIMPLE_ANIM always loops.
				e->anim_frame = 2;
			}
			e->metaframe = 7 + e->anim_frame;

/*
			if (e->state_elapsed >= kgive_anim_stop_time && e->powerup_y >= l->head.y - INTTOFIX32(12))
			{
				e->state = (prog->registered_cp_orbs < prog->collected_cp_orbs) ?
				           CP_GIVER_STATE_TAKING : CP_GIVER_STATE_IDLE;
			}
			*/
			break;
	}

	if (e->state != state_prev)
	{
		e->state_elapsed = 0;
		e->anim_cnt = 0;
		e->anim_frame = 0;
	}
	else
	{
		e->state_elapsed++;
	}

	render(e);
}

void o_load_cp_giver(Obj *o, uint16_t data)
{
	O_CpGiver *e = (O_CpGiver *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "CP Giver", OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;

	e->orb_y = INTTOFIX32(-1024);
	e->powerup_y = INTTOFIX32(-1024);
}

void o_unload_cp_giver(void)
{
	s_vram_pos = 0;
}
