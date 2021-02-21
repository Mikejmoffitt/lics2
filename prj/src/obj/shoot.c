#include "obj/shoot.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"

static uint16_t vram_pos;

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g = gfx_get(GFX_SHOOT);
	vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t constants_set;
	if (constants_set) return;
	// Set constants here.

	constants_set = 1;
}

static void main_func(Obj *o)
{
	(void)o;
}

void o_load_shoot(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Shoot) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-1), INTTOFIX16(1), INTTOFIX16(-2), 1);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_shoot(void)
{
	vram_pos = 0;
}
