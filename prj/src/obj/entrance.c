#include "obj/entrance.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "obj/map.h"

#include "game.h"
#include "cube.h"

static uint16_t vram_pos;

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g = gfx_get(GFX_TEMPLATE);
	vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static void main_func(Obj *o)
{
	O_Entrance *e = (O_Entrance *)o;
	if (o->touching_player)
	{
		map_set_next_room(e->to_room_id, e->to_entrance_num);
	}

	// Debug rendering of entrances
	int16_t sp_x = FIX32TOINT(e->head.x) - map_get_x_scroll() - 8;
	int16_t sp_y = FIX32TOINT(e->head.y) - map_get_y_scroll() - 32;

	if (sp_x < -32 || sp_x > GAME_SCREEN_W_PIXELS) return;
	if (sp_y < -32 || sp_y > GAME_SCREEN_H_PIXELS) return;

	spr_put(sp_x, sp_y,
	        SPR_ATTR(vram_pos,
	                 e->head.direction == OBJ_DIRECTION_LEFT, 0,
	                 0, 0), SPR_SIZE(2, 4));
}

void o_load_entrance(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Entrance) <= sizeof(ObjSlot));

	vram_load();

	obj_basic_init(o, OBJ_FLAG_SENSITIVE, INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-32), 127);

	o->main_func = main_func;
	o->cube_func = NULL;

	O_Entrance *e = (O_Entrance *)o;
	e->entrance_num = data & 0x000F;
	e->to_room_id = (data & 0xFF00) >> 8;
	e->to_entrance_num = (data & 0x00F0) >> 4;
}

void o_unload_entrance(void)
{
	vram_pos = 0;
}
