#include "obj/pause.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "obj/bg.h"

#include "powerup.h"
#include "progress.h"
#include "projectile.h"
#include "particle.h"
#include "game.h"
#include "lyle.h"
#include "sfx.h"
#include "map_file.h"
#include "music.h"
#include "str.h"
#include "input.h"

#include <string.h>

static struct
{
	int16_t main_cursor;
	int16_t room_cursor;
	int16_t room_last_page;
	int16_t chosen_room_id;

	int16_t sound_cursor;
	int16_t bgm_id;
	int16_t sfx_id;
	uint16_t input_cheat_idx;

	int16_t progress_cursor_main;
	int16_t progress_cursor_bit;

	uint16_t vram_view_offset;
	int16_t vram_view_pal;
} s_debug;

static O_Pause *s_pause;
static uint16_t s_vram_pos;
static uint16_t s_vram_kana_pos;

static int16_t kcursor_flash_delay;
static int16_t kdismissal_delay_frames;
static int16_t kselect_delay_frames;
static int16_t kmenu_flash_delay;

static const uint16_t kmap_left = 8;
static const uint16_t kmap_top = 5;

static void maybe_load_kana_in_vram(void)
{
	if (s_vram_kana_pos) return;
	const Gfx *g_kana = gfx_get(GFX_EX_KANA_FONT);
	s_vram_kana_pos = gfx_load(g_kana, obj_vram_alloc(g_kana->size));
}

// String printing utility. Not very fast, but it's alright for this.
static void plot_string(const char *str, int16_t x, int16_t y, int16_t pal)
{
	uint16_t plane_base = md_vdp_get_plane_base(VDP_PLANE_WINDOW) +
	                      2 * ((y * GAME_PLANE_W_CELLS) + x);
	md_vdp_set_autoinc(2);
	md_vdp_set_addr(plane_base);
	while (*str)
	{
		uint8_t *s_u = (uint8_t *)str;
		char a = *str++;
		if (!a) break;
		if (a == '\n' || a == '\r')
		{
			y++;
			plane_base = md_vdp_get_plane_base(VDP_PLANE_WINDOW) +
			             2 * ((y * GAME_PLANE_W_CELLS) + x);
			md_vdp_set_addr(plane_base);
		}
		// All hiragana, katakana, kanbun, etc. start at $3000.
		else if (s_u[0] == 0xE3)
		{
			const uint8_t high_nybble = (s_u[1] & 0x3C) >> 2;
			// We can only handle the first page, contaning Kana.
			str += 2;
			if (high_nybble != 0) continue;
			const uint8_t char_pos = (s_u[2] & 0x3F) | ((s_u[1] & 0x3) << 6);

			md_vdp_write(VDP_ATTR(s_vram_kana_pos + char_pos, 0, 0, pal, 0));
			plane_base += 2;
		}
		// TODO: Consider ES, FR
		else
		{
			if (a == '(') a = '<';
			else if (a == ')') a = '>';

			if (a >= 'a' && a <= 'z')
			{
				md_vdp_write(VDP_ATTR(s_vram_pos + 0x80 - 0x20 + (a & ~(0x20)), 0, 0, pal, 0));
			}
			else if (a == ' ')
			{
				md_vdp_write(VDP_ATTR(s_vram_pos + 0x30, 0, 0, pal, 0));
			}
			else
			{
				md_vdp_write(VDP_ATTR(s_vram_pos + 0x80 - 0x20 + (a), 0, 0, pal, 0));
			}
			plane_base += 2;
		}
	}
}

// Map drawing routines -------------------------------------------------------
static inline void clear_window_plane(void)
{
	uint16_t plane_base = md_vdp_get_plane_base(VDP_PLANE_WINDOW);
	const uint16_t plane_size = GAME_PLANE_W_CELLS * 30;
	const uint16_t fill_tile = VDP_ATTR(s_vram_pos + 0x30, 0, 0, MAP_PAL_LINE, 0);
	md_dma_fill_vram(plane_base + 1, ((fill_tile) & 0xFF00) >> 8, plane_size - 1, 2);
	md_dma_fill_vram(plane_base, (fill_tile) & 0xFF, plane_size - 1, 2);
	md_dma_process();
	md_vdp_poke(plane_base, fill_tile);  // TODO: Is DMA fill really buggy?
}

static inline void plot_map_side_borders(void)
{
	uint16_t plane_base = md_vdp_get_plane_base(VDP_PLANE_WINDOW) +
	                      2 * (((kmap_top - 1) * GAME_PLANE_W_CELLS) +
	                      kmap_left - 1);
	const uint16_t right_offset = 2 * (PROGRESS_MAP_W + 1);

	for (int16_t y = 0; y < PROGRESS_MAP_H + 2; y++)
	{
		// Left side.
		md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos, 0, 0, MAP_PAL_LINE, 0));
		md_vdp_poke(plane_base - 2,
		         VDP_ATTR(s_vram_pos + 0x31, 0, 0, MAP_PAL_LINE, 0));
		// Right side.
		md_vdp_poke(plane_base + right_offset,
		         VDP_ATTR(s_vram_pos, 0, 0, MAP_PAL_LINE, 0));
		plane_base += 2 * GAME_PLANE_W_CELLS;
	}
}

static inline void plot_map_top_bottom_borders(void)
{
	uint16_t plane_base = md_vdp_get_plane_base(VDP_PLANE_WINDOW) +
	                      2 * (((kmap_top - 2) * GAME_PLANE_W_CELLS) +
	                      kmap_left - 1);
	const uint16_t bottom_offset = 2 * (GAME_PLANE_W_CELLS * (PROGRESS_MAP_H + 2));
	const uint16_t padding_offset = 2 * (GAME_PLANE_W_CELLS);
	// Corner.
	md_vdp_poke(plane_base - 2, VDP_ATTR(s_vram_pos + 0x32,
	                                  0, 0, MAP_PAL_LINE, 0));

	for (int16_t x = 0; x < PROGRESS_MAP_W + 2; x++)
	{
		// Top side.
		md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x33,
		                              0, 0, MAP_PAL_LINE, 0));
		// Top padding.
		md_vdp_poke(plane_base + padding_offset, VDP_ATTR(s_vram_pos,
		                                               0, 0, MAP_PAL_LINE, 0));
		// Bottom side.
		md_vdp_poke(plane_base + bottom_offset,
		         VDP_ATTR(s_vram_pos, 0, 0, MAP_PAL_LINE, 0));
		plane_base += 2;
	}
}

static inline void plot_map(void)
{
	const ProgressSlot *progress = progress_get();
	uint16_t plane_base = md_vdp_get_plane_base(VDP_PLANE_WINDOW);
	const uint16_t tile_base = VDP_ATTR(s_vram_pos, 0, 0, MAP_PAL_LINE, 0);
	uint16_t pausemap_offset = 0;

	md_vdp_set_autoinc(2);

	plane_base += 2 * ((kmap_top * GAME_PLANE_W_CELLS) + kmap_left);
	for (int16_t y = 0; y < PROGRESS_MAP_H; y++)
	{
		md_vdp_set_addr(plane_base);
		for (int16_t x = 0; x < PROGRESS_MAP_W; x++)
		{
			if (map_get_world_y_tile() > PROGRESS_MAP_H ||
			    !(progress->abilities & ABILITY_MAP) || !progress->map_explored[y][x])
			{
				md_vdp_write(tile_base);
			}
			else
			{
				md_vdp_write(tile_base + res_pausemap_bin[pausemap_offset]);
			}
			pausemap_offset++;
		}
		plane_base += 2 * (GAME_PLANE_W_CELLS);
	}
}

static inline void plot_map_cube_sector_extension(void)
{
	static const uint16_t kcube_sector_cells = 13;
	static const uint16_t kleft = kmap_left +
	                              (PROGRESS_MAP_W - kcube_sector_cells) / 2;
	static const uint16_t ktop = kmap_top + PROGRESS_MAP_H + 1;

	uint16_t plane_base = md_vdp_get_plane_base(VDP_PLANE_WINDOW);

	plane_base += 2 * ((ktop * GAME_PLANE_W_CELLS) + kleft);
	for (int16_t y = 0; y < 2; y++)
	{
		// Left side border
		md_vdp_poke(plane_base - 2, VDP_ATTR(s_vram_pos + 0x31, 0, 0, MAP_PAL_LINE, 0));
		// Cells on bottom
		for (int16_t x = 0; x < kcube_sector_cells; x++)
		{
			md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos, 0, 0, MAP_PAL_LINE, 0));
			plane_base += 2;
		}
		plane_base += 2 * (GAME_PLANE_W_CELLS - kcube_sector_cells);
	}
}

static void plot_map_to_window_plane(void)
{
	plot_map_side_borders();
	plot_map_top_bottom_borders();
	plot_map_cube_sector_extension();
	plot_map();
}

static inline void plot_item_display_border(uint16_t x, uint16_t y)
{
	uint16_t plane_base = md_vdp_get_plane_base(VDP_PLANE_WINDOW);
	x /= 8;
	y /= 8;
	plane_base += 2 * ((y * GAME_PLANE_W_CELLS) + x);

	md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x85, 1, 0, ENEMY_PAL_LINE, 0));
	md_vdp_poke(plane_base + 2, VDP_ATTR(s_vram_pos + 0x85, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += GAME_PLANE_W_CELLS * 2;
	md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x85, 1, 1, ENEMY_PAL_LINE, 0));
	md_vdp_poke(plane_base + 2, VDP_ATTR(s_vram_pos + 0x85, 0, 1, ENEMY_PAL_LINE, 0));
}

static void plot_item_display_borders(void)
{
	plot_item_display_border(96, 192);
	plot_item_display_border(120, 192);
	plot_item_display_border(152, 192);
	plot_item_display_border(184, 192);
	plot_item_display_border(208, 192);
}

static void draw_cp_orb_count(void)
{
	const ProgressSlot *progress = progress_get();
	const int16_t cp_orbs = progress->collected_cp_orbs;
	md_spr_put(272, 204, VDP_ATTR(s_vram_pos + 0x3C, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(2, 2));
	md_spr_put(288, 204, VDP_ATTR(s_vram_pos + 0x40 + ((cp_orbs > 10) ? 2 : 0), 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 2));
	md_spr_put(295, 204, VDP_ATTR(s_vram_pos + 0x40 + (2 * (cp_orbs % 10)), 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 2));
}

static void draw_item_icons(void)
{
	const ProgressSlot *progress = progress_get();
	
	if (progress->abilities & ABILITY_LIFT)
	{
		md_spr_put(100, 196, VDP_ATTR(s_vram_pos + 0x70, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));
	}
	
	if (progress->abilities & ABILITY_JUMP)
	{
		md_spr_put(124, 196, VDP_ATTR(s_vram_pos + 0x72, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));
	}

	if (progress->abilities & ABILITY_PHANTOM)
	{
		md_spr_put(152, 192, VDP_ATTR(s_vram_pos + 0x77, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(2, 2));
		int16_t phantom_level = 0;
		if (progress->abilities & ABILITY_FAST_PHANTOM) phantom_level++;
		if (progress->abilities & ABILITY_CHEAP_PHANTOM) phantom_level++;
		if (progress->abilities & ABILITY_2X_DAMAGE_PHANTOM) phantom_level++;

		md_spr_put(152, 208, VDP_ATTR(s_vram_pos + 0x54, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 1));
		md_spr_put(160, 208, VDP_ATTR(s_vram_pos + 0x55 + phantom_level, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 1));
	}

	if (progress->abilities & ABILITY_KICK)
	{
		md_spr_put(184, 192, VDP_ATTR(s_vram_pos + 0x73, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));
	}

	if (progress->abilities & ABILITY_ORANGE)
	{
		md_spr_put(212, 196, VDP_ATTR(s_vram_pos + 0x71, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 1));
	}
}

static inline void draw_map_pause_text(void)
{
	static const int16_t draw_x = (GAME_SCREEN_W_PIXELS / 2) - 28;
	static const int16_t draw_y = 8;

	md_spr_put(draw_x, draw_y,
	        SPR_ATTR(s_vram_pos + 0x60, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(4, 2));
	md_spr_put(draw_x + 32, draw_y,
	        SPR_ATTR(s_vram_pos + 0x68, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(4, 2));
}

static inline void draw_cube_sector_text(void)
{
	int16_t x = (GAME_SCREEN_W_PIXELS / 2) - 38;
	static const int16_t y = 148;
	const int16_t tile_base = SPR_ATTR(s_vram_pos + 0x34, 0, 0, MAP_PAL_LINE, 0);
	static const int16_t cube_mapping[] =
	{
		0, 1, 2, 3
	};
	static const int16_t sector_mapping[] =
	{
		4, 3, 0, 5, 6, 7
	};
	
	for (uint16_t i = 0; i < ARRAYSIZE(cube_mapping); i++)
	{
		md_spr_put(x, y, tile_base + cube_mapping[i], SPR_SIZE(1, 1));
		x += 8;
	}
	x += 3;
	
	for (uint16_t i = 0; i < ARRAYSIZE(sector_mapping); i++)
	{
		md_spr_put(x, y, tile_base + sector_mapping[i], SPR_SIZE(1, 1));
		x += 8;
	}
}

static inline void draw_map_location(O_Pause *e)
{
	const ProgressSlot *progress = progress_get();
	if (map_get_world_y_tile() <= PROGRESS_MAP_H && (progress->abilities & ABILITY_MAP))
	{
		if (e->cursor_flash_frame == 0) return;
		const int16_t px = FIX32TOINT(lyle_get_x());
		const int16_t py = FIX32TOINT(lyle_get_y());
		const int16_t x_index = map_get_world_x_tile() + (px / GAME_SCREEN_W_PIXELS);
		const int16_t y_index = map_get_world_y_tile() + (py / GAME_SCREEN_H_PIXELS);
		const int16_t draw_x = 8 * kmap_left + (x_index * 8) + 2;
		const int16_t draw_y = 8 * kmap_top + (y_index * 8) + 2;

		md_spr_put(draw_x, draw_y,
		        SPR_ATTR(s_vram_pos + 0x2F, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 1));
	}
	else
	{
		static const int16_t knomap_w = 54;
		const int16_t draw_x = (8 * kmap_left + (4 * PROGRESS_MAP_W) - (knomap_w / 2));
		const int16_t draw_y = (8 * kmap_top) + (4 * PROGRESS_MAP_H) + 4;
		md_spr_put(draw_x, draw_y,
		        SPR_ATTR(s_vram_pos + 0x5A, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(2, 1));
		md_spr_put(draw_x + 27, draw_y,
		        SPR_ATTR(s_vram_pos + 0x5C, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(4, 1));
	}
}

// "You Got" screen drawing routines ------------------------------------------
typedef struct CharMapping
{
	int8_t x;
	int8_t y;
	char val;
} CharMapping;

static const CharMapping kmapping_this_is_a_cube[] =
{
	{12, 20, 'T'},
	{20, 20, 'H'},
	{28, 20, 'I'},
	{36, 20, 'S'},
	{52, 20, 'I'},
	{60, 20, 'S'},
	{16, 28, 'A'},
	{32, 28, 'C'},
	{40, 28, 'U'},
	{48, 28, 'B'},
	{56, 28, 'E'},
};

static const CharMapping kmapping_map[] =
{
	{28, 24, 'M'},
	{36, 24, 'A'},
	{44, 24, 'P'},
};

static const CharMapping kmapping_cube_lift[] =
{
	{ 4, 24, 'C'},
	{12, 24, 'U'},
	{20, 24, 'B'},
	{28, 24, 'E'},
	{44, 24, 'L'},
	{52, 24, 'I'},
	{60, 24, 'F'},
	{68, 24, 'T'},
};

static const CharMapping kmapping_cube_jump[] =
{
	{ 4, 24, 'C'},
	{12, 24, 'U'},
	{20, 24, 'B'},
	{28, 24, 'E'},
	{44, 24, 'J'},
	{52, 24, 'U'},
	{60, 24, 'M'},
	{68, 24, 'P'},
};

static const CharMapping kmapping_cube_kick[] =
{
	{ 4, 24, 'C'},
	{12, 24, 'U'},
	{20, 24, 'B'},
	{28, 24, 'E'},
	{44, 24, 'K'},
	{52, 24, 'I'},
	{60, 24, 'C'},
	{68, 24, 'K'},
};

static const CharMapping kmapping_big_cube_lift[] =
{
	{28, 20, 'B'},
	{36, 20, 'I'},
	{44, 20, 'G'},
	{ 4, 28, 'C'},
	{12, 28, 'U'},
	{20, 28, 'B'},
	{28, 28, 'E'},
	{44, 28, 'L'},
	{52, 28, 'I'},
	{60, 28, 'F'},
	{68, 28, 'T'},
};

static const CharMapping kmapping_phantom_cube_magic_1[] =
{
	{12, 20, 'P'},
	{20, 20, 'H'},
	{28, 20, 'A'},
	{36, 20, 'N'},
	{44, 20, 'T'},
	{52, 20, 'O'},
	{60, 20, 'M'},
	{ 1+2, 28, 'C'},
	{ 9+2, 28, 'U'},
	{17+2, 28, 'B'},
	{25+2, 28, 'E'},
	{36+8, 28, 'M'},
	{44+8, 28, 0x3F},  // Shorter A
	{51+8, 28, 'G'},
	{59+8, 28, 0x3A},  // Shorter I
	{62+8, 28, 'C'},
};

static const CharMapping kmapping_phantom_cube_magic_x[] =
{
	{12, 20, 'P'},
	{20, 20, 'H'},
	{28, 20, 'A'},
	{36, 20, 'N'},
	{44, 20, 'T'},
	{52, 20, 'O'},
	{60, 20, 'M'},
	{ 1, 28, 'C'},
	{ 9, 28, 'U'},
	{17, 28, 'B'},
	{25, 28, 'E'},
	{36, 28, 'M'},
	{44, 28, 0x3F},  // Shorter A
	{51, 28, 'G'},
	{59, 28, 0x3A},  // Shorter I
	{62, 28, 'C'},
};

static const CharMapping kmapping_2[] =
{
	{71, 28, '2'},
};

static const CharMapping kmapping_3[] =
{
	{71, 28, '3'},
};

static const CharMapping kmapping_4[] =
{
	{71, 28, '4'},
};

static void draw_char_mapping(int16_t base_x, int16_t base_y,
                              const CharMapping *mapping, int16_t size)
{
	for (int16_t i = 0; i < size; i++)
	{
		md_spr_put(base_x + mapping[i].x, base_y + mapping[i].y,
		        SPR_ATTR(s_vram_pos + 0x80 - 0x20 + mapping[i].val, 0, 0,
		                 ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));
	}
}

static void draw_you_got(PauseScreen screen)
{
	static const int16_t base_x = 120;
	const int16_t base_y = 32 - (system_is_ntsc() ? 8 : 0);
	int16_t show_you_got = 1;

	const uint16_t powerup_vram_pos = powerup_get_vram_pos();

	static const uint16_t pal = MAP_PAL_LINE;

	switch (screen)
	{
		case PAUSE_SCREEN_LYLE_WEAK:
			draw_char_mapping(base_x, base_y, kmapping_this_is_a_cube, ARRAYSIZE(kmapping_this_is_a_cube));
			md_spr_put(88, base_y + 12, SPR_ATTR(g_cube_vram_pos, 0, 0, BG_PAL_LINE, 0), SPR_SIZE(2, 2));
			md_spr_put(216, base_y + 12, SPR_ATTR(g_cube_vram_pos, 0, 0, BG_PAL_LINE, 0), SPR_SIZE(2, 2));
			// Uses same layout, but does not show "you got".
			show_you_got = 0;
			break;
		case PAUSE_SCREEN_GET_MAP:
			md_spr_put(88, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x18, 0, 0, pal, 0), SPR_SIZE(2, 2));
			md_spr_put(216, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x18, 0, 0, pal, 0), SPR_SIZE(2, 2));
			draw_char_mapping(base_x, base_y, kmapping_map, ARRAYSIZE(kmapping_map));
			break;
		case PAUSE_SCREEN_GET_CUBE_LIFT:
			md_spr_put(88, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x1C, 0, 0, pal, 0), SPR_SIZE(2, 2));
			md_spr_put(216, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x1C, 0, 0, pal, 0), SPR_SIZE(2, 2));
			draw_char_mapping(base_x, base_y, kmapping_cube_lift,
			                  ARRAYSIZE(kmapping_cube_lift));
			break;
		case PAUSE_SCREEN_GET_CUBE_JUMP:
			md_spr_put(88, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x20, 0, 0, pal, 0), SPR_SIZE(2, 2));
			md_spr_put(216, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x20, 0, 0, pal, 0), SPR_SIZE(2, 2));
			draw_char_mapping(base_x, base_y, kmapping_cube_jump,
			                  ARRAYSIZE(kmapping_cube_jump));
			break;
		case PAUSE_SCREEN_GET_CUBE_KICK:
			md_spr_put(88, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x28, 0, 0, pal, 0), SPR_SIZE(2, 2));
			md_spr_put(216, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x28, 0, 0, pal, 0), SPR_SIZE(2, 2));
			draw_char_mapping(base_x, base_y, kmapping_cube_kick,
			                  ARRAYSIZE(kmapping_cube_kick));
			break;
		case PAUSE_SCREEN_GET_ORANGE_CUBE:
			md_spr_put(88, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x2C, 0, 0, pal, 0), SPR_SIZE(2, 2));
			md_spr_put(216, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x2C, 0, 0, pal, 0), SPR_SIZE(2, 2));
			draw_char_mapping(base_x, base_y, kmapping_big_cube_lift,
			                  ARRAYSIZE(kmapping_big_cube_lift));
			break;
		case PAUSE_SCREEN_GET_PHANTOM:
			md_spr_put(88, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x24, 0, 0, pal, 0), SPR_SIZE(2, 2));
			md_spr_put(216, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x24, 0, 0, pal, 0), SPR_SIZE(2, 2));
			draw_char_mapping(base_x, base_y, kmapping_phantom_cube_magic_1,
			                  ARRAYSIZE(kmapping_phantom_cube_magic_1));
			break;
		case PAUSE_SCREEN_GET_PHANTOM_DOUBLE_DAMAGE:
			md_spr_put(88, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x24, 0, 0, pal, 0), SPR_SIZE(2, 2));
			md_spr_put(216, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x24, 0, 0, pal, 0), SPR_SIZE(2, 2));
			draw_char_mapping(base_x, base_y, kmapping_phantom_cube_magic_x,
			                  ARRAYSIZE(kmapping_phantom_cube_magic_x));
			draw_char_mapping(base_x, base_y, kmapping_2,
			                  ARRAYSIZE(kmapping_2));
			break;
		case PAUSE_SCREEN_GET_PHANTOM_HALF_TIME:
			md_spr_put(88, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x24, 0, 0, pal, 0), SPR_SIZE(2, 2));
			md_spr_put(216, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x24, 0, 0, pal, 0), SPR_SIZE(2, 2));
			draw_char_mapping(base_x, base_y, kmapping_phantom_cube_magic_x,
			                  ARRAYSIZE(kmapping_phantom_cube_magic_x));
			draw_char_mapping(base_x, base_y, kmapping_3,
			                  ARRAYSIZE(kmapping_3));
			break;
		case PAUSE_SCREEN_GET_PHANTOM_CHEAP:
			md_spr_put(88, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x24, 0, 0, pal, 0), SPR_SIZE(2, 2));
			md_spr_put(216, base_y + 12, VDP_ATTR(powerup_vram_pos + 0x24, 0, 0, pal, 0), SPR_SIZE(2, 2));
			draw_char_mapping(base_x, base_y, kmapping_phantom_cube_magic_x,
			                  ARRAYSIZE(kmapping_phantom_cube_magic_x));
			draw_char_mapping(base_x, base_y, kmapping_4,
			                  ARRAYSIZE(kmapping_4));
			break;
		default:
			// TODO: HP Orb, CP Orb
			// CP orb uses Lyle's pal line, while HP uses the BG common one.
			break;
	}

	if (!show_you_got) return;
	// "YOU GOT:" header.
	md_spr_put(base_x + 13, base_y + 9, SPR_ATTR(s_vram_pos + 0x7B, 0, 0,
	                       ENEMY_PAL_LINE, 0), SPR_SIZE(3, 1));
	md_spr_put(base_x + 40, base_y + 9, SPR_ATTR(s_vram_pos + 0x7E, 0, 0,
	                       ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));
	md_spr_put(base_x + 48, base_y + 9, SPR_ATTR(s_vram_pos + 0x7C, 0, 0,
	                       ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));
	md_spr_put(base_x + 56, base_y + 9, SPR_ATTR(s_vram_pos + 0x7F, 0, 0,
	                       ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));
	md_spr_put(base_x + 64, base_y + 9, SPR_ATTR(s_vram_pos + 0x8F, 0, 0,
	                       ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));

}

static void plot_get_sides(uint16_t plane_base)
{
	for (int16_t y = 0; y < 22; y++)
	{
		md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
		md_vdp_poke(plane_base + 2*22, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
		plane_base += GAME_PLANE_W_CELLS * 2;
	}
	// Corner piece.
	md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x84, 0, 0, ENEMY_PAL_LINE, 0));
}

static void plot_get_top_and_bottom(uint16_t plane_base)
{
	plane_base += 2;
	for (int16_t x = 0; x < 21; x++)
	{
		md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x83, 0, 0, ENEMY_PAL_LINE, 0));
		md_vdp_poke(plane_base + (GAME_PLANE_W_CELLS * 5 * 2), VDP_ATTR(s_vram_pos + 0x83, 0, 0, ENEMY_PAL_LINE, 0));
		md_vdp_poke(plane_base + (GAME_PLANE_W_CELLS * 22 * 2), VDP_ATTR(s_vram_pos + 0x83, 0, 0, ENEMY_PAL_LINE, 0));
		plane_base += 2;
	}
	// Corner.
	md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x85, 0, 0, ENEMY_PAL_LINE, 0));
	md_vdp_poke(plane_base + (GAME_PLANE_W_CELLS * 5 * 2), VDP_ATTR(s_vram_pos + 0x85, 0, 0, ENEMY_PAL_LINE, 0));
	md_vdp_poke(plane_base + (GAME_PLANE_W_CELLS * 22 * 2), VDP_ATTR(s_vram_pos + 0x83, 0, 0, ENEMY_PAL_LINE, 0));
}

static void plot_get_side_grid(uint16_t plane_base)
{
	/* clang-format off */
	static const uint8_t grid_mapping[] =
	{
		0x81,0x81,0x81,0x81,0x81,0x85,
		0x82,0x82,0x82,0x82,0x82,0x1A,
		0x82,0x82,0x82,0x82,0x82,0x1A,
		0x82,0x82,0x82,0x82,0x82,0x1A,
		0x82,0x82,0x82,0x82,0x82,0x1A,
	};
	/* clang-format on */
	int16_t map_index = 0;
	for (uint16_t i = 0; i < ARRAYSIZE(grid_mapping) / 6; i++)
	{
		for (int16_t j = 0; j < 6; j++)
		{
			md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + grid_mapping[map_index],
			                              0, 0, ENEMY_PAL_LINE, 0));
			plane_base += 2;
			map_index += 1;
		}
		plane_base += 2 * (GAME_PLANE_W_CELLS - 6);
	}
}

static void plot_get_right_grid_addition(uint16_t plane_base)
{
	plane_base += (2 * 16);
	md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x85, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += 2 * (GAME_PLANE_W_CELLS);
	md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += 2 * (GAME_PLANE_W_CELLS);
	md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += 2 * (GAME_PLANE_W_CELLS);
	md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += 2 * (GAME_PLANE_W_CELLS);
	md_vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += 2 * (GAME_PLANE_W_CELLS);
}

static void plot_get_dialogue_text(PauseScreen screen)
{
	static const StringId string_ids[] =
	{
		[PAUSE_SCREEN_LYLE_WEAK] = STR_LYLE_TOO_WEAK,
		[PAUSE_SCREEN_GET_MAP] = STR_GET_MAP,
		[PAUSE_SCREEN_GET_CUBE_LIFT] = STR_GET_CUBE_LIFT,
		[PAUSE_SCREEN_GET_CUBE_JUMP] = STR_GET_CUBE_JUMP,
		[PAUSE_SCREEN_GET_CUBE_KICK] = STR_GET_CUBE_KICK,
		[PAUSE_SCREEN_GET_ORANGE_CUBE] = STR_GET_ORANGE_CUBE,
		[PAUSE_SCREEN_GET_PHANTOM] = STR_GET_PHANTOM,
		[PAUSE_SCREEN_GET_PHANTOM_DOUBLE_DAMAGE] = STR_GET_PHANTOM_DOUBLE_DAMAGE,
		[PAUSE_SCREEN_GET_PHANTOM_HALF_TIME] = STR_GET_PHANTOM_HALF_TIME,
		[PAUSE_SCREEN_GET_PHANTOM_CHEAP] = STR_GET_PHANTOM_CHEAP,
		[PAUSE_SCREEN_HP_ORB_0] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_1] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_2] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_3] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_4] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_5] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_6] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_7] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_8] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_9] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_10] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_11] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_12] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_13] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_14] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_HP_ORB_15] = STR_GET_HP_ORB,
		[PAUSE_SCREEN_CP_ORB_0] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_1] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_2] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_3] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_4] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_5] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_6] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_7] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_8] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_9] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_10] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_11] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_12] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_13] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_14] = STR_GET_CP_ORB,
		[PAUSE_SCREEN_CP_ORB_15] = STR_GET_CP_ORB,
	};

	const char *str = str_get(string_ids[screen]);
	if (!str) return;
	maybe_load_kana_in_vram();

	// Draw the string to the window plane.
	static const int16_t kleft = 10;
	const int16_t ktop = 10 - (system_is_ntsc() ? 1 : 0);
	plot_string(str, kleft, ktop, ENEMY_PAL_LINE);
}

static void plot_get_dialogue_backing(PauseScreen screen)
{
	static const int base_x = 8;
	const int base_y = (system_is_ntsc() ? 3 : 4);
	const uint16_t plane_base = md_vdp_get_plane_base(VDP_PLANE_WINDOW) +
	                            (2 * (base_x + (GAME_PLANE_W_CELLS * base_y)));
	plot_get_sides(plane_base);
	plot_get_top_and_bottom(plane_base);
	plot_get_side_grid(plane_base + 2);
	plot_get_side_grid(plane_base + (2 * 16) + 2);
	plot_get_right_grid_addition(plane_base);
	plot_get_dialogue_text(screen);
}

// ----------------------------------------------------------------------------

static void wake_objects(void)
{
	// Wake all objects that were hibernated.
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *w = &g_objects[i].obj;
		if (w->status != OBJ_STATUS_HIBERNATE) continue;
		w->status = OBJ_STATUS_ACTIVE;
	}
	lyle_set_hibernate(0);
	powerup_set_hibernate(0);
	projectile_set_hibernate(0);
	particle_set_hibernate(0);
}

static void hibernate_objects()
{
	// Hibernate all objects besides this one.
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *w = &g_objects[i].obj;
		if (w->status == OBJ_STATUS_NULL) continue;
		if (w->type == OBJ_MAP) continue;
		if (w->type == OBJ_PAUSE) continue;
		w->status = OBJ_STATUS_HIBERNATE;
	}
	lyle_set_hibernate(1);
	powerup_set_hibernate(1);
	projectile_set_hibernate(1);
	particle_set_hibernate(1);
}

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_PAUSE);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static void maybe_dismiss(O_Pause *e, LyleBtn buttons, int16_t min_delay)
{
	if (e->dismissal_delay_cnt < min_delay)
	{
		e->dismissal_delay_cnt++;
		return;
	}

	static const uint16_t kbutton_mask = (LYLE_BTN_START | LYLE_BTN_JUMP);

	if (e->select_delay_cnt == 0 && (buttons & kbutton_mask) && !(e->buttons_prev & kbutton_mask))
	{
		sfx_play(SFX_SELECT_1, 0);
		sfx_play(SFX_SELECT_2, 0);
		e->select_delay_cnt = 1;
	}
}

static void maybe_map(O_Pause *e, LyleBtn buttons)
{
	if ((buttons & LYLE_BTN_START) && !(e->buttons_prev & LYLE_BTN_START))
	{
		e->screen = PAUSE_SCREEN_MAP;
	}
}

// Clears the window and plays the sound.
static void screen_reset(O_Pause *e)
{
	sfx_play(SFX_PAUSE_1, 1);
	sfx_play(SFX_PAUSE_2, 1);
	clear_window_plane();
	e->window = true;
}

static void maybe_switch_to_debug(O_Pause *e, LyleBtn buttons)
{
	static const LyleBtn kcheat_sequence[] =
	{
		LYLE_BTN_LEFT,
		LYLE_BTN_LEFT,
		LYLE_BTN_LEFT,
		LYLE_BTN_LEFT,
		LYLE_BTN_RIGHT,
		LYLE_BTN_LEFT,
		LYLE_BTN_RIGHT,
		LYLE_BTN_LEFT,
		LYLE_BTN_RIGHT,
		LYLE_BTN_LEFT,
		LYLE_BTN_LEFT,
		LYLE_BTN_RIGHT
	};

	if (s_debug.input_cheat_idx >= ARRAYSIZE(kcheat_sequence))
	{
		e->screen = PAUSE_SCREEN_DEBUG;
		s_debug.input_cheat_idx = 0;
		return;
	}

	const LyleBtn next_btn = kcheat_sequence[s_debug.input_cheat_idx];
	const LyleBtn edge = buttons & ~e->buttons_prev;

	if (!edge) return;

	if (edge == next_btn)
	{
		s_debug.input_cheat_idx++;
	}
	else
	{
		s_debug.input_cheat_idx = 0;
	}
}

// Debug top-level menu.
typedef struct DebugOption
{
	const char *label;
	PauseScreen screen;
} DebugOption;

static const DebugOption debug_options[] =
{
	{"ROOM SELECT", PAUSE_SCREEN_ROOM_SELECT},
	{"SOUND TEST", PAUSE_SCREEN_SOUND_TEST},
	{"PROGRESS EDIT", PAUSE_SCREEN_PROGRESS_EDIT},
	{"VRAM VIEW", PAUSE_SCREEN_VRAM_VIEW},
	{"BUTTON CHECK", PAUSE_SCREEN_BUTTON_CHECK},
	{"RAM VIEW", 0},
	{"EXIT", PAUSE_SCREEN_NONE},
};

static const int16_t kdebug_left = 4;
static const int16_t kdebug_top = 2;

static void plot_debug_menu(void)
{
	plot_string("@ DEBUG @", kdebug_left, kdebug_top, ENEMY_PAL_LINE);

	for (uint16_t i = 0; i < ARRAYSIZE(debug_options); i++)
	{
		const DebugOption *d = &debug_options[i];
		plot_string(d->label, kdebug_left + 4, kdebug_top + 2 + (2 * i), MAP_PAL_LINE);
	}

	plot_string("BUTTON C @ SELECT", kdebug_left, kdebug_top + 20, ENEMY_PAL_LINE);
	plot_string("BUTTON B @ EXIT", kdebug_left, kdebug_top + 21, ENEMY_PAL_LINE);
}

static void debug_menu_logic(O_Pause *e, LyleBtn buttons)
{
	static const LyleBtn btn_chk = (LYLE_BTN_JUMP | LYLE_BTN_START);
	if ((buttons & btn_chk) && !(e->buttons_prev & btn_chk))
	{
		e->screen = debug_options[s_debug.main_cursor].screen;
		return;
	}

	if ((buttons & LYLE_BTN_UP) && !(e->buttons_prev & LYLE_BTN_UP))
	{
		s_debug.main_cursor--;
		if (s_debug.main_cursor < 0)
		{
			s_debug.main_cursor = ARRAYSIZE(debug_options) - 1;
		}
	}
	else if ((buttons & LYLE_BTN_DOWN) && !(e->buttons_prev & LYLE_BTN_DOWN))
	{
		s_debug.main_cursor++;
		if (s_debug.main_cursor >= (int16_t)ARRAYSIZE(debug_options))
		{
			s_debug.main_cursor = 0;
		}
	}
}

static void draw_debug_main_cursor(O_Pause *e)
{
	if (e->cursor_flash_frame == 0) return;
	md_spr_put((kdebug_left + 2) * 8, (kdebug_top + 2 + (s_debug.main_cursor * 2)) * 8,
	        VDP_ATTR(s_vram_pos + 0x70, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));
}

// Room select

static void plot_room_select_title(void)
{
	plot_string("@ ROOM SELECT @", kdebug_left, kdebug_top, ENEMY_PAL_LINE);

	plot_string("BUTTON C @ GO TO ROOM", kdebug_left, kdebug_top + 20, ENEMY_PAL_LINE);
	plot_string("BUTTON B @ EXIT", kdebug_left, kdebug_top + 21, ENEMY_PAL_LINE);
}

static void plot_room_select_list(void)
{
	static const int16_t rooms_per_list = 16;
	const int16_t page = s_debug.room_cursor / 16;
	for (int16_t i = 0; i < rooms_per_list; i++)
	{
		const int16_t id = (page * 16) + i;
		if (page != s_debug.room_last_page)
		{
			plot_string("                                ", kdebug_left + 4, kdebug_top + 2 + i, MAP_PAL_LINE);
		}
		if (id >= map_file_count()) continue;
		const MapFile *file = map_file_by_id(id);
		if (!file)
		{
			plot_string(" (empty room slot)", kdebug_left + 4, kdebug_top + 2 + i, MAP_PAL_LINE);
		}
		else
		{
			plot_string(file->name, kdebug_left + 4, kdebug_top + 2 + i, MAP_PAL_LINE);
		}
	}
	s_debug.room_last_page = page;
}

static void draw_room_select_cursor(O_Pause *e)
{
	if (e->cursor_flash_frame == 0) return;
	md_spr_put((kdebug_left + 2) * 8, (kdebug_top + 2 + (s_debug.room_cursor % 16)) * 8,
	        VDP_ATTR(s_vram_pos + 0x70, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));
}

static void debug_room_select_logic(O_Pause *e, LyleBtn buttons)
{
	if ((buttons & LYLE_BTN_UP) && !(e->buttons_prev & LYLE_BTN_UP))
	{
		s_debug.room_cursor--;
		if (s_debug.room_cursor < 0)
		{
			s_debug.room_cursor = map_file_count() - 1;
		}
	}
	else if ((buttons & LYLE_BTN_DOWN) && !(e->buttons_prev & LYLE_BTN_DOWN))
	{
		s_debug.room_cursor++;
		if (s_debug.room_cursor >= map_file_count())
		{
			s_debug.room_cursor = 0;
		}
	}
	else if ((buttons & LYLE_BTN_LEFT) && !(e->buttons_prev & LYLE_BTN_LEFT))
	{
		s_debug.room_cursor = ((s_debug.room_cursor / 16) - 1) * 16;
		while (s_debug.room_cursor < 0)
		{
			s_debug.room_cursor += map_file_count();
		}
	}
	else if ((buttons & LYLE_BTN_RIGHT) && !(e->buttons_prev & LYLE_BTN_RIGHT))
	{
		s_debug.room_cursor = ((s_debug.room_cursor / 16) + 1) * 16;
		while (s_debug.room_cursor >= map_file_count())
		{
			s_debug.room_cursor -= map_file_count();
		}
	}

	static const LyleBtn btn_chk = (LYLE_BTN_JUMP | LYLE_BTN_START);

	if ((buttons & btn_chk) && !(e->buttons_prev & btn_chk))
	{
		s_debug.chosen_room_id = s_debug.room_cursor;
	}
	else if ((buttons & LYLE_BTN_CUBE) && !(e->buttons_prev & LYLE_BTN_CUBE))
	{
		e->screen = PAUSE_SCREEN_DEBUG;
	}
}

// Button check

static void plot_button_check(void)
{
	maybe_load_kana_in_vram();
	plot_string(str_get(STR_BUTTON_CHECK), 4, 4, 0);
#ifndef MDK_TARGET_C2
	char buffer[17];
	for (int j = 0; j < 2; j++)
	{
		uint16_t buttons = g_md_pad[j];
		uint16_t chk_bit = 0x8000;
		for (int i = 0; i < 16; i++)
		{
			buffer[i] = buttons & (chk_bit) ? '1' : '0';
			chk_bit = chk_bit >> 1;
		}
		buffer[16] = '\0';
		plot_string(j == 0 ? "PAD1" : "PAD2", 4, 6 + j, 0);
		plot_string(buffer, 12, 6 + j, 0);
	}
#else
	// TODO:
#endif
}

// Sound test

static void plot_sound_test_title(void)
{
	plot_string("@ SOUND TEST @", kdebug_left, kdebug_top, ENEMY_PAL_LINE);
	plot_string("BGM ID\n\nSFX ID\n\nSILENCE", kdebug_left + 4, kdebug_top + 2, MAP_PAL_LINE);

	plot_string("BUTTON C @ SEND COMMAND", kdebug_left, kdebug_top + 20, ENEMY_PAL_LINE);
	plot_string("BUTTON B @ EXIT", kdebug_left, kdebug_top + 21, ENEMY_PAL_LINE);
}

static void plot_sound_test_ui(void)
{
	char id_str[4];
	id_str[0] = '0' + (s_debug.bgm_id / 100) % 10;
	id_str[1] = '0' + (s_debug.bgm_id / 10) % 10;
	id_str[2] = '0' + s_debug.bgm_id % 10;
	id_str[3] = '\0';
	plot_string(id_str, kdebug_left + 11, kdebug_top + 2, MAP_PAL_LINE);
	id_str[0] = '0' + (s_debug.sfx_id / 100) % 10;
	id_str[1] = '0' + (s_debug.sfx_id / 10) % 10;
	id_str[2] = '0' + s_debug.sfx_id % 10;
	id_str[3] = '\0';
	plot_string(id_str, kdebug_left + 11, kdebug_top + 4, MAP_PAL_LINE);
}

static void draw_sound_test_cursor(O_Pause *e)
{
	if (e->cursor_flash_frame == 0) return;
	md_spr_put((kdebug_left + 2) * 8, (kdebug_top + 2 + (s_debug.sound_cursor % 16) * 2) * 8,
	        VDP_ATTR(s_vram_pos + 0x70, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));
}

static void sound_test_logic(O_Pause *e, LyleBtn buttons)
{
	if ((buttons & LYLE_BTN_UP) && !(e->buttons_prev & LYLE_BTN_UP))
	{
		s_debug.sound_cursor--;
		if (s_debug.sound_cursor < 0)
		{
			s_debug.sound_cursor = 2;
		}
	}
	else if ((buttons & LYLE_BTN_DOWN) && !(e->buttons_prev & LYLE_BTN_DOWN))
	{
		s_debug.sound_cursor++;
		if (s_debug.sound_cursor >= 3)
		{
			s_debug.sound_cursor = 0;
		}
	}

	else if ((buttons & LYLE_BTN_RIGHT) && !(e->buttons_prev & LYLE_BTN_RIGHT))
	{
		if (s_debug.sound_cursor == 0)
		{
			s_debug.bgm_id++;
		}
		else if (s_debug.sound_cursor == 1)
		{
			s_debug.sfx_id++;
		}
	}
	else if ((buttons & LYLE_BTN_LEFT) && !(e->buttons_prev & LYLE_BTN_LEFT))
	{
		if (s_debug.sound_cursor == 0)
		{
			s_debug.bgm_id--;
		}
		else if (s_debug.sound_cursor == 1)
		{
			s_debug.sfx_id--;
		}
	}
	static const LyleBtn btn_chk = (LYLE_BTN_JUMP | LYLE_BTN_START);
	if ((buttons & btn_chk) && !(e->buttons_prev & btn_chk))
	{
		if (s_debug.sound_cursor == 0)
		{
			music_play(s_debug.bgm_id);
		}
		else if (s_debug.sound_cursor == 1)
		{
			sfx_play(s_debug.sfx_id, 0);
		}
		else if (s_debug.sound_cursor == 2)
		{
			music_stop();
			sfx_stop_all();
		}
	}
	else if ((buttons & LYLE_BTN_CUBE) && !(e->buttons_prev & LYLE_BTN_CUBE))
	{
		e->screen = PAUSE_SCREEN_DEBUG;
	}
}

// Progress edit

static void plot_progress_edit_title(void)
{
	plot_string("@ PROGRESS EDIT @", kdebug_left, kdebug_top, ENEMY_PAL_LINE);

	plot_string("BUTTON C @ SELECT", kdebug_left, kdebug_top + 20, ENEMY_PAL_LINE);
	plot_string("BUTTON B @ EXIT", kdebug_left, kdebug_top + 21, ENEMY_PAL_LINE);
}

static inline void plot_hex16(uint16_t data, int16_t x, int16_t y, int16_t pal)
{
	uint16_t plane_addr = md_vdp_get_plane_base(VDP_PLANE_WINDOW) +
	                      2 * ((y * GAME_PLANE_W_CELLS) + x);
	const uint16_t tile_base = VDP_ATTR(s_vram_pos + 0x60, 0, 0, pal, 0);
	md_vdp_set_autoinc(2);
	md_vdp_set_addr(plane_addr);

	md_vdp_write(tile_base + '0');
	md_vdp_write(tile_base + 'X');

	static const char digits[] =
	{
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F'
	};

	static const int16_t data_bits = sizeof(data) * 8;
	static const int16_t digit_bits = 4;
	static const int16_t shift_bits = data_bits - digit_bits;
	for (uint16_t i = 0; i < data_bits / digit_bits; i++)
	{
		md_vdp_write(tile_base + (digits[(data & 0xF000) >> shift_bits]));
		data = data << digit_bits;
	}
}

static inline void plot_bitfield16(uint16_t data, int16_t x, int16_t y, int16_t pal)
{
	uint16_t plane_addr = md_vdp_get_plane_base(VDP_PLANE_WINDOW) +
	                      2 * ((y * GAME_PLANE_W_CELLS) + x);
	md_vdp_set_autoinc(2);
	md_vdp_set_addr(plane_addr);

	const uint16_t tile_base = VDP_ATTR(s_vram_pos + 0x60, 0, 0, pal, 0);
	static const uint16_t mask = 1 << ((sizeof(data) * 8) - 1);

	for (uint16_t i = 0; i < sizeof(data) * 8; i++)
	{
		md_vdp_write(tile_base + ((data & mask) ? '1' : '0'));
		data = data << 1;
	}
}

static inline void plot_bool(int16_t data, int16_t x, int16_t y, int16_t pal)
{
	uint16_t plane_addr = md_vdp_get_plane_base(VDP_PLANE_WINDOW) +
	                      2 * ((y * GAME_PLANE_W_CELLS) + x);
	const uint16_t tile_base = VDP_ATTR(s_vram_pos + 0x60, 0, 0, pal, 0);
	md_vdp_set_addr(plane_addr);
	md_vdp_write(tile_base + (data ? '1' : '0'));
}

typedef enum ProgressEditType
{
	PROGRESS_EDIT_BITFIELD16,
	PROGRESS_EDIT_UINT16,
	PROGRESS_EDIT_BOOL16,
} ProgressEditType;

typedef struct ProgressEditSlot
{
	const char label[32];
	uint16_t *data;
	ProgressEditType type;
} ProgressEditSlot;

static void progress_edit_logic_and_plot(O_Pause *e, LyleBtn buttons)
{
	ProgressSlot *prog = progress_get();

	ProgressEditSlot slots[] =
	{
		{"ABILITIES", &prog->abilities, PROGRESS_EDIT_BITFIELD16},
		{"TELEPORTERS", &prog->teleporters_active, PROGRESS_EDIT_BITFIELD16},
		{"HP ORBS", &prog->hp_orbs, PROGRESS_EDIT_BITFIELD16},
		{"HP CAPACITY", &prog->hp_capacity, PROGRESS_EDIT_UINT16},
		{"CP ORBS", &prog->cp_orbs, PROGRESS_EDIT_BITFIELD16},
		{"CP ORBS COLLECTED", &prog->collected_cp_orbs, PROGRESS_EDIT_UINT16},
		{"CP ORBS REGISTERED", &prog->registered_cp_orbs, PROGRESS_EDIT_UINT16},
		{"TOUCHED FIRST CUBE", &prog->touched_first_cube, PROGRESS_EDIT_BOOL16},
		{"KILLED DANCYFLOWER", &prog->killed_dancyflower, PROGRESS_EDIT_BOOL16},
		{"EGG DROPPED", &prog->egg_dropped, PROGRESS_EDIT_BOOL16},
		{"BOSS 1 DEFEATED", &prog->boss_defeated[0], PROGRESS_EDIT_BOOL16},
		{"BOSS 2 DEFEATED", &prog->boss_defeated[1], PROGRESS_EDIT_BOOL16},
	};

	static const int16_t x = kdebug_left + 4;
	int16_t y = kdebug_top + 2;

	for (uint16_t i = 0; i < ARRAYSIZE(slots); i++)
	{
		const ProgressEditSlot *s = &slots[i];
		plot_string(s->label, x, y, ENEMY_PAL_LINE);
		switch (s->type)
		{
			case PROGRESS_EDIT_BITFIELD16:
				plot_bitfield16(*s->data, x + 16, y, MAP_PAL_LINE);
				break;

			default:
			case PROGRESS_EDIT_UINT16:
				plot_hex16(*s->data, x + 26, y, MAP_PAL_LINE);
				break;

			case PROGRESS_EDIT_BOOL16:
				plot_bool(*s->data, x + 31, y, MAP_PAL_LINE);
				break;
		}
		y++;
	}

	static const LyleBtn btn_chk = (LYLE_BTN_JUMP | LYLE_BTN_START);
	ProgressEditSlot *s = &slots[s_debug.progress_cursor_main];
	if (s_debug.progress_cursor_bit < 0)
	{
		if ((buttons & LYLE_BTN_DOWN) && !(e->buttons_prev & LYLE_BTN_DOWN))
		{
			s_debug.progress_cursor_main++;
		}
		if ((buttons & LYLE_BTN_UP) && !(e->buttons_prev & LYLE_BTN_UP))
		{
			s_debug.progress_cursor_main--;
		}

		if (s_debug.progress_cursor_main >= (signed)ARRAYSIZE(slots))
		{
			s_debug.progress_cursor_main = 0;
		}
		else if (s_debug.progress_cursor_main < 0)
		{
			s_debug.progress_cursor_main = ARRAYSIZE(slots) - 1;
		}

		if ((buttons & LYLE_BTN_CUBE) && !(e->buttons_prev & LYLE_BTN_CUBE))
		{
			e->screen = PAUSE_SCREEN_DEBUG;
		}
		else if ((buttons & btn_chk) && !(e->buttons_prev & btn_chk))
		{
			if (s->type == PROGRESS_EDIT_BITFIELD16)
			{
				s_debug.progress_cursor_bit = 0;
			}
			else
			{
				s_debug.progress_cursor_bit = 127;
			}
		}
	}
	else if (s_debug.progress_cursor_bit == 127)
	{
		if ((buttons & LYLE_BTN_CUBE) && !(e->buttons_prev & LYLE_BTN_CUBE))
		{
			s_debug.progress_cursor_bit = -1;
		}
		if ((buttons & LYLE_BTN_DOWN) && !(e->buttons_prev & LYLE_BTN_DOWN))
		{
			if (s->type == PROGRESS_EDIT_BOOL16) *s->data = 0;
			else (*s->data)--;
		}
		else if ((buttons & LYLE_BTN_UP) && !(e->buttons_prev & LYLE_BTN_UP))
		{
			if (s->type == PROGRESS_EDIT_BOOL16) *s->data = 1;
			else (*s->data)++;
		}
	}
	else
	{
		if ((buttons & LYLE_BTN_LEFT) && !(e->buttons_prev & LYLE_BTN_LEFT))
		{
			s_debug.progress_cursor_bit++;
		}
		else if ((buttons & LYLE_BTN_RIGHT) && !(e->buttons_prev & LYLE_BTN_RIGHT))
		{
			s_debug.progress_cursor_bit--;
		}
		if (s_debug.progress_cursor_bit < 0)
		{
			s_debug.progress_cursor_bit = 15;
		}
		else if (s_debug.progress_cursor_bit > 15)
		{
			s_debug.progress_cursor_bit = 0;
		}
		if ((buttons & (LYLE_BTN_CUBE | btn_chk)) && !(e->buttons_prev & (LYLE_BTN_CUBE | btn_chk)))
		{
			s_debug.progress_cursor_bit = -1;
		}
		if ((buttons & LYLE_BTN_DOWN) && !(e->buttons_prev & LYLE_BTN_DOWN))
		{
			*s->data &= ~(1 << s_debug.progress_cursor_bit);
		}
		else if ((buttons & LYLE_BTN_UP) && !(e->buttons_prev & LYLE_BTN_UP))
		{
			*s->data |= (1 << s_debug.progress_cursor_bit);
		}
	}
}

static void draw_progress_edit_cursor(O_Pause *e)
{
	if (e->cursor_flash_frame == 0)
	{
		md_spr_put((kdebug_left + 2) * 8, (kdebug_top + 2 + (s_debug.progress_cursor_main)) * 8,
		        VDP_ATTR(s_vram_pos + 0x70, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));
	}
	if (e->cursor_flash_cnt % 2) return;

	if (s_debug.progress_cursor_bit == 127)
	{
		md_spr_put((kdebug_left + 32) * 8, (kdebug_top + 2 + (s_debug.progress_cursor_main)) * 8,
		        VDP_ATTR(s_vram_pos + 0x87, 0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(4, 1));
	}
	else if (s_debug.progress_cursor_bit >= 0)
	{
		md_spr_put((kdebug_left + 19 + (16 - s_debug.progress_cursor_bit)) * 8,
		        (kdebug_top + 2 + (s_debug.progress_cursor_main)) * 8,
		        VDP_ATTR(s_vram_pos + 0x87, 0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));
	}
}

static void draw_press_button(int16_t x, int16_t y, int16_t frame)
{
	const uint16_t pr_offs = (frame == 0) ? 0xC7 : 0xCC;
	const uint16_t es_offs = (frame == 0) ? 0xF3 : 0xF6;
	const uint16_t s_offs = (frame == 0) ? 0xF4 : 0xF7;
	const uint16_t bu_offs = (frame == 0) ? 0xC9 : 0xCE;
	const uint16_t t_offs = (frame == 0) ? 0xCB : 0xBB;
	const uint16_t o_offs = (frame == 0) ? 0xF9 : 0xFB;
	const uint16_t n_offs = (frame == 0) ? 0xF8 : 0xFA;

	const uint16_t base_attr = VDP_ATTR(s_vram_pos, 0, 0, ENEMY_PAL_LINE, 0);

	md_spr_put(x, y, base_attr + pr_offs, SPR_SIZE(2, 1));
	x += 16;
	md_spr_put(x, y, base_attr + es_offs, SPR_SIZE(2, 1));
	x += 16;
	md_spr_put(x, y, base_attr + s_offs, SPR_SIZE(1, 1));
	x += 11;
	md_spr_put(x, y, base_attr + bu_offs, SPR_SIZE(2, 1));
	x += 16;
	md_spr_put(x, y, base_attr + t_offs, SPR_SIZE(1, 1));
	x += 8;
	md_spr_put(x, y, base_attr + t_offs, SPR_SIZE(1, 1));
	x += 8;
	md_spr_put(x, y, base_attr + o_offs, SPR_SIZE(1, 1));
	x += 8;
	md_spr_put(x, y, base_attr + n_offs, SPR_SIZE(1, 1));
}

// vram view

static void plot_vram_view_title(void)
{
	plot_string("@ VIDEO MEMORY VIEWER @", kdebug_left, kdebug_top, ENEMY_PAL_LINE);
	plot_string("D@PAD    @ CHANGE OFFSET", kdebug_left, kdebug_top + 21, ENEMY_PAL_LINE);
	plot_string("BUTTON C @ CHANGE PALETTE", kdebug_left, kdebug_top + 22, ENEMY_PAL_LINE);
	plot_string("BUTTON B @ EXIT", kdebug_left, kdebug_top + 23, ENEMY_PAL_LINE);

	plot_string("x 0123456789ABCDEF", kdebug_left, kdebug_top + 2, ENEMY_PAL_LINE);
	plot_string("0\n1\n2\n3\n4\n5\n6\n7\n8\n9\nA\nB\nC\nD\nE\nF", kdebug_left, kdebug_top + 4, ENEMY_PAL_LINE);
}

static const int16_t kvram_view_column_count = 16;

static void plot_vram_view(uint16_t offset, uint8_t pal)
{
	static const int16_t x = kdebug_left + 2;
	static const int16_t y = kdebug_top + 4;
	static const int16_t krow_count = 16;

	uint16_t attr = VDP_ATTR(offset, 0, 0, pal, 1);


	SYSTEM_ASSERT(kvram_view_column_count % 4 == 0);

	/*
	const uint16_t base = md_vdp_get_plane_base(VDP_PLANE_WINDOW);
	for (int16_t row = 0; row < krow_count; row++)
	{
		const uint16_t addr = base + 2 * (((y + row) * GAME_PLANE_W_CELLS) + x);
		md_vdp_set_autoinc(2);
		md_vdp_set_addr(addr);
		for (int16_t col = 0; col < kvram_view_column_count; col++)
		{
			md_vdp_write(attr);
			attr++;
		}
	}
*/
	uint16_t spr_y = y * 8;
	for (int16_t row = 0; row < krow_count; row++)
	{
		uint16_t spr_x = x * 8;
		for (int16_t col = 0; col < kvram_view_column_count; col += 4)
		{
			md_spr_put(spr_x, spr_y, attr, SPR_SIZE(4, 1));
			spr_x += 32;
			attr += 4;
		}
		spr_y += 8;
	}

	char hex_str[7];
	hex_str[0] = '0';
	hex_str[1] = 'x';
	for (int16_t i = 0; i < 4; i++)
	{
		const uint8_t digit = offset & 0xF;
		const int16_t idx = 2 + (3 - i);
		if (digit >= 0xA) hex_str[idx] = 'A' + (digit - 0xA);
		else hex_str[idx] = '0' + (digit);
		offset = offset >> 4;
	}
	hex_str[6] = '\0';

	plot_string("OFFSET", kdebug_left + 24, kdebug_top + 1, ENEMY_PAL_LINE);
	plot_string(hex_str, kdebug_left + 24, kdebug_top + 2, ENEMY_PAL_LINE);

	uint16_t vpal = pal;
	for (int16_t i = 0; i < 4; i++)
	{
		const uint8_t digit = vpal & 0xF;
		const int16_t idx = 2 + (3 - i);
		if (digit >= 0xA) hex_str[idx] = 'A' + (digit - 0xA);
		else hex_str[idx] = '0' + (digit);
		vpal = vpal >> 4;
	}
	plot_string("PAL", kdebug_left + 24, kdebug_top + 4, ENEMY_PAL_LINE);
	plot_string(hex_str, kdebug_left + 24, kdebug_top + 5, ENEMY_PAL_LINE);

	plot_string("OBJ USED", kdebug_left + 24, kdebug_top + 7, ENEMY_PAL_LINE);
	uint16_t obj_tiles = obj_get_vram_pos() - OBJ_TILE_VRAM_POSITION;

	for (int16_t i = 0; i < 4; i++)
	{
		const uint8_t digit = obj_tiles & 0xF;
		const int16_t idx = 2 + (3 - i);
		if (digit >= 0xA) hex_str[idx] = 'A' + (digit - 0xA);
		else hex_str[idx] = '0' + (digit);
		obj_tiles = obj_tiles >> 4;
	}
	plot_string(hex_str, kdebug_left + 24, kdebug_top + 8, ENEMY_PAL_LINE);

	plot_string("OBJ FREE", kdebug_left + 24, kdebug_top + 10, ENEMY_PAL_LINE);

	uint16_t obj_free = OBJ_TILE_VRAM_LENGTH - (obj_get_vram_pos() - OBJ_TILE_VRAM_POSITION);

	for (int16_t i = 0; i < 4; i++)
	{
		const uint8_t digit = obj_free & 0xF;
		const int16_t idx = 2 + (3 - i);
		if (digit >= 0xA) hex_str[idx] = 'A' + (digit - 0xA);
		else hex_str[idx] = '0' + (digit);
		obj_free = obj_free >> 4;
	}
	plot_string(hex_str, kdebug_left + 24, kdebug_top + 11, ENEMY_PAL_LINE);
}

static void vram_view_logic(O_Pause *e)
{
	static const LyleBtn btn_chk = (LYLE_BTN_JUMP | LYLE_BTN_START);
	const LyleBtn buttons = input_read();

	if (buttons & LYLE_BTN_DOWN && !(e->buttons_prev & LYLE_BTN_DOWN) &&
	    s_debug.vram_view_offset <= 0x06F0)
	{
		s_debug.vram_view_offset += 0x0010;
	}
	if (buttons & LYLE_BTN_UP && !(e->buttons_prev & LYLE_BTN_UP) &&
	    s_debug.vram_view_offset >= 0x0010)
	{
		s_debug.vram_view_offset -= 0x0010;
	}

	if (buttons & LYLE_BTN_RIGHT && !(e->buttons_prev & LYLE_BTN_RIGHT) &&
	    s_debug.vram_view_offset <= 0x600)
	{
		s_debug.vram_view_offset += 0x0100;
	}
	if (buttons & LYLE_BTN_LEFT && !(e->buttons_prev & LYLE_BTN_LEFT) &&
	    s_debug.vram_view_offset >= 0x0100)
	{
		s_debug.vram_view_offset -= 0x0100;
	}

	if (buttons & btn_chk && !(e->buttons_prev & btn_chk))
	{
		s_debug.vram_view_pal++;
	}
	s_debug.vram_view_pal &= 0x03;
	if ((buttons & LYLE_BTN_CUBE) && !(e->buttons_prev & LYLE_BTN_CUBE))
	{
		e->screen = PAUSE_SCREEN_DEBUG;
	}
}

// pause menu

static void draw_pause_menu(O_Pause *e)
{
	const int16_t continue_offs = (e->pause_choice == 0 && e->menu_flash_frame) ? 0xD0 : 0xD8;
	static const int16_t continue_x = 80;
	static const int16_t continue_y = 168;
	const int16_t quit_offs = (e->pause_choice == 1 && e->menu_flash_frame) ? 0xE0 : 0xE9;
	static const int16_t quit_x = 168;
	static const int16_t quit_y = 168;
	const int16_t yes_offs = (e->pause_choice == 2 && e->menu_flash_frame) ? 0xF2 : 0xF5;
	static const int16_t yes_x = 96 + 22;
	static const int16_t yes_y = 176;
	const int16_t no_offs = (e->pause_choice == 3 && e->menu_flash_frame) ? 0xF8 : 0xFA;
	static const int16_t no_x = 96 + 83;
	static const int16_t no_y = 176;
	static const int16_t sure_x = 96 + 38;
	static const int16_t sure_y = 168;
	switch (e->pause_choice)
	{
		case 0:
		case 1:
			md_spr_put(continue_x, continue_y, VDP_ATTR(s_vram_pos + continue_offs,
			                                         0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(4, 1));
			md_spr_put(continue_x + 32, continue_y, VDP_ATTR(s_vram_pos + continue_offs + 4,
			                                              0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(4, 1));
			md_spr_put(quit_x, quit_y, VDP_ATTR(s_vram_pos + quit_offs,
			                                         0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(4, 1));
			md_spr_put(quit_x + 32, quit_y, VDP_ATTR(s_vram_pos + quit_offs + 4,
			                                              0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(4, 1));
			md_spr_put(quit_x + 64, quit_y, VDP_ATTR(s_vram_pos + quit_offs + 8,
			                                              0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));
			break;
		case 2:
		case 3:
			md_spr_put(sure_x, sure_y, VDP_ATTR(s_vram_pos + 0xC0, 0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(4, 1));
			md_spr_put(sure_x + 32, sure_y, VDP_ATTR(s_vram_pos + 0xC0 + 4, 0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(3, 1));
			md_spr_put(yes_x, yes_y, VDP_ATTR(s_vram_pos + yes_offs,
			                               0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(3, 1));
			md_spr_put(no_x, no_y, VDP_ATTR(s_vram_pos + no_offs,
			                             0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(2, 1));
			break;
	}
}

static void pause_menu_logic(O_Pause *e, LyleBtn buttons)
{
	const int16_t right_trigger = (buttons & LYLE_BTN_RIGHT) && !(e->buttons_prev & LYLE_BTN_RIGHT);
	const int16_t left_trigger = (buttons & LYLE_BTN_LEFT) && !(e->buttons_prev & LYLE_BTN_LEFT);

	if (e->pause_select_cnt == 0)
	{
		if (e->pause_choice <= 1)
		{
			if (left_trigger)
			{
				e->pause_choice = 0;
				sfx_stop(SFX_BEEP);
				sfx_play(SFX_BEEP, 1);
			}
			else if (right_trigger)
			{
				e->pause_choice = 1;
				sfx_stop(SFX_BEEP);
				sfx_play(SFX_BEEP, 1);
			}
		}
		else if (e->pause_choice >= 2 && e->pause_choice <= 3)
		{
			if (left_trigger)
			{
				e->pause_choice = 2;
				sfx_stop(SFX_BEEP);
				sfx_play(SFX_BEEP, 1);
			}
			else if (right_trigger)
			{
				e->pause_choice = 3;
				sfx_stop(SFX_BEEP);
				sfx_play(SFX_BEEP, 1);
			}
		}
		if ((buttons & (LYLE_BTN_JUMP | LYLE_BTN_START) && !(e->buttons_prev & (LYLE_BTN_JUMP | LYLE_BTN_START))))
		{
			e->pause_select_cnt = kselect_delay_frames;
			sfx_play(SFX_SELECT_1, 0);
			sfx_play(SFX_SELECT_2, 0);
		}
	}
	else
	{
		e->pause_select_cnt--;
		if (e->pause_select_cnt == 0)
		{
			switch (e->pause_choice)
			{
				case 0:
					e->screen = PAUSE_SCREEN_NONE;
					break;
				case 1:
					e->pause_choice = 3;
					break;
				case 2:
					map_set_next_room(0, 0);
					map_set_exit_trigger(MAP_EXIT_RESTART);
					break;
				case 3:
					e->pause_choice = 1;
					break;
			}
		}
	}
}

static void main_func(Obj *o)
{
	O_Pause *e = (O_Pause *)o;

	// Search for an active title or gameover object, and abort early.
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status == OBJ_STATUS_NULL) continue;
		if (o->type == OBJ_TITLE) return;
		if (o->type == OBJ_GAMEOVER) return;
	}

	const LyleBtn buttons = input_read();

	const int16_t first_frame = e->screen != e->screen_prev;
	e->screen_prev = e->screen;
	if (first_frame)
	{
		e->dismissal_delay_cnt = 0;
	}

	switch (e->screen)
	{
		case PAUSE_SCREEN_NONE:
			if (first_frame)
			{
				e->select_delay_cnt = 0;
				map_upload_palette();
				bg_upload_palette();
				lyle_upload_palette();
				e->window = false;
			}
			maybe_map(e, buttons);
			break;
		case PAUSE_SCREEN_MAP:
			if (first_frame)
			{
				screen_reset(e);
				plot_map_to_window_plane();
				plot_item_display_borders();
				md_pal_upload(MAP_TILE_CRAM_POSITION, res_pal_pause_bin,
				           sizeof(res_pal_pause_bin) / 2);
				md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_bin,
				           sizeof(res_pal_pause_bin) / 2);
				e->pause_choice = 0;
				e->pause_select_cnt = 0;
			}
			
			OBJ_SIMPLE_ANIM(e->cursor_flash_cnt, e->cursor_flash_frame,
			                2, kcursor_flash_delay);
			OBJ_SIMPLE_ANIM(e->menu_flash_cnt, e->menu_flash_frame,
			                2, kmenu_flash_delay);
			pause_menu_logic(e, buttons);
			draw_map_location(e);
			draw_map_pause_text();
			draw_cube_sector_text();
			draw_item_icons();
			draw_cp_orb_count();
			draw_pause_menu(e);
			maybe_switch_to_debug(e, buttons);
			break;
		case PAUSE_SCREEN_GET_MAP:
		case PAUSE_SCREEN_GET_CUBE_LIFT:
		case PAUSE_SCREEN_GET_CUBE_JUMP:
		case PAUSE_SCREEN_GET_CUBE_KICK:
		case PAUSE_SCREEN_GET_ORANGE_CUBE:
		case PAUSE_SCREEN_GET_PHANTOM:
		case PAUSE_SCREEN_GET_PHANTOM_DOUBLE_DAMAGE:
		case PAUSE_SCREEN_GET_PHANTOM_HALF_TIME:
		case PAUSE_SCREEN_GET_PHANTOM_CHEAP:
		case PAUSE_SCREEN_LYLE_WEAK:
			if (first_frame)
			{
				screen_reset(e);
				plot_get_dialogue_backing(e->screen);
				md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_bin,
				           sizeof(res_pal_pause_bin) / 2);
				md_pal_upload(MAP_TILE_CRAM_POSITION, res_pal_items2_bin, sizeof(res_pal_items2_bin) / 2);
			}
			OBJ_SIMPLE_ANIM(e->menu_flash_cnt, e->menu_flash_frame,
			                2, kmenu_flash_delay);
			draw_you_got(e->screen);
			if (e->dismissal_delay_cnt >= kdismissal_delay_frames)
			{
				draw_press_button(114, 192 - (system_is_ntsc() ? 8 : 0), e->menu_flash_frame);
			}
			maybe_dismiss(e, buttons, kdismissal_delay_frames);
			if (e->select_delay_cnt >= 1) e->select_delay_cnt++;
			if (e->select_delay_cnt >= kselect_delay_frames)
			{
				e->screen = PAUSE_SCREEN_NONE;
			}
			break;
		case PAUSE_SCREEN_HP_ORB_0:
		case PAUSE_SCREEN_HP_ORB_1:
		case PAUSE_SCREEN_HP_ORB_2:
		case PAUSE_SCREEN_HP_ORB_3:
		case PAUSE_SCREEN_HP_ORB_4:
		case PAUSE_SCREEN_HP_ORB_5:
		case PAUSE_SCREEN_HP_ORB_6:
		case PAUSE_SCREEN_HP_ORB_7:
		case PAUSE_SCREEN_HP_ORB_8:
		case PAUSE_SCREEN_HP_ORB_9:
		case PAUSE_SCREEN_HP_ORB_10:
		case PAUSE_SCREEN_HP_ORB_11:
		case PAUSE_SCREEN_HP_ORB_12:
		case PAUSE_SCREEN_HP_ORB_13:
		case PAUSE_SCREEN_HP_ORB_14:
		case PAUSE_SCREEN_HP_ORB_15:
			if (first_frame)
			{
				screen_reset(e);
				plot_get_dialogue_backing(e->screen);
				md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_bin,
				           sizeof(res_pal_pause_bin) / 2);
			}
			draw_you_got(e->screen);
			maybe_dismiss(e, buttons, kdismissal_delay_frames);
			if (e->select_delay_cnt >= 1) e->select_delay_cnt++;
			if (e->select_delay_cnt >= kselect_delay_frames)
			{
				e->screen = PAUSE_SCREEN_NONE;
			}
			break;
		case PAUSE_SCREEN_CP_ORB_0:
		case PAUSE_SCREEN_CP_ORB_1:
		case PAUSE_SCREEN_CP_ORB_2:
		case PAUSE_SCREEN_CP_ORB_3:
		case PAUSE_SCREEN_CP_ORB_4:
		case PAUSE_SCREEN_CP_ORB_5:
		case PAUSE_SCREEN_CP_ORB_6:
		case PAUSE_SCREEN_CP_ORB_7:
		case PAUSE_SCREEN_CP_ORB_8:
		case PAUSE_SCREEN_CP_ORB_9:
		case PAUSE_SCREEN_CP_ORB_10:
		case PAUSE_SCREEN_CP_ORB_11:
		case PAUSE_SCREEN_CP_ORB_12:
		case PAUSE_SCREEN_CP_ORB_13:
		case PAUSE_SCREEN_CP_ORB_14:
		case PAUSE_SCREEN_CP_ORB_15:
			if (first_frame)
			{
				screen_reset(e);
				plot_get_dialogue_backing(e->screen);
				md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_bin,
				           sizeof(res_pal_pause_bin) / 2);
			}
			draw_you_got(e->screen);
			maybe_dismiss(e, buttons, kdismissal_delay_frames);
			if (e->select_delay_cnt >= 1) e->select_delay_cnt++;
			if (e->select_delay_cnt >= kselect_delay_frames)
			{
				e->screen = PAUSE_SCREEN_NONE;
			}
			
			OBJ_SIMPLE_ANIM(e->cursor_flash_cnt, e->cursor_flash_frame,
			                2, kcursor_flash_delay);
			break;
		case PAUSE_SCREEN_DEBUG:
			if (first_frame)
			{
				s_debug.main_cursor = 0;
				clear_window_plane();
				md_pal_upload(MAP_TILE_CRAM_POSITION, res_pal_debug_bin,
				           sizeof(res_pal_debug_bin) / 2);
				plot_debug_menu();
			}
			draw_debug_main_cursor(e);
			maybe_dismiss(e, buttons, 2);
			if (e->select_delay_cnt != 0) e->screen = PAUSE_SCREEN_NONE;
			debug_menu_logic(e, buttons);
			
			OBJ_SIMPLE_ANIM(e->cursor_flash_cnt, e->cursor_flash_frame,
			                2, kcursor_flash_delay / 2);
			break;
		case PAUSE_SCREEN_ROOM_SELECT:
			if (first_frame)
			{
				clear_window_plane();
				plot_room_select_title();
				s_debug.room_last_page = -1;
			}
			debug_room_select_logic(e, buttons);
			plot_room_select_list();
			draw_room_select_cursor(e);
			
			OBJ_SIMPLE_ANIM(e->cursor_flash_cnt, e->cursor_flash_frame,
			                2, kcursor_flash_delay / 2);
			break;
		case PAUSE_SCREEN_SOUND_TEST:
			if (first_frame)
			{
				clear_window_plane();
				plot_sound_test_title();
			}
			sound_test_logic(e, buttons);
			plot_sound_test_ui();
			draw_sound_test_cursor(e);
			
			OBJ_SIMPLE_ANIM(e->cursor_flash_cnt, e->cursor_flash_frame,
			                2, kcursor_flash_delay / 2);
			break;
		case PAUSE_SCREEN_PROGRESS_EDIT:
			if (first_frame)
			{
				clear_window_plane();
				plot_progress_edit_title();
				s_debug.progress_cursor_main = 0;
				s_debug.progress_cursor_bit = -1;
			}
			progress_edit_logic_and_plot(e, buttons);
			draw_progress_edit_cursor(e);
			
			OBJ_SIMPLE_ANIM(e->cursor_flash_cnt, e->cursor_flash_frame,
			                2, kcursor_flash_delay / 2);
			break;
		case PAUSE_SCREEN_VRAM_VIEW:
			if (first_frame)
			{
				clear_window_plane();
				s_debug.vram_view_offset = 0;
				s_debug.vram_view_pal = 0;
				map_upload_palette();
				bg_upload_palette();
				lyle_upload_palette();
				plot_vram_view_title();
			}

			vram_view_logic(e);
			plot_vram_view(s_debug.vram_view_offset, s_debug.vram_view_pal);
			break;
		case PAUSE_SCREEN_BUTTON_CHECK:
			if (first_frame)
			{
				clear_window_plane();
			}
			plot_button_check();
			break;
	}

	if (first_frame)
	{
		if (e->screen == PAUSE_SCREEN_NONE)
		{
			wake_objects();
		}
		else
		{
			hibernate_objects();
		}
	}

	e->buttons_prev = buttons;
}

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kcursor_flash_delay = PALSCALE_DURATION(12);
	kdismissal_delay_frames = PALSCALE_DURATION(60);
	kselect_delay_frames = PALSCALE_DURATION(36);
	kmenu_flash_delay = PALSCALE_DURATION(5);
	s_constants_set = 1;
}

void o_load_pause(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(*s_pause) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");

	if (s_pause)
	{
		obj_erase(o);
		return;
	}
	s_pause = (O_Pause *)o;
	(void)data;
	set_constants();
	vram_load();

	memset(&s_debug, 0, sizeof(&s_debug));

	obj_basic_init(o, "Pause", OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-1), INTTOFIX16(1), INTTOFIX16(-2), 1);
	o->main_func = main_func;
	o->cube_func = NULL;
	s_debug.chosen_room_id = -1;

	clear_window_plane();
}

void o_unload_pause(void)
{
	s_vram_pos = 0;
	s_vram_kana_pos = 0;
	s_pause = NULL;
}

void pause_set_screen(PauseScreen screen)
{
	s_pause->screen = screen;
}

bool pause_want_window(void)
{
	return s_pause->window;
}

int16_t pause_get_debug_room_id(void)
{
	return s_debug.chosen_room_id;
}
