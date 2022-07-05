#include "obj/radio.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "obj/cube_manager.h"
#include "progress.h"
#include "sfx.h"
#include "obj/exploder.h"

static uint16_t s_vram_pos;
static int16_t kanim_speed;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_RADIO);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kanim_speed = PALSCALE_DURATION(12);

	s_constants_set = 1;
}

static void render(O_Radio *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -16;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
}

static void main_func(Obj *o)
{
	O_Radio *e = (O_Radio *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	if (o->hp >= 127)
	{
		const fix16_t kspawn_rate = INTTOFIX16(PALSCALE_1ST(1.0));
		exploder_spawn(o->x, o->y + (o->top / 2), o->dx, o->dy, PARTICLE_TYPE_FIZZLERED, 6, kspawn_rate);
		sfx_play(SFX_OBJ_BURST, 3);
		sfx_play(SFX_OBJ_BURST_HI, 3);

		Cube *c = cube_manager_spawn(o->x, o->y, e->storage, CUBE_STATUS_IDLE, 0, 0);
		if (c) cube_destroy(c);
		obj_erase(o);
	}

	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_speed);

	if (e->anim_frame == 0)
	{
		md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_radio_1_bin,
		           sizeof(res_pal_enemy_radio_1_bin) / 2);
	}
	else
	{
		md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_radio_2_bin,
		           sizeof(res_pal_enemy_radio_2_bin) / 2);
	}

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

void o_load_radio(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Radio) <= sizeof(ObjSlot));
	(void)data;

	obj_basic_init(o, "Radio", OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 2);
	o->main_func = main_func;
	o->cube_func = cube_func;

	const ProgressSlot *prog = progress_get();
	if ((data & 0xFFF0) == 0x0840)  // CP orb
	{
		const int16_t orb_id = data & 0x000F;
		if (prog->cp_orbs & (1 << orb_id))
		{
			obj_erase(o);
			return;
		}
	}
	else if ((data & 0xFFF0) == 0x0880)  // HP orb
	{
		const int16_t orb_id = data & 0x000F;
		if (prog->hp_orbs & (1 << orb_id))
		{
			obj_erase(o);
			return;
		}
	}

	O_Radio *e = (O_Radio *)o;
	e->storage = data;

	vram_load();
	set_constants();
}

void o_unload_radio(void)
{
	s_vram_pos = 0;
}
