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

static uint16_t s_loaded;

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
	uint16_t mapping_size;
} BgDescriptor;

static const BgDescriptor backgrounds[] =
{
	[0] = {GFX_NULL, res_pal_bg_bg0_bin, NULL, 0},
	[1] = {GFX_BG_1, res_pal_bg_bg1_bin, res_bgmap_bg1_bin, sizeof(res_bgmap_bg1_bin)},
	[2] = {GFX_BG_2, res_pal_bg_bg2_bin, res_bgmap_bg2_bin, sizeof(res_bgmap_bg2_bin)},
	[3] = {GFX_BG_3, res_pal_bg_bg3_bin, res_bgmap_bg3_bin, sizeof(res_bgmap_bg3_bin)},
	[4] = {GFX_BG_4, res_pal_bg_bg4_bin, res_bgmap_bg4_bin, sizeof(res_bgmap_bg4_bin)},
	[5] = {GFX_BG_5, res_pal_bg_bg5_bin, res_bgmap_bg5_bin, sizeof(res_bgmap_bg5_bin)},
	[6] = {GFX_BG_6, res_pal_bg_bg1_bin, res_bgmap_bg6_bin, sizeof(res_bgmap_bg6_bin)},  // Mapping modification of 1.
	[7] = {GFX_BG_7, res_pal_bg_bg7_bin, res_bgmap_bg7_bin, sizeof(res_bgmap_bg7_bin)},
//	[8] = {GFX_BG_8, res_pal_bg_bg8_bin, res_bgmap_bg8_bin, sizeof(res_bgmap_bg8_bin)},
	[9] = {GFX_BG_9, res_pal_bg_bg9_bin, res_bgmap_bg9_bin, sizeof(res_bgmap_bg9_bin)},
	[10] = {GFX_BG_10, res_pal_bg_bg10_bin, res_bgmap_bg10_bin, sizeof(res_bgmap_bg10_bin)},
	[11] = {GFX_BG_11, res_pal_bg_bg11_bin, res_bgmap_bg11_bin, sizeof(res_bgmap_bg11_bin)},
	[12] = {GFX_BG_12, res_pal_bg_bg10_bin, res_bgmap_bg12_bin, sizeof(res_bgmap_bg12_bin)},  // Mapping modification of 10.
	[13] = {GFX_BG_13, res_pal_bg_bg13_bin, res_bgmap_bg13_bin, sizeof(res_bgmap_bg13_bin)},
	[14] = {GFX_BG_14, res_pal_bg_bg14_bin, res_bgmap_bg13_bin, sizeof(res_bgmap_bg13_bin)},  // Palette modification of 13.
	[15] = {GFX_BG_15, res_pal_bg_bg15_bin, res_bgmap_bg15_bin, sizeof(res_bgmap_bg15_bin)},
	[16] = {GFX_BG_16, res_pal_bg_bg16_bin, res_bgmap_bg16_bin, sizeof(res_bgmap_bg16_bin)},
	[17] = {GFX_BG_2, res_pal_bg_bg2_bin, res_bgmap_bg2_bin, sizeof(res_bgmap_bg2_bin)},
	[18] = {GFX_BG_18, res_pal_bg_bg18_bin, res_bgmap_bg18_bin, sizeof(res_bgmap_bg18_bin)},
	[19] = {GFX_NULL, res_pal_bg_bg19_bin, res_bgmap_bg19_bin, sizeof(res_bgmap_bg19_bin)},
	[20] = {GFX_BG_9, res_pal_bg_bg9_bin, res_bgmap_bg9_bin, sizeof(res_bgmap_bg9_bin)},
	[21] = {GFX_BG_16, res_pal_bg_bg16_bin, res_bgmap_bg21_bin, sizeof(res_bgmap_bg21_bin)},  // Mapping modification of 16.
	[22] = {GFX_BG_22, res_pal_bg_bg22_bin, res_bgmap_bg22_bin, sizeof(res_bgmap_bg22_bin)},
	[23] = {GFX_BG_23, res_pal_bg_bg23_bin, res_bgmap_bg23_bin, sizeof(res_bgmap_bg23_bin)},
	[24] = {GFX_BG_24, res_pal_bg_bg24_bin, res_bgmap_bg24_bin, sizeof(res_bgmap_bg24_bin)},
	[25] = {GFX_BG_24, res_pal_bg_bg24_bin, res_bgmap_bg24_bin, sizeof(res_bgmap_bg24_bin)},  // Same as 24, but vertical.
	[26] = {0}
};

static void bg_city_func(int16_t x_scroll, int16_t y_scroll)
{
	const int16_t x_offset = -38;
	const fix32_t x_fixed = INTTOFIX32(-x_scroll + x_offset);
	const int16_t close_x = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.666666667)));
	if (map_get_bottom() == INTTOFIX32(240)) y_scroll -= 32;
	set_v_scroll_plane(y_scroll + 24);
	set_h_scroll_plane(-x_scroll / 2);

	// Hack to force plane scroll when the scroll position is far above, for
	// the sake of the title screen.
	if (y_scroll <= 495 - (system_is_ntsc() ? 0 : 16)) return;

	const int16_t start_row = 20 + (system_is_ntsc() ? 0 : 2);

	for (uint16_t i = start_row; i < start_row + 5; i++)
	{
		h_scroll_buffer[i] = close_x;
	}
}

static void bg_city_red_func(int16_t x_scroll, int16_t y_scroll)
{
	const int16_t x_offset = -38;
	const fix32_t x_fixed = INTTOFIX32(-x_scroll + x_offset);
	const int16_t close_x = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.666666667)));
	set_v_scroll_plane(y_scroll - 8);
	set_h_scroll_plane(-x_scroll / 2);

	const int16_t start_row = 21 + (system_is_ntsc() ? 0 : 1);

	for (uint16_t i = start_row; i < start_row + 5; i++)
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

static void bg_plane_func_offset(int16_t x_scroll, int16_t y_scroll)
{
	y_scroll -= 112;
	const fix32_t x_fixed = INTTOFIX32(-x_scroll);
	const int16_t far_x = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.666666667)));
	set_h_scroll_plane(far_x);

	// If it's a single-screen height room, hike the chevrons upwards.
	if (map_get_bottom() == INTTOFIX32(240)) y_scroll -= 384;

	set_v_scroll_plane(y_scroll/2);
}


static void bg_blue_bumps_func(int16_t x_scroll, int16_t y_scroll)
{
	const Gfx *g = gfx_get(GFX_BG_3);
	set_h_scroll_plane(x_scroll);
	v_scroll_buffer[0] = y_scroll / 2;
	v_scroll_buffer[1] = y_scroll / 2;
	v_scroll_buffer[2] = y_scroll / 2;
	v_scroll_buffer[3] = y_scroll / 8;

	v_scroll_buffer[19] = y_scroll / 2;
	v_scroll_buffer[18] = y_scroll / 2;
	v_scroll_buffer[17] = y_scroll / 2;
	v_scroll_buffer[16] = y_scroll / 8;

	const uint8_t scroll_off = ((uint8_t)(y_scroll) / 4) % 16;

	dma_q_transfer_vram(BG_TILE_VRAM_POSITION + (12 * 32), g->data + (16 * 32) + (128 * scroll_off), 32 * 4 / 2, 2);
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
	const Gfx *g = gfx_get(GFX_BG_7_EX);

	const fix32_t x_fixed = INTTOFIX32(-x_scroll);
	const int16_t x_front_scroll = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.6666666667))) % 48;
	const int16_t x_counter_index = 47 - (FIX32TOINT(FIX32MUL(-x_fixed, INTTOFIX32(0.22222222221))) % 48);

	set_v_scroll_plane(y_scroll);
	set_h_scroll_plane(x_front_scroll);

	dma_q_transfer_vram(BG_TILE_VRAM_POSITION, g->data + (6 * 6 * 32 * x_counter_index), (6 * 6 * 32) / 2, 2);
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

static void undersand_purple_columns(int16_t x_scroll)
{
	const Gfx *g = gfx_get(GFX_BG_9);
	const uint8_t *bmp = (const uint8_t *)g->data;
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
}

static void undersand_green_columns(void)
{
	const Gfx *g = gfx_get(GFX_BG_9);
	const uint8_t *bmp = (const uint8_t *)g->data;
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
}

// Woooow this one's pretty ugly
// On the other hand, it's working well enough.
// TODO: Clean up and profile
static void bg_undersand_columns_func(int16_t x_scroll, int16_t y_scroll)
{
	set_v_scroll_plane(y_scroll);

	undersand_purple_columns(x_scroll);
	undersand_green_columns();

	set_h_scroll_plane(-x_scroll/2);
	dma_q_transfer_vram(BG_TILE_VRAM_POSITION, scratch, (sizeof(scratch) / 4) / 2, 2);
}

static void bg_columns_2_func(int16_t x_scroll, int16_t y_scroll)
{
	set_v_scroll_plane(y_scroll);

	undersand_purple_columns(x_scroll);

	set_h_scroll_plane(-x_scroll/2);
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
	static int16_t s_old_x;
	const int16_t x_off = x_scroll / 8;

	if (g_elapsed % 2 == 0 || g_elapsed < 2)
	{
		// Erase the blue pattern from before.
		for (uint16_t i = 0; i < ARRAYSIZE(blue_line_instructions); i++)
		{
			const LineInstruction *l = &blue_line_instructions[i];
			if (l->x1 == l->x2) scratch_clear_line_down(s_old_x + l->x1, l->y1, (l->y2 - l->y1 + 1));
			else
			{
				scratch_plot((s_old_x + l->x1) / 2, l->y1, 0);
				scratch_plot((s_old_x + l->x2) / 2, l->y1, 0);
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
		s_old_x = x_off;
	}
}

static void technozone_vertical(int16_t y_scroll)
{
	static int16_t s_old_y;
	const int16_t y_off = (y_scroll / 8);
	if (g_elapsed % 2 == 0 || g_elapsed < 2)
	{
		// Erase the blue pattern from before.
		for (uint16_t i = 0; i < ARRAYSIZE(blue_line_instructions); i++)
		{
			const LineInstruction *l = &blue_line_instructions[i];
			if (l->x1 == l->x2) scratch_clear_line_down(16 + l->x1, y_off + l->y1 - 6, (l->y2 - l->y1 + 1) + 6);
			else scratch_clear_line_right(16 + l->x1, s_old_y + l->y1, (l->x2 - l->x1 + 1));
		}
		technozone_purple_overlay(0);
	}
	if (g_elapsed % 2 == 1 || g_elapsed < 2)
	{
		technozone_purple_overlay(1);

		for (uint16_t i = 0; i < ARRAYSIZE(blue_line_instructions); i++)
		{
			const LineInstruction *l = &blue_line_instructions[i];
			if (l->x1 == l->x2) scratch_or_line_down(16 + l->x1, y_off + l->y1, 0x11, (l->y2 - l->y1 + 1));
			else scratch_or_line_right(16 + l->x1, y_off + l->y1, 0x11, (l->x2 - l->x1 + 1));
		}

		dma_q_transfer_vram(BG_TILE_VRAM_POSITION, scratch, sizeof(scratch) / 2, 2);
		s_old_y = y_off;
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
	const fix32_t x_fixed = INTTOFIX32(-x_scroll);
	const int16_t purple_x = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.666666667)));
	set_v_scroll_plane(y_scroll);

	const int16_t split = 11 + system_is_ntsc() ? 0 : 2;

	set_h_scroll_plane(-x_scroll / 2);

	for (uint16_t i = split; i < split+5; i++)
	{
		h_scroll_buffer[i] = purple_x;
	}
}

static void bg_crazy_city_low_func(int16_t x_scroll, int16_t y_scroll)
{
	const fix32_t x_fixed = INTTOFIX32(-x_scroll);
	const int16_t purple_x = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.666666667)));
	set_v_scroll_plane(y_scroll);

	const int16_t split = 18 + system_is_ntsc() ? 0 : 2;

	set_h_scroll_plane(-x_scroll / 2);

	for (uint16_t i = split; i < split+5; i++)
	{
		h_scroll_buffer[i] = purple_x;
	}
}

static void bg_elevator_func(int16_t x_scroll, int16_t y_scroll)
{
	(void)x_scroll;
	const int16_t far_y = y_scroll / 2;  // far
	const int16_t close_y = FIX32TOINT(FIX32MUL(INTTOFIX32(y_scroll), INTTOFIX32(0.83333334)));  // close
	const int16_t med_y = FIX32TOINT(FIX32MUL(INTTOFIX32(y_scroll), INTTOFIX32(0.66666667)));  // med
	const int16_t red_y = y_scroll / 8;

	v_scroll_buffer[0] = close_y;
	v_scroll_buffer[1] = close_y;
	v_scroll_buffer[2] = med_y;
	v_scroll_buffer[3] = far_y;

	v_scroll_buffer[9] = red_y;
	v_scroll_buffer[10] = red_y;

	v_scroll_buffer[16] = far_y;
	v_scroll_buffer[17] = med_y;
	v_scroll_buffer[18] = close_y;
	v_scroll_buffer[19] = close_y;
}

static void bg_brown_grass_func(int16_t x_scroll, int16_t y_scroll)
{
	const Gfx *g = gfx_get(GFX_BG_16_EX);

	const fix32_t x_fixed = INTTOFIX32(-x_scroll);
	const int16_t x_squiggle_scroll = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.3333333334)));
	const int16_t x_front_scroll_raw = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.6666666667)));
	const int16_t x_front_scroll = x_front_scroll_raw % 24;
	const int16_t x_counter_index = 23 - (FIX32TOINT(FIX32MUL(-x_fixed, INTTOFIX32(0.22222222221))) % 24);

	set_v_scroll_plane(y_scroll);
	set_h_scroll_plane(x_front_scroll);
	for (int16_t i = 0; i < 4; i++)
	{
		h_scroll_buffer[(system_is_ntsc() ? 13 : 14) + i] = x_squiggle_scroll;
	}

	for (uint16_t i = 24; i < ARRAYSIZE(h_scroll_buffer); i++)
	{
		h_scroll_buffer[i] = x_front_scroll_raw;
	}

	dma_q_transfer_vram(BG_TILE_VRAM_POSITION + (3 * 32), g->data + (6 * 3 * 32 * x_counter_index), (32 * 6 * 3) / 2, 2);
}

// Simple far purple scrolling 48x48 tile BG, as a companio to the FG tile
// substitution front layer (horizontal ver.)
static void bg_technozone_horizontal_simple_func(int16_t x_scroll, int16_t y_scroll)
{
	const fix32_t x_fixed = INTTOFIX32(-x_scroll);
	const int16_t x_front_scroll_raw = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.333333334)));
	const int16_t x_front_scroll = x_front_scroll_raw % 48;

	set_v_scroll_plane(y_scroll);
	set_h_scroll_plane(x_front_scroll);
}

// Simple far purple scrolling 48x48 tile BG, as a companio to the FG tile
// substitution front layer (vertical ver.)
static void bg_technozone_vertical_simple_func(int16_t x_scroll, int16_t y_scroll)
{
	const fix32_t y_fixed = INTTOFIX32(-y_scroll);
	const int16_t y_front_scroll_raw = FIX32TOINT(FIX32MUL(y_fixed, INTTOFIX32(0.333333334)));
	const int16_t y_front_scroll = 47 - (y_front_scroll_raw % 48);

	set_h_scroll_plane(x_scroll);
	set_v_scroll_plane(y_front_scroll);
}

static void bg_static_func(int16_t x_scroll, int16_t y_scroll)
{
	(void)x_scroll;
	(void)y_scroll;
	set_v_scroll_plane(0);
	set_h_scroll_plane(0);
}

static void bg_guantlet_func(int16_t x_scroll, int16_t y_scroll)
{
/*

00	em
01	em
02	em
03	em
04	em
05	em
06	8f 0x3130  plane 1/3
07	7f 0x312A  plane 1/3
08	6f 0x3124  plane 1/3
09	5f 0x311E  plane 1/3
10	4f 0x3118  plane 1/3, dma 1/2
11	3f 0x3112  plane 1/3, dma 1/2
12	9f 0x3136  plane 1/2, +8 fixed
13	2f 0x310C  plane 1/2, dma 1/1.5
14	0  0x2100  plane 1/1.5, dma 1/1.2
15	1  0x2106  plane 1/1.5, dma 1/1.2
16	2  0x210C  plane 1/2, dma 1/1.5
17	9  0x2136  plane 1/2, +8 fixed
18	3  0x2112  plane 1/3, dma 1/2
19	4  0x2118  plane 1/3, dma 1/2
20	5  0x211E  plane 1/3
21	6  0x2124  plane 1/3
22	7  0x212A  plane 1/3
23	8  0x3130  plane 1/3
24	em
25	em
26	em
27	em
28	em
29	em

30	em
31	em



*/
	(void)y_scroll;
	set_v_scroll_plane((system_is_ntsc() ? 8 : 0));

	static const int16_t modulo = 48;

	const fix32_t x_fixed = INTTOFIX32(x_scroll);
	const int16_t x_1_3_raw = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(1.0 - 0.333333)));
	const int16_t x_1_3 = -(modulo - 1) - (x_1_3_raw % modulo);
	const int16_t x_1_2 = -(modulo - 1) - ((x_scroll / 2) % modulo);
	const int16_t x_1_1_5 = -(modulo - 1) - ((x_1_3_raw / 2) % modulo);
	const int16_t x_1_1_2_raw = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(1.0 - 0.8928571428)));
	const int16_t x_1_1_2 = -(modulo - 1) - (x_1_1_2_raw % modulo);

	int16_t *buffer = (system_is_ntsc() ? &h_scroll_buffer[-1] : &h_scroll_buffer[-0]);

	buffer[6] = x_1_3;
	buffer[7] = x_1_3;
	buffer[8] = x_1_3;
	buffer[9] = x_1_3;
	buffer[10] = x_1_3;
	buffer[11] = x_1_3;
	buffer[12] = x_1_2 + 8;
	buffer[13] = x_1_2;
	buffer[14] = x_1_1_5;
	buffer[15] = x_1_1_5;
	buffer[16] = x_1_2;
	buffer[17] = x_1_2 + 8;
	buffer[18] = x_1_3;
	buffer[19] = x_1_3;
	buffer[20] = x_1_3;
	buffer[21] = x_1_3;
	buffer[22] = x_1_3;
	buffer[23] = x_1_3;

	const int16_t dma1_index = (modulo + x_1_3 - x_1_2) % modulo;
	const Gfx *g1 = gfx_get(GFX_BG_23_EX);
	dma_q_transfer_vram(BG_TILE_VRAM_POSITION + (3 * 6 * 32),
	                    g1->data + (6 * 3 * 32) + (dma1_index * (32 * 6 * 5)),
	                    (32 * 6 * 2) / 2, 2);

	const int16_t dma2_index = (modulo + x_1_2 - x_1_1_5) % modulo;
	const Gfx *g2 = gfx_get(GFX_BG_23_EX);
	dma_q_transfer_vram(BG_TILE_VRAM_POSITION + (2 * 6 * 32),
	                    g2->data + (6 * 2 * 32) + (dma2_index * (32 * 6 * 5)),
	                    (32 * 6 * 1) / 2, 2);

	const int16_t dma3_index = (modulo + x_1_1_5 - x_1_1_2) % modulo;
	const Gfx *g3 = gfx_get(GFX_BG_23_EX);
	dma_q_transfer_vram(BG_TILE_VRAM_POSITION,
	                    g3->data + (dma3_index * (32 * 6 * 5)),
	                    (32 * 6 * 2) / 2, 2);
}

static void bg_longsand_func(int16_t x_scroll, int16_t y_scroll)
{
	set_v_scroll_plane(y_scroll);
	const fix32_t x_fixed = INTTOFIX32(x_scroll);
	const int16_t x_1_1_3_raw = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.23079)));
	const int16_t x_1_1_3 = -x_1_1_3_raw;
	const int16_t x_1_1_5_raw = FIX32TOINT(FIX32MUL(x_fixed, INTTOFIX32(0.333333)));
	const int16_t x_1_1_5 = -x_1_1_5_raw;
	const int16_t x_1_2 = -(x_scroll / 2);

	int16_t *buffer = &h_scroll_buffer[(-y_scroll / 8)];

	buffer[12] = x_1_1_3;
	buffer[13] = x_1_1_3;
	buffer[14] = x_1_1_3;
	buffer[15] = x_1_1_3;
	buffer[16] = x_1_1_5;
	buffer[17] = x_1_1_5;
	buffer[18] = x_1_2;
	buffer[19] = x_1_2;
	buffer[20] = x_1_2;
	buffer[21] = x_1_2;
}

static void bg_thin_bricks_func(int16_t x_scroll, int16_t y_scroll)
{
	set_v_scroll_plane(y_scroll / 2);
	set_h_scroll_plane(-x_scroll / 2);
}

static void (*bg_funcs[])(int16_t, int16_t) =
{
	[0] = NULL,
	[1] = bg_city_func,
	[2] = bg_plane_func,
	[3] = bg_blue_bumps_func,
	[4] = bg_bubbles_func,
	[5] = bg_plane_func,
	[6] = bg_city_red_func,
	[7] = bg_orange_balls_func,
	[9] = bg_undersand_columns_func,
	[10] = bg_crazy_city_func,
	[11] = bg_thin_bricks_func,
	[12] = bg_crazy_city_low_func,
	[13] = bg_elevator_func,
	[14] = bg_elevator_func,

	[16] = bg_brown_grass_func,
	[17] = bg_plane_func_offset,
	[18] = bg_longsand_func,
	[19] = bg_technozone_func,
	[20] = bg_columns_2_func,
	[21] = bg_brown_grass_func,
	[22] = bg_static_func,
	[23] = bg_guantlet_func,
	[24] = bg_technozone_horizontal_simple_func,
	[25] = bg_technozone_vertical_simple_func,
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
	SYSTEM_ASSERT(!s_loaded);
	if (s_loaded)
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

	if (b->palette)
	{
		pal_upload(BG_CRAM_POSITION, b->palette, 8);
	}
	else
	{
		pal_upload(BG_CRAM_POSITION, res_pal_bg_bg0_bin, 8);
	}


	if (b->gfx_id != GFX_NULL)
	{
		const Gfx *gfx = gfx_get(b->gfx_id);
		SYSTEM_ASSERT(gfx->size <= BG_TILE_VRAM_LENGTH);
		gfx_load(gfx, BG_TILE_VRAM_POSITION);
	}
	else
	{
		dma_q_fill_vram(BG_TILE_VRAM_POSITION, 0, BG_TILE_VRAM_LENGTH, 1);
	}

	const uint16_t bg_plane_words = GAME_PLANE_H_CELLS * GAME_PLANE_W_CELLS;

	if (b->mapping)
	{
		uint16_t s_vram_pos = vdp_get_plane_base(VDP_PLANE_B);
		SYSTEM_ASSERT(b->mapping_size / 2 <= bg_plane_words);
		// TODO: MD framework util function to get plane byte size.
		uint16_t remaining_words = bg_plane_words;
		while (remaining_words > 0)
		{
			dma_q_transfer_vram(s_vram_pos, b->mapping, b->mapping_size / 2, 2);
			s_vram_pos += b->mapping_size;
			remaining_words -= b->mapping_size / 2;
		}
	}
	else
	{
		dma_q_fill_vram(vdp_get_plane_base(VDP_PLANE_B), 0, bg_plane_words, 1);
	}

	// Set scroll.
	set_h_scroll_plane(0);
	set_v_scroll_plane(0);
	dma_q_transfer_vsram(2, v_scroll_buffer, sizeof(v_scroll_buffer) / 2, 4);
	dma_q_transfer_vram(vdp_get_hscroll_base() + 2, h_scroll_buffer, sizeof(h_scroll_buffer) / 2, 4);

	s_loaded = 1;
}

void o_unload_bg(void)
{
	s_loaded = 0;
}
