#include "obj/rockman_door.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"
#include "obj/lyle.h"

static int16_t kthresh[3];

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_ROCKMAN_DOOR);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kthresh[0] = PALSCALE_DURATION(12);
	kthresh[1] = PALSCALE_DURATION(24);
	kthresh[2] = PALSCALE_DURATION(36);

	s_constants_set = 1;
}

static inline void draw_segment(O_RockmanDoor *e, int16_t sp_x, int16_t sp_y)
{
	if (e->tile)
	{
		const uint16_t attr = SPR_ATTR(e->tile, 0, 0, MAP_PAL_LINE, 0);
		spr_put(sp_x, sp_y, attr, SPR_SIZE(2, 1));
		spr_put(sp_x, sp_y + 8, attr + 0x10, SPR_SIZE(2, 1));
	}
	else
	{
		spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
		                             LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));
	}
}

static void render(O_RockmanDoor *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -48;
	obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
	                        map_get_x_scroll(), map_get_y_scroll());

	for (uint16_t i = 0; i < ARRAYSIZE(kthresh); i++)
	{
		if (e->state < kthresh[i]) return;

		draw_segment(e, sp_x, sp_y);
		sp_y += 16;
	}
}

static void main_func(Obj *o)
{
	O_RockmanDoor *e = (O_RockmanDoor *)o;
	if (e->closed && e->state < kthresh[2])
	{
		e->state++;
	}
	else if (!e->closed && e->state > 0)
	{
		e->state--;
	}

	render(e);

	O_Lyle *l = lyle_get();

	if (!e->closed) return;
	if (o->direction == OBJ_DIRECTION_RIGHT &&
	    l->head.x + l->head.left < o->x + o->right &&
	    l->head.dx < 0)
	{
		l->head.x = o->x + o->right - l->head.left;
		l->head.dx = 0;
	}
	else if (o->direction == OBJ_DIRECTION_LEFT &&
	         l->head.x + l->head.right > o->x + o->left &&
	         l->head.dx > 0)
	{
		l->head.x = o->x + o->left - l->head.right;
		l->head.dx = 0;
	}
}

static void cube_func(Obj *o, Cube *c)
{
	if (c->dx == 0) return;
	(void)o;
	cube_destroy(c);
}

void o_load_rockman_door(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_RockmanDoor) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-48), 127);
	o->left = INTTOFIX16(-10);
	o->right = INTTOFIX16(10);
	o->main_func = main_func;
	o->cube_func = cube_func;
	o->direction = (data & 0x0001) ? OBJ_DIRECTION_LEFT : OBJ_DIRECTION_RIGHT;

	O_RockmanDoor *e = (O_RockmanDoor *)o;
	e->tile = data >> 8;
}

void o_unload_rockman_door(void)
{
	s_vram_pos = 0;
}

void rockman_door_set_closed(int16_t closed)
{
	ObjSlot *s = &g_objects[0];
	int16_t i = ARRAYSIZE(g_objects);
	while (i--)
	{
		Obj *b = (Obj *)s;
		s++;
		if (b->status == OBJ_STATUS_NULL ||
		    b->type != OBJ_ROCKMAN_DOOR)
		{
			continue;
		}
		O_RockmanDoor *e = (O_RockmanDoor *)b;
		e->closed = closed;
	}
}

void rockman_door_set_single_closed(int16_t left, int16_t right)
{
	ObjSlot *s = &g_objects[0];
	int16_t i = ARRAYSIZE(g_objects);
	while (i--)
	{
		Obj *b = (Obj *)s;
		s++;
		if (b->status == OBJ_STATUS_NULL ||
		    b->type != OBJ_ROCKMAN_DOOR)
		{
			continue;
		}
		O_RockmanDoor *e = (O_RockmanDoor *)b;
		e->closed = (b->direction == OBJ_DIRECTION_RIGHT && left) ||
		            (b->direction == OBJ_DIRECTION_LEFT && right);
	}
}
