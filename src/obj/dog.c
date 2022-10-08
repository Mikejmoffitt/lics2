#include "obj/dog.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "obj/egg.h"
#include "powerup.h"
#include "progress.h"
#include "sfx.h"

static uint16_t s_vram_pos;

static int16_t knormal_anim_speed;
static int16_t kchew_anim_speed;

static int16_t kchew_time;
static int16_t khappy_time;
static int16_t kflicker_time;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_DOG);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	knormal_anim_speed = PALSCALE_DURATION(7);
	kchew_anim_speed = PALSCALE_DURATION(6);

	kchew_time = PALSCALE_DURATION(84);
	khappy_time = PALSCALE_DURATION(36);
	kflicker_time = PALSCALE_DURATION(48);

	s_constants_set = true;
}

static void render(O_Dog *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -24;
	static const int16_t offset_y = -48;

	if (e->metaframe > 5) return;

	obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
	                        map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + (36 * e->metaframe), 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(3, 3));
	md_spr_put(sp_x + 24, sp_y, SPR_ATTR(s_vram_pos + 9 + (36 * e->metaframe), 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(3, 3));
	md_spr_put(sp_x, sp_y + 24, SPR_ATTR(s_vram_pos + 18 + (36 * e->metaframe), 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(3, 3));
	md_spr_put(sp_x + 24, sp_y + 24, SPR_ATTR(s_vram_pos + 27 + (36 * e->metaframe), 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(3, 3));
}

static void main_func(Obj *o)
{
	O_Dog *e = (O_Dog *)o;

	const DogState state_prev = e->state;

	switch (e->state)
	{
		case DOG_STATE_NORMAL:
			if (e->state_elapsed == 0)
			{
				e->anim_frame = 0;
				e->anim_cnt = 0;
			}

			e->metaframe = e->anim_frame;
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, knormal_anim_speed);

			for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
			{
				Obj *egg = &g_objects[i].obj;
				if (egg->status == OBJ_STATUS_NULL) continue;
				if (egg->type != OBJ_SMALL_EGG) continue;
				// Show open mouth frame if egg is 120px away
				if (egg->x >= o->x + INTTOFIX32(120)) continue;
				e->metaframe = 2;
				// Collision with egg
				if (egg->x > o->x &&
				    egg->x < o->x + INTTOFIX32(16) &&
				    egg->y > o->y - INTTOFIX32(20) &&
				    egg->y < o->y - INTTOFIX32(14))
				{
					obj_erase(egg);
					e->state = DOG_STATE_CHEWING;
					e->eggs_eaten++;
				}
			}

			break;

		case DOG_STATE_CHEWING:
			if (e->state_elapsed == 0)
			{
				e->anim_frame = 0;
				e->anim_cnt = 0;
			}

			if (e->anim_frame == 0 && e->anim_cnt == 0)
			{
				sfx_play(SFX_BOINGO_JUMP, 18);
			}

			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kchew_anim_speed);
			e->metaframe = 3 + e->anim_frame;

			if (e->state_elapsed >= kchew_time)
			{
				e->state = (e->eggs_eaten >= 3) ?
				           DOG_STATE_HAPPY :
				           DOG_STATE_NORMAL;
			}
			break;

		case DOG_STATE_HAPPY:
			if (e->state_elapsed == 0)
			{
				// TODO: Killzam appear sound
//				sfx_play(SFX_, 18);
			}
			e->metaframe = 5;
			if (e->state_elapsed >= khappy_time)
			{
				e->state = DOG_STATE_FLICKER;
			}
			break;

		case DOG_STATE_FLICKER:
			e->anim_frame = (e->anim_frame == 0) ? 1 : 0;
			e->metaframe = 5 + e->anim_frame;

			// Delete eggs and egg generator
			for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
			{
				Obj *egg = &g_objects[i].obj;
				if (egg->status == OBJ_STATUS_NULL) continue;
				if (egg->type != OBJ_SMALL_EGG) continue;
				obj_erase(egg);
			}

			if (e->state_elapsed >= kflicker_time)
			{
				powerup_spawn(o->x, o->y - INTTOFIX32(8),
				                      POWERUP_TYPE_CP_ORB, 6);
				Obj *egg = NULL;
				// Delete eggs and egg generator
				while ((egg = obj_find_by_type(OBJ_EGG)) != NULL)
				{
					obj_erase(egg);
				}
				obj_erase(o);
			}
			break;
	}

	if (e->state != state_prev) e->state_elapsed = 0;
	else e->state_elapsed++;

	render(e);
}

void o_load_dog(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_Dog) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;

	if (progress_get()->cp_orbs & (1 << 6))
	{
		obj_erase(o);
		return;
	}

	set_constants();
	vram_load();

	obj_basic_init(o, "Dog", 0,
	               INTTOFIX16(-24), INTTOFIX16(24), INTTOFIX16(-48), 127);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_dog(void)
{
	s_vram_pos = 0;
}
