#include "obj/title.h"
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
#include "game.h"
#include "music.h"

static const fix32_t kfloor_pos = INTTOFIX32(688);
static const fix32_t kinitial_scroll = INTTOFIX32(360);

static fix32_t kscroll_max;
static fix16_t kscroll_gravity;
static int16_t kscroll_delay_max;
static fix16_t kbounce_dead_dy;
static int16_t kappearance_delay_max;

static int16_t kcloakdude_seq;

static uint16_t s_vram_pos;
static uint16_t vram_credits_pos;
static uint16_t vram_keddums_pos;
static uint16_t vram_cloakdude_pos;


static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g_title = gfx_get(GFX_TITLE_SCR);
	s_vram_pos = gfx_load(g_title, obj_vram_alloc(g_title->size));
	const Gfx *g_credits = gfx_get(GFX_EX_CREDITS);
	vram_credits_pos = gfx_load(g_credits, obj_vram_alloc(g_credits->size));
	const Gfx *g_keddums = gfx_get(GFX_EX_KEDDUMS_INTRO);
	vram_keddums_pos = gfx_load(g_keddums, obj_vram_alloc(g_keddums->size));
	const Gfx *g_cloakdude = gfx_get(GFX_EX_CLOAKDUDE);
	vram_cloakdude_pos = gfx_load(g_cloakdude, obj_vram_alloc(g_cloakdude->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kscroll_gravity = INTTOFIX16(PALSCALE_2ND(0.16666667));
	kscroll_delay_max = PALSCALE_DURATION(125);
	kbounce_dead_dy = INTTOFIX16(PALSCALE_1ST(1.66666667));
	kappearance_delay_max = PALSCALE_DURATION(791.6666666667);

	kscroll_max = kfloor_pos - INTTOFIX32(88) + (system_is_ntsc() ? INTTOFIX32(16.0) : 0);

	s_constants_set = 1;
}

static void render(O_Title *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -56;
	static const int16_t offset_y = -72;
	static const int16_t pal = ENEMY_PAL_LINE;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	int16_t vram = s_vram_pos;

	// Title logo
	spr_put(sp_x + (0 * 32), sp_y + (0 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	spr_put(sp_x + (1 * 32), sp_y + (0 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	spr_put(sp_x + (2 * 32), sp_y + (0 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	spr_put(sp_x + (3 * 32), sp_y + (0 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(2, 4));
	vram += 8;

	spr_put(sp_x + (0 * 32), sp_y + (1 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	spr_put(sp_x + (1 * 32), sp_y + (1 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	spr_put(sp_x + (2 * 32), sp_y + (1 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	spr_put(sp_x + (3 * 32), sp_y + (1 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(2, 4));
	vram += 8;

	spr_put(sp_x + (0 * 32), sp_y + (2 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 1));
	vram += 4;
	spr_put(sp_x + (1 * 32), sp_y + (2 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 1));
	vram += 4;
	spr_put(sp_x + (2 * 32), sp_y + (2 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 1));
	vram += 4;
	spr_put(sp_x + (3 * 32), sp_y + (2 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(2, 1));
	vram += 2;

	// The corner credits
	spr_put(GAME_SCREEN_W_PIXELS - 68, 0,
	        SPR_ATTR(vram_credits_pos, 0, 0, pal, 0),
	        SPR_SIZE(3, 1));
	spr_put(GAME_SCREEN_W_PIXELS - 68 + 24, 0,
	        SPR_ATTR(vram_credits_pos + 3, 0, 0, pal, 0),
	        SPR_SIZE(3, 1));
	spr_put(GAME_SCREEN_W_PIXELS - 68 + 48, 0,
	        SPR_ATTR(vram_credits_pos + 6, 0, 0, pal, 0),
	        SPR_SIZE(3, 1));

	spr_put(GAME_SCREEN_W_PIXELS - 68, 7,
	        SPR_ATTR(vram_credits_pos + 16, 0, 0i, pal, 0),
	        SPR_SIZE(3, 1));
	spr_put(GAME_SCREEN_W_PIXELS - 68 + 24, 7,
	        SPR_ATTR(vram_credits_pos + 19, 0, 0, pal, 0),
	        SPR_SIZE(3, 1));
	spr_put(GAME_SCREEN_W_PIXELS - 68 + 48, 7,
	        SPR_ATTR(vram_credits_pos + 22, 0, 0, pal, 0),
	        SPR_SIZE(3, 1));

	// TODO: Vyle (cloaked) and kitty
}

static void set_scroll(O_Title *e)
{
	const int16_t left_bound = GAME_SCREEN_W_PIXELS / 2;
	const int16_t top_bound = GAME_SCREEN_H_PIXELS / 2;
	int16_t px = FIX32TOINT(e->head.x);
	int16_t py = FIX32TOINT(e->v_scroll_y);
	px -= left_bound;
	py -= top_bound;
	map_set_x_scroll(px);
	map_set_y_scroll(py);
}

static void draw_house_door(int16_t frame)
{
#define MAPADDR(x, y) (2 * (x + (y * GAME_PLANE_W_CELLS)))
	static const uint16_t door_closed[] =
	{
		MAPADDR(34, 18), VDP_ATTR(0x0F, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(35, 18), VDP_ATTR(0x0F, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(34, 19), VDP_ATTR(0x0E, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(35, 19), VDP_ATTR(0x1F, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(34, 20), VDP_ATTR(0x1E, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(35, 20), VDP_ATTR(0x1F, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(34, 21), VDP_ATTR(0x6D, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(35, 21), VDP_ATTR(0x6D, 0, 0, MAP_PAL_LINE, 1),
	};
	static const uint16_t door_half[] =
	{
		MAPADDR(34, 18), VDP_ATTR(0x00, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 18), VDP_ATTR(0x0F, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(34, 19), VDP_ATTR(0x00, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 19), VDP_ATTR(0x0E, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(34, 20), VDP_ATTR(0x00, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 20), VDP_ATTR(0x1E, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(34, 21), VDP_ATTR(0x52, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 21), VDP_ATTR(0x6D, 0, 0, MAP_PAL_LINE, 1),
	};
	static const uint16_t door_open[] =
	{
		MAPADDR(34, 18), VDP_ATTR(0x00, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 18), VDP_ATTR(0x00, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(34, 19), VDP_ATTR(0x00, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 19), VDP_ATTR(0x00, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(34, 20), VDP_ATTR(0x00, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 20), VDP_ATTR(0x00, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(34, 21), VDP_ATTR(0x52, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 21), VDP_ATTR(0x53, 0, 0, MAP_PAL_LINE, 0),
	};
	static const uint16_t door_closed_low[] =
	{
		MAPADDR(34, 18), VDP_ATTR(0x0F, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 18), VDP_ATTR(0x0F, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(34, 19), VDP_ATTR(0x0E, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 19), VDP_ATTR(0x1F, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(34, 20), VDP_ATTR(0x1E, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 20), VDP_ATTR(0x1F, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(34, 21), VDP_ATTR(0x6D, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 21), VDP_ATTR(0x6D, 0, 0, MAP_PAL_LINE, 0),
	};
#undef MAPADDR

	const uint16_t *src = NULL;
	uint16_t size = 0;

	switch (frame)
	{
		default:
			return;
		case 0:
			src = door_closed;
			size = ARRAYSIZE(door_closed);
			break;
		case 1:
			src = door_half;
			size = ARRAYSIZE(door_half);
			break;
		case 2:
			src = door_open;
			size = ARRAYSIZE(door_open);
			break;
		case 3:
			src = door_closed_low;
			size = ARRAYSIZE(door_closed_low);
			break;
	}

	const uint16_t scra_base = vdp_get_plane_base(VDP_PLANE_A);
	for (int16_t i = 0; i < size / 2; i++)
	{
		const uint16_t offset = src[i * 2];
		const uint16_t data = src[1 + i * 2];
		VDPPORT_CTRL32 = (VDP_CTRL_VRAM_WRITE | VDP_CTRL_ADDR(scra_base + offset));
		VDPPORT_DATA = data;
	}
}

static void draw_normal_house_tiles(void)
{
#define MAPADDR(x, y) (2 * (x + (y * GAME_PLANE_W_CELLS)))
	static const uint16_t prio_tiles[] =
	{
		MAPADDR(30, 17), VDP_ATTR(0x1C, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(31, 17), VDP_ATTR(0x1D, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(32, 17), VDP_ATTR(0x02, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(33, 17), VDP_ATTR(0x13, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(30, 18), VDP_ATTR(0x20, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(31, 18), VDP_ATTR(0x21, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(32, 18), VDP_ATTR(0x08, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(33, 18), VDP_ATTR(0x09, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(30, 19), VDP_ATTR(0x30, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(31, 19), VDP_ATTR(0x31, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(32, 19), VDP_ATTR(0x18, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(33, 19), VDP_ATTR(0x19, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(30, 20), VDP_ATTR(0x2C, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(31, 20), VDP_ATTR(0x2D, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(32, 20), VDP_ATTR(0x28, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(33, 20), VDP_ATTR(0x29, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(30, 21), VDP_ATTR(0x3C, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(31, 21), VDP_ATTR(0x3D, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(32, 21), VDP_ATTR(0x38, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(33, 21), VDP_ATTR(0x39, 0, 0, MAP_PAL_LINE, 0),
	};
#undef MAPADDR

	const uint16_t scra_base = vdp_get_plane_base(VDP_PLANE_A);
	for (uint16_t i = 0; i < ARRAYSIZE(prio_tiles) / 2; i++)
	{
		const uint16_t offset = prio_tiles[i * 2];
		const uint16_t data = prio_tiles[1 + i * 2];
		VDPPORT_CTRL32 = (VDP_CTRL_VRAM_WRITE | VDP_CTRL_ADDR(scra_base + offset));
		VDPPORT_DATA = data;
	}
}

static void draw_high_prio_house_tiles(void)
{
	// Draw high priority tiles for the part of the house that obscures Lyle.
	// This includes a different tile for the window on the left, which is
	// transparent.
#define MAPADDR(x, y) (2 * (x + (y * GAME_PLANE_W_CELLS)))
	static const uint16_t prio_tiles[] =
	{
		MAPADDR(30, 17), VDP_ATTR(0x1C, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(31, 17), VDP_ATTR(0x1D, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(32, 17), VDP_ATTR(0x02, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(33, 17), VDP_ATTR(0x13, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(30, 18), VDP_ATTR(0xC0, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(31, 18), VDP_ATTR(0xC1, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(32, 18), VDP_ATTR(0x08, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(33, 18), VDP_ATTR(0x09, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(30, 19), VDP_ATTR(0xD0, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(31, 19), VDP_ATTR(0xD1, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(32, 19), VDP_ATTR(0x18, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(33, 19), VDP_ATTR(0x19, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(30, 20), VDP_ATTR(0x2C, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(31, 20), VDP_ATTR(0x2D, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(32, 20), VDP_ATTR(0x28, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(33, 20), VDP_ATTR(0x29, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(30, 21), VDP_ATTR(0x3C, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(31, 21), VDP_ATTR(0x3D, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(32, 21), VDP_ATTR(0x38, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(33, 21), VDP_ATTR(0x39, 0, 0, MAP_PAL_LINE, 1),
	};
#undef MAPADDR

	const uint16_t scra_base = vdp_get_plane_base(VDP_PLANE_A);
	for (uint16_t i = 0; i < ARRAYSIZE(prio_tiles) / 2; i++)
	{
		const uint16_t offset = prio_tiles[i * 2];
		const uint16_t data = prio_tiles[1 + i * 2];
		VDPPORT_CTRL32 = (VDP_CTRL_VRAM_WRITE | VDP_CTRL_ADDR(scra_base + offset));
		VDPPORT_DATA = data;
	}
}

static void main_func(Obj *o)
{
	O_Title *e = (O_Title *)o;

	const MdButton buttons = io_pad_read(0);

	// On first appearance, set up the palette.
	if (!o->offscreen && !e->initialized)
	{
		e->initialized = 1;
		e->v_scroll_y = kinitial_scroll;
		lyle_set_scroll_h_en(0);
		lyle_set_scroll_v_en(0);
		lyle_set_control_en(0);
		lyle_set_pos(o->x - INTTOFIX32(16), lyle_get_y() - INTTOFIX32(8));

		set_scroll(e);
		map_redraw_room();
	}

	if (e->appearance_delay_cnt < kappearance_delay_max - 2 &&
	    e->v_scroll_delay_cnt > 2)
	{
		if ((buttons & BTN_START) && !(e->buttons_prev & BTN_START))
		{
			e->v_scroll_y = kfloor_pos;
			e->v_scroll_complete = 1;
			e->v_scroll_delay_cnt = kscroll_delay_max;
			e->appearance_delay_cnt = kappearance_delay_max - 2;
			draw_high_prio_house_tiles();
			draw_house_door(0);
			set_scroll(e);
		}
	}

	if (e->v_scroll_delay_cnt < kscroll_delay_max)
	{
		e->v_scroll_delay_cnt++;
		set_scroll(e);
		return;
	}
	else if (!e->v_scroll_complete)
	{
		draw_high_prio_house_tiles();
		draw_house_door(0);
		e->v_scroll_dy += kscroll_gravity;
		e->v_scroll_y += (fix32_t)e->v_scroll_dy;

		if (e->v_scroll_y >= kscroll_max)
		{
			if (e->v_scroll_dy > kbounce_dead_dy)
			{
				e->v_scroll_dy = e->v_scroll_dy / -2;
			}
			else if (e->v_scroll_dy > 0 && e->v_scroll_dy < kbounce_dead_dy)
			{
				// TODO: Also trigger this on button press.
				e->v_scroll_y = kfloor_pos;
				e->v_scroll_complete = 1;
			}
		}

		set_scroll(e);
	}
	else if (e->appearance_delay_cnt < kappearance_delay_max)
	{
		pal_upload(ENEMY_CRAM_POSITION, res_pal_title_bin, sizeof(res_pal_title_bin) / 2);
		e->appearance_delay_cnt++;
		if (e->appearance_delay_cnt == kappearance_delay_max)
		{
			music_play(14);
		}
	}
	else
	{
		if ((buttons & BTN_START) && !(e->buttons_prev & BTN_START))
		{
			// TODO: proper logic to begin game with delay.
			lyle_set_control_en(1);
			pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_bin, sizeof(res_pal_title_bin) / 2);
			music_play(1);
			lyle_set_scroll_h_en(1);
			lyle_set_pos(lyle_get_x() + INTTOFIX32(16), lyle_get_y() + INTTOFIX32(8));
			draw_normal_house_tiles();
			draw_house_door(3);
			// Delete WNDWBACK objects too.
			for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
			{
				Obj *o = &g_objects[i].obj;
				if (o->status == OBJ_STATUS_NULL) continue;
				if (o->type == OBJ_WNDWBACK)
				{
					o->status = OBJ_STATUS_NULL;
				}
			}
			o->status = OBJ_STATUS_NULL;
			return;
		}
		render(e);
	}

	// TODO: proper intro scene logic
	// TODO: proper menu logic

	e->buttons_prev = buttons;

}

void o_load_title(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Title) <= sizeof(ObjSlot));
	O_Title *e = (O_Title *)o;
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-56), INTTOFIX16(56), INTTOFIX16(-72), 1);
	o->main_func = main_func;
	o->cube_func = NULL;

	e->v_scroll_y = INTTOFIX32(360);
}

void o_unload_title(void)
{
	s_vram_pos = 0;
}
