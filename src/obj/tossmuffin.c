#include "obj/tossmuffin.h"
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
#include "obj/cube_manager.h"
#include "sfx.h"

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_TOSSMUFFIN);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static int16_t klift_len;
static int16_t kanim_speed;
static fix16_t ktoss_cube_dx;
static fix16_t ktoss_cube_dy;
static fix16_t kwalk_dx;
static int16_t klift_anim_speed;

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	klift_len = PALSCALE_DURATION(18);
	kanim_speed = PALSCALE_DURATION(12);
	ktoss_cube_dx = INTTOFIX16(PALSCALE_1ST(3.0));
	ktoss_cube_dy = INTTOFIX16(PALSCALE_1ST(-3.333));
	kwalk_dx = INTTOFIX16(PALSCALE_1ST(0.333));
	klift_anim_speed = PALSCALE_DURATION(2);

	s_constants_set = 1;
}

static void render(O_Tossmuffin *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -24;

	const int16_t xflip = e->head.direction == OBJ_DIRECTION_LEFT;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	if (e->lift_cnt > 0)
	{
		spr_put(sp_x, sp_y + 1, SPR_ATTR(s_vram_pos + 4, xflip, 0,
		                                 ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
		spr_put(sp_x, sp_y + 16, SPR_ATTR(s_vram_pos + 14 + (e->anim_frame ? 2 : 0), xflip, 0,
		                                 ENEMY_PAL_LINE, 0), SPR_SIZE(2, 1));
	}
	else if (e->toss_cnt > 0)
	{
		spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, xflip, 0,
		                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
		const int16_t torso_off_x = (e->head.direction == OBJ_DIRECTION_RIGHT) ? 0 : -8;
		spr_put(sp_x + torso_off_x, sp_y + 16, SPR_ATTR(s_vram_pos + 18 , xflip, 0,
		                                                ENEMY_PAL_LINE, 0), SPR_SIZE(3, 1));
	}
	else
	{
		static const int16_t frame_table[] = { 10, 8, 12, 8 };
		const int16_t frame_index = frame_table[e->anim_frame];

		const int16_t off_y = ((e->anim_frame % 2) ? 0 : -1);

		spr_put(sp_x, sp_y + off_y, SPR_ATTR(s_vram_pos, xflip, 0,
		                                     ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
		spr_put(sp_x, sp_y + off_y + 16, SPR_ATTR(s_vram_pos + frame_index, xflip, 0,
		                                     ENEMY_PAL_LINE, 0), SPR_SIZE(2, 1));
	}
}

static inline void toss_cube(O_Tossmuffin *e)
{
	Cube *cube = e->holding_cube;
	// Attempt to confirm that tossmuffin is still holding the cube (i.e. Lyle
	// didn't nab it after jumping on top).
	if (cube->x == e->head.x &&
	    cube->status == CUBE_STATUS_IDLE)
	{
		cube->dx = (e->head.direction == OBJ_DIRECTION_RIGHT) ?
		           ktoss_cube_dx : -ktoss_cube_dx;
		cube->dy = ktoss_cube_dy;
		cube->status = CUBE_STATUS_AIR;
		sfx_play(SFX_CUBE_TOSS, 5);
	}

	e->holding_cube = NULL;
}

static inline void scan_cubes(O_Tossmuffin *e)
{
	if (e->head.hp == 0) return;
	for (uint16_t i = 0; i < ARRAYSIZE(g_cubes); i++)
	{
		Cube *c = &g_cubes[i];
		if (c->status == CUBE_STATUS_IDLE &&
		    obj_touching_cube(&e->head, c))
		{

			// Hold the cube in position above his head.
			c->y = e->head.y - INTTOFIX32(22);
			c->x = e->head.x;

			obj_face_towards_obj(&e->head, &lyle_get()->head);
			e->holding_cube = c;
			e->lift_cnt = klift_len;
			sfx_play(SFX_CUBE_LIFT, 10);
			break;
		}
	}
}

static void main_func(Obj *o)
{
	O_Tossmuffin *e = (O_Tossmuffin *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	if (!e->saw_player)
	{
		obj_face_towards_obj(&e->head, &lyle_get()->head);
		e->saw_player = 1;
	}

	if (e->lift_cnt > 0)
	{
		OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, klift_anim_speed);
		e->head.dx = 0;
		e->lift_cnt--;
		if (e->lift_cnt == 0)
		{
			toss_cube(e);
		}
	}
	else if (e->toss_cnt > 0)
	{
		e->anim_frame = 0;
		e->head.dx = 0;
		e->toss_cnt--;
	}
	else
	{
		OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kanim_speed);
		e->head.dx = (e->head.direction == OBJ_DIRECTION_RIGHT) ? kwalk_dx : -kwalk_dx;
		obj_standard_physics(o);
		// Simple BG collision to restrict sideways movement
		const int16_t left = FIX32TOINT(o->x + o->left);
		const int16_t right = FIX32TOINT(o->x + o->right);
		const int16_t bottom = FIX32TOINT(o->y);
		if (e->head.direction == OBJ_DIRECTION_RIGHT &&
		    map_collision(right + 1, bottom - 4))
		{
			e->head.direction = OBJ_DIRECTION_LEFT;
			e->head.x -= INTTOFIX32(1);
		}
		else if (e->head.direction == OBJ_DIRECTION_LEFT &&
		    map_collision(left - 1, bottom - 4))
		{
			e->head.direction = OBJ_DIRECTION_RIGHT;
			e->head.x += INTTOFIX32(1);
		}
		scan_cubes(e);
	}

	render(e);
}

void o_load_tossmuffin(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Tossmuffin) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-24), 2);
	o->top = INTTOFIX16(-20);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_tossmuffin(void)
{
	s_vram_pos = 0;
}
