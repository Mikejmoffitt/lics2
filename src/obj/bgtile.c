#include "obj/bgtile.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "md/megadrive.h"

#include "objtile.h"  // 2023-04-09: Dedicated list for this.

static void main_func(Obj *o)
{
	obj_erase(o);
}

void o_load_bgtile(Obj *o, uint16_t data)
{
	const uint16_t attr = SPR_ATTR(data & 0x00FF, 0, 0, (data & 0x0300) >> 8, 0);
	objtile_place(o->x, o->y, attr);
	obj_erase(o);
}
