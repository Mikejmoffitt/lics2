#include "map.h"

#include "md/megadrive.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "gfx.h"
#include "lyle.h"
#include "obj.h"
#include "res.h"
#include "system.h"
#include "util/kosinski.h"

#include "obj/entrance.h"

const uint16_t *g_map_data;  // Cast from s_map.current_map->map_data
uint16_t g_map_row_size;  // Set to 40 * s_map.current_map->w

int16_t g_map_x_scroll;
int16_t g_map_y_scroll;

static Map s_map;
static uint16_t s_horizontal_dma_buffer[64];
static int16_t s_h_scroll_buffer[GAME_SCREEN_H_CELLS];
static int16_t s_v_scroll_buffer[GAME_SCREEN_W_CELLS / 2];

// LUT representing the data needed for a given tileset ID.
typedef struct TilesetAssets
{
	const uint8_t *tile_data;
	uint32_t tile_data_size;
	const uint8_t *pal_data;
	uint32_t pal_data_size;
} TilesetAssets;

#define TILESET_ASSETS(name) { res_gfx_fg_##name##_bin,\
                               sizeof(res_gfx_fg_##name##_bin),\
                               res_pal_fg_##name##_bin,\
                               sizeof(res_pal_fg_##name##_bin) }
static const TilesetAssets tileset_by_id[] =
{
	[0] = TILESET_ASSETS(00_outside1),
	[1] = TILESET_ASSETS(01_outside2),
	[2] = TILESET_ASSETS(02_inside1),
	[3] = TILESET_ASSETS(03_sandy1),
	[4] = TILESET_ASSETS(04_teleporter),
	[5] = TILESET_ASSETS(05_outside3),
	[6] = TILESET_ASSETS(06_purplezone),
	[7] = TILESET_ASSETS(07_rooftop),
	[8] = TILESET_ASSETS(08_technozone),
	[9] = TILESET_ASSETS(09_underpurple),
	[10] = TILESET_ASSETS(10_purplebricks),
	[11] = TILESET_ASSETS(11_purpletree),
	[12] = TILESET_ASSETS(12_gauntlet),
	[13] = TILESET_ASSETS(13_purplecity),
	[14] = TILESET_ASSETS(14_sandy2),
	[15] = TILESET_ASSETS(15_basketball),
	[16] = TILESET_ASSETS(16_finalfight),
	[17] = TILESET_ASSETS(17_gameover),
};
#undef TILESET_ASSETS

// LUT for map data by ID
typedef struct MapAssets
{
	const uint8_t *data;
	unsigned int size;
} MapAssets;

#define MAP_ASSETS(name) { res_map_##name##_map,\
                           sizeof(res_map_##name##_map) }
static const MapAssets map_by_id[] =
{
	[0] = MAP_ASSETS(00_roomzero),
	[1] = MAP_ASSETS(01_startroom),
	[2] = MAP_ASSETS(02_sidesquare),
	[3] = MAP_ASSETS(03_teleroom),
	[4] = MAP_ASSETS(04_lefttall),
	[5] = MAP_ASSETS(05_plantroom),
	[6] = MAP_ASSETS(06_earlycorridor),
	[7] = MAP_ASSETS(07_cownspikes),
	[8] = MAP_ASSETS(08_liftget),
	[9] = MAP_ASSETS(09_earlybumps),
	[10] = MAP_ASSETS(10_smallchamber),
	[11] = MAP_ASSETS(11_zigzag),
	[12] = MAP_ASSETS(12_jumpget),
	[13] = MAP_ASSETS(13_lavatower),
	[14] = MAP_ASSETS(14_ballhall),
	[15] = MAP_ASSETS(15_kickblock),
	[16] = MAP_ASSETS(16_pillatower),
	[17] = MAP_ASSETS(17_buggozone),
	[18] = MAP_ASSETS(18_poweruptower),
	[19] = MAP_ASSETS(19_boingotele),
	[20] = MAP_ASSETS(20_undersand),
	[21] = MAP_ASSETS(21_spikeshelf),
	[22] = MAP_ASSETS(22_kickget),
	[23] = MAP_ASSETS(23_cowzone),
	[24] = MAP_ASSETS(24_smallghetto),
	[25] = MAP_ASSETS(25_flargycolumn),
	[26] = MAP_ASSETS(26_bigghetto),
	[27] = MAP_ASSETS(27_elevatorroom),
	[28] = MAP_ASSETS(28_topleft),
	[29] = MAP_ASSETS(29_dogtown),
	[30] = MAP_ASSETS(30_pyramid),
	[31] = MAP_ASSETS(31_basketball),
	[32] = MAP_ASSETS(32_treesand),
	[33] = MAP_ASSETS(33_purplezone),
	[34] = MAP_ASSETS(34_littlepurple),
	[35] = MAP_ASSETS(35_orangeget),
	[36] = MAP_ASSETS(36_phantomget),
	[37] = MAP_ASSETS(37_boss1),
	[38] = MAP_ASSETS(38_rooftop),
	[39] = MAP_ASSETS(39_roofroom),
	[40] = MAP_ASSETS(40_underpurple),
	[41] = MAP_ASSETS(41_smallhouse),
	[42] = MAP_ASSETS(42_purplebricks),
	[43] = MAP_ASSETS(43_bottomtele),
	[44] = MAP_ASSETS(44_cproom),
	[45] = MAP_ASSETS(45_longsand),
	[46] = MAP_ASSETS(46_technofirst),
	[47] = MAP_ASSETS(47_technocolumn),
	[48] = MAP_ASSETS(48_technoatrium),
	[49] = MAP_ASSETS(49_technocorridor),
	[50] = MAP_ASSETS(50_technotopper),
	[51] = MAP_ASSETS(51_toxicpool),
	[52] = MAP_ASSETS(52_tvroom),
	[53] = MAP_ASSETS(53_purplecity),
	[54] = MAP_ASSETS(54_gauntlet),
	[55] = MAP_ASSETS(55_finaltele),
	[56] = MAP_ASSETS(56_boss2),
	[57] = MAP_ASSETS(57_finalboss),
	[58] = MAP_ASSETS(58_truefinalboss),

	[127] = MAP_ASSETS(127_gameover),
};
#undef MAP_ASSETS

static inline bool draw_vertical(void)
{
	Map *m = &s_map;
	if (g_map_y_scroll == m->y_scroll_prev) return false;
	const bool bottom_side = (g_map_y_scroll > m->y_scroll_prev);

	// VRAM address at which the vertical seam occurs
	const uint16_t v_seam_vram_offset = 2 * GAME_PLANE_H_CELLS *
	                                    GAME_PLANE_W_CELLS;

	// X and Y components of the source index
	uint16_t map_src_x = g_map_x_scroll / 8;
	uint16_t map_src_y = g_map_row_size * (g_map_y_scroll / 8);

	if (map_src_x == m->v_x_map_src_prev &&
	    map_src_y == m->v_y_map_src_prev)
	{
		return true;
	}

	m->v_x_map_src_prev = map_src_x;
	m->v_y_map_src_prev = map_src_y;

	// What is the position of the tile shown at g_map_x_scroll, g_map_y_scroll?
	const uint16_t x_scroll_tile = (g_map_x_scroll / 8) % GAME_PLANE_W_CELLS;
	const uint16_t y_scroll_tile = (g_map_y_scroll / 8) % GAME_PLANE_H_CELLS;

	const uint16_t *dma_src[2] = {0};
	uint16_t dma_dest[2] = {0};
	uint16_t dma_len[2] = {0};

	dma_src[0] = &s_map.current_map->map_data[map_src_x + map_src_y];
	dma_dest[0] = md_vdp_get_plane_base(VDP_PLANE_A) +
	              (2 * (x_scroll_tile + (GAME_PLANE_W_CELLS * y_scroll_tile)));

	// Handle crossing the horizontal seam.
	if (x_scroll_tile + GAME_SCREEN_W_CELLS + 1 >= GAME_PLANE_W_CELLS)
	{
		// DMA 0 gets cut short at the seam.
		dma_len[0] = GAME_PLANE_W_CELLS - x_scroll_tile;

		// DMA 1 fills in the rest of the line.
		dma_len[1] = GAME_SCREEN_W_CELLS - dma_len[0] + 1;
		dma_src[1] = dma_src[0] + (dma_len[0]);
		dma_dest[1] = md_vdp_get_plane_base(VDP_PLANE_A) +
		              (2 * (GAME_PLANE_W_CELLS * y_scroll_tile));
	}
	else
	{
		dma_len[0] = GAME_SCREEN_W_CELLS + 1;
	}

	// Work on the bottom of the screen instead of the top
	if (bottom_side)
	{
		dma_src[0] += g_map_row_size * 28;
		dma_dest[0] += 2 * (GAME_PLANE_W_CELLS * 28);

		if (!system_is_ntsc())
		{
			dma_src[0] += 2 * g_map_row_size;
			dma_dest[0] += 2 * (GAME_PLANE_W_CELLS * 2);
		}
		if (dma_len[1] > 0)
		{
			dma_src[1] += g_map_row_size * 28;
			dma_dest[1] += 2 * (GAME_PLANE_W_CELLS * 28);
			if (!system_is_ntsc())
			{
				dma_src[1] += 2 * g_map_row_size;
				dma_dest[1] += 2 * (GAME_PLANE_W_CELLS * 2);
			}
		}

		// Handle seam crossings.
		while (dma_dest[0] >= md_vdp_get_plane_base(VDP_PLANE_A) + v_seam_vram_offset)
		{
			dma_dest[0] -= v_seam_vram_offset;
		}
		while (dma_dest[1] >= md_vdp_get_plane_base(VDP_PLANE_A) + v_seam_vram_offset)
		{
			dma_dest[1] -= v_seam_vram_offset;
		}
	}

	// TODO: Were these DMA limits busted anyway>
//	while ((uint8_t *)dma_src[0] >= (uint8_t *)(s_map.current_map) + s_map.current_map_size) dma_src[0] -= s_map.current_map_size;
//	while ((uint8_t *)dma_src[1] >= (uint8_t *)(s_map.current_map) + s_map.current_map_size) dma_src[1] -= s_map.current_map_size;

	md_dma_transfer_vram(dma_dest[0], dma_src[0], dma_len[0], 2);
	if (dma_len[1] > 0) md_dma_transfer_vram(dma_dest[1], dma_src[1], dma_len[1], 2);
	return true;
}

static inline bool draw_horizontal(void)
{
	Map *m = &s_map;
	if (g_map_x_scroll == m->x_scroll_prev) return false;
	const bool right_side = (g_map_x_scroll > m->x_scroll_prev);
	const int16_t scroll_idx_x = g_map_x_scroll / 8;
	const int16_t scroll_idx_y = g_map_y_scroll / 8;

	const uint16_t plane_base = md_vdp_get_plane_base(VDP_PLANE_A);

	// VRAM address at which the vertical seam occurs
	const uint16_t v_seam_vram_offset = 2 * GAME_PLANE_H_CELLS *
	                                    GAME_PLANE_W_CELLS;

	// X and Y components of the source index (top-left visible corner)
	const uint16_t map_src_x = right_side ?
	                           GAME_SCREEN_W_CELLS + scroll_idx_x :
	                           scroll_idx_x;
	const uint16_t map_src_y = g_map_row_size * (g_map_y_scroll / 8);

	if (map_src_x == m->h_x_map_src_prev &&
	    map_src_y == m->h_y_map_src_prev)
	{
		return true;
	}

	m->h_x_map_src_prev = map_src_x;
	m->h_y_map_src_prev = map_src_y;

	// What is the position of the tile shown at g_map_x_scroll, g_map_y_scroll?
	const uint16_t x_scroll_tile = (scroll_idx_x) % GAME_PLANE_W_CELLS;
	const uint16_t y_scroll_tile = (scroll_idx_y) % GAME_PLANE_H_CELLS;

	// Destination; points at the top of the seam. Operates in terms of VRAM address.
	uint16_t dma_dest = plane_base +
	                    (2 * (x_scroll_tile + (GAME_PLANE_W_CELLS * y_scroll_tile)));

	// Dsource
	const uint16_t *dma_src = &s_map.current_map->map_data[map_src_x +
	                                                      map_src_y];

	if (right_side)
	{
		dma_dest += GAME_SCREEN_W_CELLS * 2;
		// If going to the right side crosses the horizontal seam, wrap.
		if (((x_scroll_tile + GAME_SCREEN_W_CELLS) >= GAME_PLANE_W_CELLS))
		{
			dma_dest -= (GAME_PLANE_W_CELLS * 2);
		}
	}

	// Set up between 1 and 2 DMAs; since the DMA controller does not have a
	// source stride register, s_horizontal_dma_buffer is used to construct
	// a copy of the data to transfer.
	uint16_t map_dma_h_dest[2] = {0};
	uint16_t map_dma_h_len[2] = {0};
	uint16_t current_dma = 0;

	map_dma_h_dest[0] = dma_dest;
	map_dma_h_len[0] = 0;

	const uint16_t row_count = system_is_ntsc() ? 29 : 31;

	for (uint16_t i = 0; i < row_count; i++)
	{
		// TODO: Redo broken map limits
//		while ((uint8_t *)(dma_src) >= (uint8_t *)(s_map.current_map) + s_map.current_map_size)
//		{
//			dma_src -= s_map.current_map_size;
//		}
		s_horizontal_dma_buffer[i] = *dma_src;
		dma_src += g_map_row_size;
		map_dma_h_len[current_dma]++;
		dma_dest += GAME_PLANE_W_CELLS * 2;
		// Have we crossed the vertical seam?
		if (current_dma == 0 && dma_dest >= plane_base + v_seam_vram_offset)
		{
			// Loop back around, and split to the next DMA.
			dma_dest -= v_seam_vram_offset;
			current_dma++;
			map_dma_h_dest[current_dma] = dma_dest;
		}
	}

	// DMA 0 starts from the beginning of the horizontal DMA buffer. DMA 1, if
	// the legnth is nonzero, starts from the end of DMA 0.
	const int16_t stride = GAME_PLANE_W_CELLS * 2;
	md_dma_transfer_vram(map_dma_h_dest[0], s_horizontal_dma_buffer,
	                    map_dma_h_len[0], stride);
	if (map_dma_h_len[1] > 0)
	{
		md_dma_transfer_vram(map_dma_h_dest[1],
		                    &s_horizontal_dma_buffer[map_dma_h_len[0]],
		                    map_dma_h_len[1], stride);
	}
	return true;
}

static inline void draw_full(void)
{
	// VRAM address at which the vertical seam occurs
	const uint16_t v_seam_vram_offset = 2 * GAME_PLANE_H_CELLS *
	                                    GAME_PLANE_W_CELLS;

	// X and Y components of the source index
	uint16_t map_src_x = g_map_x_scroll / 8;
	uint16_t map_src_y = g_map_row_size * (g_map_y_scroll / 8);

	// What is the position of the tile shown at g_map_x_scroll, g_map_y_scroll?
	const uint16_t x_scroll_tile = (g_map_x_scroll / 8) % GAME_PLANE_W_CELLS;
	const uint16_t y_scroll_tile = (g_map_y_scroll / 8) % GAME_PLANE_H_CELLS;

	const uint16_t *dma_src[2] = {0};
	uint16_t dma_dest[2] = {0};
	uint16_t dma_len[2] = {0};

	dma_src[0] = &s_map.current_map->map_data[map_src_x + map_src_y];
	dma_dest[0] = md_vdp_get_plane_base(VDP_PLANE_A) +
	             (2 * (x_scroll_tile + (GAME_PLANE_W_CELLS * y_scroll_tile)));
	dma_len[0] = 0;

	// Handle crossing the horizontal seam.
	if (x_scroll_tile + GAME_SCREEN_W_CELLS + 1 >= GAME_PLANE_W_CELLS)
	{
		// DMA 0 gets cut short at the seam.
		dma_len[0] = GAME_PLANE_W_CELLS - x_scroll_tile;

		// DMA 1 fills in the rest of the line.
		dma_len[1] = GAME_SCREEN_W_CELLS - dma_len[0] + 1;
		dma_src[1] = dma_src[0] + (dma_len[0]);
		dma_dest[1] = md_vdp_get_plane_base(VDP_PLANE_A) +
		              (2 * (GAME_PLANE_W_CELLS * y_scroll_tile));
	}
	else
	{
		dma_len[0] = GAME_SCREEN_W_CELLS + 1;
	}

	const uint16_t v_seam_vram_address = md_vdp_get_plane_base(VDP_PLANE_A) + v_seam_vram_offset;

	const uint16_t row_count = system_is_ntsc() ? 29 : 31;
	for (uint16_t y = 0; y < row_count + 1; y++)
	{
		for (uint16_t i = 0; i < ARRAYSIZE(dma_src); i++)
		{
			if (dma_len[i] == 0) continue;

			// TODO: Broken map limits
//			if ((uint8_t *)(dma_src[i]) >= (uint8_t *)(s_map.current_map) + s_map.current_map_size)
//			{
//				dma_src[i] -= s_map.current_map_size;
//			}
			md_dma_transfer_vram(dma_dest[i], dma_src[i], dma_len[i], 2);
			dma_src[i] += g_map_row_size;
			dma_dest[i] += GAME_PLANE_W_CELLS * 2;
			if (dma_dest[i] >= v_seam_vram_address) dma_dest[i] -= v_seam_vram_offset;
		}
	}
}

static inline void prepare_hscroll(void)
{
	uint16_t i = ARRAYSIZE(s_h_scroll_buffer);
	while (i--)
	{
		s_h_scroll_buffer[i] = -g_map_x_scroll;
	}
}

static inline void prepare_vscroll(void)
{
	uint16_t i = ARRAYSIZE(s_v_scroll_buffer);
	while (i--)
	{
		s_v_scroll_buffer[i] = g_map_y_scroll;
	}
}

void map_poll(void)
{
	// Update plane A.
	if (s_map.fresh_room)
	{
		prepare_vscroll();
		prepare_hscroll();
		draw_full();
		s_map.fresh_room = false;

		md_dma_transfer_vram(md_vdp_get_hscroll_base(), s_h_scroll_buffer, sizeof(s_h_scroll_buffer) / 2, 32);
		md_dma_transfer_vsram(0, s_v_scroll_buffer, sizeof(s_v_scroll_buffer) / 2, 4);
	}
	else
	{
		if (draw_vertical())
		{
			prepare_vscroll();
			md_dma_transfer_vsram(0, s_v_scroll_buffer, sizeof(s_v_scroll_buffer) / 2, 4);
		}
		if (draw_horizontal())
		{
			prepare_hscroll();
			md_dma_transfer_vram(md_vdp_get_hscroll_base(), s_h_scroll_buffer, sizeof(s_h_scroll_buffer) / 2, 32);
		}
	}

	s_map.x_scroll_prev = g_map_x_scroll;
	s_map.y_scroll_prev = g_map_y_scroll;
}

// Public functions -----------------------------------------------------------

void map_load(uint8_t id, uint8_t entrance_num)
{
	s_map.x_scroll_prev = -32767;
	s_map.y_scroll_prev = -32767;

	s_map.h_x_map_src_prev = 0;
	s_map.h_y_map_src_prev = 0;
	s_map.v_x_map_src_prev = 0;
	s_map.v_y_map_src_prev = 0;

	s_map.exit_trigger = MAP_EXIT_NONE;

	// Decompress the map data into RAM.
	const size_t kos_offs = offsetof(typeof(s_map.map_file), music);
	kosinski_decomp(&map_by_id[id].data[kos_offs], &s_map.map_raw[kos_offs]);
	s_map.current_map = &s_map.map_file;
	g_map_data = s_map.current_map->map_data;

	s_map.fresh_room = true;

	// Set geometry data.
	g_map_row_size = s_map.current_map->w * GAME_SCREEN_W_CELLS;
	s_map.right_px = s_map.current_map->w * GAME_SCREEN_W_PIXELS;
	s_map.bottom_px = s_map.current_map->h * GAME_SCREEN_H_PIXELS;
	s_map.right = INTTOFIX32(s_map.right_px);
	s_map.bottom = INTTOFIX32(s_map.bottom_px);
	SYSTEM_ASSERT(s_map.right >= INTTOFIX32(320));
	SYSTEM_ASSERT(s_map.bottom >= INTTOFIX32(240));

	// Set scroll mode based on room geometry.
	if (s_map.current_map->w <= 1) md_vdp_set_vscroll_mode(VDP_VSCROLL_CELL);
	else md_vdp_set_vscroll_mode(VDP_VSCROLL_PLANE);
	md_vdp_set_hscroll_mode(VDP_HSCROLL_CELL);

	g_map_x_scroll = 0;
	g_map_y_scroll = 0;

	// Asset management.
	map_upload_tiles();
	map_upload_palette();
	md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_bin,
	              sizeof(res_pal_enemy_bin) / 2);

	// Build the object list.
	bool found_entrance = false;
	for (uint8_t i = 0; i < ARRAYSIZE(s_map.current_map->objects); i++)
	{
		const MapObj *b = &s_map.current_map->objects[i];
		const Obj *o = obj_spawn(b->x, b->y, (ObjType)b->type, b->data);
		if (!o) continue;

		// If we have found the desired entrance number, move Lyle to it.
		if (!found_entrance && o->type == OBJ_ENTRANCE)
		{
			const O_Entrance *e = (O_Entrance *)o;
			if (e->entrance_num == entrance_num)
			{
				lyle_set_pos(o->x, o->y);
				found_entrance = true;
			}
		}
	}
}

void map_redraw_room(void)
{
	s_map.fresh_room = true;
}

fix32_t map_get_right(void)
{
	return s_map.right;
}

fix32_t map_get_bottom(void)
{
	return s_map.bottom;
}

int16_t map_get_right_px(void)
{
	return s_map.right_px;
}

int16_t map_get_bottom_px(void)
{
	return s_map.bottom_px;
}

void map_set_x_scroll(int16_t x)
{
	if (s_map.current_map->w <= 1) x = 0;
	if (x < 0) x = 0;
	const int16_t right_bound = (s_map.current_map->w - 1) * GAME_SCREEN_W_PIXELS;
	if (x > right_bound) x = right_bound;
	g_map_x_scroll = x;
}

void map_set_y_scroll(int16_t y)
{
	const int16_t top_bound = 0;
	const int16_t bottom_bound =
	    ((s_map.current_map->h - 1) * GAME_SCREEN_H_PIXELS) +
	    (system_is_ntsc() ? 16 : 0);
	if (y < top_bound) y = top_bound;
	if (y > bottom_bound) y = bottom_bound;
	g_map_y_scroll = y;
}

void map_set_next_room(uint8_t id, uint8_t entrance)
{
	s_map.next_room_id = id;
	s_map.next_room_entrance = entrance;
}

uint8_t map_get_next_room_id(void)
{
	return s_map.next_room_id;
}

uint8_t map_get_next_room_entrance(void)
{
	return s_map.next_room_entrance;
}

uint8_t map_get_music_track(void)
{
	if (!s_map.current_map) return 0;
	return s_map.current_map->music;
}

uint8_t map_get_background(void)
{
	if (!s_map.current_map) return 0;
	return s_map.current_map->background;
}

void map_set_exit_trigger(MapExitTrigger t)
{
	s_map.exit_trigger = t;
}

MapExitTrigger map_get_exit_trigger(void)
{
	return s_map.exit_trigger;
}

int16_t map_get_world_x_tile(void)
{
	return s_map.current_map->map_x;
}

int16_t map_get_world_y_tile(void)
{
	return s_map.current_map->map_y;
}

void map_upload_tiles(void)
{
	SYSTEM_ASSERT(s_map.current_map->tileset < ARRAYSIZE(tileset_by_id));
	if (s_map.current_map->tileset < ARRAYSIZE(tileset_by_id))
	{
		const TilesetAssets *tsa = &tileset_by_id[s_map.current_map->tileset];
		SYSTEM_ASSERT(tsa->tile_data_size <= MAP_TILE_VRAM_LENGTH);
		md_dma_transfer_vram(MAP_TILE_VRAM_POSITION, tsa->tile_data, tsa->tile_data_size / 2, 2);
	}
}

void map_upload_palette(void)
{
	SYSTEM_ASSERT(s_map.current_map->tileset < ARRAYSIZE(tileset_by_id));
	if (s_map.current_map->tileset < ARRAYSIZE(tileset_by_id))
	{
		const TilesetAssets *tsa = &tileset_by_id[s_map.current_map->tileset];
		md_pal_upload(MAP_TILE_CRAM_POSITION, tsa->pal_data, tsa->pal_data_size / 2);
	}
}

int16_t map_file_count(void)
{
	return ARRAYSIZE(map_by_id);
}

const char *map_name_by_id(uint8_t id)
{
	if (id >= ARRAYSIZE(map_by_id)) return NULL;
	const MapFile *mf = (const MapFile *)(map_by_id[id].data);
	return mf->name;
}
