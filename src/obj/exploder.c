#include "obj/exploder.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"

void exploder_spawn(fix32_t x, fix32_t y, fix16_t dx, fix16_t dy,
                    ParticleType type, uint16_t count, fix16_t rate)
{
	Obj *o = obj_spawn(x, y, OBJ_EXPLODER, count);
	if (!o) return;
	o->x = x;
	o->y = y;
	o->dx = dx;
	o->dy = dy;
	O_Exploder *f = (O_Exploder *)o;
	f->rate = rate;
	f->type = type;
}

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	s_constants_set = 1;
}

static void main_func(Obj *o)
{
	O_Exploder *f = (O_Exploder *)o;
	if (f->count <= 0)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}

	f->accumulator += f->rate;
	while (f->accumulator > INTTOFIX16(1.0))
	{
		particle_manager_spawn(o->x, o->y, f->type);
		f->accumulator -= INTTOFIX16(1.0);
		f->count--;
	}

	obj_standard_physics(o);

	(void)o;
}

void o_load_exploder(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Exploder) <= sizeof(ObjSlot));

	obj_basic_init(o, 0, 0, 0, 0, 127);
	o->main_func = main_func;
	o->cube_func = NULL;

	O_Exploder *f = (O_Exploder *)o;
	f->count = data;
}
