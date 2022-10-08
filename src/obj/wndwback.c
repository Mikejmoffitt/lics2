#include "obj/wndwback.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"


static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_WNDWBACK);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	s_constants_set = true;
}

static void render(O_Wndwback *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -16;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
	                             LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));
}

static void main_func(Obj *o)
{
	O_Wndwback *e = (O_Wndwback *)o;
	if (e->visible) render(e);
}

void o_load_wndwback(Obj *o, uint16_t data)
{
	(void)data;
	_Static_assert(sizeof(O_Wndwback) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	set_constants();
	vram_load();

	obj_basic_init(o, "WndwBack", 0,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_wndwback(void)
{
	s_vram_pos = 0;
}

void wndwback_set_visible(int16_t visible)
{
	ObjSlot *s = &g_objects[0];
	int16_t i = ARRAYSIZE(g_objects);
	while (i--)
	{
		Obj *b = (Obj *)s;
		s++;
		if (b->status == OBJ_STATUS_NULL ||
		    b->type != OBJ_WNDWBACK)
		{
			continue;
		}
		O_Wndwback *e = (O_Wndwback *)b;
		e->visible = visible;
	}
}
