#include "obj/cube_manager.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"

#include "palscale.h"
#include "util/fixed.h"
#include "cube.h"

uint16_t g_cube_vram_pos;
static uint16_t s_vram_pos;

static uint8_t phantom_anim_counter;
uint8_t g_cube_phantom_anim_frame;
static uint8_t kphantom_anim_counter_max;

Cube g_cubes[CUBE_COUNT_MAX];

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_CUBES);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
	g_cube_vram_pos = s_vram_pos;
}


static void main_func(Obj *o)
{
	(void)o;
	uint16_t i = ARRAYSIZE(g_cubes);
	while (i--)
	{
		Cube *c = &g_cubes[i];
		if (c->status == CUBE_STATUS_NULL) continue;
		cube_run(c);
	}

	phantom_anim_counter++;
	if (phantom_anim_counter >= kphantom_anim_counter_max)
	{
		phantom_anim_counter = 0;
		g_cube_phantom_anim_frame++;
		if (g_cube_phantom_anim_frame >= 4) g_cube_phantom_anim_frame = 0;
	}
}

void o_load_cube_manager(Obj *o, uint16_t data)
{
	(void)data;
	SYSTEM_ASSERT(sizeof(O_CubeManager) <= sizeof(ObjSlot));

	// If VRAM is already loaded, then a cube manager already is present.
	if (s_vram_pos)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}

	phantom_anim_counter = 0;
	g_cube_phantom_anim_frame = 0;

	cube_set_constants();

	vram_load();

	uint16_t i = ARRAYSIZE(g_cubes);
	while (i--)
	{
		Cube *c = &g_cubes[i];
		c->status = CUBE_STATUS_NULL;
	}

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE, 0, 0, 0, 127);
	o->main_func = main_func;
	
	kphantom_anim_counter_max = PALSCALE_DURATION(6);

}

void o_unload_cube_manager(void)
{
	s_vram_pos = 0;
}
