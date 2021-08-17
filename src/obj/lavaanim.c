#include "obj/lavaanim.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"

static int16_t kanim_speed;

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kanim_speed = PALSCALE_DURATION(14);
	s_constants_set = 1;
}

static void update_tiles(O_LavaAnim *e)
{
	const Gfx *g = gfx_get(GFX_LAVAANIM);
	const uint8_t *src = g->data + (e->anim_frame ? (4 * 32) : 0) + (e->variant * 8 * 32);
	dma_q_transfer_vram(MAP_TILE_VRAM_POSITION + (0xEE * 32), src, 32 / 2, 2);
	dma_q_transfer_vram(MAP_TILE_VRAM_POSITION + (0xFE * 32), src + 32, 32 / 2, 2);
	dma_q_transfer_vram(MAP_TILE_VRAM_POSITION + (0xEF * 32), src + 64, 32 / 2, 2);
	dma_q_transfer_vram(MAP_TILE_VRAM_POSITION + (0xFF * 32), src + 96, 32 / 2, 2);
}

static void main_func(Obj *o)
{
	O_LavaAnim *e = (O_LavaAnim *)o;

	const int16_t anim_frame_prev = e->anim_frame;

	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_speed);

	if (e->variant == 1)
	{
		pal_upload(MAP_TILE_CRAM_POSITION, res_pal_bg_greenlava_bin, sizeof(res_pal_bg_greenlava_bin) / 2);
	}

	if (e->anim_frame != anim_frame_prev)
	{
		update_tiles(e);
	}
}

void o_load_lavaanim(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_LavaAnim) <= sizeof(ObjSlot));
	
	set_constants();

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;

	O_LavaAnim *e = (O_LavaAnim *)o;
	e->variant = data;
	if (e->variant)
	{
		pal_upload(MAP_TILE_CRAM_POSITION, res_pal_bg_greenlava_bin, sizeof(res_pal_bg_greenlava_bin) / 2);
	}
	update_tiles(e);
}

void o_unload_lavaanim(void)
{
}
