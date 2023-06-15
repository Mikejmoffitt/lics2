#include "obj/shoot.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"
#include "lyle.h"


static uint16_t s_vram_pos;

static fix16_t kdx;
static fix16_t kddy;
static fix16_t kdy_cutoff;
static fix16_t kdy_cutoff_big;
static int16_t kanim_delay;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_SHOOT);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;

	kdx = INTTOFIX16(PALSCALE_1ST(0.833333334));
	kddy = INTTOFIX16(PALSCALE_2ND(0.2));
	kdy_cutoff = INTTOFIX16(PALSCALE_1ST(2.4));  // TODO: Verify
	kdy_cutoff_big = INTTOFIX16(PALSCALE_1ST(5.2));  // TODO: Verify
	kanim_delay = PALSCALE_DURATION(10);

	s_constants_set = true;
}

static inline void render(O_Shoot *f)
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
	O_Shoot *f = (O_Shoot *)o;

	if (o->hurt_stun > 0)
	{
		render(f);
		return;
	}

	// Horizontal movement.
	if (o->x >= f->x_max) o->dx = -kdx;
	else if (o->x <= f->x_min) o->dx = kdx;

	o->direction = (o->dx >= 0) ? OBJ_DIRECTION_RIGHT : OBJ_DIRECTION_LEFT;

	// Vertical sinusoidal movement.
	const fix16_t dy_cutoff = f->swoop_en ? kdy_cutoff_big : kdy_cutoff;
	if (f->moving_up)
	{
		o->dy += kddy;
		if (o->dy > dy_cutoff) f->moving_up = 0;
	}
	else
	{
		o->dy -= kddy;
		if (o->dy < -dy_cutoff)
		{
			f->moving_up = 1;
			// Do a swoop if the player is near.
			const O_Lyle *l = lyle_get();
			static const fix32_t prox = INTTOFIX32(100);
			f->swoop_en = (l->head.x < o->x + prox && l->head.x > o->x - prox);
		}
	}

	obj_mixed_physics_h(o);

	// Animation.
	OBJ_SIMPLE_ANIM(f->anim_cnt, f->anim_frame, 2, kanim_delay);
	
	render(f);
}

void o_load_shoot(Obj *o, uint16_t data)
{
	O_Shoot *f = (O_Shoot *)o;
	_Static_assert(sizeof(*f) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Shoot", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-11), INTTOFIX16(11), INTTOFIX16(-12), 2);
	o->main_func = main_func;
	o->cube_func = NULL;

	o->y += INTTOFIX32(4);
	o->direction = OBJ_DIRECTION_LEFT;
	o->dx = -kdx;

	f->x_min = o->x - INTTOFIX32(200);
	f->x_max = o->x;
	f->moving_up = 1;
}

void o_unload_shoot(void)
{
	s_vram_pos = 0;
}
