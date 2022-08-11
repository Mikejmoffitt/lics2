#include "obj/falseblock.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "lyle.h"
#include "obj/exploder.h"
#include "powerup.h"

static void render(O_Falseblock *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -16;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	md_spr_put(sp_x, sp_y, SPR_ATTR(e->base_tile_id, 0, 0,
	                             MAP_PAL_LINE, 0), SPR_SIZE(2, 1));
	md_spr_put(sp_x, sp_y + 8, SPR_ATTR(e->base_tile_id + 0x10, 0, 0,
	                             MAP_PAL_LINE, 0), SPR_SIZE(2, 1));
}

// Returns 1if any hoops exist.
static int16_t check_hoops(void)
{
	ObjSlot *s = &g_objects[0];
	while (s < &g_objects[OBJ_COUNT_MAX])
	{
		Obj *other = (Obj *)s;
		s++;
		if (other->status != OBJ_STATUS_ACTIVE) continue;
		if (other->type != OBJ_HOOP) continue;
		return 1;
	}
	return 0;
}

// TODO: It won't matter for the room this is used in, but it'd be nice to do
// this scan just once and then store the powerup pointer instead.
static void check_hp_orb(Obj *o)
{
	for (uint16_t i = 0; i < ARRAYSIZE(g_powerups); i++)
	{
		Powerup *p = &g_powerups[i];
		if (p->active && p->type == POWERUP_TYPE_HP_ORB)
		{
			if (p->dy > 0 && p->y >= o->y + o->top) powerup_bounce(p);
			return;
		}
	}
}

static void main_func(Obj *o)
{
	O_Falseblock *e = (O_Falseblock *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	O_Lyle *l = lyle_get();

	if (l->head.x + l->head.right >= o->x + o->left &&
	    l->head.x + l->head.right <= o->x + o->right &&
	    l->head.y + l->head.top <= o->y && l->head.dy < 0)
	{
		l->head.dy = 0;
	}

	check_hp_orb(o);

	if (!check_hoops())
	{
		const fix16_t kspawn_rate = INTTOFIX16(PALSCALE_1ST(1.0));
		exploder_spawn(o->x, o->y + (o->top / 2), o->dx, o->dy, PARTICLE_TYPE_FIZZLERED, 6, kspawn_rate);
		obj_erase(o);
		// TODO: Sound (not sure which cue)
		return;
	}

	render(e);
}

static void cube_func(Obj *o, Cube *c)
{
	(void)o;
	if (c->type != CUBE_TYPE_GREEN) return;
	if (c->dy < 0) c->dy = -c->dy;
}

void o_load_falseblock(Obj *o, uint16_t data)
{
	O_Falseblock *e = (O_Falseblock *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");

	e->base_tile_id = data;

	obj_basic_init(o, "FalseBlk", OBJ_FLAG_ALWAYS_ACTIVE | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = cube_func;
}

void o_unload_falseblock(void)
{
}
