#include "obj/cp_meter.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "progress.h"

static int16_t kflicker_speed;

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_CP_METER);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	kflicker_speed = PALSCALE_DURATION(1);

	s_constants_set = 1;
}

static void render(O_CpMeter *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -8;

	obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
	                        map_get_x_scroll(), map_get_y_scroll());
	const ProgressSlot *progress = progress_get();
	const int16_t orbs = progress->registered_cp_orbs;
	for (int16_t i = 0; i < orbs; i++)
	{
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
		                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 1));
		sp_y -= 8;
	}
}

static void main_func(Obj *o)
{
	O_CpMeter *e = (O_CpMeter *)o;
	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kflicker_speed);
	if (e->anim_frame == 1) render(e);
}

void o_load_cp_meter(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_CpMeter) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "CP Meter", OBJ_FLAG_ALWAYS_ACTIVE, INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-80), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_cp_meter(void)
{
	s_vram_pos = 0;
}
