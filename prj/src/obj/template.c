#include "obj/template.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"

#include "cube.h"

static uint16_t vram_pos;

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g = gfx_get(GFX_TEMPLATE);
	vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static void main_func(Obj *o)
{
	(void)o;
}

void o_load_template(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Template) <= sizeof(ObjSlot));
	(void)data;
	vram_load();

	obj_basic_init(o, OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-1), INTTOFIX16(1), INTTOFIX16(-2), 1);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_template(void)
{
	vram_pos = 0;
}
