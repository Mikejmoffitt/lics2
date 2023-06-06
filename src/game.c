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

#include "hud.h"
#include "powerup.h"
#include "physics.h"
#include "map.h"
#include "lyle.h"
#include "particle.h"
#include "projectile.h"
#include "obj/pause.h"
#include "objtile.h"

#include <stdlib.h>
#include <string.h>

uint32_t g_elapsed;
static int32_t s_room_elapsed = 0;

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
	int16_t want_display_en = 0;
	ProgressSlot *prog = progress_get();
	system_profile(0);
	PersistentState *persistent_state = persistent_state_get();
	switch (s_game_state)
	{
		case GAME_STATE_INIT:
			system_init();
			gfx_init();
			obj_init();
			music_init();
			sfx_init();
			progress_init();
			persistent_state_init();
			physics_init();
			objtile_clear();
			want_display_en = 0;
			md_vdp_set_window_top(0);
			md_vdp_register_vblank_wait_callback(sfx_poll);
			str_set_locale(md_sys_is_overseas() ? LOCALE_EN : LOCALE_JA);
			s_game_state++;
			break;

		case GAME_STATE_NEW_ROOM:
			// Clear object list, and reset bump VRAM allocator.
			obj_clear();
			// Static singletons load or allocate VRAM first.
			hud_load();
			powerup_load();
			objtile_clear();
			projectile_load();
			particle_load();
			lyle_load();
			// TODO: Remove these singleton objects.
			// The order of objects is important.
			obj_spawn(0, 0, OBJ_CUBE_MANAGER, 0);
			map_load(persistent_state->next_room_id,
			         persistent_state->next_room_entrance);
			obj_spawn(0, 0, OBJ_BG, 0);
			obj_spawn(0, 0, OBJ_PAUSE, 0);

			// TODO: Move this functionality into lyle.h
			load_lyle_persistent_state();
			music_play(map_get_music_track());
			s_room_elapsed = 0;
			progress_save();
			md_vdp_set_window_top(0);
			s_game_state++;
			break;

		case GAME_STATE_RUN:
			md_vdp_set_window_top(pause_want_window() ? 31 : 0);
			// Static singleton objects / managers
			music_handle_pending();
			hud_render();
			powerup_poll();
			projectile_poll();
			particle_poll();
			lyle_poll();
			map_poll();
			// Game actor/object system
			obj_exec();
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
				want_display_en = 0;
			}
			else
			{
				want_display_en = s_room_elapsed >= 1;
			}
			s_room_elapsed++;
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

	while (1)
	{
		run_frame();
		g_elapsed++;
	}
}
