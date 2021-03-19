#include "obj/pause.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"
#include "progress.h"
#include "game.h"

static int16_t kcursor_flash_delay;

static uint16_t s_vram_pos;

static inline void draw_blank_window(void)
{
	uint16_t plane_base = vdp_get_plane_base(VDP_PLANE_WINDOW);
	const uint16_t plane_size = GAME_PLANE_W_CELLS * 30;
	vdp_wait_dma();
	dma_set_stride(2);
	const uint16_t fill_tile = VDP_ATTR(s_vram_pos + 0x30, 0, 0, MAP_PAL_LINE, 0);
	dma_fill_vram(plane_base, (fill_tile) & 0xFF, plane_size - 1);
	dma_fill_vram(plane_base + 1, ((fill_tile) & 0xFF00) >> 8, plane_size - 1);
	vdp_poke(plane_base, fill_tile);  // TODO: Is DMA fill really buggy?
}

static const uint16_t kmap_left = 8;
static const uint16_t kmap_top = 7;

static inline void draw_map_side_borders(void)
{
	uint16_t plane_base = vdp_get_plane_base(VDP_PLANE_WINDOW) +
	                      2 * (((kmap_top - 1) * GAME_PLANE_W_CELLS) +
	                      kmap_left - 1);
	const uint16_t right_offset = 2 * (PROGRESS_MAP_W + 1);

	for (int16_t y = 0; y < PROGRESS_MAP_H + 2; y++)
	{
		// Left side.
		vdp_poke(plane_base, VDP_ATTR(s_vram_pos, 0, 0, MAP_PAL_LINE, 0));
		vdp_poke(plane_base - 2,
		         VDP_ATTR(s_vram_pos + 0x31, 0, 0, MAP_PAL_LINE, 0));
		// Right side.
		vdp_poke(plane_base + right_offset,
		         VDP_ATTR(s_vram_pos, 0, 0, MAP_PAL_LINE, 0));
		plane_base += 2 * GAME_PLANE_W_CELLS;
	}
}

static inline void draw_map_top_bottom_borders(void)
{
	uint16_t plane_base = vdp_get_plane_base(VDP_PLANE_WINDOW) +
	                      2 * (((kmap_top - 2) * GAME_PLANE_W_CELLS) +
	                      kmap_left - 1);
	const uint16_t bottom_offset = 2 * (GAME_PLANE_W_CELLS * (PROGRESS_MAP_H + 2));
	const uint16_t padding_offset = 2 * (GAME_PLANE_W_CELLS);
	// Corner.
	vdp_poke(plane_base - 2, VDP_ATTR(s_vram_pos + 0x32,
	                                  0, 0, MAP_PAL_LINE, 0));

	for (int16_t x = 0; x < PROGRESS_MAP_W + 2; x++)
	{
		// Top side.
		vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x33,
		                              0, 0, MAP_PAL_LINE, 0));
		// Top padding.
		vdp_poke(plane_base + padding_offset, VDP_ATTR(s_vram_pos,
		                                               0, 0, MAP_PAL_LINE, 0));
		// Bottom side.
		vdp_poke(plane_base + bottom_offset,
		         VDP_ATTR(s_vram_pos, 0, 0, MAP_PAL_LINE, 0));
		plane_base += 2;
	}
}

static inline void draw_map(void)
{
	const ProgressSlot *progress = progress_get();
	uint16_t plane_base = vdp_get_plane_base(VDP_PLANE_WINDOW);
	const uint16_t tile_base = VDP_ATTR(s_vram_pos, 0, 0, MAP_PAL_LINE, 0);
	uint16_t pausemap_offset = 0;

	plane_base += 2 * ((kmap_top * GAME_PLANE_W_CELLS) + kmap_left);
	for (int16_t y = 0; y < PROGRESS_MAP_H; y++)
	{
		for (int16_t x = 0; x < PROGRESS_MAP_W; x++)
		{
			if ((progress->abilities & ABILITY_MAP) && progress->map_explored[y][x])
			{
				vdp_poke(plane_base, tile_base + res_pausemap_bin[pausemap_offset]);
			}
			else
			{
				vdp_poke(plane_base, tile_base);
			}
			pausemap_offset++;
			plane_base += 2;
		}
		plane_base += 2 * (GAME_PLANE_W_CELLS - PROGRESS_MAP_W);
	}
}

static inline void draw_cube_sector_extension(void)
{
	static const uint16_t kcube_sector_cells = 13;
	static const uint16_t kleft = kmap_left +
	                              (PROGRESS_MAP_W - kcube_sector_cells) / 2;
	static const uint16_t ktop = kmap_top + PROGRESS_MAP_H + 1;

	uint16_t plane_base = vdp_get_plane_base(VDP_PLANE_WINDOW);

	plane_base += 2 * ((ktop * GAME_PLANE_W_CELLS) + kleft);
	for (int16_t y = 0; y < 2; y++)
	{
		// Left side border
		vdp_poke(plane_base - 2, VDP_ATTR(s_vram_pos + 0x31, 0, 0, MAP_PAL_LINE, 0));
		// Cells on bottom
		for (int16_t x = 0; x < kcube_sector_cells; x++)
		{
			vdp_poke(plane_base, VDP_ATTR(s_vram_pos, 0, 0, MAP_PAL_LINE, 0));
			plane_base += 2;
		}
		plane_base += 2 * (GAME_PLANE_W_CELLS - kcube_sector_cells);
	}
}

static void update_window_plane(void)
{
	draw_blank_window();
	draw_map_side_borders();
	draw_map_top_bottom_borders();
	draw_cube_sector_extension();
	draw_map();
}

static inline void draw_pause(void)
{
	const int16_t draw_x = (GAME_SCREEN_W_PIXELS / 2) - 28;
	const int16_t draw_y = 24;

	spr_put(draw_x, draw_y,
	        SPR_ATTR(s_vram_pos + 0x60, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(4, 2));
	spr_put(draw_x + 32, draw_y,
	        SPR_ATTR(s_vram_pos + 0x68, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(4, 2));
}

static inline void draw_location(O_Pause *e)
{
	const ProgressSlot *progress = progress_get();
	if ((progress->abilities & ABILITY_MAP))
	{
		if (e->cursor_flash_frame == 0) return;
		const int16_t px = FIX32TOINT(lyle_get_x());
		const int16_t py = FIX32TOINT(lyle_get_y());
		const int16_t x_index = map_get_world_x_tile() + (px / GAME_SCREEN_W_PIXELS);
		const int16_t y_index = map_get_world_y_tile() + (py / GAME_SCREEN_H_PIXELS);
		const int16_t draw_x = 8 * kmap_left + (x_index * 8) + 1;
		const int16_t draw_y = 8 * kmap_top + (y_index * 8) + 2;

		spr_put(draw_x, draw_y,
		        SPR_ATTR(s_vram_pos + 0x2F, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 1));
	}
	else
	{
		static const int16_t knomap_w = 54;
		const int16_t draw_x = (8 * kmap_left + (4 * PROGRESS_MAP_W) - (knomap_w / 2));
		const int16_t draw_y = (8 * kmap_top) + (4 * PROGRESS_MAP_H) + 4;
		spr_put(draw_x, draw_y,
		        SPR_ATTR(s_vram_pos + 0x5A, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(2, 1));
		spr_put(draw_x + 27, draw_y,
		        SPR_ATTR(s_vram_pos + 0x5C, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(4, 1));
	}
}

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_PAUSE);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static void main_func(Obj *o)
{
	O_Pause *e = (O_Pause *)o;

	// Search for an active title object, and abort early if it's present.
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status == OBJ_STATUS_NULL) continue;
		if (o->type == OBJ_TITLE) return;
	}

	const MdButton buttons = io_pad_read(0);

	const int16_t was_paused = e->paused;

	if ((buttons & BTN_START) && !(e->buttons_prev & BTN_START))
	{
		e->paused = !e->paused;
	}

	// Pause/unpause transition and BG plane setup.
	if (e->paused && !was_paused)
	{
		update_window_plane();
		// Hibernate all objects besides this one.
		for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
		{
			Obj *w = &g_objects[i].obj;
			if (w->status == OBJ_STATUS_NULL) continue;
			if (w->type == OBJ_HUD) continue;
			if (w->type == OBJ_MAP) continue;
			if (w == o) continue;
			w->status = OBJ_STATUS_HIBERNATE;
		}
		pal_upload(MAP_TILE_CRAM_POSITION, res_pal_pause_bin, sizeof(res_pal_pause_bin) / 2);
	}
	else if (!e->paused && was_paused)
	{
		draw_blank_window();
		// Wake all objects that were hibernated.
		for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
		{
			Obj *w = &g_objects[i].obj;
			if (w->status != OBJ_STATUS_HIBERNATE) continue;
			w->status = OBJ_STATUS_ACTIVE;
		}
		map_upload_palette();
	}

	// Render details.
	if (e->paused)
	{
		OBJ_SIMPLE_ANIM(e->cursor_flash_cnt, e->cursor_flash_frame, 2, kcursor_flash_delay);
		draw_location(e);
		draw_pause();
	}

	vdp_set_window_top(was_paused ? 31 : 0);

	e->buttons_prev = buttons;
}

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kcursor_flash_delay = PALSCALE_DURATION(12);
	s_constants_set = 1;
}

void o_load_pause(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Pause) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-1), INTTOFIX16(1), INTTOFIX16(-2), 1);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_pause(void)
{
	s_vram_pos = 0;
}
