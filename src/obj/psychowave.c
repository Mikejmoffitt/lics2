#include "obj/psychowave.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "particle.h"
#include "map.h"
#include "sfx.h"

enum
{
	PW_SPR_BUTTON,
	PW_SPR_MARQUEE_0,
	PW_SPR_MARQUEE_1,
	PW_SPR_SCREENBACK_0,
	PW_SPR_SCREENBACK_1,
	PW_SPR_CATFACE,
	PW_SPR_LIGHTR_0,
	PW_SPR_LIGHTR_1,
	PW_SPR_LIGHTG,
	PW_SPR_ARM_0,
	PW_SPR_ARM_1,
	PW_SPR_ARM_2,
	PW_SPR_ARM_3,
	PW_SPR_GRID,
	PW_SPR_GLASSBALL1,
	PW_SPR_GLASSBALL2,
	PW_SPR_GLASSBALL3,
	PW_SPR_GLASSBALL4,
	PW_SPR_COUNT
};

static SprParam s_spr[PW_SPR_COUNT];

static O_Psychowave *s_pwave;

static uint16_t s_vram_pos;

static int kanim_speed;
static int kwave_emit_delay;

#define PWAVE_OFFS_BUTTON 0
#define PWAVE_OFFS_MARQUEE (PWAVE_OFFS_BUTTON+2)
#define PWAVE_OFFS_SCREENBACK (PWAVE_OFFS_MARQUEE+(6*2))
#define PWAVE_OFFS_CATFACE (PWAVE_OFFS_SCREENBACK+(4*3))
#define PWAVE_OFFS_LIGHTR (PWAVE_OFFS_CATFACE+(2*2))
#define PWAVE_OFFS_LIGHTG (PWAVE_OFFS_LIGHTR+(3*1))
#define PWAVE_OFFS_ARM (PWAVE_OFFS_LIGHTG+(1*3))
#define PWAVE_OFFS_GLASSBALL (PWAVE_OFFS_ARM+(17))

static void vram_load(void)
{
	if (s_vram_pos) return;
	const Gfx *g;

	g = gfx_get(GFX_EX_PWAVE_BUTTON);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
	g = gfx_get(GFX_EX_PWAVE_MARQUEE);
	gfx_load(g, obj_vram_alloc(g->size));
	g = gfx_get(GFX_EX_PWAVE_SCREENBACK);
	gfx_load(g, obj_vram_alloc(g->size));
	g = gfx_get(GFX_EX_PWAVE_CATFACE);
	gfx_load(g, obj_vram_alloc(g->size));
	g = gfx_get(GFX_EX_PWAVE_LIGHTR);
	gfx_load(g, obj_vram_alloc(g->size));
	g = gfx_get(GFX_EX_PWAVE_LIGHTG);
	gfx_load(g, obj_vram_alloc(g->size));
	g = gfx_get(GFX_EX_PWAVE_ARM);
	gfx_load(g, obj_vram_alloc(g->size));
	g = gfx_get(GFX_EX_PWAVE_GLASSBALL);
	gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kanim_speed = PALSCALE_DURATION(8);
	kwave_emit_delay = PALSCALE_DURATION(10 * 6.0 / 5.0);

	s_constants_set = true;
}

static void render(O_Psychowave *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	obj_render_setup(o, &sp_x, &sp_y, 0, 0,
	                 map_get_x_scroll(), map_get_y_scroll());

	for (uint16_t i = 0; i < ARRAYSIZE(s_spr); i++)
	{
		if (s_spr[i].size & 0x80) continue;
		const int16_t x_store = s_spr[i].x;
		const int16_t y_store = s_spr[i].y;
		s_spr[i].x += sp_x;
		s_spr[i].y += sp_y;
		if (s_spr[i].x > -32 && s_spr[i].x < 320) md_spr_put_st(&s_spr[i]);
		s_spr[i].x = x_store;
		s_spr[i].y = y_store;
	}
}

static void main_func(Obj *o)
{
	O_Psychowave *e = (O_Psychowave *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	switch (e->state)
	{
		case PWAVE_STATE_OFF:
			s_spr[PW_SPR_MARQUEE_0].size |= 0x80;
			s_spr[PW_SPR_MARQUEE_1].size |= 0x80;
			s_spr[PW_SPR_SCREENBACK_0].size |= 0x80;
			s_spr[PW_SPR_SCREENBACK_1].size |= 0x80;
			s_spr[PW_SPR_CATFACE].size |= 0x80;
			s_spr[PW_SPR_LIGHTR_0].size |= 0x80;
			s_spr[PW_SPR_LIGHTR_1].size |= 0x80;
			s_spr[PW_SPR_LIGHTG].size |= 0x80;
			s_spr[PW_SPR_ARM_0].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_ARM, 0, 0, ENEMY_PAL_LINE, 0);
			s_spr[PW_SPR_GLASSBALL1].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_GLASSBALL, 0, 0, BG_PAL_LINE, 0);
			s_spr[PW_SPR_BUTTON].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_BUTTON, 0, 0, ENEMY_PAL_LINE, 0);
			e->emit_cnt = 0;
			break;

		case PWAVE_STATE_ON:
			s_spr[PW_SPR_BUTTON].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_BUTTON + 1, 0, 0, ENEMY_PAL_LINE, 0);
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_speed);
			if (e->anim_cnt == 0)
			{
//				sfx_play(SFX_LASER, 0);
			}

			e->emit_cnt++;
			if (e->emit_cnt >= kwave_emit_delay)
			{
				e->emit_cnt = 0;
				particle_spawn(o->x + INTTOFIX32(-40 + 16), o->y + INTTOFIX32(-40 + 12), PARTICLE_TYPE_PSYCHOWAVE);
			}
			if (e->anim_frame == 0)
			{
				s_spr[PW_SPR_MARQUEE_0].size &= ~0x80;
				s_spr[PW_SPR_MARQUEE_1].size &= ~0x80;
				s_spr[PW_SPR_SCREENBACK_0].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_SCREENBACK, 0, 0, BG_PAL_LINE, 0);
				s_spr[PW_SPR_LIGHTR_0].x = 3;
				s_spr[PW_SPR_LIGHTR_1].x = 27;
				s_spr[PW_SPR_LIGHTR_1].size = SPR_SIZE(3, 1);
				s_spr[PW_SPR_LIGHTG].y = 26;
				s_spr[PW_SPR_LIGHTG].size = SPR_SIZE(1, 3);
				s_spr[PW_SPR_GLASSBALL1].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_GLASSBALL, 0, 0, BG_PAL_LINE, 0);
			}
			else
			{
				s_spr[PW_SPR_MARQUEE_0].size |= 0x80;
				s_spr[PW_SPR_MARQUEE_1].size |= 0x80;
				s_spr[PW_SPR_SCREENBACK_0].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_SCREENBACK + 6, 0, 0, ENEMY_PAL_LINE, 0);
				s_spr[PW_SPR_LIGHTR_0].x = 6;
				s_spr[PW_SPR_LIGHTR_1].x = 30;
				s_spr[PW_SPR_LIGHTR_1].size = SPR_SIZE(2, 1);
				s_spr[PW_SPR_LIGHTG].y = 30;
				s_spr[PW_SPR_LIGHTG].size = SPR_SIZE(1, 2);
				s_spr[PW_SPR_GLASSBALL1].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_GLASSBALL + 18, 0, 0, BG_PAL_LINE, 0);
			}
			s_spr[PW_SPR_SCREENBACK_0].size &= ~0x80;
			s_spr[PW_SPR_SCREENBACK_1].size &= ~0x80;
			s_spr[PW_SPR_CATFACE].size &= ~0x80;
			s_spr[PW_SPR_LIGHTR_0].size &= ~0x80;
			s_spr[PW_SPR_LIGHTR_1].size &= ~0x80;
			break;

		case PWAVE_STATE_BROKEN:
			s_spr[PW_SPR_GLASSBALL1].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_GLASSBALL + 36, 0, 0, BG_PAL_LINE, 0);
			s_spr[PW_SPR_BUTTON].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_BUTTON, 0, 0, ENEMY_PAL_LINE, 0);
			break;
		default:
			break;
	}

	s_spr[PW_SPR_GLASSBALL2].attr = s_spr[PW_SPR_GLASSBALL1].attr + 9;
	s_spr[PW_SPR_GLASSBALL3].attr = s_spr[PW_SPR_GLASSBALL1].attr + (9*6);
	s_spr[PW_SPR_GLASSBALL4].attr = s_spr[PW_SPR_GLASSBALL1].attr + (9*6) + 9;

	render(e);
}

void o_load_psychowave(Obj *o, uint16_t data)
{
	O_Psychowave *e = (O_Psychowave *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Psychowave", 0,
	               INTTOFIX16(0), INTTOFIX16(0), INTTOFIX16(0), 127);
	o->main_func = main_func;
	o->cube_func = NULL;

	s_pwave = e;

	// Button
	s_spr[PW_SPR_BUTTON].x = 9;
	s_spr[PW_SPR_BUTTON].y = 32;
	s_spr[PW_SPR_BUTTON].size = SPR_SIZE(1, 1);
	s_spr[PW_SPR_BUTTON].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_BUTTON, 0, 0, ENEMY_PAL_LINE, 0);
	// Marquee
	s_spr[PW_SPR_MARQUEE_0].x = 0;
	s_spr[PW_SPR_MARQUEE_0].y = 0;
	s_spr[PW_SPR_MARQUEE_0].size = SPR_SIZE(3, 2);
	s_spr[PW_SPR_MARQUEE_0].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_MARQUEE, 0, 0, BG_PAL_LINE, 0);
	s_spr[PW_SPR_MARQUEE_1].x = 24;
	s_spr[PW_SPR_MARQUEE_1].y = 0;
	s_spr[PW_SPR_MARQUEE_1].size = SPR_SIZE(3, 2);
	s_spr[PW_SPR_MARQUEE_1].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_MARQUEE + 6, 0, 0, BG_PAL_LINE, 0);
	// Screenback
	s_spr[PW_SPR_SCREENBACK_0].x = 30;
	s_spr[PW_SPR_SCREENBACK_0].y = 25;
	s_spr[PW_SPR_SCREENBACK_0].size = SPR_SIZE(2, 3);
	s_spr[PW_SPR_SCREENBACK_0].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_SCREENBACK, 0, 0, BG_PAL_LINE, 0);
	// Cat Face
	s_spr[PW_SPR_CATFACE].x = 33;
	s_spr[PW_SPR_CATFACE].y = 31;
	s_spr[PW_SPR_CATFACE].size = SPR_SIZE(2, 2);
	s_spr[PW_SPR_CATFACE].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_CATFACE, 0, 0, LYLE_PAL_LINE, 0);
	// Red Lights
	s_spr[PW_SPR_LIGHTR_0].x = 3;
	s_spr[PW_SPR_LIGHTR_0].y = 19;
	s_spr[PW_SPR_LIGHTR_0].size = SPR_SIZE(3, 1);
	s_spr[PW_SPR_LIGHTR_0].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_LIGHTR, 0, 0, LYLE_PAL_LINE, 0);
	s_spr[PW_SPR_LIGHTR_1].x = 27;
	s_spr[PW_SPR_LIGHTR_1].y = 19;
	s_spr[PW_SPR_LIGHTR_1].size = SPR_SIZE(2, 1);
	s_spr[PW_SPR_LIGHTR_1].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_LIGHTR, 0, 0, LYLE_PAL_LINE, 0);
	// Green lights
	s_spr[PW_SPR_LIGHTG].x = 25;
	s_spr[PW_SPR_LIGHTG].y = 26;
	s_spr[PW_SPR_LIGHTG].size = SPR_SIZE(1, 3);
	s_spr[PW_SPR_LIGHTG].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_LIGHTG, 0, 0, BG_PAL_LINE, 0);
	// Arm top
	s_spr[PW_SPR_ARM_0].x = -40;
	s_spr[PW_SPR_ARM_0].y = -40;
	s_spr[PW_SPR_ARM_0].size = SPR_SIZE(4, 2);
	s_spr[PW_SPR_ARM_0].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_ARM, 0, 0, ENEMY_PAL_LINE, 0);
	s_spr[PW_SPR_ARM_1].x = -8;
	s_spr[PW_SPR_ARM_1].y = -40;
	s_spr[PW_SPR_ARM_1].size = SPR_SIZE(2, 1);
	s_spr[PW_SPR_ARM_1].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_ARM + 8, 0, 0, ENEMY_PAL_LINE, 0);
	s_spr[PW_SPR_ARM_2].x = 8;
	s_spr[PW_SPR_ARM_2].y = -40;
	s_spr[PW_SPR_ARM_2].size = SPR_SIZE(2, 2);
	s_spr[PW_SPR_ARM_2].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_ARM + 10, 0, 0, ENEMY_PAL_LINE, 0);
	s_spr[PW_SPR_ARM_3].x = 16;
	s_spr[PW_SPR_ARM_3].y = -24;
	s_spr[PW_SPR_ARM_3].size = SPR_SIZE(1, 3);
	s_spr[PW_SPR_ARM_3].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_ARM + 14, 0, 0, ENEMY_PAL_LINE, 0);
	// Glass ball
	s_spr[PW_SPR_GLASSBALL1].x = 9*8;
	s_spr[PW_SPR_GLASSBALL1].y = -4*8;
	s_spr[PW_SPR_GLASSBALL1].size = SPR_SIZE(3, 3);
	s_spr[PW_SPR_GLASSBALL1].attr = SPR_ATTR(s_vram_pos + PWAVE_OFFS_GLASSBALL, 0, 0, BG_PAL_LINE, 0);
	s_spr[PW_SPR_GLASSBALL2].x = 9*8 + 24;
	s_spr[PW_SPR_GLASSBALL2].y = -4*8;
	s_spr[PW_SPR_GLASSBALL2].size = SPR_SIZE(3, 3);
	s_spr[PW_SPR_GLASSBALL3].x = 9*8;
	s_spr[PW_SPR_GLASSBALL3].y = -4*8 + 24;
	s_spr[PW_SPR_GLASSBALL3].size = SPR_SIZE(3, 3);
	s_spr[PW_SPR_GLASSBALL4].x = 9*8 + 24;
	s_spr[PW_SPR_GLASSBALL4].y = -4*8 + 24;
	s_spr[PW_SPR_GLASSBALL4].size = SPR_SIZE(3, 3);

	// Scope grid on right machine
	s_spr[PW_SPR_GRID].x = 184;
	s_spr[PW_SPR_GRID].y = -8;
	s_spr[PW_SPR_GRID].size = SPR_SIZE(2, 2);
	s_spr[PW_SPR_GRID].attr = SPR_ATTR(0x00C0, 0, 0, LYLE_PAL_LINE, 0);
}

void o_unload_psychowave(void)
{
	s_vram_pos = 0;
}

void psychowave_set_state(PsychowaveState state)
{
	if (!s_pwave) return;
//	if (state != PWAVE_STATE_ON) sfx_stop(SFX_LASER);
	s_pwave->state = state;
}
