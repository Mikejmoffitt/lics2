#include "obj/hoop.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_HOOP);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static void main_func(Obj *o)
{
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -18;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
	                             MAP_PAL_LINE, 0), SPR_SIZE(2, 2));
}

void o_load_hoop(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Hoop) <= sizeof(ObjSlot));
	(void)data;
	vram_load();

	obj_basic_init(o, 0,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_hoop(void)
{
	s_vram_pos = 0;
}
