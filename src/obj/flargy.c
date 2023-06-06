#include "obj/flargy.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"


static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_FLARGY);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static int16_t kanim_speed;
static int16_t kfist_wobble_anim_speed;
static int16_t kpunch_extend_cutoff;
static int16_t kpunch_frames;
static fix16_t kpunch_cube_dy;
static fix16_t kpunch_cube_dx;
static fix16_t kwalk_dx;

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kanim_speed = PALSCALE_DURATION(10);
	kfist_wobble_anim_speed = PALSCALE_DURATION(2);
	kpunch_frames = PALSCALE_DURATION(24);
	kpunch_extend_cutoff = PALSCALE_DURATION(22);
	kpunch_cube_dy = INTTOFIX16(PALSCALE_1ST(-1.0));
	kpunch_cube_dx = INTTOFIX16(PALSCALE_1ST(4.0));
	kwalk_dx = INTTOFIX16(PALSCALE_1ST(0.4167));

	s_constants_set = true;
}

static void render(O_Flargy *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -32;

	const int16_t xflip = e->head.direction == OBJ_DIRECTION_LEFT;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	if (e->punch_cnt == 0)
	{
		static const int16_t frame_table[] = {0, 8, 16, 8};
		const int16_t frame_index = frame_table[e->anim_frame];

		// Flargy with his walking animation.
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + frame_index, xflip, 0,
		                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
	}
	else
	{
		// Wobbling of outstretched fist sprite.
		int16_t fist_x_offset = 10;
		if (e->punch_cnt >= kpunch_extend_cutoff)
		{
			fist_x_offset += 2;
		}
		else if (e->punch_cnt >= (kpunch_frames / 2))
		{
			fist_x_offset += (e->anim_frame == 0) ? 0 : 1;
		}
		if (xflip) fist_x_offset = -fist_x_offset;

		// Draw Flargy with arm outstretched.
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + 24, xflip, 0,
		                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));

		// Get the big fist in there.
		md_spr_put(sp_x + fist_x_offset, sp_y + 12, SPR_ATTR(s_vram_pos + 32, xflip, 0,
		                                                  ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
	}
}

static void main_func(Obj *o)
{
	O_Flargy *e = (O_Flargy *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	// Walking left/right and changing directions.
	if (o->x > e->max_x)
	{
		o->direction = OBJ_DIRECTION_LEFT;
	}
	else if (o->x < e->min_x)
	{
		o->direction = OBJ_DIRECTION_RIGHT;
	}

	if (e->punch_cnt > 0)
	{
		e->punch_cnt--;
		o->dx = 0;
		OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kfist_wobble_anim_speed);
	}
	else
	{
		OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kanim_speed);
		o->dx = (o->direction == OBJ_DIRECTION_RIGHT) ? kwalk_dx : -kwalk_dx;
		obj_accurate_physics(o);
	}

	render(e);
}

static void cube_func(Obj *o, Cube *c)
{
	O_Flargy *e = (O_Flargy *)o;
	if (e->punch_cnt > 0) return;
	if (c->status != CUBE_STATUS_AIR && c->status != CUBE_STATUS_KICKED) return;

	// Check if the Cube hitting Flargy is
	// * moving in the opposite direction of Flargy's walking
	// * is on front of Flargy
	const ObjDirection cube_direction = (c->dx >= 0) ? OBJ_DIRECTION_RIGHT : OBJ_DIRECTION_LEFT;
	const int16_t h_fault = (o->direction == OBJ_DIRECTION_RIGHT && c->x >= o->x) ||
	                        (o->direction == OBJ_DIRECTION_LEFT && c->x <= o->x);
	if (h_fault && cube_direction != o->direction)
	{
		// Send the cube flying away from Flargy.
		c->dx = (o->direction == OBJ_DIRECTION_RIGHT) ? kpunch_cube_dx : -kpunch_cube_dx;
		c->dy = kpunch_cube_dy;
		c->status = CUBE_STATUS_AIR;
		e->punch_cnt = kpunch_frames;
	}
	else
	{
		// Otherwise, the cube registers a normal hit.
		obj_standard_cube_response(o, c);
	}
}

void o_load_flargy(Obj *o, uint16_t data)
{
	O_Flargy *e = (O_Flargy *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Flargy", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-7), INTTOFIX16(7), INTTOFIX16(-32), 3);
	o->top = INTTOFIX16(-28);
	o->main_func = main_func;
	o->cube_func = cube_func;

	e->min_x = o->x - INTTOFIX32(19);
	e->max_x = o->x;
}

void o_unload_flargy(void)
{
	s_vram_pos = 0;
}
