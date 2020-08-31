#include "obj/hud.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"
#include "obj/lyle.h"
#include "progress.h"

// On-screen height of CP meter. Should be a multiple of 8.
#define HUD_CP_DISP_HEIGHT 48

// The internal maximum value for the CP counter.
#define HUD_CP_MAX 29

static uint16_t vram_pos;

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g = gfx_get(GFX_EX_HUD);
	vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void draw_hp(void)
{
	const ProgressSlot *progress = progress_get();
	const int16_t hud_x = 4;
	const int16_t y_offset = vdp_get_raster_height() == 240 ? 0 : -8;

	// Label.
	spr_put(hud_x, y_offset + 15,
	        SPR_ATTR(vram_pos + 4, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 2));

	// Show health.
	const int16_t hp_capacity = progress->hp_capacity;
	int16_t plot_y = y_offset + 31;
	int16_t i = lyle_get_hp();
	while (i--)
	{
		spr_put(hud_x, plot_y, SPR_ATTR(vram_pos + 8, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 1));
		plot_y += 8;
	}
	i = hp_capacity - lyle_get_hp();
	if (i <= 0) return;
	while (i--)
	{
		spr_put(hud_x, plot_y, SPR_ATTR(vram_pos + 10, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 1));
		plot_y += 8;
	}
}

static inline void draw_cp(void)
{
	const int16_t hud_x = 4;
	const int16_t y_offset = vdp_get_raster_height() == 240 ? 0 : -8;

	// Label.
	spr_put(hud_x, y_offset + 206,
	        SPR_ATTR(vram_pos, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 2));

	// Top and bottom of the bar.
	spr_put(hud_x, y_offset + 147, SPR_ATTR(vram_pos + 12, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 1));
	spr_put(hud_x, y_offset + 147 + HUD_CP_DISP_HEIGHT + 1, SPR_ATTR(vram_pos + 12, 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 1));

	int16_t scaled_cp = FIX16TOINT(FIX16MUL(INTTOFIX16(HUD_CP_DISP_HEIGHT / (float)HUD_CP_MAX), INTTOFIX16(lyle_get_cp())));
	int16_t i = HUD_CP_DISP_HEIGHT / 8;
	int16_t plot_y = y_offset + 147 + HUD_CP_DISP_HEIGHT;
	while (i--)
	{
		spr_put(hud_x, plot_y, SPR_ATTR(vram_pos + 14 + (scaled_cp < 8 ? 2 + (2 * (scaled_cp % 8)) : 0), 0, 0, BG_PAL_LINE, 1), SPR_SIZE(2, 1));
		plot_y -= 8;
		scaled_cp -= 8;
		if (scaled_cp < 0) scaled_cp = 0;
	}
}

static void main_func(Obj *o)
{
	(void)o;
	const ProgressSlot *progress = progress_get();
	draw_hp();
	if (progress->abilities & ABILITY_PHANTOM) draw_cp();
}

void o_load_hud(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Hud) <= sizeof(ObjSlot));
	(void)data;
	vram_load();

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE, 0, 0, 0, 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_hud(void)
{
	vram_pos = 0;
}
