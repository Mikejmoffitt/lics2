#include "obj/keddums.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"

static uint16_t s_vram_pos;

static int16_t float_anim_speed;
static int16_t shake_anim_speed;

static O_Keddums *s_keddums;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_KEDDUMS);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	float_anim_speed = PALSCALE_DURATION(20);
	shake_anim_speed = PALSCALE_DURATION(2);

	s_constants_set = true;
}

static void render(O_Keddums *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -12;
	static const int16_t offset_y = -16;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	switch (e->state)
	{
		default:
			md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
			                                LYLE_PAL_LINE, 0), SPR_SIZE(3, 2));
			break;
		case KEDDUMS_SHAKE:
			md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos+6, 0, 0,
			                                LYLE_PAL_LINE, 0), SPR_SIZE(3, 2));
			break;
	}
}

static void main_func(Obj *o)
{
	O_Keddums *e = (O_Keddums *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	switch (e->state)
	{
		default:
			break;
		case KEDDUMS_FLOAT:
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, float_anim_speed);
			if (e->anim_cnt == 0)
			{
				if (e->anim_frame < 2)
				{
					e->head.y += INTTOFIX32(1);
				}
				else
				{
					e->head.y -= INTTOFIX32(1);
				}
			}
			break;
		case KEDDUMS_SHAKE:
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, shake_anim_speed);
			if (e->anim_cnt == 0)
			{
				if (e->anim_frame == 0)
				{
					e->head.x += INTTOFIX32(1);
				}
				else
				{
					e->head.x -= INTTOFIX32(1);
				}
			}
			break;
	}

	render(e);
}

void o_load_keddums(Obj *o, uint16_t data)
{
	O_Keddums *e = (O_Keddums *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Keddums", 0,
	               INTTOFIX16(-12), INTTOFIX16(12), INTTOFIX16(-16), 1);
	o->main_func = main_func;
	o->cube_func = NULL;

	s_keddums = e;
}

void o_unload_keddums(void)
{
	s_vram_pos = 0;
}

void keddums_set_state(KeddumsState state)
{
	if (!s_keddums) return;
	s_keddums->state = state;
}
