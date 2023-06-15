#include "game.h"
#include "system.h"
#include "obj.h"

#include "gfx.h"
#include "md/megadrive.h"
#include "music.h"
#include "sfx.h"
#include "progress.h"
#include "persistent_state.h"
#include "input.h"

#include "util/text.h"
#include "palscale.h"
#include "res.h"
#include "str.h"

#include "bg.h"
#include "cube_manager.h"
#include "hud.h"
#include "powerup.h"
#include "physics.h"
#include "map.h"
#include "lyle.h"
#include "fixed_vram.h"
#include "particle.h"
#include "projectile.h"
#include "pause.h"
#include "objtile.h"

#include <stdlib.h>
#include <string.h>

uint32_t g_elapsed;
static bool s_room_first_frame_shown = false;

typedef enum GameState
{
	GAME_STATE_INIT,
	GAME_STATE_NEW_ROOM,
	GAME_STATE_RUN,
} GameState;

static GameState s_game_state = GAME_STATE_INIT;

static void load_lyle_persistent_state(void)
{
	const PersistentState *persistent_state = persistent_state_get();
	O_Lyle *l = lyle_get();
	l->head.dx = persistent_state->lyle_entry_dx;
	l->head.dy = persistent_state->lyle_entry_dy;
	l->phantom_cnt = persistent_state->lyle_phantom_cnt;
	l->cp = persistent_state->lyle_cp;
	l->head.hp = persistent_state->lyle_hp;
	l->head.direction = l->head.dx < 0 ? OBJ_DIRECTION_LEFT :
	                                     OBJ_DIRECTION_RIGHT;
	l->tele_in_cnt = persistent_state->lyle_tele_in_cnt;
}

static void run_frame(void)
{
	bool want_display_en = false;
	ProgressSlot *prog = progress_get();
	PersistentState *persistent_state = persistent_state_get();
	switch (s_game_state)
	{
		case GAME_STATE_INIT:
			system_init();
			gfx_init();
			obj_init();
			music_init();
			hud_init();
			progress_init();
			powerup_init();
			projectile_init();
			particle_init();
			persistent_state_init();
			physics_init();
			objtile_clear();
			want_display_en = false;
			md_vdp_set_window_top(0);
			str_set_locale(md_sys_is_overseas() ? LOCALE_EN : LOCALE_JA);
			s_game_state++;
			break;

		case GAME_STATE_NEW_ROOM:
			obj_clear();
			sfx_init();
			fixed_vram_load();
			powerup_clear();
			objtile_clear();
			projectile_clear();
			particle_clear();
			lyle_init();
			cube_manager_init();
			map_load(persistent_state->next_room_id,
			         persistent_state->next_room_entrance);
			bg_init();
			pause_init();

			// TODO: Move this functionality into lyle.h
			load_lyle_persistent_state();
			music_play(map_get_music_track());
			s_room_first_frame_shown = false;
			progress_save();
			md_vdp_set_window_top(0);
			s_game_state++;
			break;

		case GAME_STATE_RUN:
			md_vdp_set_window_top(pause_want_window() ? 31 : 0);
			music_handle_pending();
			hud_render();
			powerup_poll();
			projectile_poll();
			particle_poll();
			lyle_poll();
			bg_poll();
			map_poll();
			cube_manager_poll();
			obj_exec();
			pause_poll();
			objtile_poll();

			const int16_t debug_room_id = pause_get_debug_room_id();
			const MapExitTrigger exit_trigger = map_get_exit_trigger();

			if ((exit_trigger != MAP_EXIT_NONE) || debug_room_id >= 0)
			{
				O_Lyle *l = lyle_get();
				if (debug_room_id >= 0)
				{
					l->head.hp = prog->hp_capacity;
					l->cp = LYLE_MAX_CP;
					persistent_state->next_room_id = debug_room_id;
					persistent_state->next_room_entrance = 0;
				}
				else if (exit_trigger == MAP_EXIT_DEAD)
				{
					persistent_state->next_room_id = 127;
				}
				else if (exit_trigger == MAP_EXIT_RESTART)
				{
					persistent_state_init();
					persistent_state->next_room_id = map_get_next_room_id();
					persistent_state->next_room_entrance =
					    map_get_next_room_entrance();
					l->cp = persistent_state->lyle_cp;
					l->head.hp = persistent_state->lyle_hp;
					l->head.dx = 0;
					l->head.dy = 0;
				}
				else
				{
					persistent_state->next_room_id = map_get_next_room_id();
					persistent_state->next_room_entrance =
					    map_get_next_room_entrance();
				}

				// Save Lyle's position and status information.
				persistent_state->lyle_entry_dx = l->head.dx;
				// Have Lyle thrust upwards if exiting from the top.
				persistent_state->lyle_entry_dy = (map_get_exit_trigger() == MAP_EXIT_TOP) ?
				                                   INTTOFIX16(PALSCALE_1ST(-3.0)) : l->head.dy;

				persistent_state->lyle_phantom_cnt = l->phantom_cnt;
				persistent_state->lyle_cp = l->cp;
				persistent_state->lyle_hp = l->head.hp;
				persistent_state->lyle_tele_in_cnt = l->tele_in_cnt;
				s_game_state = GAME_STATE_NEW_ROOM;
				want_display_en = false;
			}
			else
			{
				want_display_en = s_room_first_frame_shown;
			}
			s_room_first_frame_shown = true;
			break;
	}

#ifdef MDK_TARGET_C2
// Hack to copy BG palettes into sprite area for C2.
	for (uint16_t i = 0; i < 64; i++)
	{
		g_palette[256 + i] = g_palette[i];
	}
	md_pal_mark_dirty(256, 256 + 63);
#endif

	megadrive_finish();
	input_poll();
	md_vdp_set_display_en(want_display_en);
}

// Main dispatch ==============================================================
void game_main(void)
{
	s_game_state = GAME_STATE_INIT;

	while (true)
	{
		run_frame();
		g_elapsed++;
	}
}
