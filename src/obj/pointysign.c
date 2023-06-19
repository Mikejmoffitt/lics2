#include "obj/pointysign.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_POINTYSIGN);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static void render(O_PointySign *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -24;
	static const int16_t offset_y = -32;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(3, 4));
	md_spr_put(sp_x+24, sp_y, SPR_ATTR(s_vram_pos+12, 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(3, 4));
}

static void main_func(Obj *o)
{
	O_PointySign *e = (O_PointySign *)o;
	render(e);
}

void o_load_pointysign(Obj *o, uint16_t data)
{
	O_PointySign *e = (O_PointySign *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	vram_load();

	obj_basic_init(o, "PointySign", 0,
	               INTTOFIX16(-24), INTTOFIX16(24), INTTOFIX16(-32), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_pointysign(void)
{
	s_vram_pos = 0;
}
