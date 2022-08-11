#include "obj/title.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "obj/lyle.h"
#include "game.h"
#include "hud.h"
#include "music.h"
#include "progress.h"
#include "persistent_state.h"
#include "obj/wndwback.h"
#include "obj/metagrub.h"
#include "obj/pause.h"
#include "sfx.h"
#include "input.h"

static const fix32_t kfloor_pos = INTTOFIX32(688);
static const fix32_t kinitial_scroll = INTTOFIX32(360);

static fix32_t kscroll_max;
static fix16_t kscroll_gravity;
static int16_t kscroll_delay_duration;
static fix16_t kbounce_dead_dy;
static int16_t kmenu_flash_period;

static int16_t kkitty_sleep_anim_speed;
static int16_t kcloakdude_walk_anim_speed;
static int16_t kcloakdude_look_anim_speed;
static int16_t kcloakdude_run1_anim_speed;
static int16_t kcloakdude_run2_anim_speed;
static int16_t kcloakdude_seq[6];

static int16_t klyle_seq[7];
static int16_t klyle_anim_speed;

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kscroll_gravity = INTTOFIX16(PALSCALE_2ND(0.16666667));
	kscroll_delay_duration = PALSCALE_DURATION(125);
	kbounce_dead_dy = INTTOFIX16(PALSCALE_1ST(1.66666667));
	kmenu_flash_period = PALSCALE_DURATION(16);

	kscroll_max = kfloor_pos - INTTOFIX32(88) + (system_is_ntsc() ? INTTOFIX32(16.0) : 0);

	// TODO: These are made up, not final
	kkitty_sleep_anim_speed = PALSCALE_DURATION(40);
	kcloakdude_walk_anim_speed = PALSCALE_DURATION(14);
	kcloakdude_look_anim_speed = PALSCALE_DURATION(32);
	kcloakdude_run1_anim_speed = PALSCALE_DURATION(5);
	kcloakdude_run2_anim_speed = PALSCALE_DURATION(3);

	kcloakdude_seq[0] = PALSCALE_DURATION(1);    // walk at dx = 0.2083333
	kcloakdude_seq[1] = PALSCALE_DURATION(120);  // stop, look around
	kcloakdude_seq[2] = PALSCALE_DURATION(240);  // walk at dx = 0.2083333
	kcloakdude_seq[3] = PALSCALE_DURATION(600);  // stop, look around
	kcloakdude_seq[4] = PALSCALE_DURATION(624);  // run at dx = 1.6666667
	kcloakdude_seq[5] = PALSCALE_DURATION(648);  // kitty stands, eyes open

	klyle_seq[0] = PALSCALE_DURATION(20);  // Door f1
	klyle_seq[1] = PALSCALE_DURATION(25);  // Door f2
	klyle_seq[2] = PALSCALE_DURATION(40);  // Lyle begins to walk
	klyle_seq[3] = PALSCALE_DURATION(52);  // Lyle's y += 8 (step on ground)
	klyle_seq[4] = PALSCALE_DURATION(95);  // Door f3
	klyle_seq[5] = PALSCALE_DURATION(100); // Door f4
	klyle_seq[6] = PALSCALE_DURATION(115); // Start

	klyle_anim_speed = PALSCALE_DURATION(6.8);

	s_constants_set = 1;
}

static uint16_t s_vram_pos;

static uint16_t s_vram_shared_pos;  // Used by the following four.
static uint16_t s_vram_credits_pos;
static uint16_t s_vram_keddums_pos;
static uint16_t s_vram_cloakdude_pos;
static uint16_t s_vram_title_menu_pos;

uint16_t title_get_vram_pos(void)
{
	return s_vram_pos;
}

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *gfx_title = gfx_get(GFX_TITLE_SCR);
	s_vram_pos = obj_vram_alloc(gfx_title->size);
	// Keddums is only seen in the intro.
	const Gfx *gfx_keddums = gfx_get(GFX_EX_KEDDUMS_INTRO);
	// The credits and the menu are only seen after the intro.
	const Gfx *gfx_credits = gfx_get(GFX_EX_CREDITS);
	const Gfx *gfx_title_menu = gfx_get(GFX_EX_TITLE_MENU);
	// So, they can share the same area in VRAM to save some space.
	const int shared_sub_vram_size = MAX(gfx_keddums->size,
	                                     gfx_credits->size + gfx_title_menu->size);
	s_vram_shared_pos = obj_vram_alloc(shared_sub_vram_size);
}

static void render_title_logo(int16_t sp_x, int16_t sp_y)
{
	static const int16_t pal = ENEMY_PAL_LINE;
	int16_t vram = s_vram_pos;

	// Title logo
	md_spr_put(sp_x + (0 * 32), sp_y + (0 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	md_spr_put(sp_x + (1 * 32), sp_y + (0 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	md_spr_put(sp_x + (2 * 32), sp_y + (0 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	md_spr_put(sp_x + (3 * 32), sp_y + (0 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(2, 4));
	vram += 8;

	md_spr_put(sp_x + (0 * 32), sp_y + (1 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	md_spr_put(sp_x + (1 * 32), sp_y + (1 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	md_spr_put(sp_x + (2 * 32), sp_y + (1 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 4));
	vram += 16;
	md_spr_put(sp_x + (3 * 32), sp_y + (1 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(2, 4));
	vram += 7;

	md_spr_put(sp_x + (0 * 32), sp_y + (2 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 1));
	vram += 4;
	md_spr_put(sp_x + (1 * 32), sp_y + (2 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 1));
	vram += 4;
	md_spr_put(sp_x + (2 * 32), sp_y + (2 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(4, 1));
	vram += 4;
	md_spr_put(sp_x + (3 * 32), sp_y + (2 * 32),
	        SPR_ATTR(vram,
	        0, 0, pal, 0),
	        SPR_SIZE(1, 1));
	vram += 1;
}

static void render_credits(void)
{
	static const int16_t pal = ENEMY_PAL_LINE;
	// The corner credits
	md_spr_put(GAME_SCREEN_W_PIXELS - 68, 0,
	        SPR_ATTR(s_vram_credits_pos, 0, 0, pal, 0),
	        SPR_SIZE(3, 1));
	md_spr_put(GAME_SCREEN_W_PIXELS - 68 + 24, 0,
	        SPR_ATTR(s_vram_credits_pos + 3, 0, 0, pal, 0),
	        SPR_SIZE(3, 1));
	md_spr_put(GAME_SCREEN_W_PIXELS - 68 + 48, 0,
	        SPR_ATTR(s_vram_credits_pos + 6, 0, 0, pal, 0),
	        SPR_SIZE(3, 1));

	md_spr_put(GAME_SCREEN_W_PIXELS - 68, 7,
	        SPR_ATTR(s_vram_credits_pos, 0, 0, pal, 0),
	        SPR_SIZE(1, 1));
	md_spr_put(GAME_SCREEN_W_PIXELS - 68 + 8, 7,
	        SPR_ATTR(s_vram_credits_pos + 9, 0, 0, pal, 0),
	        SPR_SIZE(4, 1));
	md_spr_put(GAME_SCREEN_W_PIXELS - 68 + 40, 7,
	        SPR_ATTR(s_vram_credits_pos + 13, 0, 0, pal, 0),
	        SPR_SIZE(1, 1));
	md_spr_put(GAME_SCREEN_W_PIXELS - 68 + 48, 7,
	        SPR_ATTR(s_vram_credits_pos + 6, 0, 0, pal, 0),
	        SPR_SIZE(3, 1));
}

static void render_menu(O_Title *e, int16_t sp_y)
{
	const int16_t flash_on = (e->menu_flash_cnt <= (kmenu_flash_period / 2));
	const int16_t newgame_pal = (e->menu_choice == 0 && flash_on) ?
	                            ENEMY_PAL_LINE : BG_PAL_LINE;
	const int16_t continue_pal = (e->menu_choice == 1 && flash_on) ?
	                              ENEMY_PAL_LINE : BG_PAL_LINE;

	const int16_t x = GAME_SCREEN_W_PIXELS/2;
	md_spr_put(x - 72, sp_y + 80, SPR_ATTR(s_vram_title_menu_pos, 0, 0, newgame_pal, 1), SPR_SIZE(4, 1));
	md_spr_put(x - 40, sp_y + 80, SPR_ATTR(s_vram_title_menu_pos + 4, 0, 0, newgame_pal, 1), SPR_SIZE(4, 1));
	md_spr_put(x + 8, sp_y + 80, SPR_ATTR(s_vram_title_menu_pos + 8, 0, 0, continue_pal, 1), SPR_SIZE(4, 1));
	md_spr_put(x + 40, sp_y + 80, SPR_ATTR(s_vram_title_menu_pos + 12, 0, 0, continue_pal, 1), SPR_SIZE(4, 1));
}

static void render_title_full(O_Title *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -56;
	static const int16_t offset_y = -72;

	obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
	                        map_get_x_scroll(), map_get_y_scroll());

	render_title_logo(sp_x, sp_y);
	render_credits();
	render_menu(e, sp_y);
}

static void render_kitty(O_Title *e, int16_t sp_x, int16_t sp_y)
{
	if (e->kitty_anim_state == 0)
	{
		OBJ_SIMPLE_ANIM(e->kitty_anim_cnt, e->kitty_anim_frame,
		3, kkitty_sleep_anim_speed);
		md_spr_put(sp_x + 80, sp_y + 168,
		        SPR_ATTR(s_vram_keddums_pos + (6 * e->kitty_anim_frame),
		        0, 0, ENEMY_PAL_LINE, 1), SPR_SIZE(3, 2));

	}
	else if (e->kitty_anim_state == 1)
	{
		md_spr_put(sp_x + 80, sp_y + 168,
		        SPR_ATTR(s_vram_keddums_pos + (6 * 3),
		        0, 0, ENEMY_PAL_LINE, 1), SPR_SIZE(3, 2));
	}
}

static void render_cloakdude(O_Title *e, int16_t sp_x, int16_t sp_y)
{
	sp_x += FIX32TOINT(e->cloakdude_x);

	if (e->cloakdude_anim_state == 0)
	{
		OBJ_SIMPLE_ANIM(e->cloakdude_anim_cnt, e->cloakdude_anim_frame,
		2, kcloakdude_walk_anim_speed);
		md_spr_put(sp_x, sp_y + 160,
		        SPR_ATTR(s_vram_cloakdude_pos + (9 * e->cloakdude_anim_frame),
		        0, 0, ENEMY_PAL_LINE, 1), SPR_SIZE(3, 3));
	}
	else if (e->cloakdude_anim_state == 1)
	{
		OBJ_SIMPLE_ANIM(e->cloakdude_anim_cnt, e->cloakdude_anim_frame,
		2, kcloakdude_look_anim_speed);
		md_spr_put(sp_x, sp_y + 160,
		        SPR_ATTR(s_vram_cloakdude_pos + (e->cloakdude_anim_frame ? 18 : 0),
		        0, 0, ENEMY_PAL_LINE, 1), SPR_SIZE(3, 3));
	}
	else if (e->cloakdude_anim_state == 2)
	{
		OBJ_SIMPLE_ANIM(e->cloakdude_anim_cnt, e->cloakdude_anim_frame,
		2, kcloakdude_run1_anim_speed);
		md_spr_put(sp_x, sp_y + 160,
		        SPR_ATTR(s_vram_cloakdude_pos + (9 * e->cloakdude_anim_frame),
		        0, 0, ENEMY_PAL_LINE, 1), SPR_SIZE(3, 3));
	}
	else if (e->cloakdude_anim_state == 3)
	{
		OBJ_SIMPLE_ANIM(e->cloakdude_anim_cnt, e->cloakdude_anim_frame,
		4, kcloakdude_run2_anim_speed);
		md_spr_put(sp_x, sp_y + 160,
		        SPR_ATTR(s_vram_cloakdude_pos + 27 + (9 * e->cloakdude_anim_frame),
		        0, 0, ENEMY_PAL_LINE, 1), SPR_SIZE(3, 3));
	}
}

static void render_cutscene(O_Title *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -56;
	static const int16_t offset_y = -72;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	render_kitty(e, sp_x, sp_y);
	render_cloakdude(e, sp_x, sp_y);
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
		MAPADDR(34, 21), VDP_ATTR(0x52, 0, 0, MAP_PAL_LINE, 1),
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
		MAPADDR(34, 21), VDP_ATTR(0x52, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(35, 21), VDP_ATTR(0x53, 0, 0, MAP_PAL_LINE, 1),
	};
	static const uint16_t door_half_low[] =
	{
		MAPADDR(34, 18), VDP_ATTR(0xC2, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 18), VDP_ATTR(0x0F, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(34, 19), VDP_ATTR(0xC2, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 19), VDP_ATTR(0x0E, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(34, 20), VDP_ATTR(0xC2, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 20), VDP_ATTR(0x1E, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(34, 21), VDP_ATTR(0xD2, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(35, 21), VDP_ATTR(0x6D, 0, 0, MAP_PAL_LINE, 0),
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
			src = door_half_low;
			size = ARRAYSIZE(door_half_low);
			break;
		case 4:
			src = door_closed_low;
			size = ARRAYSIZE(door_closed_low);
			break;
	}

	const uint16_t scra_base = md_vdp_get_plane_base(VDP_PLANE_A);
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
		MAPADDR(28, 18), VDP_ATTR(0x0A, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(29, 18), VDP_ATTR(0x0B, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(28, 19), VDP_ATTR(0x1A, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(29, 19), VDP_ATTR(0x1B, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(28, 20), VDP_ATTR(0x0A, 0, 0, MAP_PAL_LINE, 0),
		MAPADDR(29, 20), VDP_ATTR(0x0B, 0, 0, MAP_PAL_LINE, 0),

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

	const uint16_t scra_base = md_vdp_get_plane_base(VDP_PLANE_A);
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
		MAPADDR(28, 18), VDP_ATTR(0x0A, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(29, 18), VDP_ATTR(0x0B, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(28, 19), VDP_ATTR(0x1A, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(29, 19), VDP_ATTR(0x1B, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(28, 20), VDP_ATTR(0x0A, 0, 0, MAP_PAL_LINE, 1),
		MAPADDR(29, 20), VDP_ATTR(0x0B, 0, 0, MAP_PAL_LINE, 1),

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

	const uint16_t scra_base = md_vdp_get_plane_base(VDP_PLANE_A);
	for (uint16_t i = 0; i < ARRAYSIZE(prio_tiles) / 2; i++)
	{
		const uint16_t offset = prio_tiles[i * 2];
		const uint16_t data = prio_tiles[1 + i * 2];
		VDPPORT_CTRL32 = (VDP_CTRL_VRAM_WRITE | VDP_CTRL_ADDR(scra_base + offset));
		VDPPORT_DATA = data;
	}
}

static void maybe_skip_to_menu(O_Title *e)
{
	if (e->state_elapsed <= 2)
	{
		return;
	}
	const LyleBtn buttons = input_read();
	if ((buttons & (LYLE_BTN_START | LYLE_BTN_JUMP)) && !(e->buttons_prev & (LYLE_BTN_JUMP | LYLE_BTN_START)))
	{
		e->v_scroll_y = kfloor_pos;
		e->state_elapsed = 0;
		e->state = TITLE_STATE_MENU;
	}
}

static void main_func(Obj *o)
{
	O_Title *e = (O_Title *)o;

	const LyleBtn buttons = input_read();

	e->state_prev = e->state;
	switch (e->state)
	{
		case TITLE_STATE_INIT:
			// On first appearance, the palette is set up, Lyle is placed in
			// position, and his scroll control is taken away.
			e->v_scroll_y = kinitial_scroll;
			e->cloakdude_x = -INTTOFIX32(130);
			lyle_set_scroll_h_en(0);
			lyle_set_scroll_v_en(0);
			lyle_set_control_en(0);
			lyle_set_pos(o->x - INTTOFIX32(16), lyle_get_y() - INTTOFIX32(8));
			lyle_set_direction(OBJ_DIRECTION_LEFT);
			lyle_set_pos(lyle_get_x(), INTTOFIX32(64));
			wndwback_set_visible(0);
			map_redraw_room();

			hud_set_visible(0);
			metagrub_set_enable(0);

			e->state = TITLE_STATE_INTRO;
			break;
		case TITLE_STATE_INTRO:
			if (e->state_elapsed == 0)
			{
				const Gfx *gfx_keddums = gfx_get(GFX_EX_KEDDUMS_INTRO);
				s_vram_keddums_pos = gfx_load(gfx_keddums, s_vram_shared_pos);
			}

			if (e->state_elapsed >= kscroll_delay_duration) // was v_scroll_complete
			{
				draw_high_prio_house_tiles();
				draw_house_door(0);

				// Gravity physics, and bouncing at the bottom.
				e->v_scroll_dy += kscroll_gravity;
				e->v_scroll_y += (fix32_t)e->v_scroll_dy;
				if (e->v_scroll_y >= kscroll_max / 2)
				{
					md_pal_upload(ENEMY_CRAM_POSITION, res_pal_title_bin, sizeof(res_pal_title_bin) / 2);
				}
				if (e->v_scroll_y >= kscroll_max)
				{
					if (e->v_scroll_dy > kbounce_dead_dy)
					{
						e->v_scroll_dy = e->v_scroll_dy / -2;
					}
					else if (e->v_scroll_dy > 0 &&
					         e->v_scroll_dy < kbounce_dead_dy)
					{
						// After the bouncing has stopped, go to next state.
						e->state = TITLE_STATE_CUTSCENE;
						e->v_scroll_y = kfloor_pos;
						wndwback_set_visible(1);
					}
				}
			}
			maybe_skip_to_menu(e);
			render_cutscene(e);
			break;
		case TITLE_STATE_CUTSCENE:
			if (e->state_elapsed == 0)
			{
				md_pal_upload(ENEMY_CRAM_POSITION, res_pal_title_bin, sizeof(res_pal_title_bin) / 2);
				// Cloakdude overwrites the bogologo in VRAM, as he is only seen one the screen scroll stops.
				const Gfx *gfx_cloakdude = gfx_get(GFX_EX_CLOAKDUDE);
				s_vram_cloakdude_pos = gfx_load(gfx_cloakdude, s_vram_pos);
			}

			if (e->state_elapsed == kcloakdude_seq[0])
			{
				e->cloakdude_anim_state = 0;
				e->cloakdude_dx = INTTOFIX16(PALSCALE_1ST(0.2777778));
			}
			else if (e->state_elapsed == kcloakdude_seq[1])
			{
				e->cloakdude_anim_state = 1;
				e->cloakdude_dx = 0;
			}
			else if (e->state_elapsed == kcloakdude_seq[2])
			{
				e->cloakdude_anim_state = 0;
				e->cloakdude_dx = INTTOFIX16(PALSCALE_1ST(0.2777778));
			}
			else if (e->state_elapsed == kcloakdude_seq[3])
			{
				e->cloakdude_anim_state = 1;
				e->cloakdude_dx = 0;
			}
			else if (e->state_elapsed == kcloakdude_seq[4])
			{
				e->cloakdude_anim_state = 2;
				e->cloakdude_dx = INTTOFIX16(PALSCALE_1ST(1.6666667));
			}
			else if (e->state_elapsed == kcloakdude_seq[5])
			{
				e->kitty_anim_state = 1;
			}
			else if (e->cloakdude_dx > 0 && e->cloakdude_x >= INTTOFIX32(74))
			{
				e->cloakdude_dx = -INTTOFIX16(PALSCALE_1ST(2.500000));
				e->cloakdude_anim_state = 3;
				e->kitty_anim_state = 2;
			}

			e->cloakdude_x += e->cloakdude_dx;
			if (e->cloakdude_x <= -INTTOFIX32(208))
			{
				if (e->cloakdude_dx != 0)
				{
					lyle_set_pos(lyle_get_x(), INTTOFIX32(679));
					e->cloakdude_dx = 0;
				}
				const fix32_t lyle_x = lyle_get_x();
				if (lyle_get_x() > o->x - INTTOFIX32(32))
				{
					lyle_set_pos(lyle_x - INTTOFIX32(PALSCALE_1ST(1.0)),
					             lyle_get_y());
				}
				else
				{
					e->state = TITLE_STATE_MENU;
				}
			}
			render_cutscene(e);

			maybe_skip_to_menu(e);
			break;
		case TITLE_STATE_MENU:
			if (e->state_elapsed == 0)
			{
				md_pal_upload(ENEMY_CRAM_POSITION, res_pal_title_bin, sizeof(res_pal_title_bin) / 2);
				draw_high_prio_house_tiles();
				draw_house_door(0);
				e->menu_choice = 1;  // Continue.
				music_play(14);  // Alone in the Dark
				lyle_set_pos(o->x - INTTOFIX32(32), INTTOFIX32(679));
				wndwback_set_visible(1);
			}
			else if (e->state_elapsed == 1)
			{
				const Gfx *gfx_title = gfx_get(GFX_TITLE_SCR);
				s_vram_pos = gfx_load(gfx_title, s_vram_pos);
				const Gfx *gfx_credits = gfx_get(GFX_EX_CREDITS);
				const Gfx *gfx_title_menu = gfx_get(GFX_EX_TITLE_MENU);
				s_vram_credits_pos = gfx_load(gfx_credits, s_vram_shared_pos);
				s_vram_title_menu_pos = gfx_load(gfx_title_menu, s_vram_shared_pos + gfx_credits->size);
			}

			e->menu_flash_cnt++;
			if (e->menu_flash_cnt >= kmenu_flash_period)
			{
				e->menu_flash_cnt = 0;
			}

			if ((buttons & LYLE_BTN_LEFT) && !(e->buttons_prev & LYLE_BTN_LEFT) &&
				e->menu_choice != 0)
			{
				e->menu_choice = 0;
				sfx_stop(SFX_BEEP);
				sfx_play(SFX_BEEP, 1);
			}

			if ((buttons & LYLE_BTN_RIGHT) && !(e->buttons_prev & LYLE_BTN_RIGHT) &&
				e->menu_choice != 1)
			{
				e->menu_choice = 1;
				sfx_stop(SFX_BEEP);
				sfx_play(SFX_BEEP, 1);
			}

			if ((buttons & (LYLE_BTN_START | LYLE_BTN_JUMP)) && !(e->buttons_prev & (LYLE_BTN_START | LYLE_BTN_JUMP)))
			{
				if (e->menu_choice == 0)
				{
					// TODO: This isn't actually working somehow, fix!
					progress_erase();
					persistent_state_init();
				}
				e->state = TITLE_STATE_BEGIN;
				sfx_play(SFX_SELECT_1, 0);
				sfx_play(SFX_SELECT_2, 0);
			}
			
			if (e->state_elapsed >= 1)
			{
				render_title_full(e);
			}

			break;
		case TITLE_STATE_BEGIN:
			if (e->state_elapsed == klyle_seq[0])
			{
				draw_house_door(1);
			}
			else if (e->state_elapsed == klyle_seq[1])
			{
				draw_house_door(2);
			}
			else if (e->state_elapsed >= klyle_seq[2])
			{
				lyle_set_direction(OBJ_DIRECTION_RIGHT);
				O_Lyle *l = lyle_get();

				const fix32_t lyle_x = lyle_get_x();
				if (lyle_x < o->x)
				{
					OBJ_SIMPLE_ANIM(e->lyle_anim_cnt, e->lyle_anim_frame, 4, klyle_anim_speed);
					if (e->lyle_anim_frame == 0)
					{
						l->anim_frame = 0x01;
					}
					else if (e->lyle_anim_frame == 2)
					{
						l->anim_frame = 0x03;
					}
					else
					{
						l->anim_frame = 0x02;
					}
					lyle_set_pos(lyle_x + INTTOFIX32(PALSCALE_1ST(1.0)),
					             lyle_get_y());
				}
				else
				{
					l->priority = 1;
					l->anim_frame = 0x00;
				}
			}

			if (e->state_elapsed == klyle_seq[3])
			{
				lyle_set_pos(lyle_get_x(), lyle_get_y() + INTTOFIX32(8));
			}
			else if (e->state_elapsed == klyle_seq[4])
			{
				// Delete WNDWBACK objects near door
				for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
				{
					Obj *w = &g_objects[i].obj;
					if (w->status == OBJ_STATUS_NULL) continue;
					if (w->x < o->x - INTTOFIX32(8) ||
					    w->x > o->x + INTTOFIX32(8))
					{
						continue;
					}
					if (w->type == OBJ_WNDWBACK)
					{
						obj_erase(w);
					}
				}
			}
			else if (e->state_elapsed == klyle_seq[4] + 1)
			{
				draw_house_door(3);
			}
			else if (e->state_elapsed == klyle_seq[5])
			{
				draw_house_door(4);
			}
			else if (e->state_elapsed == klyle_seq[6] - 1)
			{
				// Delete remaining WNDWBACK objects
				for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
				{
					Obj *w = &g_objects[i].obj;
					if (w->status == OBJ_STATUS_NULL) continue;
					if (w->type == OBJ_WNDWBACK)
					{
						obj_erase(w);
					}
				}
			}
			else if (e->state_elapsed >= klyle_seq[6])
			{
				md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_bin,
				           sizeof(res_pal_title_bin) / 2);
				draw_normal_house_tiles();
				lyle_set_control_en(1);
				lyle_get()->priority = 0;
				music_play(1);  // Stage music
				obj_erase(o);
				hud_set_visible(1);
				metagrub_set_enable(1);
				return;
			}
			render_title_full(e);
			break;
	}

	if (e->state != e->state_prev) e->state_elapsed = 0;
	else e->state_elapsed++;

	set_scroll(e);
	e->buttons_prev = buttons;
}

void o_load_title(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Title) <= sizeof(ObjSlot));
	O_Title *e = (O_Title *)o;
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Title", OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-56), INTTOFIX16(56), INTTOFIX16(-72), 1);
	o->main_func = main_func;
	o->cube_func = NULL;

	e->v_scroll_y = INTTOFIX32(360);
	hud_set_visible(0);
	metagrub_set_enable(0);
}

void o_unload_title(void)
{
	s_vram_pos = 0;
}

