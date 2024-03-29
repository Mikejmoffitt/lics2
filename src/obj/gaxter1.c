#include "obj/gaxter1.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "map.h"
#include "lyle.h"

#include "cube.h"
#include "palscale.h"

#define GAXTER1_DEADZONE INTTOFIX32(20)

// Constants.

static bool s_constants_set;

static int8_t kanim_len;
static fix16_t kaccel;
static fix16_t ktop_speed;

static void set_constants(void)
{
	if (s_constants_set) return;

	kanim_len = PALSCALE_DURATION(2);
	kaccel = INTTOFIX16(PALSCALE_2ND(0.09920634920634));
	ktop_speed = INTTOFIX16(PALSCALE_1ST(2.38));

	s_constants_set = true;
}

// VRAM.

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_GAXTER);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Main.

static inline void render(O_Gaxter1 *f)
{
	Obj *o = &f->head;
	int16_t sp_x, sp_y;
	obj_render_setup(o, &sp_x, &sp_y, -8, -8,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (f->anim_frame * 4),
	                    o->direction == OBJ_DIRECTION_LEFT, 0,
	                    ENEMY_PAL_LINE, 1), SPR_SIZE(2, 2));
}

static void main_func(Obj *o)
{
	O_Gaxter1 *f = (O_Gaxter1 *)o;
	const O_Lyle *l = lyle_get();

	if (o->hurt_stun > 0)
	{
		render(f);
		return;
	}

	// Lazily accelerate towards the player.
	if (o->x < l->head.x - GAXTER1_DEADZONE && o->dx < ktop_speed)
	{
		o->dx += kaccel;
		o->direction = OBJ_DIRECTION_RIGHT;
	}
	else if (o->x > l->head.x + GAXTER1_DEADZONE && o->dx > -ktop_speed)
	{
		o->dx -= kaccel;
		o->direction = OBJ_DIRECTION_LEFT;
	}
	if (o->y < l->head.y - GAXTER1_DEADZONE && o->dy < ktop_speed)
	{
		o->dy += kaccel;
	}
	else if (o->y > l->head.y + GAXTER1_DEADZONE && o->dy > -ktop_speed)
	{
		o->dy -= kaccel;
	}

	if (o->dx > ktop_speed) o->dx = ktop_speed;
	else if (o->dx < -ktop_speed) o->dx = -ktop_speed;
	if (o->dy > ktop_speed) o->dy = ktop_speed;
	else if (o->dy < -ktop_speed) o->dy = -ktop_speed;

	obj_standard_physics(o);

	// Animate.
	if (f->anim_cnt == kanim_len)
	{
		f->anim_cnt = 0;
		if (f->anim_frame == 2)
		{
			f->anim_frame = 0;
		}
		else
		{
			f->anim_frame++;
		}
	}
	else
	{
		f->anim_cnt++;
	}

	render(f);
}

// Boilerplate.

void o_load_gaxter1(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_Gaxter1) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	vram_load();
	set_constants();

	obj_basic_init(o, "Gaxter 1", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-6), INTTOFIX16(6), INTTOFIX16(-10), 2);
	o->main_func = main_func;
	o->direction = OBJ_DIRECTION_LEFT;
}

void o_unload_gaxter1(void)
{
	s_vram_pos = 0;
}
