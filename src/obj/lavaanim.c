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
	const uint8_t *src = g->data + e->info.src_data_offset +
	                     (e->anim_frame ? (e->info.src_anim_offset) : 0);

	md_dma_transfer_vram(MAP_TILE_VRAM_POSITION + e->info.dest_vram_offset,
	                    src, e->info.line_transfer_words, 2);

	src += (e->info.line_transfer_words * 2);

	md_dma_transfer_vram(MAP_TILE_VRAM_POSITION + e->info.dest_vram_offset + (0x10 * 32),
	                    src, e->info.line_transfer_words, 2);
}

static void main_func(Obj *o)
{
	O_LavaAnim *e = (O_LavaAnim *)o;

	const int16_t anim_frame_prev = e->anim_frame;

	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_speed);

	if (e->info.pal)
	{
		md_pal_upload(MAP_TILE_CRAM_POSITION, e->info.pal, e->info.pal_size);
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

	obj_basic_init(o, "LavaAnim", OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;

	static const LavaAnimInfo anim_infos[] = 
	{
		[LAVA_ANIM_REGULAR] =
		{
			/*pal=*/NULL,
			/*pal_size=*/0,
			/*src_data_offset=*/0,
			/*src_anim_offset=*/4 * 32,
			/*dest_vram_offset=*/0xEE * 32,
			/*line_transfer_words=*/(2 * 32) / 2,
		},
		[LAVA_ANIM_GREEN] =
		{
			/*pal=*/res_pal_bg_greenlava_bin,
			/*pal_size=*/sizeof(res_pal_bg_greenlava_bin) / 2,
			/*src_data_offset=*/8 * 32,
			/*src_anim_offset=*/4 * 32,
			/*dest_vram_offset=*/0xEE * 32,
			/*line_transfer_words=*/(2 * 32) / 2,
		},
		[LAVA_ANIM_TECHNO] =
		{
			/*pal=*/NULL,
			/*pal_size=*/0,
			/*src_data_offset=*/16 * 32,
			/*src_anim_offset=*/8 * 32,
			/*dest_vram_offset=*/0xEC * 32,
			/*line_transfer_words=*/(4 * 32) / 2,
		},
	};

	O_LavaAnim *e = (O_LavaAnim *)o;
	SYSTEM_ASSERT(data < ARRAYSIZE(anim_infos));
	e->info = anim_infos[data];

	if (e->info.pal)
	{
		md_pal_upload(MAP_TILE_CRAM_POSITION, e->info.pal, e->info.pal_size);
	}
	update_tiles(e);
}

void o_unload_lavaanim(void)
{
}
