#include "game.h"
#include "system.h"
#include "obj.h"
#include "common.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "music.h"
#include "sfx.h"
#include "progress.h"

#include "util/text.h"
#include "palscale.h"
#include "res.h"

#include "obj/map.h"
#include "obj/lyle.h"
#include "obj/particle_manager.h"
#include "obj/projectile_manager.h"
#include "obj/powerup_manager.h"
#include "obj/pause.h"

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

// Data that is preserved across rooms, and is not contained in the player's
// save data in SRAM.
typedef struct PersistentState
{
	uint8_t track_id;
	uint8_t next_room_id;
	uint8_t next_room_entrance;
	int8_t lyle_tele_in_cnt;
	int16_t lyle_phantom_cnt;
	int16_t lyle_cp;
	int16_t lyle_hp;
	fix16_t lyle_entry_dx;
	fix16_t lyle_entry_dy;
} PersistentState;

static PersistentState s_persistent_state;

static void run_frame(void)
{
	int16_t want_display_en = 0;
	ProgressSlot *prog = progress_get();
	system_profile(0);
	switch (s_game_state)
	{
		case GAME_STATE_INIT:
			system_init();
			gfx_init();
			obj_init();
			music_init();
			sfx_init();
			progress_init();
			s_game_state++;
			want_display_en = 0;
			s_persistent_state.lyle_hp = prog->hp_capacity;
			break;

		case GAME_STATE_NEW_ROOM:
			obj_clear();

			// The order of objects is important.
			obj_spawn(0, 0, OBJ_HUD, 0);
			obj_spawn(0, 0, OBJ_POWERUP_MANAGER, 0);
			obj_spawn(0, 0, OBJ_PROJECTILE_MANAGER, 0);
			obj_spawn(0, 0, OBJ_PARTICLE_MANAGER, 0);
			obj_spawn(32, 32, OBJ_LYLE, 0);
			obj_spawn(0, 0, OBJ_CUBE_MANAGER, 0);
			obj_spawn(0, 0, OBJ_MAP, 0);
			map_load(s_persistent_state.next_room_id, s_persistent_state.next_room_entrance);
			obj_spawn(0, 0, OBJ_BG, 0);
			obj_spawn(0, 0, OBJ_PAUSE, 0);

			O_Lyle *l = lyle_get();
			l->head.dx = s_persistent_state.lyle_entry_dx;
			l->head.dy = s_persistent_state.lyle_entry_dy;
			l->phantom_cnt = s_persistent_state.lyle_phantom_cnt;
			l->cp = s_persistent_state.lyle_cp;
			l->head.hp = s_persistent_state.lyle_hp;
			l->head.direction = l->head.dx < 0 ? OBJ_DIRECTION_LEFT :
			                                     OBJ_DIRECTION_RIGHT;
			l->tele_in_cnt = s_persistent_state.lyle_tele_in_cnt;

			s_persistent_state.track_id = map_get_music_track();
			music_play(s_persistent_state.track_id);
			// s_room_loaded = 1;
			s_room_elapsed = 0;
			progress_save();
			s_game_state++;
			break;

		case GAME_STATE_RUN:
			music_handle_pending();
			obj_exec();
			// TODO: used to have progress save here on frame 0. Why?

			// TODO: Remove BTN_A debug hack.
			if (map_get_exit_trigger() || ((io_pad_read(0) & BTN_A)))
			{
				if (io_pad_read(0) & BTN_A)
				{
					prog->abilities = ABILITY_MASK;
					O_Lyle *l = lyle_get();
					l->head.hp = prog->hp_capacity;
					l->cp = LYLE_MAX_CP;
					s_persistent_state.next_room_id++;
					s_persistent_state.next_room_entrance = 0;
				}
				else
				{
					s_persistent_state.next_room_id = map_get_next_room_id();
					s_persistent_state.next_room_entrance = map_get_next_room_entrance();
				}

				// Save Lyle's position and status information.
				const O_Lyle *l = lyle_get();
				s_persistent_state.lyle_entry_dx = l->head.dx;
				// Have Lyle thrust upwards if exiting from the top.
				s_persistent_state.lyle_entry_dy = (map_get_exit_trigger() == MAP_EXIT_TOP) ?
				                                   INTTOFIX16(PALSCALE_1ST(-3.0)) : l->head.dy;

				s_persistent_state.lyle_phantom_cnt = l->phantom_cnt;
				s_persistent_state.lyle_cp = l->cp;
				s_persistent_state.lyle_hp = l->head.hp;
				s_persistent_state.lyle_tele_in_cnt = l->tele_in_cnt;
				s_game_state = GAME_STATE_NEW_ROOM;
				want_display_en = 0;
			}
			else
			{
				want_display_en = !pause_want_blank() && s_room_elapsed >= 1;
			}
			s_room_elapsed++;
			break;
	}

	megadrive_finish();
	vdp_set_display_en(want_display_en);
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
