#include "obj/bogologo.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"

static uint16_t vram_pos;

static const uint16_t kpalette_red[] =
{
	
};

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g = gfx_get(GFX_BOGOLOGO);
	vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t constants_set;
	if (constants_set) return;
	// Set constants here.

	constants_set = 1;
}

static void render(O_Bogologo *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -64;
	static const int16_t offset_y = -48;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	for (int16_t x = 0; x < 4; x++)
	{
		spr_put(sp_x + (x * 32), sp_y,
		        SPR_ATTR(vram_pos + (x * 12),
		                 0, 0, ENEMY_PAL_LINE, 0),
		        SPR_SIZE(4, 3));
		spr_put(sp_x + (x * 32), sp_y + 24,
		        SPR_ATTR(vram_pos + 48 + (x * 12),
		                 0, 0, ENEMY_PAL_LINE, 0),
		        SPR_SIZE(4, 3));
	}
}

static void main_func(Obj *o)
{
	O_Bogologo *e = (O_Bogologo *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}
	if (o->offscreen)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}

	pal_upload(ENEMY_CRAM_POSITION, res_pal_bogologo_bin, sizeof(res_pal_bogologo_bin) / 4);
	render(e);
}

void o_load_bogologo(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Bogologo) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_TANGIBLE | OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-64), INTTOFIX16(64), INTTOFIX16(-48), 1);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_bogologo(void)
{
	vram_pos = 0;
}
