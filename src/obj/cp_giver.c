#include "obj/cp_giver.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"
#include "obj/lyle.h"
#include "obj/pause.h"
#include "sfx.h"

static uint16_t s_vram_pos;

static int16_t kanim_speed;  // 50 in MMF2 terms
static int16_t ktake_anim_speed;  // 30 in MMF2 terms

static int16_t ktake_anim_start_time;  // when taking anim begins and first sound is played
static int16_t ktake_orb_rise_time;  // when orb comes out of lyle and sound plays again
static fix16_t korb_ddy;
static int16_t korb_solid_time;
static int16_t korb_anim_speed;

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
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kanim_speed = PALSCALE_DURATION(6);
	ktake_anim_speed = PALSCALE_DURATION(9);

	ktake_anim_start_time = PALSCALE_DURATION(30);
	ktake_orb_rise_time = PALSCALE_DURATION(42);
	korb_ddy = INTTOFIX16(PALSCALE_1ST(-0.1041666666667));
	korb_solid_time = ktake_orb_rise_time + PALSCALE_DURATION(24);
	korb_anim_speed = PALSCALE_DURATION(6);  // TODO: Totally made up, compare to MMF2 one.

	kgive_anim_start_time = PALSCALE_DURATION(24);
	kgive_anim_stop_time = PALSCALE_DURATION(36);

	kpowerup_ddy = INTTOFIX16(PALSCALE_1ST(0.16666666667));

	klyle_center_dx = INTTOFIX16(PALSCALE_1ST(0.83333333334));

	s_constants_set = 1;
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

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), yscroll);
	spr_put(sp_x + giver_frames[e->metaframe].xoff,
	        sp_y + giver_frames[e->metaframe].yoff,
	        SPR_ATTR(s_vram_pos + giver_frames[e->metaframe].tile, 0, 0, ENEMY_PAL_LINE, 0),
	        SPR_SIZE(2, 2));

	if (e->orb_y >= o->y)
	{
		spr_put(sp_x,
		        FIX32TOINT(e->orb_y) + offset_y - yscroll,
		        SPR_ATTR(s_vram_pos + 12 + (4 * e->orb_anim_frame), 0, 0, ENEMY_PAL_LINE, 0),
		        SPR_SIZE(2, 2));
	}

	if (e->powerup_y >= o->y)
	{
		spr_put(sp_x,
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
				sfx_play(SFX_GIVER_1, 0);
				sfx_play(SFX_GIVER_2, 1);
				sfx_play(SFX_GIVER_3, 1);
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_speed);
			e->metaframe = 1 + e->anim_frame;

			if (e->state_elapsed == ktake_orb_rise_time)
			{
				e->orb_y = l->head.y - INTTOFIX32(12);
				e->orb_dy = 0;
				e->orb_flicker_cnt = 0;
				e->orb_anim_cnt = 0;
				e->orb_anim_frame = 0;
				sfx_play(SFX_GIVER_1, 0);
				sfx_play(SFX_GIVER_2, 1);
				sfx_play(SFX_GIVER_3, 2);
			}
			else if (e->state_elapsed > ktake_orb_rise_time)
			{
				e->orb_dy += korb_ddy;
				e->orb_y += e->orb_dy;
				e->orb_flicker_cnt++;
				OBJ_SIMPLE_ANIM(e->orb_anim_cnt, e->orb_anim_frame, 2, korb_anim_speed);

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
				// If special thresholds were passed, prepare to give the CP orb.
				if (prog->registered_cp_orbs >= 3 && !(prog->abilities & ABILITY_2X_DAMAGE_PHANTOM))
				{
					e->powerup_ability = ABILITY_2X_DAMAGE_PHANTOM;
					e->state = CP_GIVER_STATE_GIVING;
					break;
				}
				if (prog->registered_cp_orbs >= 7 && !(prog->abilities & ABILITY_FAST_PHANTOM))
				{
					e->powerup_ability = ABILITY_FAST_PHANTOM;
					e->state = CP_GIVER_STATE_GIVING;
					break;
				}
				else if (prog->registered_cp_orbs >= 10 && !(prog->abilities & ABILITY_CHEAP_PHANTOM))
				{
					e->powerup_ability = ABILITY_CHEAP_PHANTOM;
					e->state = CP_GIVER_STATE_GIVING;
					break;
				}
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
			e->powerup_y += e->powerup_dy;

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
	SYSTEM_ASSERT(sizeof(O_CpGiver) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;

	O_CpGiver *e = (O_CpGiver *)o;
	e->orb_y = INTTOFIX32(-1024);
	e->powerup_y = INTTOFIX32(-1024);
}

void o_unload_cp_giver(void)
{
	s_vram_pos = 0;
}
