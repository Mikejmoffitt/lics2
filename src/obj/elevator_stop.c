#include "obj/elevator_stop.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "common.h"

static void main_func(Obj *o)
{
	(void)o;
}

void o_load_elevator_stop(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_ElevatorStop) <= sizeof(ObjSlot));
	(void)data;

	O_ElevatorStop *e = (O_ElevatorStop *)o;

	obj_basic_init(o, 0,
	               INTTOFIX16(-16), INTTOFIX16(16), INTTOFIX16(-8), 127);
	o->main_func = main_func;

	e->id = data;
}

void o_unload_elevator_stop(void)
{
}
