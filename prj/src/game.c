#include "game.h"
#include "system.h"
#include "obj.h"
#include "common.h"
#include "gfx.h"
#include "md/megadrive.h"

#include "obj/map.h"
#include "obj/lyle.h"

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
	void (*shutdown_func)(void);
	int status;
} InitFunc;

static InitFunc init_funcs[] =
{
	{"system", system_init, NULL, 0},
	{"gfx", gfx_init, NULL, 0},
	{"obj", obj_init, NULL, 0},
};

static void ge_init(void)
{
	for (unsigned int i = 0; i < ARRAYSIZE(init_funcs); i++)
	{
		InitFunc *f = &init_funcs[i];
		if (!f->init_func) continue;
		f->status = f->init_func();
		if (f->status <= 0)
		{
			// TODO: Do something about a failed init, for non-MD platforms...
			exec_change(GE_SHUTDOWN);
			return;
		}
	}

	exec_change(FIRST_EXEC);
}

static void ge_shutdown(void)
{
	for (unsigned int i = 0; i < ARRAYSIZE(init_funcs); i++)
	{
		InitFunc *f = &init_funcs[i];
		if (f->shutdown_func &&
		    f->status > 0)
		{
			f->shutdown_func();
		}
		f->status = 0;
	}
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

static void ge_game_ingame(void)
{
	static uint8_t next_room_id = 1;
	static uint8_t next_room_entrance = 0;
	static fix16_t lyle_entry_dx = 0;
	static fix16_t lyle_entry_dy = 0;
	static int16_t lyle_cp = 0;
	static int16_t lyle_phantom_cnt = 0;

	if (g_elapsed == 0)
	{
		vdp_set_display_en(0);
		obj_clear();

		// The order of objects is important.
		obj_spawn(64, 73, OBJ_LYLE, 0);
		obj_spawn(0, 0, OBJ_CUBE_MANAGER, 0);
		obj_spawn(0, 0, OBJ_BG, 0);
		obj_spawn(0, 0, OBJ_MAP, 0);

		map_load(next_room_id, next_room_entrance);

		O_Lyle *l = lyle_get();
		l->head.dx = lyle_entry_dx;
		l->head.dy = lyle_entry_dy;
		l->phantom_cnt = lyle_phantom_cnt;
		l->cp = lyle_cp;

		dma_q_set_budget(100000);
		dma_q_complete();
		dma_q_set_budget(DMA_Q_BUDGET_AUTO);

		vdp_set_display_en(1);

		return;
	}

	if (g_elapsed < 30) return;

	pal_set(0, PALRGB(6, 1, 2));
	obj_exec();
	pal_set(0, PALRGB(3, 4, 5));

	if (lyle_has_exited())
	{
		next_room_id = map_get_next_room_id();
		next_room_entrance = map_get_next_room_entrance();
		const O_Lyle *l = lyle_get();
		lyle_entry_dx = l->head.dx;
		lyle_entry_dy = l->head.dy;
		lyle_phantom_cnt = l->phantom_cnt;
		lyle_cp = l->cp;
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
	app_alive = 1;
	while (app_alive)
	{
		if (!dispatch_funcs[exec]) return;
		dispatch_funcs[exec]();
		megadrive_finish();

		exec_end_of_frame();
	}
}
