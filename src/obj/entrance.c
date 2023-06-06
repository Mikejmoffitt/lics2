#include "obj/entrance.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "map.h"

#include "game.h"
#include "cube.h"

static void main_func(Obj *o)
{
	O_Entrance *e = (O_Entrance *)o;
	if (o->touching_player)
	{
		map_set_next_room(e->to_room_id, e->to_entrance_num);
	}
}

void o_load_entrance(Obj *o, uint16_t data)
{
	O_Entrance *e = (O_Entrance *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");

	obj_basic_init(o, "Entrance", OBJ_FLAG_SENSITIVE, INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-31), 127);

	o->main_func = main_func;
	o->cube_func = NULL;

	e->entrance_num = data & 0x000F;
	e->to_room_id = (data & 0xFF00) >> 8;
	e->to_entrance_num = (data & 0x00F0) >> 4;
}

void o_unload_entrance(void)
{
}
