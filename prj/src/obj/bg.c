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

// 4K of scratch memory, used for software background tiling.
static uint8_t scratch[64 * 64 / 2];

// Some hacky drawing operations for scratch memory.
static inline void scratch_clear(void)
{
	uint16_t i = ARRAYSIZE(scratch);
	while (i--) scratch[i] = 0;
}

static inline int16_t scratch_index(int16_t x, int16_t y)
{
	const int16_t coarse_x = (x / 4) * 32;
	const int16_t coarse_y = (y / 8) * 256;
	const int16_t index = (coarse_x + coarse_y) + (x % 4) + (y % 8) * 4;
	return index;
}

static inline uint8_t scratch_fetch(int16_t x, int16_t y)
{
	x = x % 32;
	y = y % 64;
	return scratch[scratch_index(x, y)];
}

static inline void scratch_plot(int16_t x, int16_t y, uint8_t value)
{
	const int16_t idx_x = x % 32;
	const int16_t idx_y = y % 64;
	scratch[scratch_index(idx_x, idx_y)] = value;
}

static inline void scratch_plot_or(int16_t x, int16_t y, uint8_t value)
{
	scratch_plot(x, y, scratch_fetch(x, y) | value);
}

static inline void scratch_or_line_right(int16_t x, int16_t y, uint8_t value, uint8_t len)
{
	x = x % 64;
	y = y % 64;
	while (len--)
	{
		const int16_t coarse_x = (x / 8) * 32;
		const int16_t coarse_y = (y / 8) * 256;
		const int16_t index = (coarse_x + coarse_y) + ((x / 2) % 4) + (y % 8) * 4;

		if (len > 2 && x % 2 == 0)
		{
			scratch[index] |= value;
			x++;
			len--;
		}
		else if (x % 2 == 0)
		{
			scratch[index] |= (value & 0xF0);
		}
		else
		{
			scratch[index] |= (value & 0x0F);
		}

		x++;
		x = x % 64;
	}
}

static inline void scratch_clear_line_right(int16_t x, int16_t y, uint8_t len)
{
	x = x % 64;
	y = y % 64;
	while (len--)
	{
		const int16_t coarse_x = (x / 8) * 32;
		const int16_t coarse_y = (y / 8) * 256;
		const int16_t index = (coarse_x + coarse_y) + ((x / 2) % 4) + (y % 8) * 4;

		scratch[index] = 0;

		x++;
		x = x % 64;
	}
}

static inline void scratch_clear_line_down(int16_t x, int16_t y, uint8_t len)
{
	x = x % 64;
	y = y % 64;
	while (len--)
	{
		const int16_t coarse_x = (x / 8) * 32;
		const int16_t coarse_y = (y / 8) * 256;
		const int16_t index = (coarse_x + coarse_y) + ((x / 2) % 4) + (y % 8) * 4;

		scratch[index] = 0;

		y++;
		y = y % 64;
	}
}

static inline void scratch_or_line_down(int16_t x, int16_t y, uint8_t value, uint8_t len)
{
	x = x % 64;
	y = y % 64;
	while (len--)
	{
		const int16_t coarse_x = (x / 8) * 32;
		const int16_t coarse_y = (y / 8) * 256;
		const int16_t index = (coarse_x + coarse_y) + ((x / 2) % 4) + (y % 8) * 4;

		if (x % 2 == 0) scratch[index] |= (value & 0xF0);
		else scratch[index] |= (value & 0x0F);

		y++;
		y = y % 64;
	}
}

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
	[12] = {GFX_BG_12, res_pal_bg_bg10_bin, res_bgmap_bg12_bin, BG_SCROLL_H_CELL},  // Mapping modification of 10.
	[13] = {GFX_BG_13, res_pal_bg_bg13_bin, res_bgmap_bg13_bin, BG_SCROLL_V_CELL},
	[14] = {GFX_BG_14, res_pal_bg_bg13_bin, res_bgmap_bg13_bin, BG_SCROLL_V_CELL},
	[15] = {GFX_BG_15, res_pal_bg_bg15_bin, res_bgmap_bg15_bin, BG_SCROLL_H_CELL},
	[19] = {GFX_NULL, res_pal_bg_bg19_bin, res_bgmap_bg19_bin, BG_SCROLL_PLANE},

	[20] = {0}
};

static void bg_city_func(int16_t x_scroll, int16_t y_scroll)
{
	(void)y_scroll;
	const fix32_t x_fixed = INTTOFIX32(-x_scroll);
	const int16_t close_x = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.666666667)));
	set_v_scroll_plane(system_is_ntsc() ? 8 : 0);
	set_h_scroll_plane(-x_scroll / 2);

	for (uint16_t i = system_is_ntsc() ? 20 : 21; i < 25; i++)
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
	(void)x_scroll;
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

typedef struct LineInstruction
{
	int8_t x1, y1, x2, y2;
} LineInstruction;

const LineInstruction blue_line_instructions[] =
{
	{0, 56, 30, 56},
	{52, 56, 63, 56},
	{30, 41, 30, 56},
	{21, 41, 30, 41},
	{21, 22, 21, 41},
	{22, 22, 52, 22},
	{52, 57, 52, 85}
};

// Woooow this one's pretty ugly
// On the other hand, it's working well enough.
// TODO: Clean up and profile
static void bg_undersand_columns_func(int16_t x_scroll, int16_t y_scroll)
{
	(void)y_scroll;
	const Gfx *g = gfx_get(GFX_BG_9);
	const uint8_t *bmp = (const uint8_t *)g->data;
	set_h_scroll_plane(-x_scroll/2);

	const uint16_t scroll_off = ((uint8_t)(x_scroll) / 4) % 64;
	const uint16_t data_off = scroll_off % 2 ? 64 : 0;

	// The purple parts in the back.
	for (int16_t i = 0; i < 2; i++)
	{
		const uint16_t base_x = i * 32;
		for (int16_t y = 0; y < 8; y++)
		{
			for (int16_t x = 0; x < 4; x++)
			{
				scratch_plot((base_x + scroll_off) / 2 + x, y, bmp[(4 * y) + (10 * 32) + x + data_off]);
				scratch_plot((base_x + scroll_off) / 2 + x, y + 8, bmp[(4 * y) + (11 * 32) + x + data_off]);
			}
		}
	}

	// For transparent parts that don't align with even pixels, some bit fussing
	// is needed so as to support transparency properly.
	for (int16_t y = 5; y < 8; y++)
	{
		scratch_plot(0, y, (scratch_fetch(0, y) & 0xF0) | bmp[(4 * y) + (2 * 32)]);
	}
	for (int16_t y = 8; y < 11; y++)
	{
		scratch_plot(0, y, (scratch_fetch(0, y) & 0xF0) | bmp[(4 * (y % 8)) + (4 * 32)]);
	}
	for (int16_t y = 0; y < 3; y++)
	{
		scratch_plot(4, y, (scratch_fetch(4, y) & 0x0F) | bmp[(4 * y) + (3 * 32)]);
	}
	for (int16_t y = 9; y < 16; y++)
	{
		scratch_plot(4, y, (scratch_fetch(4, y) & 0x0F) | bmp[(4 * (y % 8)) + (5 * 32)]);
	}

	// The fixed green parts in front.
	for (int16_t y = 0; y < 8; y++)
	{
		for (int16_t x = 1; x < 4; x++)
		{
			scratch_plot(x, y, bmp[(4 * y) + (2 * 32) + x]);
			scratch_plot(x, y + 8, bmp[(4 * y) + (4 * 32) + x]);
		}
	}

	// Little left-side details not included in the main loop.
	for (int16_t y = 0; y < 5; y++)
	{
		scratch_plot(0, y, bmp[(4 * y) + (2 * 32)]);
	}
	for (int16_t y = 11; y < 16; y++)
	{
		scratch_plot(0, y, bmp[(4 * (y % 8)) + (4 * 32)]);
	}

	// Some right-side details.
	for (int16_t y = 3; y < 8; y++)
	{
		scratch_plot(4, y, bmp[(4 * (y % 8)) + (3 * 32)]);
	}
	scratch_plot(4, 8, bmp[(5 * 32)]);

	dma_q_transfer_vram(BG_TILE_VRAM_POSITION, scratch, (sizeof(scratch) / 4) / 2, 2);
}

static inline void technozone_purple_overlay(uint8_t half)
{
	const uint8_t purple_full = 0x33;
	const uint8_t purple_left = 0x35;
	const uint8_t purple_right = 0x53;
	if (half == 0)
	{
		for (int16_t i = 0; i < 16; i++)
		{
			scratch_plot(i, 6, purple_full);
			scratch_plot(i, 7, purple_left);
		}
		scratch_plot(16, 6, purple_full);
		scratch_plot(16, 7, purple_full);

		for (int16_t i = 8; i < 32; i++)
		{
			scratch_plot(16, i, (i % 2 ? purple_full : purple_right));
		}

		for (int16_t i = 30; i < 40; i++)
		{
			scratch_plot(9, i, (i % 2 ? purple_full : purple_right));
		}

		for (int16_t i = 26; i < 32; i++)
		{
			scratch_plot(i, 6, purple_full);
			scratch_plot(i, 7, purple_left);
		}
	}
	else
	{
		for (int16_t i = 9; i < 17; i++)
		{
			scratch_plot(i, 30, purple_right);
			scratch_plot(i, 31, purple_full);
		}

		for (int16_t i = 9; i < 26; i++)
		{
			scratch_plot(i, 40, purple_full);
			scratch_plot(i, 41, purple_left);
		}
		scratch_plot(9, 40, purple_right);

		scratch_plot(26, 40, purple_full);
		for (int16_t i = 41; i < 71; i++)
		{
			scratch_plot(26, i, (i % 2 ? purple_full : purple_right));
		}

		scratch_plot(26, 71, purple_left);
	}
}

static void technozone_horizontal(int16_t x_scroll)
{
	static int16_t old_x;
	const int16_t x_off = x_scroll / 8;

	if (g_elapsed % 2 == 0 || g_elapsed < 2)
	{
		// Erase the blue pattern from before.
		for (uint16_t i = 0; i < ARRAYSIZE(blue_line_instructions); i++)
		{
			const LineInstruction *l = &blue_line_instructions[i];
			if (l->x1 == l->x2) scratch_clear_line_down(old_x + l->x1, l->y1, (l->y2 - l->y1 + 1));
			else
			{
				scratch_plot((old_x + l->x1) / 2, l->y1, 0);
				scratch_plot((old_x + l->x2) / 2, l->y1, 0);
			}
		}
		technozone_purple_overlay(0);
	}
	if (g_elapsed % 2 == 1 || g_elapsed < 2)
	{
		technozone_purple_overlay(1);

		for (uint16_t i = 0; i < ARRAYSIZE(blue_line_instructions); i++)
		{
			const LineInstruction *l = &blue_line_instructions[i];
			if (l->x1 == l->x2) scratch_or_line_down(x_off + l->x1, l->y1, 0x11, (l->y2 - l->y1 + 1));
			else scratch_or_line_right(x_off + l->x1, l->y1, 0x11, (l->x2 - l->x1 + 1));
		}

		dma_q_transfer_vram(BG_TILE_VRAM_POSITION, scratch, sizeof(scratch) / 2, 2);
		old_x = x_off;
	}
}

static void technozone_vertical(int16_t y_scroll)
{
	static int16_t old_y;
	const int16_t y_off = (y_scroll / 8);
	if (g_elapsed % 2 == 0 || g_elapsed < 2)
	{
		// Erase the blue pattern from before.
		for (int16_t i = 0; i < ARRAYSIZE(blue_line_instructions); i++)
		{
			const LineInstruction *l = &blue_line_instructions[i];
			if (l->x1 == l->x2) scratch_clear_line_down(16 + l->x1, y_off + l->y1 - 6, (l->y2 - l->y1 + 1) + 6);
			else scratch_clear_line_right(16 + l->x1, old_y + l->y1, (l->x2 - l->x1 + 1));
		}
		technozone_purple_overlay(0);
	}
	if (g_elapsed % 2 == 1 || g_elapsed < 2)
	{
		technozone_purple_overlay(1);

		for (int16_t i = 0; i < ARRAYSIZE(blue_line_instructions); i++)
		{
			const LineInstruction *l = &blue_line_instructions[i];
			if (l->x1 == l->x2) scratch_or_line_down(16 + l->x1, y_off + l->y1, 0x11, (l->y2 - l->y1 + 1));
			else scratch_or_line_right(16 + l->x1, y_off + l->y1, 0x11, (l->x2 - l->x1 + 1));
		}

		dma_q_transfer_vram(BG_TILE_VRAM_POSITION, scratch, sizeof(scratch) / 2, 2);
		old_y = y_off;
	}
}

// Technozone is an odd and ugly duck. Performance is tight, and this C isn't
// particularly well-optimized. Here's how it works:
// * A repeating 8x8 tile pattern is baked into the Plane B mappings.
// * The purple "foreground" of Plane B is drawn statically, without any
//   variation in positioning. This is split into two halves to let it happen
//   across two frames, and is done via technozone_purple_overlay()./
// * The indigo "background" lines of Plane B are drawn with dynamic
//   positioning. Since Technozone only has horizontal and vertical rooms, but
//   none that are both, there are separate horizontal and vertical rendering
//   functions in order to let the compiler inline a constant for one of the
//   offsets.
//   These lines are drawn with an OR operation. This way, it is not necessary
//   to do an opacity test when drawing. The purple lines use index 0x03 while
//   the indigo ones use 0x01. The black parts of the purple line are 0x05. As
//   a result, no change occurs when the indigo lines overlap the foreground.
// * Before all of the above occurs, the dynamic indigo lines from the prior
//   frame are erased from the scratch memory. The previous X or Y offset is
//   stored in a static variable.
//   Since movement only occurs in one direction, only the tail ends of lines
//   parallel to movement are erased so as to save time hitting scratch memory.
// * The entirety of the scratch framebuffer is queued for a VRAM transfer.
static void bg_technozone_func(int16_t x_scroll, int16_t y_scroll)
{
	if (map_get_bottom() == INTTOFIX32(240)) technozone_horizontal(x_scroll);
	else technozone_vertical(y_scroll);

	set_h_scroll_plane(-x_scroll/2);
	set_v_scroll_plane(y_scroll / 2);

	return;
}

static void bg_crazy_city_func(int16_t x_scroll, int16_t y_scroll)
{
	(void)y_scroll;
	const fix32_t x_fixed = INTTOFIX32(-x_scroll);
	const int16_t purple_x = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.666666667)));
	set_v_scroll_plane(0);

	set_h_scroll_plane(-x_scroll / 2);

	for (uint16_t i = 12; i < 12+5; i++)
	{
		h_scroll_buffer[i] = purple_x;
	}
}

static void bg_crazy_city_low_func(int16_t x_scroll, int16_t y_scroll)
{
	(void)y_scroll;
	const fix32_t x_fixed = INTTOFIX32(-x_scroll);
	const int16_t purple_x = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.666666667)));
	set_v_scroll_plane(0);

	set_h_scroll_plane(-x_scroll / 2);

	for (uint16_t i = 18; i < 18+5; i++)
	{
		h_scroll_buffer[i] = purple_x;
	}
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
	[9] = bg_undersand_columns_func,
	[10] = bg_crazy_city_func,
	[12] = bg_crazy_city_low_func,
	[19] = bg_technozone_func,
};

static void main_func(Obj *o)
{
	system_profile(PALRGB(0, 4, 0));
	O_Bg *f = (O_Bg *)o;

	const int16_t x_scroll = map_get_x_scroll();
	const int16_t y_scroll = map_get_y_scroll();

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

	scratch_clear();

	f->bg_id = map_get_background();
	const BgDescriptor *b = &backgrounds[f->bg_id];

	// Load palette.
	pal_upload(BG_COMMON_CRAM_POSITION, res_pal_bg_common_bin,
	           sizeof(res_pal_bg_common_bin) / 2);
	// Load graphics tiles.

	if (b->palette) pal_upload(BG_CRAM_POSITION, b->palette, 8);
	else pal_upload(BG_CRAM_POSITION, res_pal_bg_bg0_bin, 8);

	if (b->gfx_id != GFX_NULL) gfx_load(gfx_get(b->gfx_id), BG_TILE_VRAM_POSITION);
	else dma_q_fill_vram(BG_TILE_VRAM_POSITION, 0, 64 * 32 * 2, 1);

	if (b->mapping) dma_q_transfer_vram(vdp_get_plane_base(VDP_PLANE_B), b->mapping, 64 * 32, 2);
	else dma_q_fill_vram(vdp_get_plane_base(VDP_PLANE_B), 0, 64 * 32, 1);

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