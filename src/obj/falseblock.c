#include "obj/falseblock.h"
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

static void render(O_Falseblock *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -16;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	spr_put(sp_x, sp_y, SPR_ATTR(e->base_tile_id, 0, 0,
	                             MAP_PAL_LINE, 0), SPR_SIZE(2, 1));
	spr_put(sp_x, sp_y + 8, SPR_ATTR(e->base_tile_id + 0x10, 0, 0,
	                             MAP_PAL_LINE, 0), SPR_SIZE(2, 1));
}

static void main_func(Obj *o)
{
	O_Falseblock *e = (O_Falseblock *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	O_Lyle *l = lyle_get();

	if (l->head.x + l->head.right >= o->x + o->left &&
	    l->head.x + l->head.right <= o->x + o->right &&
	    l->head.y + l->head.top <= o->y && l->head.dy < 0)
	{
		l->head.dy = 0;
	}
	render(e);
}

static void cube_func(Obj *o, Cube *c)
{
	(void)o;
	if (c->type != CUBE_TYPE_GREEN) return;
	if (c->dy < 0) c->dy = -c->dy;
}

void o_load_falseblock(Obj *o, uint16_t data)
{
	O_Falseblock *e = (O_Falseblock *)o;
	SYSTEM_ASSERT(sizeof(O_Falseblock) <= sizeof(ObjSlot));
	e->base_tile_id = data;

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = cube_func;
}

void o_unload_falseblock(void)
{
}
