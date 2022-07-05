#include "obj/gameover.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "obj/lyle.h"
#include "obj/hud.h"
#include "sfx.h"
#include "progress.h"
#include "music.h"

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_GAMEOVER);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static fix16_t kgravity;
static fix16_t kgravity_strong;
static int16_t klyle_spin_anim_speed;
static int16_t klyle_dazed_anim_speed;
static fix16_t kbounce_dy;
static int16_t kchoose_delay;

static float klyle_anim_speed_coef;

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kgravity = INTTOFIX16(PALSCALE_2ND(0.09259259259259));  // Same as kgravity_dead in lyle.c
	kgravity_strong = INTTOFIX16(PALSCALE_2ND(0.17361111111));
	klyle_spin_anim_speed = PALSCALE_DURATION(6);
	klyle_anim_speed_coef = PALSCALE_DURATION(6.0 / 5.0);
	klyle_dazed_anim_speed = PALSCALE_DURATION(8);
	kbounce_dy = INTTOFIX16(PALSCALE_1ST(-1.5));
	kchoose_delay = PALSCALE_DURATION(36);

	s_constants_set = 1;
}

static void render(O_GameOver *e)
{
	Obj *o = &e->head;
	const int16_t yscroll = map_get_y_scroll();
	if (e->state >= GAMEOVER_CUBE_FALL)
	{
		int16_t sp_x, sp_y;
		static const int16_t offset_x = -32;
		static const int16_t offset_y = -64;

		obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
		                        map_get_x_scroll(), yscroll);
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
		                             ENEMY_PAL_LINE, 0), SPR_SIZE(4, 4));
		md_spr_put(sp_x + 32, sp_y, SPR_ATTR(s_vram_pos + 16, 0, 0,
		                                  ENEMY_PAL_LINE, 0), SPR_SIZE(4, 4));
		md_spr_put(sp_x, sp_y + 32, SPR_ATTR(s_vram_pos + 32, 0, 0,
		                                  ENEMY_PAL_LINE, 0), SPR_SIZE(4, 4));
		md_spr_put(sp_x + 32, sp_y + 32, SPR_ATTR(s_vram_pos + 48, 0, 0,
		                                       ENEMY_PAL_LINE, 0), SPR_SIZE(4, 4));
	}

	if (e->state >= GAMEOVER_CUBE_BOUNCE)
	{
		const int16_t tile_offs_c = (e->choose_cnt == 0 &&
		                             e->menu_choice == 0 &&
		                             e->lyle_anim_frame == 1) ? 0x48 : 0x40;
		md_spr_put(80, 216 - yscroll, SPR_ATTR(s_vram_pos + tile_offs_c, 0, 0, ENEMY_PAL_LINE, 1), SPR_SIZE(4, 1));
		md_spr_put(112, 216 - yscroll, SPR_ATTR(s_vram_pos + 4 + tile_offs_c, 0, 0, ENEMY_PAL_LINE, 1), SPR_SIZE(4, 1));

		const int16_t tile_offs_s = (e->choose_cnt == 0 &&
		                             e->menu_choice == 1 &&
		                             e->lyle_anim_frame == 1) ? 0x59 : 0x50;
		md_spr_put(168, 216 - yscroll, SPR_ATTR(s_vram_pos + tile_offs_s, 0, 0, ENEMY_PAL_LINE, 1), SPR_SIZE(4, 1));
		md_spr_put(200, 216 - yscroll, SPR_ATTR(s_vram_pos + 4 + tile_offs_s, 0, 0, ENEMY_PAL_LINE, 1), SPR_SIZE(4, 1));
		md_spr_put(232, 216 - yscroll, SPR_ATTR(s_vram_pos + 8 + tile_offs_s, 0, 0, ENEMY_PAL_LINE, 1), SPR_SIZE(1, 1));
	}
}

static void main_func(Obj *o)
{
	O_GameOver *e = (O_GameOver *)o;
	O_Lyle *l = lyle_get();

	const GameOverState state_prev = e->state;

	const MdButton buttons = md_io_pad_read(0);

	switch (e->state)
	{
		case GAMEOVER_LYLE_FALL:
			e->lyle_dy += kgravity;
			e->lyle_y += e->lyle_dy;

			OBJ_SIMPLE_ANIM(e->lyle_anim_cnt, e->lyle_anim_frame, 4, klyle_spin_anim_speed);
			if (e->lyle_anim_cnt == 0 && e->lyle_anim_frame % 2 == 0)
			{
				l->head.direction = (l->head.direction == OBJ_DIRECTION_LEFT) ?
				                    OBJ_DIRECTION_RIGHT : OBJ_DIRECTION_LEFT;
			}
			l->anim_frame = (e->lyle_anim_frame % 2) ? 0x0F : 0x10;

			if (e->lyle_y >= e->max_y)
			{
				e->lyle_y = e->max_y;
				e->state = GAMEOVER_LYLE_ANIM;
				l->head.direction = OBJ_DIRECTION_RIGHT;
				l->anim_frame = 0x11;
			}

			l->head.x = o->x;
			l->head.y = e->lyle_y;
			break;

		case GAMEOVER_LYLE_ANIM:
			{
				typedef struct LyleAnim
				{
					int8_t anim_frame;
					int16_t duration;
					int8_t xoff;
					int8_t yoff;
					ObjDirection dir;
					SfxId sfx;
				} LyleAnim;

				static const LyleAnim anim[] =
				{
					{0x11, 10, 0, 0, OBJ_DIRECTION_RIGHT, SFX_BOINGO_JUMP},
					{0x12, 20, 0, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, 0, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, 1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, -1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, 1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, -1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, 1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, -1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, 1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, -1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x11, 10, 0, 0, OBJ_DIRECTION_RIGHT, SFX_BOINGO_JUMP},
					{0x12, 20, 0, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, 0, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, 1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, -1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, 1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, -1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, 1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, -1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, 1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x13, 2, -1, 0, OBJ_DIRECTION_RIGHT, SFX_NULL},
					{0x0D, 20, 0, 0, OBJ_DIRECTION_RIGHT, SFX_WALK1},
					{0x0D, 20, 0, 0, OBJ_DIRECTION_LEFT, SFX_WALK2},
					{0x0D, 20, 0, 0, OBJ_DIRECTION_RIGHT, SFX_WALK1},
					{0x0D, 10, 0, 0, OBJ_DIRECTION_LEFT, SFX_WALK2},
					{0x0E, 1, 0, 0, OBJ_DIRECTION_RIGHT, SFX_WALK1},
				};
				if (e->state_elapsed == 0)
				{
					e->lyle_anim_cnt = 0;
					e->lyle_anim_frame = 0;
				}

				const LyleAnim *frame = &anim[e->lyle_anim_frame];
				if (e->lyle_anim_cnt == 0)
				{
					if (frame->sfx != SFX_NULL) sfx_play(frame->sfx, 0);
					l->head.x += INTTOFIX32(frame->xoff);
					l->head.y += INTTOFIX32(frame->yoff);
				}

				l->head.direction = frame->dir;
				l->anim_frame = frame->anim_frame;

				if (e->lyle_anim_frame >= ARRAYSIZE(anim) - 1)
				{
					e->state = GAMEOVER_CUBE_FALL;
					break;
				}

				if (e->lyle_anim_cnt >= frame->duration * klyle_anim_speed_coef)
				{
					e->lyle_anim_cnt = 0;
					e->lyle_anim_frame++;
				}
				else
				{
					e->lyle_anim_cnt++;
				}
			}
			break;

		case GAMEOVER_CUBE_FALL:
			o->dy += kgravity;
			o->y += o->dy;
			if (o->y >= e->max_y && o->dy > 0)
			{
				o->y = e->max_y;
				o->dy = kbounce_dy;
				e->state = GAMEOVER_CUBE_BOUNCE;
				l->anim_frame = 0x15;
				l->head.x -= INTTOFIX32(3);
				// TODO: Play crash sound
			}
			break;

		case GAMEOVER_CUBE_BOUNCE:
			if (e->state_elapsed == 0)
			{
				e->lyle_anim_cnt = 0;
				e->lyle_anim_frame = 0;
				music_play(13);
			}
			o->dy += kgravity_strong;
			o->y += o->dy;
			if (o->y >= e->max_y && o->dy > 0)
			{
				o->y = e->max_y;
				o->dy = 0;
				e->state = GAMEOVER_CUBE_MENU;
			}
			// Fall-through intentional.
		case GAMEOVER_CUBE_MENU:
			OBJ_SIMPLE_ANIM(e->lyle_anim_cnt, e->lyle_anim_frame, 2, klyle_dazed_anim_speed);
			l->anim_frame = e->lyle_anim_frame ? 0x15 : 0x14;

			if (e->choose_cnt == 0)
			{
				if ((buttons & BTN_LEFT) && !(e->buttons_prev & BTN_LEFT))
				{
					e->menu_choice = 0;
					sfx_stop(SFX_BEEP);
					sfx_play(SFX_BEEP, 1);
				}
				else if ((buttons & BTN_RIGHT) && !(e->buttons_prev & BTN_RIGHT))
				{
					e->menu_choice = 1;
					sfx_stop(SFX_BEEP);
					sfx_play(SFX_BEEP, 1);
				}
				
				if ((buttons & (BTN_A | BTN_C | BTN_START)) &&
				    !(e->buttons_prev & (BTN_A | BTN_C | BTN_START)))
				{
					e->choose_cnt = 1;
					sfx_play(SFX_SELECT_1, 0);
					sfx_play(SFX_SELECT_2, 0);
				}
			}
			else
			{
				e->choose_cnt++;
				if (e->choose_cnt >= kchoose_delay)
				{
					map_set_next_room(e->menu_choice ? 0 : 1, 0);
					map_set_exit_trigger(MAP_EXIT_RESTART);
					hud_set_visible(1);
				}
			}
			break;
	}

	if (e->state != state_prev) e->state_elapsed = 0;
	else e->state_elapsed++;

	md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_gameover_bin,
	           sizeof(res_pal_enemy_gameover_bin) / 2);

	e->buttons_prev = buttons;
	
	render(e);
}

void o_load_gameover(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_GameOver) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "GameOver", 0,
	               INTTOFIX16(-32), INTTOFIX16(32), INTTOFIX16(-64), 127);
	o->main_func = main_func;
	o->cube_func = NULL;

	O_GameOver *e = (O_GameOver *)o;
	e->max_y = o->y;
	e->lyle_y = INTTOFIX32(-28);
	o->y = INTTOFIX32(-48);

	hud_set_visible(0);
	lyle_set_master_en(0);
	progress_save();
}

void o_unload_gameover(void)
{
	s_vram_pos = 0;
}
