#include "obj/template.h"

#include "obj.h"
#include "system.h"
#include "gfx.h"

const uint16_t VRAM_LEN = 4;

static uint16_t vram_loaded;

static void vram_load(void)
{
	if (vram_loaded) return;

	const Gfx *g = gfx_get(GFX_TEMPLATE);
	gfx_load(g, obj_vram_alloc(g->size));

	vram_loaded = 1;
}

void o_load_template(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Template) <= sizeof(*o));
	(void)data;
	vram_load();
}

void o_unload_template(void)
{
	if (!vram_loaded) return;

	vram_loaded = 0;
}
