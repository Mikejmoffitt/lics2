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

#define HUD_VRAM_TILE (HUD_VRAM_POSITION/32)

static bool s_visible;

static SprParam s_spr_hp_label;
static SprParam s_spr_cp_label;
static SprParam s_spr_hp_seg;
static SprParam s_spr_cp_seg;
static SprParam s_spr_cp_bar[2];

void hud_init(void)
{
	static const int16_t hud_x = 5;
	const int16_t y_offset = (md_vdp_get_raster_height() == 240) ? 0 : -8;
	const int16_t x = hud_x + 128;
	const int16_t y = y_offset + 128;

	s_spr_hp_label.x = x;
	s_spr_hp_label.y = y + 15;
	s_spr_hp_label.attr = SPR_ATTR(HUD_VRAM_TILE + 4, 0, 0, BG_PAL_LINE, 1);
	s_spr_hp_label.size = SPR_SIZE(2, 2);

	s_spr_hp_seg.x = x;
	s_spr_hp_seg.y = y + 31;
	// attr set at runtime.
	s_spr_hp_seg.size = SPR_SIZE(2, 1);

	s_spr_cp_label.x = x;
	s_spr_cp_label.y = y + 206;
	s_spr_cp_label.attr = SPR_ATTR(HUD_VRAM_TILE, 0, 0, BG_PAL_LINE, 1);
	s_spr_cp_label.size = SPR_SIZE(2, 2);

	s_spr_cp_seg.x = x;
	s_spr_cp_seg.y = y + 147 + HUD_CP_DISP_HEIGHT;
	// attr set at runtime.
	s_spr_cp_seg.size = SPR_SIZE(2, 1);

	s_spr_cp_bar[0].x = x;
	s_spr_cp_bar[0].y = y + 147;
	s_spr_cp_bar[0].attr = SPR_ATTR(HUD_VRAM_TILE + 12, 0, 0, BG_PAL_LINE, 1);
	s_spr_cp_bar[0].size = SPR_SIZE(2, 1);

	s_spr_cp_bar[1].x = x;
	s_spr_cp_bar[1].y = y + 147 + HUD_CP_DISP_HEIGHT + 1;
	s_spr_cp_bar[1].attr = SPR_ATTR(HUD_VRAM_TILE + 12, 0, 0, BG_PAL_LINE, 1);
	s_spr_cp_bar[1].size = SPR_SIZE(2, 1);
}

static void draw_hp(void)
{
	const ProgressSlot *progress = progress_get();

	// Label.
	md_spr_put_st_fast(&s_spr_hp_label);

	// Show health.
	const int16_t hp_capacity = progress->hp_capacity;
	const int16_t lyle_hp = lyle_get_hp();

	int16_t i = lyle_hp;
	const int16_t original_y = s_spr_hp_seg.y;
	s_spr_hp_seg.attr = SPR_ATTR(HUD_VRAM_TILE + 8, 0, 0, BG_PAL_LINE, 1);
	while (i--)
	{
		md_spr_put_st_fast(&s_spr_hp_seg);
		s_spr_hp_seg.y += 8;
	}
	i = hp_capacity - lyle_hp;
	if (i <= 0) goto done;

	s_spr_hp_seg.attr = SPR_ATTR(HUD_VRAM_TILE + 10, 0, 0, BG_PAL_LINE, 1);
	while (i--)
	{
		md_spr_put_st_fast(&s_spr_hp_seg);
		s_spr_hp_seg.y += 8;
	}

done:
	s_spr_hp_seg.y = original_y;
}

static void draw_cp(void)
{
	// Label.
	md_spr_put_st_fast(&s_spr_cp_label);

	// Bar top and bottom.
	md_spr_put_st_fast(&s_spr_cp_bar[0]);
	md_spr_put_st_fast(&s_spr_cp_bar[1]);

	const fix16_t kcp_scale_factor = FIX16DIV(INTTOFIX16(HUD_CP_DISP_HEIGHT), INTTOFIX16(HUD_CP_MAX));

	int16_t scaled_cp = FIX16TOINT(FIX16MUL(kcp_scale_factor, INTTOFIX16(lyle_get_cp())));
	int16_t i = HUD_CP_DISP_HEIGHT / 8;
	const int16_t original_y = s_spr_cp_seg.y;
	while (i--)
	{
		s_spr_cp_seg.attr = SPR_ATTR(HUD_VRAM_TILE + 14 + (scaled_cp < 8 ? 2 + (2 * (scaled_cp % 8)) : 0), 0, 0, BG_PAL_LINE, 1);
		md_spr_put_st_fast(&s_spr_cp_seg);
		s_spr_cp_seg.y -= 8;
		scaled_cp -= 8;
		if (scaled_cp < 0) scaled_cp = 0;
	}
	s_spr_cp_seg.y = original_y;
}

void hud_render(void)
{
	if (!s_visible) return;
	draw_hp();
	const ProgressSlot *progress = progress_get();
	if (progress->abilities & ABILITY_PHANTOM) draw_cp();
}

void hud_set_visible(bool visible)
{
	s_visible = visible;
}
