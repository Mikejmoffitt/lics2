#include "obj/map.h"

#include <stdlib.h>
#include "gfx.h"
#include "obj.h"
#include "system.h"
#include "md/megadrive.h"
#include "game.h"
#include "common.h"
#include "obj/entrance.h"
#include "obj/lyle.h"

#include "res.h"

const uint16_t *g_map_data;  // Cast from map->current_map->map_data
uint16_t g_map_row_size;  // Set to 40 * map->current_map->w

int16_t g_map_x_scroll;
int16_t g_map_y_scroll;

static O_Map *map;
static uint16_t s_horizontal_dma_buffer[64];
static int16_t s_h_scroll_buffer[GAME_SCREEN_H_CELLS];
static int16_t s_v_scroll_buffer[GAME_SCREEN_W_CELLS / 2];

#define TILESET_ASSETS(name) { res_gfx_fg_##name##_bin,\
                               sizeof(res_gfx_fg_##name##_bin),\
                               res_pal_fg_##name##_bin,\
                               sizeof(res_pal_fg_##name##_bin) }

#define MAP_ASSETS(name) { (const MapFile *)(res_map_##name##_bin),\
                           sizeof(res_map_##name##_bin) }

// LUT representing the data needed for a given tileset ID.
typedef struct TilesetAssets
{
	const uint8_t *tile_data;
	uint32_t tile_data_size;
	const uint8_t *pal_data;
	uint32_t pal_data_size;
} TilesetAssets;

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
};

// LUT for map data by ID
typedef struct MapAssets
{
	const MapFile *data;
	unsigned int size;
} MapAssets;

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
};

static inline void draw_vertical(O_Map *m)
{
	if (g_map_y_scroll == m->y_scroll_prev) return;
	const uint16_t bottom_side = (g_map_y_scroll > m->y_scroll_prev);

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

	dma_src[0] = &map->current_map->map_data[map_src_x + map_src_y];
	dma_dest[0] = vdp_get_plane_base(VDP_PLANE_A) +
	              (2 * (x_scroll_tile + (GAME_PLANE_W_CELLS * y_scroll_tile)));

	// Handle crossing the horizontal seam.
	if (x_scroll_tile + GAME_SCREEN_W_CELLS + 1 >= GAME_PLANE_W_CELLS)
	{
		// DMA 0 gets cut short at the seam.
		dma_len[0] = GAME_PLANE_W_CELLS - x_scroll_tile;

		// DMA 1 fills in the rest of the line.
		dma_len[1] = GAME_SCREEN_W_CELLS - dma_len[0] + 1;
		dma_src[1] = dma_src[0] + (dma_len[0]);
		dma_dest[1] = vdp_get_plane_base(VDP_PLANE_A) +
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
		while (dma_dest[0] >= vdp_get_plane_base(VDP_PLANE_A) + v_seam_vram_offset)
		{
			dma_dest[0] -= v_seam_vram_offset;
		}
		while (dma_dest[1] >= vdp_get_plane_base(VDP_PLANE_A) + v_seam_vram_offset)
		{
			dma_dest[1] -= v_seam_vram_offset;
		}
	}
	while ((uint8_t *)dma_src[0] >= (uint8_t *)(map->current_map) + map->current_map_size) dma_src[0] -= map->current_map_size;
	while ((uint8_t *)dma_src[1] >= (uint8_t *)(map->current_map) + map->current_map_size) dma_src[1] -= map->current_map_size;

	dma_q_transfer_vram(dma_dest[0], dma_src[0], dma_len[0], 2);
	if (dma_len[1] > 0) dma_q_transfer_vram(dma_dest[1], dma_src[1], dma_len[1], 2);
}

static inline void draw_horizontal(O_Map *m)
{
	if (g_map_x_scroll == m->x_scroll_prev) return;
	const uint16_t right_side = (g_map_x_scroll > m->x_scroll_prev);
	const int16_t scroll_idx_x = g_map_x_scroll / 8;
	const int16_t scroll_idx_y = g_map_y_scroll / 8;

	const uint16_t plane_base = vdp_get_plane_base(VDP_PLANE_A);

	// VRAM address at which the vertical seam occurs
	const uint16_t v_seam_vram_offset = 2 * GAME_PLANE_H_CELLS *
	                                    GAME_PLANE_W_CELLS;

	// X and Y components of the source index (top-left visible corner)
	const uint16_t map_src_x = right_side ?
	                           GAME_SCREEN_W_CELLS + scroll_idx_x :
	                           scroll_idx_x;
	const uint16_t map_src_y = g_map_row_size * (g_map_y_scroll / 8);

	// What is the position of the tile shown at g_map_x_scroll, g_map_y_scroll?
	const uint16_t x_scroll_tile = (scroll_idx_x) % GAME_PLANE_W_CELLS;
	const uint16_t y_scroll_tile = (scroll_idx_y) % GAME_PLANE_H_CELLS;

	// Destination; points at the top of the seam. Operates in terms of VRAM address.
	uint16_t dma_dest = plane_base +
	                    (2 * (x_scroll_tile + (GAME_PLANE_W_CELLS * y_scroll_tile)));

	// Dsource
	const uint16_t *dma_src = &map->current_map->map_data[map_src_x +
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
		while ((uint8_t *)(dma_src) >= (uint8_t *)(map->current_map) + map->current_map_size)
		{
			dma_src -= map->current_map_size;
		}
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
	dma_q_transfer_vram(map_dma_h_dest[0], s_horizontal_dma_buffer,
	                    map_dma_h_len[0], stride);
	if (map_dma_h_len[1] > 0)
	{
		dma_q_transfer_vram(map_dma_h_dest[1],
		                    &s_horizontal_dma_buffer[map_dma_h_len[0]],
		                    map_dma_h_len[1], stride);
	}
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

	dma_src[0] = &map->current_map->map_data[map_src_x + map_src_y];
	dma_dest[0] = vdp_get_plane_base(VDP_PLANE_A) +
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
		dma_dest[1] = vdp_get_plane_base(VDP_PLANE_A) +
		              (2 * (GAME_PLANE_W_CELLS * y_scroll_tile));
	}
	else
	{
		dma_len[0] = GAME_SCREEN_W_CELLS + 1;
	}

	const uint16_t v_seam_vram_address = vdp_get_plane_base(VDP_PLANE_A) + v_seam_vram_offset;

	for (uint16_t y = 0; y < GAME_PLANE_H_CELLS; y++)
	{
		for (uint16_t i = 0; i < ARRAYSIZE(dma_src); i++)
		{
			if (dma_len[i] == 0) continue;
			if ((uint8_t *)(dma_src[i]) >= (uint8_t *)(map->current_map) + map->current_map_size)
			{
				dma_src[i] -= map->current_map_size;
			}
			dma_q_transfer_vram(dma_dest[i], dma_src[i], dma_len[i], 2);
			dma_src[i] += g_map_row_size;
			dma_dest[i] += GAME_PLANE_W_CELLS * 2;
			if (dma_dest[i] >= v_seam_vram_address) dma_dest[i] -= v_seam_vram_offset;
		}
	}
}

static void main_func(Obj *o)
{
	O_Map *m = (O_Map *)o;

	uint16_t i = ARRAYSIZE(s_h_scroll_buffer);
	while (i--)
	{
		s_h_scroll_buffer[i] = -g_map_x_scroll;
	}

	i = ARRAYSIZE(s_v_scroll_buffer);
	while (i--)
	{
		s_v_scroll_buffer[i] = g_map_y_scroll;
	}

	dma_q_transfer_vram(vdp_get_hscroll_base(), s_h_scroll_buffer, sizeof(s_h_scroll_buffer) / 2, 32);
	dma_q_transfer_vsram(0, s_v_scroll_buffer, sizeof(s_v_scroll_buffer) / 2, 4);

	// Update plane A.
	if (m->fresh_room)
	{
		draw_full();
		m->fresh_room = 0;
	}
	else
	{
		draw_vertical(m);
		draw_horizontal(m);
	}

	m->x_scroll_prev = g_map_x_scroll;
	m->y_scroll_prev = g_map_y_scroll;
}

void o_load_map(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Map) <= sizeof(ObjSlot));
	(void)data;

	map = (O_Map *)o;
	g_map_data = NULL;

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE, 0, 0, 0, 1);
	o->main_func = main_func;
	o->cube_func = NULL;
}

// Public functions -----------------------------------------------------------

// Load a map by ID number. In particular:
// * Sets the current map pointer
// * Populates the object list with entities from the map file
// * Sets the BG based on map file
// * Queues DMA for the sprite, enemy palettes
// * Queues DMA for the tileset
void map_load(uint8_t id, uint8_t entrance_num)
{
	SYSTEM_ASSERT(map != NULL);
	map->current_map = map_by_id[id].data;
	map->current_map_size = map_by_id[id].size;
	g_map_data = map->current_map->map_data;
	g_map_row_size = map->current_map->w * GAME_SCREEN_W_CELLS;
	map->right = INTTOFIX32(map->current_map->w * GAME_SCREEN_W_PIXELS);
	map->bottom = INTTOFIX32(map->current_map->h * GAME_SCREEN_H_PIXELS);
	SYSTEM_ASSERT(map->right >= INTTOFIX32(320));
	SYSTEM_ASSERT(map->bottom >= INTTOFIX32(240));
	map->fresh_room = 1;

	g_map_x_scroll = 0;
	g_map_y_scroll = 0;

	// Set up tiles and palettes.
	SYSTEM_ASSERT(map->current_map->tileset < ARRAYSIZE(tileset_by_id));
	if (map->current_map->tileset < ARRAYSIZE(tileset_by_id))
	{
		const TilesetAssets *tsa = &tileset_by_id[map->current_map->tileset];
		SYSTEM_ASSERT(tsa->tile_data_size <= MAP_TILE_VRAM_LENGTH);
		pal_upload(MAP_TILE_CRAM_POSITION, tsa->pal_data, tsa->pal_data_size / 2);
		pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_bin, sizeof(res_pal_enemy_bin) / 2);
		dma_q_transfer_vram(MAP_TILE_VRAM_POSITION, tsa->tile_data, tsa->tile_data_size / 2, 2);
	}

	// Set scroll mode based on room geometry.
	if (map->current_map->w <= 1) vdp_set_vscroll_mode(VDP_VSCROLL_CELL);
	else vdp_set_vscroll_mode(VDP_VSCROLL_PLANE);
	vdp_set_hscroll_mode(VDP_HSCROLL_CELL);

	// Build the object list.
	uint16_t found_entrance = 0;
	for (uint8_t i = 0; i < ARRAYSIZE(map->current_map->objects); i++)
	{
		const MapObj *b = &map->current_map->objects[i];
		const Obj *o = obj_spawn(b->x, b->y, (ObjType)b->type, b->data);
		if (!o) continue;

		// If we have found the desired entrance number, move Lyle to it.
		if (!found_entrance && o->type == OBJ_ENTRANCE)
		{
			const O_Entrance *e = (O_Entrance *)o;
			if (e->entrance_num == entrance_num)
			{
				lyle_set_pos(o->x, o->y);
				found_entrance = 1;
			}
		}
	}
}

void map_redraw_room(void)
{
	map->fresh_room = 1;
}

fix32_t map_get_right(void)
{
	return map->right;
}

fix32_t map_get_bottom(void)
{
	return map->bottom;
}

void map_set_x_scroll(int16_t x)
{
	if (map->current_map->w <= 1) x = 0;
	if (x < 0) x = 0;
	const int16_t right_bound = (map->current_map->w - 1) * GAME_SCREEN_W_PIXELS;
	if (x > right_bound) x = right_bound;
	g_map_x_scroll = x;
}

void map_set_y_scroll(int16_t y)
{
	const int16_t top_bound = 0;
	const int16_t bottom_bound = ((map->current_map->h - 1) * GAME_SCREEN_H_PIXELS) + (system_is_ntsc() ? 16 : 0);
	if (y < top_bound) y = top_bound;
	if (y > bottom_bound) y = bottom_bound;
	g_map_y_scroll = y;
}

void map_set_next_room(uint8_t id, uint8_t entrance)
{
	map->next_room_id = id;
	map->next_room_entrance = entrance;
}

uint8_t map_get_next_room_id(void)
{
	return map->next_room_id;
}

uint8_t map_get_next_room_entrance(void)
{
	return map->next_room_entrance;
}

uint8_t map_get_music_track(void)
{
	if (!map) return 0;
	if (!map->current_map) return 0;
	return map->current_map->music;
}

uint8_t map_get_background(void)
{
	if (!map) return 0;
	if (!map->current_map) return 0;
	return map->current_map->background;
}

void map_set_exit_trigger(MapExitTrigger t)
{
	if (!map) return;
	map->exit_trigger = t;
}

MapExitTrigger map_get_exit_trigger(void)
{
	if (!map) return MAP_EXIT_NONE;
	return map->exit_trigger;
}
