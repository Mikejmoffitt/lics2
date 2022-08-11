#include "hud.h"
#include <stdlib.h>
#include "system.h"
#include "gfx.h"

#include "lyle.h"
#include "progress.h"

// On-screen height of CP meter. Should be a multiple of 8.
#define HUD_CP_DISP_HEIGHT 48

// The internal maximum value for the CP counter.
#define HUD_CP_MAX 29

static uint16_t s_vram_pos;
static uint16_t s_visible;

static void draw_hp(void)
{
	const ProgressSlot *progress = progress_get();
	static const int16_t hud_x = 5;
	const int16_t y_offset = md_vdp_get_raster_height() == 240 ? 0 : -8;

	// Label.
	md_spr_put(hud_x, y_offset + 15,
	        SPR_ATTR(s_vram_pos + 4, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 2));

	// Show health.
	const int16_t hp_capacity = progress->hp_capacity;
	int16_t plot_y = y_offset + 31;
	int16_t i = lyle_get_hp();
	while (i--)
	{
		md_spr_put(hud_x, plot_y, SPR_ATTR(s_vram_pos + 8, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 1));
		plot_y += 8;
	}
	i = hp_capacity - lyle_get_hp();
	if (i <= 0) return;
	while (i--)
	{
		md_spr_put(hud_x, plot_y, SPR_ATTR(s_vram_pos + 10, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 1));
		plot_y += 8;
	}
}

static void draw_cp(void)
{
	static const int16_t hud_x = 5;
	const int16_t y_offset = md_vdp_get_raster_height() == 240 ? 0 : -2;

	// Label.
	md_spr_put(hud_x, y_offset + 206,
	        SPR_ATTR(s_vram_pos, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 2));

	// Top and bottom of the bar.
	md_spr_put(hud_x, y_offset + 147, SPR_ATTR(s_vram_pos + 12, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 1));
	md_spr_put(hud_x, y_offset + 147 + HUD_CP_DISP_HEIGHT + 1, SPR_ATTR(s_vram_pos + 12, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 1));

	int16_t scaled_cp = FIX16TOINT(FIX16MUL(INTTOFIX16(HUD_CP_DISP_HEIGHT / (float)HUD_CP_MAX), INTTOFIX16(lyle_get_cp())));
	int16_t i = HUD_CP_DISP_HEIGHT / 8;
	int16_t plot_y = y_offset + 147 + HUD_CP_DISP_HEIGHT;
	while (i--)
	{
		md_spr_put(hud_x, plot_y, SPR_ATTR(s_vram_pos + 14 + (scaled_cp < 8 ? 2 + (2 * (scaled_cp % 8)) : 0), 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 1));
		plot_y -= 8;
		scaled_cp -= 8;
		if (scaled_cp < 0) scaled_cp = 0;
	}
}

void hud_render(void)
{
	if (!s_visible) return;
	draw_hp();
	const ProgressSlot *progress = progress_get();
	if (progress->abilities & ABILITY_PHANTOM) draw_cp();
}

void hud_load(void)
{
	const Gfx *g = gfx_get(GFX_SYS_HUD);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

void hud_set_visible(uint16_t visible)
{
	s_visible = visible;
}
