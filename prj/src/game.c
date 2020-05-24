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

typedef enum Exec
{
	GE_INIT = 0,
	GE_SHUTDOWN,
	GE_INTRO,
	GE_TITLE,
	GE_CONFIG,
	GE_GAME_START,
	GE_GAME_INGAME,
	GE_GAMEOVER,
	GE_ENDING,
	GE_INVALID,
} Exec;

#define FIRST_EXEC GE_GAME_INGAME

static Exec exec_next;  // exec takes this at the end of the current exec.
static Exec exec;
uint32_t g_elapsed;  // Reset when exec is set.

static int app_alive;

// Exec flow ==================================================================

static inline void exec_change(Exec next)
{
	exec_next = next;
	g_elapsed = 0;
}

static inline void exec_end_of_frame(void)
{
	if (exec_next != GE_INVALID)
	{
		exec = exec_next;
		exec_next = GE_INVALID;
		g_elapsed = 0;
	}
	else
	{
		g_elapsed++;
	}
}

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

static void ge_init(void)
{
	for (unsigned int i = 0; i < ARRAYSIZE(init_funcs); i++)
	{
		const InitFunc *f = &init_funcs[i];
		if (!f->init_func) continue;
		f->init_func();
	}

	exec_change(FIRST_EXEC);
}

static void ge_shutdown(void)
{
}

static void ge_intro(void)
{

}

static void ge_title(void)
{

}

static void ge_config(void)
{

}

static void ge_game_start(void)
{

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

static void ge_game_ingame(void)
{
	static uint8_t track_id = 1;
	static uint8_t next_room_id = 1;
	static uint8_t next_room_entrance = 0;
	static fix16_t lyle_entry_dx = 0;
	static fix16_t lyle_entry_dy = 0;
	static int16_t lyle_cp = 0;
	static int16_t lyle_phantom_cnt = 0;
	static int8_t lyle_tele_in_cnt = 0;
	system_profile(0);

	if (g_elapsed == 0)
	{
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
		map_load(next_room_id, next_room_entrance);
		obj_spawn(0, 0, OBJ_BG, 0);

		O_Lyle *l = lyle_get();
		l->head.dx = lyle_entry_dx;
		l->head.dy = lyle_entry_dy;
		l->phantom_cnt = lyle_phantom_cnt;
		l->cp = lyle_cp;
		l->head.direction = l->head.dx < 0 ? OBJ_DIRECTION_LEFT :
		                                     OBJ_DIRECTION_RIGHT;
		l->tele_in_cnt = lyle_tele_in_cnt;

		track_id = map_get_music_track();
		music_play(track_id);

		return;
	}
	else if (g_elapsed == 2)
	{
		vdp_set_display_en(1);
	}
	music_handle_pending();
	obj_exec();

	static uint8_t pad_prev;
	if (io_pad_read(0) & BTN_START && !(pad_prev & BTN_START))
	{
		track_id++;
		if (track_id > 15) track_id = 0;
		music_play(track_id);
	}
	pad_prev = io_pad_read(0);

	if (map_get_exit_trigger() || io_pad_read(0) & BTN_A)
	{
		if (io_pad_read(0) & BTN_A)
		{
			next_room_id++;
			next_room_entrance = 0;
		}
		else
		{
			next_room_id = map_get_next_room_id();
			next_room_entrance = map_get_next_room_entrance();
		}
		const O_Lyle *l = lyle_get();
		lyle_entry_dx = l->head.dx;
		if (map_get_exit_trigger() == MAP_EXIT_TOP)
		{
			// TODO: Determine appropriate bottom entrance dy
			lyle_entry_dy = INTTOFIX16(PALSCALE_1ST(-3.0));
		}
		else
		{
			lyle_entry_dy = l->head.dy;
		}
		lyle_phantom_cnt = l->phantom_cnt;
		lyle_cp = l->cp;
		lyle_tele_in_cnt = l->tele_in_cnt;
		exec_change(GE_GAME_INGAME);
	}
}

static void ge_gameover(void)
{

}

static void ge_ending(void)
{

}

// Main dispatch ==============================================================

static void (*dispatch_funcs[])(void) =
{
	[GE_INIT] = ge_init,
	[GE_SHUTDOWN] = ge_shutdown,
	[GE_INTRO] = ge_intro,
	[GE_TITLE] = ge_title,
	[GE_CONFIG] = ge_config,
	[GE_GAME_START] = ge_game_start,
	[GE_GAME_INGAME] = ge_game_ingame,
	[GE_GAMEOVER] = ge_gameover,
	[GE_ENDING] = ge_ending,
	[GE_INVALID] = NULL,
};

void game_main(void)
{
	system_set_debug_enabled(0);

	app_alive = 1;
	while (app_alive)
	{
		if (!dispatch_funcs[exec]) return;
		dispatch_funcs[exec]();
		megadrive_finish();

		exec_end_of_frame();
	}
}
