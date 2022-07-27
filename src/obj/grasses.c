#include "obj/grasses.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"


static void main_func(Obj *o)
{
	(void)o;
	const Gfx *g = gfx_get(GFX_EX_GRASSES);

	const fix32_t x_fixed = INTTOFIX32(map_get_x_scroll());
	const int16_t x_int = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.222222222221)));
	uint16_t x_index = x_int % 24;

	md_dma_transfer_vram(MAP_TILE_VRAM_POSITION + (0x60 * 32), g->data + (7 * 3 * 32 * x_index), (32 * 7 * 3) / 2, 2);
}

void o_load_grasses(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_Grasses) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;

	obj_basic_init(o, "Grasses", OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}
