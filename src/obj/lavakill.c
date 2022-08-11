#include "obj/lavakill.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "md/megadrive.h"

#include "lyle.h"

static void main_func(Obj *o)
{
	O_Lyle *l = lyle_get();

	if (l->head.x < o->x) return;
	if (l->head.y < o->y) return;



	l->head.hp = 0;
}

void o_load_lavakill(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_Lavakill) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;

	obj_basic_init(o, "LavaKill", OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}
