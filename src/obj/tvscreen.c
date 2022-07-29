#include "obj/tvscreen.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "palscale.h"
#include "obj/map.h"
#include "progress.h"

static uint16_t kanim_speed;
static uint16_t kanim_speed_static;

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_TVSCREEN);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kanim_speed = PALSCALE_DURATION(2);
	kanim_speed_static = PALSCALE_DURATION(3);

	s_constants_set = 1;
}

static void render(O_TvScreen *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -12;
	static const int16_t offset_y = -17;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, e->attr, SPR_SIZE(3, 2));
}

static void main_func(Obj *o)
{
	O_TvScreen *e = (O_TvScreen *)o;

	if (!e->active)
	{
		OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 3, kanim_speed_static);
		e->attr = SPR_ATTR(s_vram_pos + ((3 + e->anim_frame) * 6),
		                   0, 0, LYLE_PAL_LINE, 0);
	}
	else
	{
		OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_speed);
		if (e->anim_frame == 0)
		{
			md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_tvscreen_bin,
			           sizeof(res_pal_enemy_tvscreen_bin) / 2);
		}
		else
		{
			md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_tvscreen_dk_bin,
			           sizeof(res_pal_enemy_tvscreen_dk_bin) / 2);
		}
	}

	render(e);
}

void o_load_tvscreen(Obj *o, uint16_t data)
{
	O_TvScreen *e = (O_TvScreen *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "TvScreen", 0,
	               INTTOFIX16(-12), INTTOFIX16(12), INTTOFIX16(-16), 1);
	o->main_func = main_func;
	o->cube_func = NULL;

	e->id = data;
	if (e->id > 2)
	{
		obj_erase(o);
		return;
	}

	const ProgressSlot *progress = progress_get();
	e->active = (progress->teleporters_active & (1 << e->id)) ? 1 : 0;

	if (e->active)
	{
		e->attr = SPR_ATTR(s_vram_pos + (data * 6),
		                   0, 0, ENEMY_PAL_LINE, 0);
	}
}

void o_unload_tvscreen(void)
{
	s_vram_pos = 0;
}
