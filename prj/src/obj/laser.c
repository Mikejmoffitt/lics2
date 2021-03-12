#include "obj/laser.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"

static uint16_t s_vram_pos;
static int16_t ksequence[4];
static int16_t kanim_delay;

// The laser animation sequence goes like this:
// 0 - start flickering
// 1 - draw solid, set harmful flag
// 2 - flicker out, clear harmful
// 3 - invisible (and reset)

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_LASER);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kanim_delay = PALSCALE_DURATION(2);
	ksequence[0] = PALSCALE_DURATION(36);
	ksequence[1] = PALSCALE_DURATION(54);
	ksequence[2] = PALSCALE_DURATION(114);
	ksequence[3] = PALSCALE_DURATION(132);
	s_constants_set = 1;
}

static void render(O_Laser *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -16;

	int16_t offset = 0;
	if (e->phase == 1 || e->phase == 3)
	{
		offset = 4 + (4 * (e->anim_frame / 2));
	}

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	for (int16_t i = 0 ; i < e->height; i++)
	{
		spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + offset, e->anim_frame % 2, 0,
		                             LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));
		sp_y -= 16;
	}
}

static void main_func(Obj *o)
{
	O_Laser *e = (O_Laser *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	e->timer++;
	if (e->timer >= ksequence[3])
	{
		e->timer = 0;
	}

	if (e->timer < ksequence[0])
	{
		e->phase = 0;
	}
	else if (e->timer < ksequence[1])
	{
		e->phase = 1;
	}
	else if (e->timer < ksequence[2])
	{
		e->phase = 2;
	}
	else
	{
		e->phase = 3;
	}

	switch (e->phase)
	{
		default:
		case 0:
			o->flags &= ~(OBJ_FLAG_HARMFUL);
			return;
		case 1:
		case 3:
			o->flags &= ~(OBJ_FLAG_HARMFUL);
			break;
		case 2:
			o->flags |= OBJ_FLAG_HARMFUL;
			break;
	}

	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kanim_delay);

	render(e);
}

void o_load_laser(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Laser) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, 0,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 1);
	const int16_t x_center = FIX32TOINT(o->x);

	// Search to see how tall this laser is.
	O_Laser *e = (O_Laser *)o;
	for (int16_t i = 0; i < 16; i++)
	{
		e->height++;
		const int16_t bottom = FIX32TOINT(o->y) + 1;
		if (map_collision(x_center, bottom))
		{
			break;
		}
		o->y += INTTOFIX32(16);
		o->top -= INTTOFIX16(16);
	}
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_laser(void)
{
	s_vram_pos = 0;
}
