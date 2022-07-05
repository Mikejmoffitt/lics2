#include "obj/bounds.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"


static void main_func(Obj *o)
{
	(void)o;
}

void o_load_bounds(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Bounds) <= sizeof(ObjSlot));
	(void)data;

	obj_basic_init(o, "Bounds", 0,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-6 * 16), 127);
	o->main_func = main_func;
}

void o_unload_bounds(void)
{
}
