#include "obj/laser.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"


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
	static bool s_constants_set;
	if (s_constants_set) return;

	kanim_delay = PALSCALE_DURATION(2);
	ksequence[0] = PALSCALE_DURATION(36);
	ksequence[1] = PALSCALE_DURATION(54);
	ksequence[2] = PALSCALE_DURATION(114);
	ksequence[3] = PALSCALE_DURATION(132);
	s_constants_set = true;
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

	obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
	                        map_get_x_scroll(), map_get_y_scroll());
	for (int16_t i = 0 ; i < e->height; i++)
	{
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + offset, e->anim_frame % 2, 0,
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

	if (e->mode == LASER_MODE_ON && (e->phase == 0 || e->phase == 3))
	{
		e->phase = 1;
		e->timer = ksequence[0];
	}

	else if (e->mode == LASER_MODE_OFF && (e->phase == 1 || e->phase == 2))
	{
		e->phase = 3;
		e->timer = ksequence[2];
	}

	e->timer++;
	if (e->timer >= ksequence[3])
	{
		e->timer = 0;
	}

	if (e->timer < ksequence[0])
	{
		// Laser is fully off.
		e->phase = 0;
		if (e->mode == LASER_MODE_OFF)
		{
			e->timer = 0;
		}
	}
	else if (e->timer < ksequence[1])
	{
		// Laser is flickering on.
		e->phase = 1;
	}
	else if (e->timer < ksequence[2])
	{
		// Laser is on.
		e->phase = 2;
		if (e->mode == LASER_MODE_ON)
		{
			e->timer = ksequence[1];
		}
	}
	else
	{
		// Laser is flickering off.
		e->phase = 3;
	}

	if (e->phase == 2)
	{
		if (e->mode == LASER_MODE_ON)
		{
			o->flags |= OBJ_FLAG_HARMFUL | OBJ_FLAG_BOUNCE_ANY | OBJ_FLAG_ALWAYS_HARMFUL;
		}
		else
		{
			o->flags |= OBJ_FLAG_HARMFUL;
		}
	}
	else
	{
		o->flags &= ~(OBJ_FLAG_HARMFUL | OBJ_FLAG_BOUNCE_ANY | OBJ_FLAG_ALWAYS_HARMFUL);
	}


	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kanim_delay);

	if (e->phase != 0) render(e);
}

void o_load_laser(Obj *o, uint16_t data)
{
	O_Laser *e = (O_Laser *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Laser", 0,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 1);
	const int16_t x_center = FIX32TOINT(o->x);

	// Search to see how tall this laser is.
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

	e->mode = data;
	SYSTEM_ASSERT(e->mode < 3);
}

void o_unload_laser(void)
{
	s_vram_pos = 0;
}

void laser_set_mode(LaserMode mode)
{
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status == OBJ_STATUS_NULL) continue;
		if (o->type != OBJ_LASER) continue;
		O_Laser *e = (O_Laser *)o;
		e->mode = mode;
	}
}
