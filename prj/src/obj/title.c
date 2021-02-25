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

static fix16_t kscroll_gravity;
static int16_t kscroll_delay_max;
static fix16_t kbounce_dead_dy;
static int16_t kappearance_delay_max;

static uint16_t vram_pos;
static uint16_t vram_credits_pos;

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g_title = gfx_get(GFX_TITLE_SCR);
	vram_pos = gfx_load(g_title, obj_vram_alloc(g_title->size));
	const Gfx *g_credits = gfx_get(GFX_EX_CREDITS);
	vram_credits_pos = gfx_load(g_credits, obj_vram_alloc(g_credits->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t constants_set;
	if (constants_set) return;
	// Set constants here.

	kscroll_gravity = INTTOFIX16(PALSCALE_2ND(0.16666667));
	kscroll_delay_max = PALSCALE_DURATION(125);
	kbounce_dead_dy = INTTOFIX16(PALSCALE_1ST(1.66666667));
	kappearance_delay_max = PALSCALE_DURATION(791.6666666667);

	constants_set = 1;
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

	int16_t vram = vram_pos;

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
}

static void set_scroll(O_Title *e)
{
	const int16_t left_bound = GAME_SCREEN_W_PIXELS / 2;
	const int16_t top_bound = GAME_SCREEN_H_PIXELS / 2;
	int16_t px = FIX32TOINT(e->head.x);
	int16_t py = FIX32TOINT(e->v_scroll_y);
	px -= left_bound;
	py -= top_bound;
	map_set_scroll(px, py);
}

static void main_func(Obj *o)
{
	O_Title *e = (O_Title *)o;

	const MdButton buttons = io_pad_read(0);

	// On first appearance, set up the palette.
	if (!o->offscreen && !e->initialized)
	{
		e->initialized = 1;
		lyle_set_scroll_en(0);
		lyle_set_control_en(0);
		set_scroll(e);
		map_redraw_room();
	}

	if (e->appearance_delay_cnt < kappearance_delay_max - 2)
	{
		if ((buttons & BTN_START) && !(e->buttons_prev & BTN_START))
		{
			e->v_scroll_y = lyle_get_y();
			e->v_scroll_complete = 1;
			e->v_scroll_delay_cnt = kscroll_delay_max;
			e->appearance_delay_cnt = kappearance_delay_max - 2;
			lyle_set_scroll_en(1);
			map_redraw_room();
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
		static const fix32_t kscroll_max = INTTOFIX32(360 + 224) + (system_is_ntsc ? INTTOFIX32(32.0) : 0);;
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
				e->v_scroll_y = lyle_get_y();
				e->v_scroll_complete = 1;
				lyle_set_scroll_en(1);
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
			o->status = OBJ_STATUS_NULL;
			pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_bin, sizeof(res_pal_title_bin) / 2);
			music_play(1);
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
	vram_pos = 0;
}
