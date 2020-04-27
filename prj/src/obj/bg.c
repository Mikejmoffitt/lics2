#include "obj/bg.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "obj/map.h"
#include "common.h"

#include "game.h"
#include "res.h"

static uint16_t loaded;

static int16_t h_scroll_buffer[GAME_SCREEN_H_CELLS];
static int16_t v_scroll_buffer[GAME_SCREEN_W_CELLS / 2];

static uint8_t scratch[64 * 64 / 2];

static inline void set_h_scroll_plane(int16_t value)
{
	uint16_t i = ARRAYSIZE(h_scroll_buffer);
	while (i--)
	{
		h_scroll_buffer[i] = value;
	}
}

static inline void set_v_scroll_plane(int16_t value)
{
	uint16_t i = ARRAYSIZE(v_scroll_buffer);
	while (i--)
	{
		v_scroll_buffer[i] = value;
	}
}

typedef struct BgDescriptor
{
	GfxId gfx_id;
	const uint8_t *palette;
	const uint8_t *mapping;
	BgScroll scroll;
} BgDescriptor;

static const BgDescriptor backgrounds[] =
{
	[0] = {GFX_NULL, res_pal_bg_bg0_bin, NULL, BG_SCROLL_NONE},
	[1] = {GFX_BG_1, res_pal_bg_bg1_bin, res_bgmap_bg1_bin, BG_SCROLL_H_CELL},
	[2] = {GFX_BG_2, res_pal_bg_bg2_bin, res_bgmap_bg2_bin, BG_SCROLL_PLANE},
	[3] = {GFX_BG_3, res_pal_bg_bg3_bin, res_bgmap_bg3_bin, BG_SCROLL_V_CELL},
	[4] = {GFX_BG_4, res_pal_bg_bg4_bin, res_bgmap_bg4_bin, BG_SCROLL_V_CELL},
	[5] = {GFX_BG_5, res_pal_bg_bg5_bin, res_bgmap_bg5_bin, BG_SCROLL_PLANE},
	[6] = {GFX_BG_6, res_pal_bg_bg1_bin, res_bgmap_bg6_bin, BG_SCROLL_H_CELL},  // Mapping modification of 1.
	[7] = {GFX_BG_7, res_pal_bg_bg7_bin, res_bgmap_bg7_bin, BG_SCROLL_H_CELL},
//	[8] = {GFX_BG_8, res_pal_bg_bg8_bin, res_bgmap_bg8_bin, BG_SCROLL_PLANE},
	[9] = {GFX_BG_9, res_pal_bg_bg9_bin, res_bgmap_bg9_bin, BG_SCROLL_PLANE},
	[10] = {GFX_BG_10, res_pal_bg_bg10_bin, res_bgmap_bg10_bin, BG_SCROLL_H_CELL},
	[11] = {GFX_BG_11, res_pal_bg_bg11_bin, res_bgmap_bg11_bin, BG_SCROLL_PLANE},
	[12] = {GFX_BG_12, res_pal_bg_bg10_bin, res_bgmap_bg10_bin, BG_SCROLL_H_CELL},  // Scrolled lower at runtime.
	[13] = {GFX_BG_13, res_pal_bg_bg13_bin, res_bgmap_bg13_bin, BG_SCROLL_V_CELL},
	[14] = {GFX_BG_14, res_pal_bg_bg13_bin, res_bgmap_bg13_bin, BG_SCROLL_V_CELL},
	[15] = {GFX_BG_15, res_pal_bg_bg15_bin, res_bgmap_bg15_bin, BG_SCROLL_H_CELL},

	[20] = {0}
};

static void bg_city_func(int16_t x_scroll, int16_t y_scroll)
{
	const fix32_t x_fixed = INTTOFIX32(-x_scroll);
	const int16_t close_x = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.666666667)));
	set_v_scroll_plane(0);
	set_h_scroll_plane(-x_scroll / 2);

	for (uint16_t i = 21; i < 25; i++)
	{
		h_scroll_buffer[i] = close_x;
	}
}

static void bg_plane_func(int16_t x_scroll, int16_t y_scroll)
{
	const fix32_t x_fixed = INTTOFIX32(-x_scroll);
	const int16_t far_x = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.666666667)));
	set_h_scroll_plane(far_x);

	// If it's a single-screen height room, hike the chevrons upwards.
	if (map_get_bottom() == INTTOFIX32(240)) y_scroll -= 384;

	set_v_scroll_plane(y_scroll/2);
}

static void bg_blue_bumps_func(int16_t x_scroll, int16_t y_scroll)
{
	(void)x_scroll;
	v_scroll_buffer[0] = y_scroll / 2;
	v_scroll_buffer[1] = y_scroll / 2;
	v_scroll_buffer[2] = y_scroll / 2;
	v_scroll_buffer[3] = y_scroll / 8;

	v_scroll_buffer[19] = y_scroll / 2;
	v_scroll_buffer[18] = y_scroll / 2;
	v_scroll_buffer[17] = y_scroll / 2;
	v_scroll_buffer[16] = y_scroll / 8;

	const uint8_t scroll_off = ((uint8_t)(y_scroll) / 4) % 16;

	dma_q_copy_vram(BG_TILE_VRAM_POSITION + (12 * 32),
	                BG_TILE_VRAM_POSITION + (16 * 32) + 128 * scroll_off,
	                32 * 4, 1);
}

static void bg_bubbles_func(int16_t x_scroll, int16_t y_scroll)
{
	for (uint16_t i = 0; i < ARRAYSIZE(v_scroll_buffer); i++)
	{
		if (i % 4 >= 2)
		{
			v_scroll_buffer[i] = y_scroll/2;
		}
		else
		{
			const fix32_t y_fixed = INTTOFIX32(y_scroll);
			const int16_t far_y = FIX32TOINT(FIX32MUL(y_fixed, INTTOFIX32(0.666666667)));
			v_scroll_buffer[i] = far_y;
		}
	}
}

static void bg_orange_balls_func(int16_t x_scroll, int16_t y_scroll)
{
	for (uint16_t i = 0; i < 7; i++)
	{

	}
}

static inline uint8_t scratch_fetch(int x, int y)
{
	int coarse_x = (x / 4) * 32;
	int coarse_y = (y / 8) * 256;
	const int index = (coarse_x + coarse_y) + (x % 4) + (y % 8) * 4;
	return scratch[index];
}

static inline void scratch_plot(int x, int y, int8_t value)
{
	x = x % 32;
	int coarse_x = (x / 4) * 32;
	int coarse_y = (y / 8) * 256;
	const int index = (coarse_x + coarse_y) + (x % 4) + (y % 8) * 4;
	scratch[index] = value;
}

// Woooow this one's pretty ugly
// On the other hand, it's working well enough.
// TODO: Clean up and profile
static void bg_undersand_columns(int16_t x_scroll, int16_t y_scroll)
{
	const Gfx *g = gfx_get(GFX_BG_9);
	set_h_scroll_plane(-x_scroll/2);
	uint16_t i = ARRAYSIZE(scratch);
	while (i--) scratch[i] = 0;

	const uint8_t *bmp = (const uint8_t *)g->data;

	const uint16_t scroll_off = ((uint8_t)(x_scroll) / 4) % 64;
	const uint16_t data_off = scroll_off % 2 ? 64 : 0;

	// The purple parts in the back.
	for (int i = 0; i < 2; i++)
	{
		const uint16_t base_x = i * 32;
		for (int y = 0; y < 8; y++)
		{
			for (int x = 0; x < 4; x++)
			{
				scratch_plot((base_x + scroll_off) / 2 + x, y, bmp[(4 * y) + (10 * 32) + x + data_off]);
				scratch_plot((base_x + scroll_off) / 2 + x, y + 8, bmp[(4 * y) + (11 * 32) + x + data_off]);
			}
		}
	}

	// For transparent parts that don't align with even pixels, some bit fussing
	// is needed so as to support transparency properly.
	for (int y = 5; y < 8; y++)
	{
		scratch_plot(0, y, (scratch_fetch(0, y) & 0xF0) | bmp[(4 * y) + (2 * 32)]);
	}
	for (int y = 8; y < 11; y++)
	{
		scratch_plot(0, y, (scratch_fetch(0, y) & 0xF0) | bmp[(4 * (y % 8)) + (4 * 32)]);
	}
	for (int y = 0; y < 3; y++)
	{
		scratch_plot(4, y, (scratch_fetch(4, y) & 0x0F) | bmp[(4 * y) + (3 * 32)]);
	}
	for (int y = 9; y < 16; y++)
	{
		scratch_plot(4, y, (scratch_fetch(4, y) & 0x0F) | bmp[(4 * (y % 8)) + (5 * 32)]);
	}

	// The fixed green parts in front.
	for (int y = 0; y < 8; y++)
	{
		for (int x = 1; x < 4; x++)
		{
			scratch_plot(x, y, bmp[(4 * y) + (2 * 32) + x]);
			scratch_plot(x, y + 8, bmp[(4 * y) + (4 * 32) + x]);
		}
	}

	// Little left-side details not included in the main loop.
	for (int y = 0; y < 5; y++)
	{
		scratch_plot(0, y, bmp[(4 * y) + (2 * 32)]);
	}
	for (int y = 11; y < 16; y++)
	{
		scratch_plot(0, y, bmp[(4 * (y % 8)) + (4 * 32)]);
	}

	// Some right-side details.
	for (int y = 3; y < 8; y++)
	{
		scratch_plot(4, y, bmp[(4 * (y % 8)) + (3 * 32)]);
	}
	scratch_plot(4, 8, bmp[(5 * 32)]);

	dma_q_transfer_vram(BG_TILE_VRAM_POSITION, scratch, sizeof(scratch) / 2, 2);
}

static void (*bg_funcs[])(int16_t, int16_t) =
{
	[0] = NULL,
	[1] = bg_city_func,
	[2] = bg_plane_func,
	[3] = bg_blue_bumps_func,
	[4] = bg_bubbles_func,
	[5] = bg_plane_func,
	[6] = bg_city_func,
	[7] = bg_orange_balls_func,
	NULL,
	[9] = bg_undersand_columns,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static void main_func(Obj *o)
{
	system_profile(PALRGB(0, 4, 0));
	O_Bg *f = (O_Bg *)o;
	if (f->bg_id < ARRAYSIZE(bg_funcs) && bg_funcs[f->bg_id])
	{
		bg_funcs[f->bg_id](map_get_x_scroll(), map_get_y_scroll());
	}

	dma_q_transfer_vsram(2, v_scroll_buffer, sizeof(v_scroll_buffer) / 2, 4);
	dma_q_transfer_vram(vdp_get_hscroll_base() + 2, h_scroll_buffer, sizeof(h_scroll_buffer) / 2, 32);
	system_profile(PALRGB(0, 0, 0));
}

void o_load_bg(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Bg) <= sizeof(ObjSlot));
	(void)data;

	// Only allow one BG object to be loaded.
	SYSTEM_ASSERT(!loaded);
	if (loaded)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}

	O_Bg *f = (O_Bg *)o;

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE, 0, 0, 0, 1);
	o->main_func = main_func;
	o->cube_func = NULL;

	f->bg_id = map_get_background();
	const BgDescriptor *b = &backgrounds[f->bg_id];

	// Load palette.
	pal_upload(BG_COMMON_CRAM_POSITION, res_pal_bg_common_bin,
	           sizeof(res_pal_bg_common_bin) / 2);
	// Load graphics tiles.
	if (b->gfx_id != GFX_NULL)
	{
		pal_upload(BG_CRAM_POSITION, b->palette, 8);
		const Gfx *g = gfx_get(b->gfx_id);
		SYSTEM_ASSERT(g->size <= BG_TILE_VRAM_LENGTH);
		gfx_load(g, BG_TILE_VRAM_POSITION);
		SYSTEM_ASSERT(b->mapping);
		dma_q_transfer_vram(vdp_get_plane_base(VDP_PLANE_B), b->mapping, 64 * 32, 2);
		SYSTEM_ASSERT(b->palette);
	}
	else
	{
		pal_upload(BG_CRAM_POSITION, res_pal_bg_bg0_bin, 8);
		dma_q_fill_vram(BG_TILE_VRAM_POSITION, 0, 64 * 32 * 2, 1);
		dma_q_fill_vram(vdp_get_plane_base(VDP_PLANE_B), 0, 64 * 32, 1);
	}

	// Set scroll.
	f->scroll_mode = b->scroll;
	set_h_scroll_plane(0);
	set_v_scroll_plane(0);
	dma_q_transfer_vsram(2, v_scroll_buffer, sizeof(v_scroll_buffer) / 2, 4);
	dma_q_transfer_vram(vdp_get_hscroll_base() + 2, h_scroll_buffer, sizeof(h_scroll_buffer) / 2, 4);

	loaded = 1;
}

void o_unload_bg(void)
{
	loaded = 0;
}
