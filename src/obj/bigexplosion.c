#include "obj/bigexplosion.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"

static int16_t kanim_speed;

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_BIGEXPLOSION);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kanim_speed = PALSCALE_DURATION(4);

	s_constants_set = true;
}

static void render(O_BigExplosion *e)
{
	Obj *o = &e->head;

	SprParam spr;

	int16_t offset;

	switch (e->anim_frame)
	{
		case 0:
		case 2:
			offset = -24;
			spr.attr = SPR_ATTR(s_vram_pos + 4, 0, 0, LYLE_PAL_LINE, 0);
			spr.size = SPR_SIZE(3, 3);
			break;
		case 1:
		case 4:
			offset = -16;
			spr.attr = SPR_ATTR(s_vram_pos, 0, 0, LYLE_PAL_LINE, 0);
			spr.size = SPR_SIZE(2, 2);
			break;
		case 3:
		case 5:
		case 6:
			offset = -32;
			spr.attr = SPR_ATTR(s_vram_pos + 13, 0, 0, LYLE_PAL_LINE, 0);
			spr.size = SPR_SIZE(4, 4);
			break;
		default:
			return;
			break;
	}

	obj_render_setup(o, &spr.x, &spr.y, offset, offset,
	                 map_get_x_scroll(), map_get_y_scroll());

	md_spr_put_st(&spr);
	spr.x += 24;
	spr.attr ^= SPR_ATTR(0, 1, 0, 0, 0);
	md_spr_put_st(&spr);
	spr.y += 24;
	spr.attr ^= SPR_ATTR(0, 0, 1, 0, 0);
	md_spr_put_st(&spr);
	spr.x -= 24;
	spr.attr ^= SPR_ATTR(0, 1, 1, 0, 0);
	md_spr_put_st(&spr);
}

static void main_func(Obj *o)
{
	O_BigExplosion *e = (O_BigExplosion *)o;
	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 7, kanim_speed);
	if (e->anim_frame == 6)
	{
		obj_erase(o);
	}
	else
	{
		obj_accurate_physics(o);
		render(e);
	}
}

void o_load_bigexplosion(Obj *o, uint16_t data)
{
	O_BigExplosion *e = (O_BigExplosion *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "BigExplosion", 0,
	               INTTOFIX16(-1), INTTOFIX16(1), INTTOFIX16(-1), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_bigexplosion(void)
{
	s_vram_pos = 0;
}
