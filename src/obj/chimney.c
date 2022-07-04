#include "obj/chimney.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"
#include "obj/exploder.h"
#include "obj/cube_manager.h"
#include "progress.h"
#include "sfx.h"

static uint16_t s_vram_pos;

static int16_t kanim_speed;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_CHIMNEY);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kanim_speed = PALSCALE_DURATION(12);

	s_constants_set = 1;
}

static void render(O_Chimney *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -16;

	typedef struct ChimneyFrame
	{
		int16_t tile;
		uint16_t pal;
	} ChimneyFrame;

	static const ChimneyFrame frames[] =
	{
		{0, MAP_PAL_LINE},
		{4, BG_PAL_LINE},
		{8, BG_PAL_LINE},
		{4, BG_PAL_LINE},
		{0, MAP_PAL_LINE},
		{0, MAP_PAL_LINE},
		{0, MAP_PAL_LINE},
	};

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + frames[e->anim_frame].tile, 0, 0,
	                             frames[e->anim_frame].pal, 0), SPR_SIZE(2, 2));
}

static void main_func(Obj *o)
{
	O_Chimney *e = (O_Chimney *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	if (o->hp == 127)
	{
		const fix16_t kspawn_rate = INTTOFIX16(PALSCALE_1ST(1.0));
		exploder_spawn(o->x, o->y + (o->top / 2), o->dx, o->dy, PARTICLE_TYPE_FIZZLERED, 6, kspawn_rate);
		sfx_play(SFX_OBJ_BURST, 3);
		sfx_play(SFX_OBJ_BURST_HI, 3);
		o->status = OBJ_STATUS_NULL;

		O_Chimney *e = (O_Chimney *)o;
		Cube *c = cube_manager_spawn(o->x, o->y, e->data, CUBE_STATUS_IDLE, 0, 0);
		if (c) cube_destroy(c);
	}

	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 7, kanim_speed);

	render(e);
}

static void cube_func(Obj *o, Cube *c)
{
	obj_standard_cube_response(o, c);
	if (o->hp == 0) o->hp = 127;
}

void o_load_chimney(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Chimney) <= sizeof(ObjSlot));

	const ProgressSlot *prog = progress_get();
	if ((data & 0xFFF0) == 0x0840)  // CP orb
	{
		const int16_t orb_id = data & 0x000F;
		if (prog->cp_orbs & (1 << orb_id))
		{
			o->status = OBJ_STATUS_NULL;
			return;
		}
	}
	else if ((data & 0xFFF0) == 0x0880)  // HP orb
	{
		const int16_t orb_id = data & 0x000F;
		if (prog->hp_orbs & (1 << orb_id))
		{
			o->status = OBJ_STATUS_NULL;
			return;
		}
	}

	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 3);
	o->main_func = main_func;
	o->cube_func = cube_func;

	O_Chimney *e = (O_Chimney *)o;
	e->data = data;
}

void o_unload_chimney(void)
{
	s_vram_pos = 0;
}
