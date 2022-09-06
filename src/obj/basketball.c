#include "obj/basketball.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "obj/hoop.h"
#include "lyle.h"
#include "obj/exploder.h"

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_BASKETBALL);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static fix16_t kgravity;
static fix16_t kbounce_dy_down;
static fix16_t kbounce_dy_offset;
static fix16_t kbounce_dy_offset_cube;
static fix16_t kbounce_dy_high;
static fix16_t kbounce_dx;

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kgravity = INTTOFIX16(PALSCALE_1ST(0.16666666667));
	kbounce_dx = INTTOFIX16(PALSCALE_1ST(0.83333333333));
	kbounce_dy_down = INTTOFIX16(PALSCALE_1ST(0.83333333333));
	kbounce_dy_offset = INTTOFIX16(PALSCALE_1ST(2.0));
	kbounce_dy_offset_cube = INTTOFIX16(PALSCALE_1ST(1.66666666667));
	kbounce_dy_high = INTTOFIX16(PALSCALE_1ST(5.0));

	s_constants_set = 1;
}

static void handle_hoop(O_Basketball *e, O_Hoop *h)
{
	// Hoops that have been swished do not have collision.
	if (h->destroy_cnt > 0) return;
	if (e->head.dy < 0) return;
	if (e->head.y > h->head.y + h->head.top + INTTOFIX32(6)) return;

	static const int32_t center_margin = INTTOFIX32(4);
	if ((e->head.dy > 0) &&
	    (e->head.x >= h->head.x - center_margin) &&
	    (e->head.x <= h->head.x + center_margin))
	{
		e->head.x = h->head.x;
		e->head.dy = 0;
		e->head.dx = 0;
		e->head.y = h->head.y + h->head.top + INTTOFIX32(2);
		hoop_mark_for_destruction(h);
	}
	else if(e->head.x >= h->head.x)
	{
		e->head.dy = -kbounce_dy_offset_cube;
		e->head.dx = kbounce_dx;
	}
	else if (e->head.x < h->head.x)
	{
		e->head.dy = -kbounce_dy_offset_cube;
		e->head.dx = -kbounce_dx;
	}
}

static void bounce_against_hoops(O_Basketball *e)
{
	ObjSlot *s = &g_objects[0];
	while (s < &g_objects[OBJ_COUNT_MAX])
	{
		Obj *other = (Obj *)s;
		s++;
		if (other->status != OBJ_STATUS_ACTIVE) continue;
		if (other->type == OBJ_HOOP)
		{
			// If in range of a stop, stop upon faulting vertical position.
			if (obj_touching_obj(&e->head, other))
			{
				handle_hoop(e, (O_Hoop *)other);
			}
		}
		else if (other->type == OBJ_FALSEBLOCK)
		{
			if (obj_touching_obj(&e->head, other))
			{
				e->head.dy = kbounce_dy_down;
			}
		}

	// TODO: Check against false block and set dy to kbounce_dy_offset_cube.
	}
}

static void render(O_Basketball *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -16;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
	                             MAP_PAL_LINE, 0), SPR_SIZE(2, 2));
}

static inline void cube_bounce_impact(O_Basketball *e, fix32_t other_x, int16_t low_bounce)
{
	Obj *o = &e->head;
	o->dy = low_bounce ?
	        -(o->dy / 2) - kbounce_dy_offset_cube : 
	        -kbounce_dy_high;
	// TODO: Play bounce sound
	
	if (o->x > other_x + INTTOFIX32(4)) o->dx = kbounce_dx;
	else if (o->x < other_x - INTTOFIX32(4)) o->dx = -kbounce_dx;
	else o->dx = 0;
}

static inline void bounce_against_lyle_cube(O_Basketball *e)
{
	Obj *o = (Obj *)e;

	const O_Lyle *l = lyle_get();
	if (!l->holding_cube) return;

	// Ball half-width plus cube half-width.
	const fix32_t adj_x = INTTOFIX32(16);
	if (o->x + adj_x < l->head.x) return;
	if (o->x - adj_x > l->head.x) return;
	if (o->y < l->head.y - INTTOFIX32(35)) return;
	if (o->y + o->top > l->head.y) return;

	cube_bounce_impact(e, l->head.x, l->grounded);
}

static inline void bounce_against_map(O_Basketball *e)
{
	Obj *o = (Obj *)e;

	const int16_t left = FIX32TOINT(o->x + o->left);
	const int16_t right = FIX32TOINT(o->x + o->right);
	const int16_t x_center = FIX32TOINT(o->x);
	const int16_t top = FIX32TOINT(o->y + o->top);
	const int16_t bottom = FIX32TOINT(o->y);
	const int16_t y_center = FIX32TOINT(o->y + (o->top / 2));

	if (o->dy < 0 && map_collision(x_center, top - 1))
	{
		o->dy = kbounce_dy_down;
	}
	else if (o->dy > 0 && map_collision(x_center, bottom + 1))
	{
		// TODO: Play bounce sound
		o->dy = -(o->dy / 2) - kbounce_dy_offset;
		while (map_collision(x_center, FIX32TOINT(o->y) + 1))
		{
			o->y -= INTTOFIX32(1);
		}
	}

	// Horizontal bounce against wall.
	if (o->dx > 0 && (
	    map_collision(right + 1, top) ||
	    map_collision(right + 1, y_center)))
	{
		o->dx = -kbounce_dx;
//		o->x -= INTTOFIX32(4);
	}
	else if (o->dx < 0 && (
	    map_collision(left + 1, top) ||
	    map_collision(left + 1, y_center)))
	{
		o->dx = kbounce_dx;
//		o->x += INTTOFIX32(4);
	}
}

static void cube_func(Obj *o, Cube *c)
{
	cube_bounce_impact((O_Basketball *)o, c->x, 1);
}

static void main_func(Obj *o)
{
	O_Basketball *e = (O_Basketball *)o;

	bounce_against_map(e);
	bounce_against_hoops(e);
	bounce_against_lyle_cube(e);
	// Cubes in the air are checked in cube_func.
	if (o->dx < 0 && o->x + o->left <= INTTOFIX32(4))
	{
		o->dx = kbounce_dx;
	}

	o->dy += kgravity;
	obj_mixed_physics_h(o);

	render(e);
}

void o_load_basketball(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(*o) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "BsktBall", OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = cube_func;
}

void o_unload_basketball(void)
{
	s_vram_pos = 0;
}
