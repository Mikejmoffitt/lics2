#include "obj/columns.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"


static void main_func(Obj *o)
{
	(void)o;
	const Gfx *g = gfx_get(GFX_EX_COLUMNS);

	uint16_t x_index = 31 - (((map_get_x_scroll() + 1) / 2) % 32);

	md_dma_transfer_vram(MAP_TILE_VRAM_POSITION + (0x20 * 32), g->data + (4 * 32 * x_index), (4 * 32) / 2, 2);
	md_dma_transfer_vram(MAP_TILE_VRAM_POSITION + (0x30 * 32), g->data + 4096 + (4 * 32 * x_index), (4 * 32) / 2, 2);
	md_dma_transfer_vram(MAP_TILE_VRAM_POSITION + (0x40 * 32), g->data + 8192 + (4 * 32 * x_index), (4 * 32) / 2, 2);
}

void o_load_columns(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_Columns) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;

	obj_basic_init(o, "Columns", OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}
