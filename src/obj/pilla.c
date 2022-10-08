#include "obj/pilla.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"


static uint16_t s_vram_pos;

static fix16_t kdx;
static fix16_t kgravity;
static int16_t kanim_speed;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_PILLA);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kdx = INTTOFIX16(PALSCALE_1ST(0.833333333334));
	kgravity = INTTOFIX16(PALSCALE_2ND(0.1666666667));
	kanim_speed = PALSCALE_DURATION(6.2);

	s_constants_set = true;
}

static inline void render(O_Pilla *f)
{
	Obj *o = &f->head;
	if (o->offscreen) return;
	int16_t sp_x, sp_y;

	uint16_t offset = 0;
	if (f->anim_frame == 0) offset = 4;
	else if (f->anim_frame == 2) offset = 8;
	if (!f->is_head) offset += 12;

	obj_render_setup(o, &sp_x, &sp_y, -8, -15,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + offset,
	                    o->direction == OBJ_DIRECTION_LEFT, 0,
	                    ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
}

static void main_func(Obj *o)
{
	O_Pilla *f = (O_Pilla *)o;

	if (o->hurt_stun > 0)
	{
		render(f);
		return;
	}
	// When Pilla is first activated, it goes through the object list and
	// marks other Pillas as always active. This is to prevent the non-head
	// Pilla parts from separating from the head. There are no rooms with
	// multiple separated Pilla sets, so this should not cause any problems.
	if (f->is_head && !(o->flags & OBJ_FLAG_ALWAYS_ACTIVE))
	{
		uint16_t i = ARRAYSIZE(g_objects);
		while (i--)
		{
			Obj *other = &g_objects[i].obj;
			if (other->type != o->type) continue;
			other->flags |= OBJ_FLAG_ALWAYS_ACTIVE;
		}
	}

	// Left/right movement, and collision.
	const int16_t left = FIX32TOINT(o->x + o->left);
	const int16_t right = FIX32TOINT(o->x + o->right);
	const int16_t bottom = FIX32TOINT(o->y);

	if (o->dx > 0 && map_collision(right + 1, bottom - 7))
	{
		o->dx = -kdx;
		o->x = INTTOFIX32(FIX32TOINT(o->x));
		o->direction = OBJ_DIRECTION_LEFT;
	}
	else if (o->dx < 0 && map_collision(left - 1, bottom - 7))
	{
		o->dx = kdx;
		o->x = INTTOFIX32(FIX32TOINT(o->x));
		o->direction = OBJ_DIRECTION_RIGHT;
	}

	obj_mixed_physics_h(o);

	// Vertical movement.
	if (!map_collision(left + 2, bottom + 1) && !map_collision(right - 2, bottom + 1))
	{
		o->dy += kgravity;
	}
	else
	{
		o->dy = 0;
		o->y = INTTOFIX32((8 * ((bottom + 1) / 8)) - 1);
	}

	// Animate.
	f->anim_cnt++;
	if (f->anim_cnt >= kanim_speed)
	{
		f->anim_cnt = 0;
		f->anim_frame++;
		if (f->anim_frame >= 4)
		{
			f->anim_frame = 0;
		}
	}

	render(f);
}

void o_load_pilla(Obj *o, uint16_t data)
{
	O_Pilla *f = (O_Pilla *)o;
	_Static_assert(sizeof(*f) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	set_constants();
	vram_load();

	obj_basic_init(o, "Pilla", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-7), INTTOFIX16(7), INTTOFIX16(-14), 1);
	o->main_func = main_func;
	o->cube_func = NULL;
	o->direction = OBJ_DIRECTION_RIGHT;
	o->dx = kdx;

	f->is_head = !!data;
	if (!f->is_head)
	{
		o->name[5] = 'B';
		o->name[6] = 'd';
		o->name[7] = 'y';
	}
	else
	{
		o->name[5] = 'H';
		o->name[6] = 'd';
	}
}

void o_unload_pilla(void)
{
	s_vram_pos = 0;
}
