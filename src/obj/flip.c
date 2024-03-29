#include "obj/flip.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "map.h"

#include "cube.h"
#include "palscale.h"

// Constants.

static fix16_t kddy;
static fix16_t kdy_cutoff;
static fix16_t kdx;
static int8_t kanim_delay;

static bool s_constants_set;

static void set_constants(void)
{
	if (s_constants_set) return;

	kddy = INTTOFIX16(PALSCALE_2ND(0.2));
	kdy_cutoff = INTTOFIX16(PALSCALE_1ST(2.8));
	kdx = INTTOFIX16(PALSCALE_1ST(0.41666666667));
	kanim_delay = PALSCALE_DURATION(10);

	s_constants_set = true;
}

// VRAM.

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_FLIP);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Main.

static inline void render(O_Flip *f)
{
	Obj *o = &f->head;
	int16_t sp_x, sp_y;
	obj_render_setup(o, &sp_x, &sp_y, -11, -12,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (f->anim_frame ? 6 : 0),
	                    o->direction == OBJ_DIRECTION_LEFT, 0,
	                    ENEMY_PAL_LINE, 0), SPR_SIZE(3, 2));
}

static void main_func(Obj *o)
{
	(void)o;
	O_Flip *f = (O_Flip *)o;

	if (o->hurt_stun > 0)
	{
		render(f);
		return;
	}

	// Horizontal movement.
	if (o->x >= f->x_max) o->dx = -kdx;
	else if (o->x <= f->x_min) o->dx = kdx;

	// Collision and direction.
	if (o->dx > 0 &&
	    map_collision(FIX32TOINT(o->x + o->right), FIX32TOINT(o->y)))
	{
		o->dx = -kdx;
		f->x_min = o->x - INTTOFIX32(100);
		f->x_max = o->x;
	}
	else if (o->dx < 0 &&
	         map_collision(FIX32TOINT(o->x + o->left), FIX32TOINT(o->y)))
	{
		o->dx = kdx;
		f->x_min = o->x;
		f->x_max = o->x + INTTOFIX32(100);
	}

	o->direction = (o->dx >= 0) ? OBJ_DIRECTION_RIGHT : OBJ_DIRECTION_LEFT;

	// Vertical sinusoidal movement.
	if (f->moving_up)
	{
		o->dy += kddy;
		if (o->dy > kdy_cutoff) f->moving_up = 0;
	}
	else
	{
		o->dy -= kddy;
		if (o->dy < -kdy_cutoff) f->moving_up = 1;
	}

	obj_mixed_physics_h(o);

	// Animation.
	OBJ_SIMPLE_ANIM(f->anim_cnt, f->anim_frame, 2, kanim_delay);
	
	render(f);
}

// Boilerplate.

void o_load_flip(Obj *o, uint16_t data)
{
	O_Flip *f = (O_Flip *)o;
	_Static_assert(sizeof(*f) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	vram_load();
	set_constants();

	obj_basic_init(o, "Flip", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-11), INTTOFIX16(11), INTTOFIX16(-12), 2);
	o->main_func = main_func;
	o->cube_func = NULL;
	o->y += INTTOFIX32(4);
	o->direction = OBJ_DIRECTION_LEFT;
	o->dx = -kdx;

	f->x_min = o->x - INTTOFIX32(100);
	f->x_max = o->x;
	f->moving_up = 1;

}

void o_unload_flip(void)
{
	s_vram_pos = 0;
}
