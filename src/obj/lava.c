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
	ksplat_anim_speed = PALSCALE_DURATION(3);
	kfall_dy = INTTOFIX16(PALSCALE_1ST(1.66667));
	kgenerator_speed = PALSCALE_DURATION(76);

	s_constants_set = 1;
}

static inline void render_splat(O_Lava *e)
{
	int16_t sp_x, sp_y;

	sp_x = e->px - map_get_x_scroll();
	sp_y = e->splat_py - map_get_y_scroll();
	if (e->splat_anim_frame == 0)
	{
		spr_put(sp_x + 8, sp_y, SPR_ATTR(s_vram_pos, 0, 0,
		                                 ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
	}
	else
	{
		spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->splat_anim_frame == 2 ? 16 : 8), 0, 0,
		                             ENEMY_PAL_LINE, 0), SPR_SIZE(4, 2));
	}
}

static inline void render(O_Lava *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;

	static const int16_t offset_x = -8;
	switch (e->size)
	{
		default:
		case 0:
			break;
		case 1:
			obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, -16,
			                        map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame ? 4 : 0), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
			break;
		case 2:
			obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, -32,
			                        map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			break;
		case 3:
			obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, -48,
			                        map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 32, SPR_ATTR(s_vram_pos + (e->anim_frame ? 4 : 0), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
			break;
		case 4:
			obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, -64,
			                        map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 32, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			break;
		case 5:
			obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, -80,
			                        map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 32, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 64, SPR_ATTR(s_vram_pos + (e->anim_frame ? 4 : 0), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
			break;
		case 6:
			obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, -96,
			                        map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 32, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 64, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			break;
		case 7:
			obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, -112,
			                        map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 32, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 64, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 96, SPR_ATTR(s_vram_pos + (e->anim_frame ? 4 : 0), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
			break;
		case 8:
			obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, -128,
			                        map_get_x_scroll(), map_get_y_scroll());
			spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 32, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 64, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			spr_put(sp_x, sp_y + 96, SPR_ATTR(s_vram_pos + (e->anim_frame ? 32 : 24), 0, 0,
			                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
			break;
	}
}

static void generator_func(Obj *o)
{
	O_Lava *e = (O_Lava *)o;
	e->generator_cnt++;
	if (e->generator_cnt >= kgenerator_speed)
	{
		e->generator_cnt = 0;
		O_Lava *new_lava = (O_Lava *)obj_spawn(FIX32TOINT(o->x) - 8, FIX32TOINT(o->y) - 48,
		                                       OBJ_LAVA, (0x8000 | (FIX32TOINT(e->max_y) & 0x0FFF)));
		new_lava->cow = e->cow;
		new_lava->px = e->px;
		new_lava->splat_py = e->splat_py;
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
//	o->dy = 0;
//	e->anim_cnt = 0;
//	e->anim_frame = 0;
//	o->flags = 0;
//	o->main_func = splat_func;
	e->splat_cnt = ksplat_anim_speed * 3;
	e->splat_anim_cnt = 0;
	e->splat_anim_frame = 0;
	e->splat_py = FIX32TOINT(o->y) - 16;
	if (e->size > 1)
	{
		e->size--;
		o->y -= INTTOFIX32(16);
		// TODO: Splat particle
	}
	else
	{
		o->status = OBJ_STATUS_NULL;
		// TODO: Splat particle
	}
//	if (e->size > 1)
//	{
//		O_Lava *new_lava = (O_Lava *)obj_spawn(FIX32TOINT(o->x) - 8, FIX32TOINT(o->y) - 32,
//		                                       OBJ_LAVA, ((e->size - 1) << 12) | (FIX32TOINT(e->max_y) & 0x0FFF));
//		new_lava->cow = e->cow;
}

static inline void check_collision_with_orange_cube(Obj *o)
{
	const O_Lyle *l = lyle_get();

	// Lava half-width plus cube half-width. Hardcoded to speed up this
	// collision check as it gets intensive with 40+ lava objects in the
	// technozone hallway section.
	const fix32_t adj_x = INTTOFIX32(20);
	if (o->x + adj_x < l->head.x) return;
	if (o->x - adj_x > l->head.x) return;
	if (o->y < l->head.y - INTTOFIX32(55)) return;
	if (o->y + o->top > l->head.y) return;

	become_splat(o);
}

static void main_func(Obj *o)
{
	O_Lava *e = (O_Lava *)o;
	const O_Lyle *l = lyle_get();

	if (e->splat_cnt > 0)
	{
		e->splat_cnt--;
		if (e->splat_anim_cnt >= ksplat_anim_speed)
		{
			e->splat_anim_cnt = 0;
			e->splat_anim_frame++;
		}
		else
		{
			e->splat_anim_cnt++;
		}
		render_splat(e);
	}

	if (e->size <= 0)
	{
		if (e->splat_cnt <= 0)
		{
			o->status = OBJ_STATUS_NULL;
		}
		return;
	}

	if (l->holding_cube == CUBE_TYPE_ORANGE)
	{
		// If Lyle is holding an orange cube, disable collision detection, a
		// a simpler check will be used.
		o->flags = 0;
		check_collision_with_orange_cube(o);
	}
	else if (e->cow && obj_touching_obj(o, e->cow))
	{
		o->flags = OBJ_FLAG_HARMFUL | OBJ_FLAG_ALWAYS_HARMFUL;
		become_splat(o);
	}
	else
	{
		o->flags = OBJ_FLAG_HARMFUL | OBJ_FLAG_ALWAYS_HARMFUL;
	}

	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_speed);
	o->y += o->dy;
	if (o->y >= e->max_y)
	{
		become_splat(o);
		o->y = INTTOFIX32(FIX32TOINT(o->y) & 0xFFFFFFF8);
	}

	render(e);
}

static void initial_main_func(Obj *o)
{
	O_Lava *e = (O_Lava *)o;
	e->cow = obj_find_by_type(OBJ_COW);
	o->main_func = main_func;
	main_func(o);
}

void o_load_lava(Obj *o, uint16_t data)
{
	O_Lava *e = (O_Lava *)o;
	SYSTEM_ASSERT(sizeof(O_Lava) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	const fix16_t height = INTTOFIX16(((data & 0x8000) ? -16 : -32));

	obj_basic_init(o, OBJ_FLAG_HARMFUL | OBJ_FLAG_ALWAYS_HARMFUL,
	               INTTOFIX16(-8), INTTOFIX16(8), height, 127);
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

		e->is_generator = 1;

		e->generator_cnt = kgenerator_speed;
		e->px = FIX32TOINT(o->x) - 16;
	}
	else
	{
		o->main_func = initial_main_func;
		o->left = INTTOFIX16(-4);
		o->right = INTTOFIX16(4);
		o->dy = kfall_dy;
		e->max_y = INTTOFIX32(data & 0x0FFF);
		e->size = (data & 0xF000) >> 12;
	}
}

void o_unload_lava(void)
{
	s_vram_pos = 0;
}
