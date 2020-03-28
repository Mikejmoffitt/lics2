#include "obj/cube_manager.h"

#include "obj.h"
#include "system.h"
#include "gfx.h"

static uint16_t vram_loaded;

static void vram_load(void)
{
	if (vram_loaded) return;

	const Gfx *g = gfx_get(GFX_CUBES);
	gfx_load(g, obj_vram_alloc(g->size));

	vram_loaded = 1;
}

void o_load_cube_manager(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_CubeManager) <= sizeof(*o));
	(void)data;
	vram_load();
}

void o_unload_cube_manager(void)
{
	if (!vram_loaded) return;

	vram_loaded = 0;
}

