#include "obj/technobg.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"

// Offsets into the source tile data have been precalculated to avoid an
// expensive 32-bit multiply on the poor 68000.
static const int source_data_offset_tbl[] =
{
	8 * 6 * 32 * 0,
	8 * 6 * 32 * 1,
	8 * 6 * 32 * 2,
	8 * 6 * 32 * 3,
	8 * 6 * 32 * 4,
	8 * 6 * 32 * 5,
	8 * 6 * 32 * 6,
	8 * 6 * 32 * 7,
	8 * 6 * 32 * 8,
	8 * 6 * 32 * 9,
	8 * 6 * 32 * 10,
	8 * 6 * 32 * 11,
	8 * 6 * 32 * 12,
	8 * 6 * 32 * 13,
	8 * 6 * 32 * 14,
	8 * 6 * 32 * 15,
	8 * 6 * 32 * 16,
	8 * 6 * 32 * 17,
	8 * 6 * 32 * 18,
	8 * 6 * 32 * 19,
	8 * 6 * 32 * 20,
	8 * 6 * 32 * 21,
	8 * 6 * 32 * 22,
	8 * 6 * 32 * 23,
	8 * 6 * 32 * 24,
	8 * 6 * 32 * 25,
	8 * 6 * 32 * 26,
	8 * 6 * 32 * 27,
	8 * 6 * 32 * 28,
	8 * 6 * 32 * 29,
	8 * 6 * 32 * 30,
	8 * 6 * 32 * 31,
	8 * 6 * 32 * 32,
	8 * 6 * 32 * 33,
	8 * 6 * 32 * 34,
	8 * 6 * 32 * 35,
	8 * 6 * 32 * 36,
	8 * 6 * 32 * 37,
	8 * 6 * 32 * 38,
	8 * 6 * 32 * 39,
	8 * 6 * 32 * 40,
	8 * 6 * 32 * 41,
	8 * 6 * 32 * 42,
	8 * 6 * 32 * 43,
	8 * 6 * 32 * 44,
	8 * 6 * 32 * 45,
	8 * 6 * 32 * 46,
	8 * 6 * 32 * 47,
	8 * 6 * 32 * 48,
	8 * 6 * 32 * 49,
	8 * 6 * 32 * 50,
	8 * 6 * 32 * 51,
	8 * 6 * 32 * 52,
	8 * 6 * 32 * 53,
	8 * 6 * 32 * 54,
	8 * 6 * 32 * 55,
	8 * 6 * 32 * 56,
	8 * 6 * 32 * 57,
	8 * 6 * 32 * 58,
	8 * 6 * 32 * 59,
	8 * 6 * 32 * 60,
	8 * 6 * 32 * 61,
	8 * 6 * 32 * 62,
	8 * 6 * 32 * 63,
};

// The horizontal scroller transfers an 8 x 5 region of tiles into FG memory.
static void hori_func(Obj *o)
{
	O_TechnoBg *e = (O_TechnoBg *)o;

	uint16_t x_index = 63 - (((map_get_x_scroll() + 1) / 2) % 64);
	if (x_index == e->last_index) return;
	e->last_index = x_index;

	uint16_t dest_vram = MAP_TILE_VRAM_POSITION + 0x08;
	const uint8_t *source = e->source + source_data_offset_tbl[x_index];
	const int16_t data_size = (8 * 32);
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
	dest_vram += 0x10;
	source += data_size;
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
	dest_vram += 0x10;
	source += data_size;
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
	dest_vram += 0x10;
	source += data_size;
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
	dest_vram += 0x10;
	source += data_size;
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
}

// The vertical scroller transfers a 6 x 8 region of tiles into FG memory.
static void vert_func(Obj *o)
{
	O_TechnoBg *e = (O_TechnoBg *)o;
	uint16_t y_index = 63 - (((map_get_y_scroll() + 1) / 2) % 64);
	if (y_index == e->last_index) return;
	e->last_index = y_index;

	uint16_t dest_vram = MAP_TILE_VRAM_POSITION + 0x0A;
	const uint8_t *source = e->source + source_data_offset_tbl[y_index];
	const int16_t data_size = (6 * 32);
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
	dest_vram += 0x10;
	source += data_size;
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
	dest_vram += 0x10;
	source += data_size;
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
	dest_vram += 0x10;
	source += data_size;
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
	dest_vram += 0x10;
	source += data_size;
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
	dest_vram += 0x10;
	source += data_size;
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
	dest_vram += 0x10;
	source += data_size;
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
	dest_vram += 0x10;
	source += data_size;
	md_dma_transfer_vram(dest_vram * 32, source, data_size / 2, 2);
}

void o_load_technobg(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_TechnoBg) <= sizeof(ObjSlot));

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = data ? vert_func : hori_func;
	o->cube_func = NULL;

	O_TechnoBg *e = (O_TechnoBg *)o;
	e->source = data ? gfx_get(GFX_EX_TECHNOBGV)->data : gfx_get(GFX_EX_TECHNOBGH)->data;
}
