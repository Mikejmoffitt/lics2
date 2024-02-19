#include "obj/endlyle.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"
#include "lyle.h"
#include "hud.h"
#include "input.h"

#define ENDLYLE_JANKY_VRAM_NT_BASE 0xE000
#define ENDLYLE_JANKY_VRAM_HS_BASE 0xF800

static int16_t get_y_offs(void)
{
	return system_is_ntsc() ? 9 : 0;
}

static void obliterate_lyle_himself(void)
{
	lyle_set_hibernate(true);
	lyle_set_control_en(false);
	lyle_set_master_en(false);
	lyle_set_scroll_h_en(false);
	lyle_set_scroll_v_en(false);
	lyle_get()->tele_in_cnt = 3;
	lyle_set_pos(INTTOFIX32(-32), INTTOFIX32(-32));
	hud_set_visible(false);
}

static void main_func(Obj *o)
{
	O_EndLyle *e = (O_EndLyle *)o;
	obliterate_lyle_himself();

	if (!e->initialized)
	{
		// Background
		md_dma_transfer_vram(32, res_end_bg_chr, sizeof(res_end_bg_chr)/2, 2);
		md_cspr_upload_tiles(e->cspr[0].cspr_data, e->cspr[0].vram_base);
		md_cspr_upload_tiles(e->cspr[1].cspr_data, e->cspr[1].vram_base);
		// Palettes
		md_cspr_upload_pal(e->cspr[0].cspr_data, LYLE_PAL_LINE);
		md_cspr_upload_pal(e->cspr[1].cspr_data, ENEMY_PAL_LINE);
		md_pal_upload(MAP_TILE_CRAM_POSITION, res_end_bg_pal, sizeof(res_end_bg_pal)/2);
		// Hack VDP stuff
		md_vdp_set_plane_base(VDP_PLANE_A, ENDLYLE_JANKY_VRAM_NT_BASE);
		md_vdp_set_plane_base(VDP_PLANE_B, ENDLYLE_JANKY_VRAM_NT_BASE);
		md_vdp_set_vscroll_mode(VDP_VSCROLL_PLANE);
		md_vdp_set_hscroll_mode(VDP_HSCROLL_PLANE);
		md_vdp_set_hscroll_base(ENDLYLE_JANKY_VRAM_HS_BASE);

		md_vdp_poke(ENDLYLE_JANKY_VRAM_HS_BASE, 10);

		e->ys[0] = get_y_offs();
		e->ys[1] = get_y_offs();

		e->initialized = true;
	}
	else
	{
		// Lyle
		md_cspr_put_st(&e->cspr[0]);
		// Cube from bank 1
		e->cspr[1].x = 19;
		e->cspr[1].y = 148 - get_y_offs();
		e->cspr[1].frame = 0;
		md_cspr_put_st(&e->cspr[1]);
		// Keddums from bank 1
		e->cspr[1].x = 209;
		e->cspr[1].y = 119 - get_y_offs();
		e->cspr[1].frame = 1;
		md_cspr_put_st(&e->cspr[1]);

		if (input_read() & LYLE_BTN_START)
		{
			md_vdp_set_display_en(false);
			system_init();
			map_set_next_room(0, 0);
			map_set_exit_trigger(MAP_EXIT_OTHER);
		}
	}
	md_dma_transfer_vsram(0, e->ys, sizeof(e->ys) / 2, 4);
}


void o_load_endlyle(Obj *o, uint16_t data)
{
	O_EndLyle *e = (O_EndLyle *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	obliterate_lyle_himself();

	// Lyle
	e->cspr[0].cspr_data = res_csp_end_lyle_csp;
	e->cspr[0].vram_base = VRAM_FREE_START;
	e->cspr[0].x = 40;
	e->cspr[0].y = 26 - get_y_offs();
	e->cspr[0].attr = SPR_ATTR(0, 0, 0, LYLE_PAL_LINE, 0);
	e->cspr[0].frame = 0;
	e->cspr[0].use_dma = false;
	// Cube, keddums
	e->cspr[1].cspr_data = res_csp_end_cube_keddums_csp;
	e->cspr[1].vram_base = VRAM_FREE_START + 0x3400;
	e->cspr[1].attr = SPR_ATTR(0, 0, 0, ENEMY_PAL_LINE, 0);
	e->cspr[1].frame = 0;
	e->cspr[1].use_dma = false;

	// Set up bacground
	md_dma_transfer_vram(ENDLYLE_JANKY_VRAM_NT_BASE, res_end_bg_map, sizeof(res_end_bg_map)/2, 2);

	obj_basic_init(o, "EndLyle", 0,
	               INTTOFIX16(-1), INTTOFIX16(1), INTTOFIX16(-2), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_endlyle(void)
{
}
