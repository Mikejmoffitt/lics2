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

#include <stdlib.h>

uint32_t g_elapsed;

static int app_alive;

// Exec functions =============================================================
typedef struct InitFunc
{
	char name[32];
	int (*init_func)(void);
} InitFunc;

static const InitFunc init_funcs[] =
{
	{"system", system_init},
	{"gfx", gfx_init},
	{"obj", obj_init},
	{"music", music_init},
	{"sfx", sfx_init},
	{"progress", progress_init},
};

static void init(void)
{
	for (unsigned int i = 0; i < ARRAYSIZE(init_funcs); i++)
	{
		const InitFunc *f = &init_funcs[i];
		if (!f->init_func) continue;
		f->init_func();
	}
}

static inline void print_hex(VdpPlane p, int16_t x, int16_t y, uint8_t num)
{
	char nums[3];
	nums[2] = '\0';

	const uint8_t low = num & 0x0F;
	const uint8_t high = (num & 0xF0) >> 4;

	if (low < 10) nums[1] = '0' + low;
	else nums[1] = 'A' + (low - 0xA);
	if (high < 10) nums[0] = '0' + high;
	else nums[0] = 'A' + (high - 0xA);

	text_puts(p, x, y, nums);
}

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
	int16_t non_fresh;
} PersistentState;

static PersistentState s_persistent_state = {0};

static void game_loop(void)
{
	ProgressSlot *prog = progress_get();
	static int16_t s_room_elapsed = 0;
	static int16_t s_room_loaded = 0;

	system_profile(0);

	if (!s_room_loaded)
	{
		if (!s_persistent_state.non_fresh)
		{
			s_persistent_state.non_fresh = 1;
			s_persistent_state.lyle_hp = prog->hp_capacity;
		}
		vdp_set_display_en(0);
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
		s_room_loaded = 1;
		megadrive_finish();
		s_room_elapsed = 0;
		return;
	}
	music_handle_pending();
	obj_exec();

	static uint8_t pad_prev;
	if (io_pad_read(0) & BTN_START && !(pad_prev & BTN_START))
	{
		ProgressSlot *prog = progress_get();
		prog->abilities = ABILITY_MASK;
		O_Lyle *l = lyle_get();
		l->head.hp = prog->hp_capacity;
		l->cp = LYLE_MAX_CP;
	}
	pad_prev = io_pad_read(0);

	if (map_get_exit_trigger() || (io_pad_read(0) & BTN_A))
	{
		if (io_pad_read(0) & BTN_A)
		{
			s_persistent_state.next_room_id++;
			s_persistent_state.next_room_entrance = 0;
		}
		else
		{
			s_persistent_state.next_room_id = map_get_next_room_id();
			s_persistent_state.next_room_entrance = map_get_next_room_entrance();
		}
		const O_Lyle *l = lyle_get();
		s_persistent_state.lyle_entry_dx = l->head.dx;
		if (map_get_exit_trigger() == MAP_EXIT_TOP)
		{
			// TODO: Determine appropriate bottom entrance dy
			s_persistent_state.lyle_entry_dy = INTTOFIX16(PALSCALE_1ST(-3.0));
		}
		else
		{
			s_persistent_state.lyle_entry_dy = l->head.dy;
		}
		s_persistent_state.lyle_phantom_cnt = l->phantom_cnt;
		s_persistent_state.lyle_cp = l->cp;
		s_persistent_state.lyle_hp = l->head.hp;
		s_persistent_state.lyle_tele_in_cnt = l->tele_in_cnt;

		s_room_loaded = 0;;
	}
	vdp_set_display_en(s_room_elapsed >= 2);
	s_room_elapsed++;
}

// Main dispatch ==============================================================
void game_main(void)
{
	init();
	megadrive_finish();
	while (1)
	{
		game_loop();
		megadrive_finish();
		g_elapsed++;
	}
}
