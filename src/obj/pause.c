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
#include "obj/lyle.h"
#include "sfx.h"
#include "map_file.h"
#include "music.h"

static O_Pause *s_pause;
static uint16_t s_vram_pos;

static int16_t kcursor_flash_delay;
static int16_t kdismissal_delay_frames;

static const uint16_t kmap_left = 8;
static const uint16_t kmap_top = 7;

// String printing utility. Not very fast, but it's alright for this.
static void window_puts(const char *str, int16_t x, int16_t y, int16_t pal)
{
	uint16_t plane_base = vdp_get_plane_base(VDP_PLANE_WINDOW) +
	                      2 * ((y * GAME_PLANE_W_CELLS) + x);
	while (*str)
	{
		char a = *str++;
		if (!a) break;
		if (a == '\n' || a == '\r')
		{
			y++;
			plane_base = vdp_get_plane_base(VDP_PLANE_WINDOW) +
	                      2 * ((y * GAME_PLANE_W_CELLS) + x);
		}
		else
		{
			if (a == '(')
			{
				a = '<';
			}
			else if (a == ')')
			{
				a = '>';
			}
			if (a >= 'a' && a <= 'z')
			{
				vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80 - 0x20 + (a & ~(0x20)), 0, 0, pal, 0));
			}
			else if (a == ' ')
			{
				vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x30, 0, 0, pal, 0));
			}
			else
			{
				vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80 - 0x20 + (a), 0, 0, pal, 0));
			}
			plane_base += 2;
		}
	}
}

// Map drawing routines -------------------------------------------------------
static inline void clear_window_plane(void)
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

static inline void plot_map_side_borders(void)
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

static inline void plot_map_top_bottom_borders(void)
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

static inline void plot_map(void)
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

static inline void plot_map_cube_sector_extension(void)
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

static void plot_map_to_window_plane(void)
{
	plot_map_side_borders();
	plot_map_top_bottom_borders();
	plot_map_cube_sector_extension();
	plot_map();
}

static inline void plot_item_display(uint16_t x, uint16_t y)
{
	uint16_t plane_base = vdp_get_plane_base(VDP_PLANE_WINDOW);
	x /= 8;
	y /= 8;
	plane_base += 2 * ((y * GAME_PLANE_W_CELLS) + x);

	vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x85, 1, 0, ENEMY_PAL_LINE, 0));
	vdp_poke(plane_base + 2, VDP_ATTR(s_vram_pos + 0x85, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += GAME_PLANE_W_CELLS * 2;
	vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x85, 1, 1, ENEMY_PAL_LINE, 0));
	vdp_poke(plane_base + 2, VDP_ATTR(s_vram_pos + 0x85, 0, 1, ENEMY_PAL_LINE, 0));
}

static void plot_item_displays(void)
{
	plot_item_display(96, 192);
	plot_item_display(120, 192);
	plot_item_display(152, 192);
	plot_item_display(184, 192);
	plot_item_display(208, 192);
}

static void draw_cp_orb_count(void)
{
	const ProgressSlot *progress = progress_get();
	const int16_t cp_orbs = progress->collected_cp_orbs;
	spr_put(272, 204, VDP_ATTR(s_vram_pos + 0x3C, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(2, 2));
	spr_put(288, 204, VDP_ATTR(s_vram_pos + 0x40, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 2));
	spr_put(295, 204, VDP_ATTR(s_vram_pos + 0x40 + (2 * cp_orbs), 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 2));
}

static void draw_item_icons(void)
{
	const ProgressSlot *progress = progress_get();
	
	if (progress->abilities & ABILITY_LIFT)
	{
		spr_put(100, 196, VDP_ATTR(s_vram_pos + 0x70, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));
	}
	
	if (progress->abilities & ABILITY_JUMP)
	{
		spr_put(124, 196, VDP_ATTR(s_vram_pos + 0x72, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));
	}

	if (progress->abilities & ABILITY_PHANTOM)
	{
		spr_put(152, 192, VDP_ATTR(s_vram_pos + 0x77, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(2, 2));
		int16_t phantom_level = 0;
		if (progress->abilities & ABILITY_FAST_PHANTOM) phantom_level++;
		if (progress->abilities & ABILITY_CHEAP_PHANTOM) phantom_level++;
		if (progress->abilities & ABILITY_2X_DAMAGE_PHANTOM) phantom_level++;

		spr_put(152, 208, VDP_ATTR(s_vram_pos + 0x54, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 1));
		spr_put(160, 208, VDP_ATTR(s_vram_pos + 0x55 + phantom_level, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 1));
	}

	if (progress->abilities & ABILITY_KICK)
	{
		spr_put(184, 192, VDP_ATTR(s_vram_pos + 0x73, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));
	}

	if (progress->abilities & ABILITY_ORANGE)
	{
		spr_put(212, 196, VDP_ATTR(s_vram_pos + 0x71, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(1, 1));
	}
}

static inline void draw_map_pause_text(void)
{
	static const int16_t draw_x = (GAME_SCREEN_W_PIXELS / 2) - 28;
	static const int16_t draw_y = 24;

	spr_put(draw_x, draw_y,
	        SPR_ATTR(s_vram_pos + 0x60, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(4, 2));
	spr_put(draw_x + 32, draw_y,
	        SPR_ATTR(s_vram_pos + 0x68, 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(4, 2));
}

static inline void draw_cube_sector_text(void)
{
	int16_t x = (GAME_SCREEN_W_PIXELS / 2) - 38;
	static const int16_t y = 164;
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
		spr_put(x, y, tile_base + cube_mapping[i], SPR_SIZE(1, 1));
		x += 8;
	}
	x += 3;
	
	for (uint16_t i = 0; i < ARRAYSIZE(sector_mapping); i++)
	{
		spr_put(x, y, tile_base + sector_mapping[i], SPR_SIZE(1, 1));
		x += 8;
	}
}

static inline void draw_map_location(O_Pause *e)
{
	const ProgressSlot *progress = progress_get();
	if ((progress->abilities & ABILITY_MAP))
	{
		if (e->cursor_flash_frame == 0) return;
		const int16_t px = FIX32TOINT(lyle_get_x());
		const int16_t py = FIX32TOINT(lyle_get_y());
		const int16_t x_index = map_get_world_x_tile() + (px / GAME_SCREEN_W_PIXELS);
		const int16_t y_index = map_get_world_y_tile() + (py / GAME_SCREEN_H_PIXELS);
		const int16_t draw_x = 8 * kmap_left + (x_index * 8) + 2;
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

// "You Got" screen drawing routines ------------------------------------------
typedef struct CharMapping
{
	int8_t x;
	int8_t y;
	char val;
} CharMapping;

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
	{44+8, 28, 0x5E},  // Shorter A
	{51+8, 28, 'G'},
	{59+8, 28, 0x5F},  // Shorter I
	{61+8, 28, 'C'},
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
	{44, 28, 0x5E},  // Shorter A
	{51, 28, 'G'},
	{59, 28, 0x5F},  // Shorter I
	{61, 28, 'C'},
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
		spr_put(base_x + mapping[i].x, base_y + mapping[i].y,
		        SPR_ATTR(s_vram_pos + 0x80 - 0x20 + mapping[i].val, 0, 0,
		                 ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));
	}
}

static void draw_you_got(PauseScreen screen)
{
	static const int16_t base_x = 120;
	const int16_t base_y = 32 - (system_is_ntsc() ? 8 : 0);

	switch (screen)
	{
		case PAUSE_SCREEN_LYLE_WEAK:
			// Uses same layout, but does not show "you got".
			return;
		case PAUSE_SCREEN_GET_MAP:
			draw_char_mapping(base_x, base_y, kmapping_map, ARRAYSIZE(kmapping_map));
			break;
		case PAUSE_SCREEN_GET_CUBE_LIFT:
			draw_char_mapping(base_x, base_y, kmapping_cube_lift,
			                  ARRAYSIZE(kmapping_cube_lift));
			break;
		case PAUSE_SCREEN_GET_CUBE_JUMP:
			draw_char_mapping(base_x, base_y, kmapping_cube_jump,
			                  ARRAYSIZE(kmapping_cube_jump));
			break;
		case PAUSE_SCREEN_GET_CUBE_KICK:
			draw_char_mapping(base_x, base_y, kmapping_cube_kick,
			                  ARRAYSIZE(kmapping_cube_kick));
			break;
		case PAUSE_SCREEN_GET_ORANGE_CUBE:
			draw_char_mapping(base_x, base_y, kmapping_big_cube_lift,
			                  ARRAYSIZE(kmapping_big_cube_lift));
			break;
		case PAUSE_SCREEN_GET_PHANTOM:
			draw_char_mapping(base_x, base_y, kmapping_phantom_cube_magic_1,
			                  ARRAYSIZE(kmapping_phantom_cube_magic_1));
			break;
		case PAUSE_SCREEN_GET_PHANTOM_DOUBLE_DAMAGE:
			draw_char_mapping(base_x, base_y, kmapping_phantom_cube_magic_x,
			                  ARRAYSIZE(kmapping_phantom_cube_magic_x));
			draw_char_mapping(base_x, base_y, kmapping_2,
			                  ARRAYSIZE(kmapping_2));
			break;
		case PAUSE_SCREEN_GET_PHANTOM_HALF_TIME:
			draw_char_mapping(base_x, base_y, kmapping_phantom_cube_magic_x,
			                  ARRAYSIZE(kmapping_phantom_cube_magic_x));
			draw_char_mapping(base_x, base_y, kmapping_3,
			                  ARRAYSIZE(kmapping_3));
			break;
		case PAUSE_SCREEN_GET_PHANTOM_CHEAP:
			draw_char_mapping(base_x, base_y, kmapping_phantom_cube_magic_x,
			                  ARRAYSIZE(kmapping_phantom_cube_magic_x));
			draw_char_mapping(base_x, base_y, kmapping_4,
			                  ARRAYSIZE(kmapping_4));
			break;
		default:
			// TODO: HP Orb, CP Orb
			break;
	}

	// "YOU GOT:" header.
	spr_put(base_x + 13, base_y + 9, SPR_ATTR(s_vram_pos + 0x7B, 0, 0,
	                       ENEMY_PAL_LINE, 0), SPR_SIZE(3, 1));
	spr_put(base_x + 40, base_y + 9, SPR_ATTR(s_vram_pos + 0x7E, 0, 0,
	                       ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));
	spr_put(base_x + 48, base_y + 9, SPR_ATTR(s_vram_pos + 0x7C, 0, 0,
	                       ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));
	spr_put(base_x + 56, base_y + 9, SPR_ATTR(s_vram_pos + 0x7F, 0, 0,
	                       ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));
	spr_put(base_x + 64, base_y + 9, SPR_ATTR(s_vram_pos + 0x9B, 0, 0,
	                       ENEMY_PAL_LINE, 0), SPR_SIZE(1, 1));

}

static void plot_get_sides(uint16_t plane_base)
{
	for (int16_t y = 0; y < 22; y++)
	{
		vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
		vdp_poke(plane_base + 2*22, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
		plane_base += GAME_PLANE_W_CELLS * 2;
	}
	// Corner piece.
	vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x84, 0, 0, ENEMY_PAL_LINE, 0));
}

static void plot_get_top_and_bottom(uint16_t plane_base)
{
	plane_base += 2;
	for (int16_t x = 0; x < 21; x++)
	{
		vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x83, 0, 0, ENEMY_PAL_LINE, 0));
		vdp_poke(plane_base + (GAME_PLANE_W_CELLS * 5 * 2), VDP_ATTR(s_vram_pos + 0x83, 0, 0, ENEMY_PAL_LINE, 0));
		vdp_poke(plane_base + (GAME_PLANE_W_CELLS * 22 * 2), VDP_ATTR(s_vram_pos + 0x83, 0, 0, ENEMY_PAL_LINE, 0));
		plane_base += 2;
	}
	// Corner.
	vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x85, 0, 0, ENEMY_PAL_LINE, 0));
	vdp_poke(plane_base + (GAME_PLANE_W_CELLS * 5 * 2), VDP_ATTR(s_vram_pos + 0x85, 0, 0, ENEMY_PAL_LINE, 0));
	vdp_poke(plane_base + (GAME_PLANE_W_CELLS * 22 * 2), VDP_ATTR(s_vram_pos + 0x83, 0, 0, ENEMY_PAL_LINE, 0));
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
			vdp_poke(plane_base, VDP_ATTR(s_vram_pos + grid_mapping[map_index],
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
	vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x85, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += 2 * (GAME_PLANE_W_CELLS);
	vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += 2 * (GAME_PLANE_W_CELLS);
	vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += 2 * (GAME_PLANE_W_CELLS);
	vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += 2 * (GAME_PLANE_W_CELLS);
	vdp_poke(plane_base, VDP_ATTR(s_vram_pos + 0x80, 0, 0, ENEMY_PAL_LINE, 0));
	plane_base += 2 * (GAME_PLANE_W_CELLS);
}

static void plot_get_dialogue_text(PauseScreen screen)
{
	static const char hp_orb_string[] =
		//   XXXXXXXXXXXXXXXXXXXX x 13
			"@MAXIMUM HP\n"
			"INCREASED 1 UNIT\n"
			"\n"
			"@HP RESTORED";

	static const char *strings[] =
	{
		//   XXXXXXXXXXXXXXXXXXXX x 13
		[PAUSE_SCREEN_LYLE_WEAK] =
			"@LYLE IS TOO MUCH OF\n"
			"A WEAKLING TO PICK\n"
			"THESE UP JUST YET\n"
			"\n"
			"@TIME TO DO SOME\n"
			"SEARCHING",
		//   XXXXXXXXXXXXXXXXXXXX x 13
		[PAUSE_SCREEN_GET_MAP] =
			"@MAP NOW VIEWABLE ON\n"
			"PAUSE SCREEN\n"
			"\n"
			"@PRESS START DURING\n"
			"GAME TO PAUSE",
		//   XXXXXXXXXXXXXXXXXXXX x 13
		[PAUSE_SCREEN_GET_CUBE_LIFT] = 
			"@STAND ON TOP OF\n"
			"CUBE & PRESS B\n"
			"TO LIFT\n"
			"\n"
			"@PRESS B TO THROW\n"
			"@UP & B FOR UPWARD\n"
			"THROW\n"
			"@DOWN & B FOR SHORT\n"
			"THROW\n"
			"@LEFT OR RIGHT & B\n"
			"FOR LONG THROW\n"
			"@CAREFUL NOT TO GET\n"
			"HIT BY THROWN CUBES",
		//   XXXXXXXXXXXXXXXXXXXX x 13
		[PAUSE_SCREEN_GET_CUBE_JUMP] =
			"@PRESS C WHILE\n"
			"JUMPING & HOLDING\n"
			"CUBE TO THROW CUBE\n"
			"DOWNWARDS & JUMP\n"
			"HIGHER",
		//   XXXXXXXXXXXXXXXXXXXX x 13
		[PAUSE_SCREEN_GET_CUBE_KICK] =
			"@STAND NEXT TO CUBE\n"
			"& PRESS B TO KICK\n",
		//   XXXXXXXXXXXXXXXXXXXX x 13
		[PAUSE_SCREEN_GET_ORANGE_CUBE] =
			"@LIFT THE LARGE\n"
			"ORANGE CUBES",
		//   XXXXXXXXXXXXXXXXXXXX x 13
		[PAUSE_SCREEN_GET_PHANTOM] =
			"@HOLD B TO CREATE\n"
			"PHANTOM CUBE\n"
			"\n"
			"@CONSUMES\n"
			"CUBE POINTS <CP>",
		//   XXXXXXXXXXXXXXXXXXXX x 13
		[PAUSE_SCREEN_GET_PHANTOM_DOUBLE_DAMAGE] =
			"@PHANTOM CUBE DAMAGE\n"
			"2X",
		//   XXXXXXXXXXXXXXXXXXXX x 13
		[PAUSE_SCREEN_GET_PHANTOM_HALF_TIME] =
			"@PHANTOM CUBE\n"
			"CREATION TIME HALVED",
		//   XXXXXXXXXXXXXXXXXXXX x 13
		[PAUSE_SCREEN_GET_PHANTOM_CHEAP] =
			"@CP CONSUMPTION\n"
			"HALVED",
		[PAUSE_SCREEN_HP_ORB_0] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_1] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_2] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_3] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_4] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_5] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_6] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_7] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_8] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_9] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_10] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_11] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_12] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_13] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_14] = hp_orb_string,
		[PAUSE_SCREEN_HP_ORB_15] = hp_orb_string,
		[PAUSE_SCREEN_CP_ORB_0] = NULL,
		[PAUSE_SCREEN_CP_ORB_1] = NULL ,
		[PAUSE_SCREEN_CP_ORB_2] = NULL ,
		[PAUSE_SCREEN_CP_ORB_3] = NULL ,
		[PAUSE_SCREEN_CP_ORB_4] = NULL ,
		[PAUSE_SCREEN_CP_ORB_5] = NULL ,
		[PAUSE_SCREEN_CP_ORB_6] = NULL ,
		[PAUSE_SCREEN_CP_ORB_7] = NULL ,
		[PAUSE_SCREEN_CP_ORB_8] = NULL ,
		[PAUSE_SCREEN_CP_ORB_9] = NULL ,
		[PAUSE_SCREEN_CP_ORB_10] = NULL ,
		[PAUSE_SCREEN_CP_ORB_11] = NULL ,
		[PAUSE_SCREEN_CP_ORB_12] = NULL ,
		[PAUSE_SCREEN_CP_ORB_13] = NULL ,
		[PAUSE_SCREEN_CP_ORB_14] = NULL ,
		[PAUSE_SCREEN_CP_ORB_15] = NULL ,
	};

	const char *str = strings[screen];
	if (!str) return;

	// Draw the string to the window plane.
	static const int16_t kleft = 10;
	const int16_t ktop = 10 - (system_is_ntsc() ? 1 : 0);
	window_puts(str, kleft, ktop, ENEMY_PAL_LINE);
}

static void plot_get_dialogue_backing(PauseScreen screen)
{
	static const int base_x = 8;
	const int base_y = (system_is_ntsc() ? 3 : 4);
	const uint16_t plane_base = vdp_get_plane_base(VDP_PLANE_WINDOW) +
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
}

static void hibernate_objects()
{
	// Hibernate all objects besides this one.
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *w = &g_objects[i].obj;
		if (w->status == OBJ_STATUS_NULL) continue;
		if (w->type == OBJ_HUD) continue;
		if (w->type == OBJ_MAP) continue;
		if (w->type == OBJ_PAUSE) continue;
		w->status = OBJ_STATUS_HIBERNATE;
	}
}

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_PAUSE);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static void maybe_dismiss(O_Pause *e, MdButton buttons, int16_t min_delay)
{
	if (e->dismissal_delay_cnt < min_delay)
	{
		e->dismissal_delay_cnt++;
		return;
	}

	if ((buttons & BTN_START) && !(e->buttons_prev & BTN_START))
	{
		e->screen = PAUSE_SCREEN_NONE;
	}
}

static void maybe_map(O_Pause *e, MdButton buttons)
{
	if ((buttons & BTN_START) && !(e->buttons_prev & BTN_START))
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
	e->window = 1;
}

static void maybe_switch_to_debug(O_Pause *e, MdButton buttons)
{
	if (buttons & BTN_C)
	{
		e->screen = PAUSE_SCREEN_DEBUG;
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
	{"LYLE EDIT", 0},
	{"VRAM VIEW", 0},
	{"PRG VIEW", 0},
	{"RAM VIEW", 0},
	{"EXIT", PAUSE_SCREEN_NONE},
};

static const int16_t kdebug_left = 4;
static const int16_t kdebug_top = 2;

static void plot_debug_menu(void)
{
	window_puts("@ DEBUG @", kdebug_left, kdebug_top, ENEMY_PAL_LINE);

	for (uint16_t i = 0; i < ARRAYSIZE(debug_options); i++)
	{
		const DebugOption *d = &debug_options[i];
		window_puts(d->label, kdebug_left + 4, kdebug_top + 2 + (2 * i), MAP_PAL_LINE);
	}

	window_puts("BUTTON C @ SELECT", kdebug_left, kdebug_top + 20, ENEMY_PAL_LINE);
	window_puts("BUTTON B @ EXIT", kdebug_left, kdebug_top + 21, ENEMY_PAL_LINE);
}

static void debug_menu_logic(O_Pause *e, MdButton buttons)
{
	static const MdButton btn_chk = (BTN_C | BTN_A | BTN_START);
	if ((buttons & btn_chk) && !(e->buttons_prev & btn_chk))
	{
		e->screen = debug_options[e->debug.main_cursor].screen;
		return;
	}

	if ((buttons & BTN_UP) && !(e->buttons_prev & BTN_UP))
	{
		e->debug.main_cursor--;
		if (e->debug.main_cursor < 0)
		{
			e->debug.main_cursor = ARRAYSIZE(debug_options) - 1;
		}
	}
	else if ((buttons & BTN_DOWN) && !(e->buttons_prev & BTN_DOWN))
	{
		e->debug.main_cursor++;
		if (e->debug.main_cursor >= (int16_t)ARRAYSIZE(debug_options))
		{
			e->debug.main_cursor = 0;
		}
	}
}

static void draw_debug_main_cursor(O_Pause *e)
{
	spr_put((kdebug_left + 2) * 8, (kdebug_top + 2 + (e->debug.main_cursor * 2)) * 8,
	        VDP_ATTR(s_vram_pos + 0x70, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));
}

// Room select

static void plot_room_select_title(void)
{
	window_puts("@ ROOM SELECT @", kdebug_left, kdebug_top, ENEMY_PAL_LINE);

	window_puts("BUTTON C @ GO TO ROOM", kdebug_left, kdebug_top + 20, ENEMY_PAL_LINE);
	window_puts("BUTTON B @ EXIT", kdebug_left, kdebug_top + 21, ENEMY_PAL_LINE);
}

static void plot_room_select_list(O_Pause *e)
{
	static const int16_t rooms_per_list = 16;
	const int16_t page = e->debug.room_cursor / 16;
	for (int16_t i = 0; i < rooms_per_list; i++)
	{
		const int16_t id = (page * 16) + i;
		if (page != e->debug.room_last_page)
		{
			window_puts("                                ", kdebug_left + 4, kdebug_top + 2 + i, MAP_PAL_LINE);
		}
		if (id >= map_file_count()) continue;
		const MapFile *file = map_file_by_id(id);
		window_puts(file->name, kdebug_left + 4, kdebug_top + 2 + i, MAP_PAL_LINE);
	}
	e->debug.room_last_page = page;
}

static void draw_room_select_cursor(O_Pause *e)
{
	spr_put((kdebug_left + 2) * 8, (kdebug_top + 2 + (e->debug.room_cursor % 16)) * 8,
	        VDP_ATTR(s_vram_pos + 0x70, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));
}

static void debug_room_select_logic(O_Pause *e, MdButton buttons)
{
	if ((buttons & BTN_UP) && !(e->buttons_prev & BTN_UP))
	{
		e->debug.room_cursor--;
		if (e->debug.room_cursor < 0)
		{
			e->debug.room_cursor = map_file_count() - 1;
		}
	}
	else if ((buttons & BTN_DOWN) && !(e->buttons_prev & BTN_DOWN))
	{
		e->debug.room_cursor++;
		if (e->debug.room_cursor >= map_file_count())
		{
			e->debug.room_cursor = 0;
		}
	}
	else if ((buttons & BTN_LEFT) && !(e->buttons_prev & BTN_LEFT))
	{
		e->debug.room_cursor = ((e->debug.room_cursor / 16) - 1) * 16;
		while (e->debug.room_cursor < 0)
		{
			e->debug.room_cursor += map_file_count();
		}
	}
	else if ((buttons & BTN_RIGHT) && !(e->buttons_prev & BTN_RIGHT))
	{
		e->debug.room_cursor = ((e->debug.room_cursor / 16) + 1) * 16;
		while (e->debug.room_cursor >= map_file_count())
		{
			e->debug.room_cursor -= map_file_count();
		}
	}

	static const MdButton btn_chk = (BTN_C | BTN_A | BTN_START);

	if ((buttons & btn_chk) && !(e->buttons_prev & btn_chk))
	{
		e->debug.chosen_room_id = e->debug.room_cursor;
	}
	else if ((buttons & BTN_B) && !(e->buttons_prev & BTN_B))
	{
		e->screen = PAUSE_SCREEN_DEBUG;
	}
}

// Sound test

static void plot_sound_test_title(void)
{
	window_puts("@ SOUND TEST @", kdebug_left, kdebug_top, ENEMY_PAL_LINE);
	window_puts("BGM ID\n\nSFX ID\n\nSILENCE", kdebug_left + 4, kdebug_top + 2, MAP_PAL_LINE);

	window_puts("BUTTON C @ SEND COMMAND", kdebug_left, kdebug_top + 20, ENEMY_PAL_LINE);
	window_puts("BUTTON B @ EXIT", kdebug_left, kdebug_top + 21, ENEMY_PAL_LINE);
}

static void plot_sound_test_ui(O_Pause *e)
{
	char id_str[4];
	id_str[0] = '0' + (e->debug.bgm_id / 100) % 10;
	id_str[1] = '0' + (e->debug.bgm_id / 10) % 10;
	id_str[2] = '0' + e->debug.bgm_id % 10;
	id_str[3] = '\0';
	window_puts(id_str, kdebug_left + 11, kdebug_top + 2, MAP_PAL_LINE);
	id_str[0] = '0' + (e->debug.sfx_id / 100) % 10;
	id_str[1] = '0' + (e->debug.sfx_id / 10) % 10;
	id_str[2] = '0' + e->debug.sfx_id % 10;
	id_str[3] = '\0';
	window_puts(id_str, kdebug_left + 11, kdebug_top + 4, MAP_PAL_LINE);
}

static void draw_sound_test_cursor(O_Pause *e)
{
	spr_put((kdebug_left + 2) * 8, (kdebug_top + 2 + (e->debug.sound_cursor % 16) * 2) * 8,
	        VDP_ATTR(s_vram_pos + 0x70, 0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(1, 1));
}

static void sound_test_logic(O_Pause *e, MdButton buttons)
{
	if ((buttons & BTN_UP) && !(e->buttons_prev & BTN_UP))
	{
		e->debug.sound_cursor--;
		if (e->debug.sound_cursor < 0)
		{
			e->debug.sound_cursor = 2;
		}
	}
	else if ((buttons & BTN_DOWN) && !(e->buttons_prev & BTN_DOWN))
	{
		e->debug.sound_cursor++;
		if (e->debug.sound_cursor >= 3)
		{
			e->debug.sound_cursor = 0;
		}
	}

	else if ((buttons & BTN_RIGHT) && !(e->buttons_prev & BTN_RIGHT))
	{
		if (e->debug.sound_cursor == 0)
		{
			e->debug.bgm_id++;
		}
		else if (e->debug.sound_cursor == 1)
		{
			e->debug.sfx_id++;
		}
	}
	else if ((buttons & BTN_LEFT) && !(e->buttons_prev & BTN_LEFT))
	{
		if (e->debug.sound_cursor == 0)
		{
			e->debug.bgm_id--;
		}
		else if (e->debug.sound_cursor == 1)
		{
			e->debug.sfx_id--;
		}
	}
	static const MdButton btn_chk = (BTN_C | BTN_A | BTN_START);
	if ((buttons & btn_chk) && !(e->buttons_prev & btn_chk))
	{
		if (e->debug.sound_cursor == 0)
		{
			music_play(e->debug.bgm_id);
		}
		else if (e->debug.sound_cursor == 1)
		{
			sfx_play(e->debug.sfx_id, 0);
		}
		else if (e->debug.sound_cursor == 2)
		{
			music_stop();
			sfx_stop_all();
		}
	}
	else if ((buttons & BTN_B) && !(e->buttons_prev & BTN_B))
	{
		e->screen = PAUSE_SCREEN_DEBUG;
	}
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
				map_upload_palette();
				e->window = 0;
			}
			maybe_map(e, buttons);
			break;
		case PAUSE_SCREEN_MAP:
			if (first_frame)
			{
				screen_reset(e);
				plot_map_to_window_plane();
				plot_item_displays();
				pal_upload(MAP_TILE_CRAM_POSITION, res_pal_pause_bin,
				           sizeof(res_pal_pause_bin) / 2);
			}
			
			OBJ_SIMPLE_ANIM(e->cursor_flash_cnt, e->cursor_flash_frame,
			                2, kcursor_flash_delay);
			draw_map_location(e);
			draw_map_pause_text();
			draw_cube_sector_text();
			draw_item_icons();
			draw_cp_orb_count();
			maybe_dismiss(e, buttons, 0);
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
			}
			draw_you_got(e->screen);
			maybe_dismiss(e, buttons, kdismissal_delay_frames);
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
			}
			draw_you_got(e->screen);
			maybe_dismiss(e, buttons, kdismissal_delay_frames);
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
			}
			draw_you_got(e->screen);
			maybe_dismiss(e, buttons, kdismissal_delay_frames);
			break;
		case PAUSE_SCREEN_DEBUG:
			if (first_frame)
			{
				clear_window_plane();
				pal_upload(MAP_TILE_CRAM_POSITION, res_pal_debug_bin,
				           sizeof(res_pal_debug_bin) / 2);
				plot_debug_menu();
			}
			draw_debug_main_cursor(e);
			maybe_dismiss(e, buttons, 0);
			debug_menu_logic(e, buttons);
			break;
		case PAUSE_SCREEN_ROOM_SELECT:
			if (first_frame)
			{
				clear_window_plane();
				plot_room_select_title();
				e->debug.room_last_page = -1;
			}
			debug_room_select_logic(e, buttons);
			plot_room_select_list(e);
			draw_room_select_cursor(e);
			break;
		case PAUSE_SCREEN_SOUND_TEST:
			if (first_frame)
			{
				clear_window_plane();
				plot_sound_test_title();
			}
			sound_test_logic(e, buttons);
			plot_sound_test_ui(e);
			draw_sound_test_cursor(e);
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
	s_constants_set = 1;
}

void o_load_pause(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Pause) <= sizeof(ObjSlot));
	if (s_pause)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}
	s_pause = (O_Pause *)o;
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-1), INTTOFIX16(1), INTTOFIX16(-2), 1);
	o->main_func = main_func;
	o->cube_func = NULL;
	s_pause->debug.chosen_room_id = -1;

	clear_window_plane();
}

void o_unload_pause(void)
{
	s_vram_pos = 0;
	s_pause = NULL;
}

void pause_set_screen(PauseScreen screen)
{
	s_pause->screen = screen;
}

int16_t pause_want_window(void)
{
	return s_pause->window;
}

int16_t pause_get_debug_room_id(void)
{
	return s_pause->debug.chosen_room_id;
}
