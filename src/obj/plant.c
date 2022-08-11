#include "obj/plant.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "projectile.h"
#include "lyle.h"


static uint16_t s_vram_pos;

static uint16_t kidle_duration;
static uint16_t kcharge_duration;

static uint16_t kidle_anim_delay;
static uint16_t kcharge_anim_delay;
static uint16_t kcooldown_anim_delay;

static fix16_t kshot_speed;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_PLANT);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kidle_anim_delay = PALSCALE_DURATION(12);
	kcharge_anim_delay = PALSCALE_DURATION(6);
	kcooldown_anim_delay = PALSCALE_DURATION(6);

	kidle_duration = (PALSCALE_DURATION(120));
	kcharge_duration = (PALSCALE_DURATION(36));

	// TODO: This number was just made up. Measure the original game.
	kshot_speed = INTTOFIX16(PALSCALE_1ST(3.0));

	s_constants_set = 1;
}

static void render(O_Plant *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;

	obj_render_setup(o, &sp_x, &sp_y, -1, 4,
	                 map_get_x_scroll(), map_get_y_scroll());

	const int16_t flip = (o->direction == OBJ_DIRECTION_LEFT);
	
	const int16_t base_y = sp_y - 32;
	const int16_t pal = ENEMY_PAL_LINE;

	static const int16_t cooldown_head_x_offs[] =
	{
		0, -4, -1, -3, -2, -3, -2, -3,
	};

	switch (e->state)
	{
		case PLANT_STATE_IDLE:
		{
			const int16_t head_x = (!flip) ?
			                       (sp_x - 10 - (((e->anim_frame + 1) / 2) % 2)) :
			                       (sp_x - 12 + (((e->anim_frame + 1) / 2) % 2));
			const int16_t head_y = sp_y - 48 + (((e->anim_frame) / 2) % 2);
			const int16_t head_tile = s_vram_pos + 56;
			md_spr_put(head_x, head_y, SPR_ATTR(head_tile, flip, 0, pal, 0),
			        SPR_SIZE(3, 3));
			const int16_t base_x = sp_x - 8;
			const int16_t base_tile = s_vram_pos + ((e->anim_frame % 2) ? 8 : 0);
			md_spr_put(base_x, base_y, SPR_ATTR(base_tile, flip, 0, pal, 0),
			        SPR_SIZE(2, 4));
			break;
		}
		case PLANT_STATE_CHARGE:
		{
			const int16_t head_x = (!flip) ?
			                       (sp_x - ((e->anim_frame % 2) ? 17 : 16)) :
			                       (sp_x - ((e->anim_frame % 2) ? 8 : 9));
			const int16_t head_y = sp_y - 47;
			const int16_t head_tile = s_vram_pos + 56;
			md_spr_put(head_x, head_y, SPR_ATTR(head_tile, flip, 0, pal, 0),
			        SPR_SIZE(3, 3));
			const int16_t base_x = sp_x - 12;
			const int16_t base_tile = s_vram_pos + ((e->anim_frame % 2) ? 24 : 36);
			md_spr_put(base_x, base_y, SPR_ATTR(base_tile, flip, 0, pal, 0),
			        SPR_SIZE(3, 4));
			break;
		}
		case PLANT_STATE_COOLDOWN:
		{
			const int16_t head_x = (!flip) ?
			                       (sp_x + 2 + cooldown_head_x_offs[e->anim_frame]) :
			                       (sp_x - 17 - cooldown_head_x_offs[e->anim_frame]);
			const int16_t head_y = sp_y - 50;
			const int16_t head_tile = s_vram_pos + 48;
			md_spr_put(head_x, head_y, SPR_ATTR(head_tile, flip, 0, pal, 0),
			        SPR_SIZE(2, 4));
			const int16_t base_x = sp_x - 8;
			const int16_t base_tile = s_vram_pos + 16;
			md_spr_put(base_x, base_y, SPR_ATTR(base_tile, flip, 0, pal, 0),
			        SPR_SIZE(2, 4));
			break;
		}
	}
}

static void main_func(Obj *o)
{
	O_Plant *e = (O_Plant *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	const fix32_t lyle_x = lyle_get_x();
	const fix32_t lyle_y = lyle_get_y();

	o->direction = (lyle_x >= o->x) ? OBJ_DIRECTION_RIGHT : OBJ_DIRECTION_LEFT;

	switch (e->state)
	{
		case PLANT_STATE_IDLE:
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kidle_anim_delay);
			if (e->state_elapsed >= kidle_duration)
			{
				e->state_elapsed = 0;
				e->state++;
			}
			break;

		case PLANT_STATE_CHARGE:
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kcharge_anim_delay);
			if (e->state_elapsed >= kcharge_duration)
			{
				e->state_elapsed = 0;
				e->state++;
				const fix32_t x_offset = (o->direction == OBJ_DIRECTION_RIGHT) ? INTTOFIX32(10) : INTTOFIX32(-10);
				projectile_shoot_at(o->x + x_offset, o->y - INTTOFIX32(32), PROJECTILE_TYPE_BALL2,
				                            lyle_x, lyle_y - INTTOFIX32(10), kshot_speed);
				// TODO: Shot speed
				// TODO: Shot sound
			}
			break;

		case PLANT_STATE_COOLDOWN:
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 8, kcooldown_anim_delay);
			if (e->anim_frame >= 7)
			{
				e->state = PLANT_STATE_IDLE;
				// state_elapsed is NOT reset. The time spent in the cooldown
				// state overlaps the idle one, differentiated by animation.
			}
			break;
	}

	e->state_elapsed++;

	render(e);
}

void o_load_plant(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_Plant) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "EvelPlnt", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-9), INTTOFIX16(9), INTTOFIX16(-44), 3);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_plant(void)
{
	s_vram_pos = 0;
}
