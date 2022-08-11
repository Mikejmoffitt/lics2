#include "obj/bgscroll.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "lyle.h"

static void main_func(Obj *o)
{
	O_BgScroll *e = (O_BgScroll *)o;

	lyle_set_scroll_v_en(0);
	map_set_y_scroll(e->scroll_y);

}

void o_load_bgscroll(Obj *o, uint16_t data)
{
	O_BgScroll *e = (O_BgScroll *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");

	e->scroll_y = data;

	obj_basic_init(o, "BgScroll", OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;

	lyle_set_scroll_v_en(0);
}

void o_unload_bgscroll(void)
{
}
