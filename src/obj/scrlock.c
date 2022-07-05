#include "obj/scrlock.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "game.h"
#include "obj/lyle.h"

static void main_func(Obj *o)
{
	(void)o;

	// Search for an active title object, and abort early if it's present.
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *g = &g_objects[i].obj;
		if (g->status == OBJ_STATUS_NULL) continue;
		if (g->type == OBJ_TITLE) return;
	}

	lyle_set_scroll_h_en(1);
	lyle_set_scroll_v_en(0);
}

void o_load_scrlock(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_ScrLock) <= sizeof(ObjSlot));
	(void)data;

	obj_basic_init(o, "ScrlLock", OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-4), INTTOFIX16(4), INTTOFIX16(-19), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}
