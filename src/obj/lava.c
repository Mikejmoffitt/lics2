#include "obj/lava.h"
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

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_LAVA);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static int16_t kanim_speed;
static fix16_t kfall_dy;
static int16_t kgenerator_speed;
static int16_t ksplat_anim_speed;

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kanim_speed = PALSCALE_DURATION(6);
	ksplat_anim_speed = PALSCALE_DURATION(4);
	kfall_dy = INTTOFIX16(PALSCALE_1ST(1.66667));
	kgenerator_speed = PALSCALE_DURATION(9);

	s_constants_set = 1;
}

static inline void render_splat(O_Lava *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;

	static const int16_t offset_x = -16;
	static const int16_t offset_y = -16;
	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame ? 16 : 8), 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(4, 2));
}

static inline void render(O_Lava *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;

	static const int16_t offset_x = -8;
	static const int16_t offset_y = -16;
	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());
	spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame ? 4 : 0), 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
}

static void generator_func(Obj *o)
{
	O_Lava *e = (O_Lava *)o;
	e->generator_cnt++;
	if (e->generator_cnt >= kgenerator_speed)
	{
		e->generator_cnt = 0;
		O_Lava *new_lava = (O_Lava *)obj_spawn(FIX32TOINT(o->x) - 8, FIX32TOINT(o->y) - 16,
		                                       OBJ_LAVA, FIX32TOINT(e->max_y));
		new_lava->cow = e->cow;
	}

	// TODO: Disable generator if orb #5 is collected (cow) or the other orb in the technozone lava tunnel.
}

static void splat_func(Obj *o)
{
	O_Lava *e = (O_Lava *)o;
	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 3, ksplat_anim_speed);
	if (e->anim_frame >= 2)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}
	render_splat(e);
}

static inline void become_splat(Obj *o)
{
	O_Lava *e = (O_Lava *)o;
	o->y = INTTOFIX32(FIX32TOINT(o->y) & 0xFFFFFFF8);
	o->dy = 0;
	e->anim_cnt = 0;
	e->anim_frame = 0;
	o->flags = 0;
	o->main_func = splat_func;
}

static void check_collision_with_orange_cube(Obj *o)
{
	const O_Lyle *l = lyle_get();
	if (o->x + o->right < l->head.x - INTTOFIX32(16)) return;
	if (o->x + o->left > l->head.x + INTTOFIX32(16)) return;
	if (o->y < l->head.y - INTTOFIX32(51)) return;
	if (o->y + o->top > l->head.y) return;

	become_splat(o);
}

static void main_func(Obj *o)
{
	O_Lava *e = (O_Lava *)o;
	const O_Lyle *l = lyle_get();

	if (l->holding_cube && l->holding_cube == CUBE_TYPE_ORANGE)
	{
		// If Lyle is holding an orange cube, disable collision detection, as
		// a simpler check will be used.
		o->flags = 0;

		check_collision_with_orange_cube(o);
	}
	else
	{
		o->flags = OBJ_FLAG_HARMFUL;
	}

	if (e->cow && obj_touching_obj(o, e->cow))
	{
		become_splat(o);
	}

	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_speed);
	o->y += o->dy;
	if (o->y >= e->max_y)
	{
		become_splat(o);
	}
	render(e);
}

void o_load_lava(Obj *o, uint16_t data)
{
	O_Lava *e = (O_Lava *)o;
	SYSTEM_ASSERT(sizeof(O_Lava) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_HARMFUL,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->cube_func = NULL;

	if (data == 0)
	{
		o->main_func = generator_func;
		o->flags = 0;

		// Find the ground position.
		int16_t y_px = FIX32TOINT(o->y);
		const int16_t x_center = FIX32TOINT(o->x);

		e->max_y = o->y;

		for (int16_t i = 0; i < 256; i++)
		{
			y_px += 8;
			if (map_collision(x_center, y_px))
			{
				e->max_y = INTTOFIX32(y_px & 0xFFFFFFF8);
				break;
			}
		}

		// If ground wasn't found within accepted bounds, cancel spawn.
		if (e->max_y == o->y) o->status = OBJ_STATUS_NULL;
	}
	else
	{
		o->main_func = main_func;
		o->left = INTTOFIX16(-4);
		o->right = INTTOFIX16(4);
		o->dy = kfall_dy;
		e->max_y = INTTOFIX32(data);
	}
}

void o_unload_lava(void)
{
	s_vram_pos = 0;
}
