#include "obj/hedgedog.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "obj/lyle.h"
#include "projectile.h"
#include "trig.h"

static uint16_t s_vram_pos;

static fix16_t kddy;
static fix16_t kjump_str_base;
static fix16_t kjump_str_mod;
static fix16_t kdx;
static uint16_t kjump_delay;
static uint16_t kground_anim_delay;
static uint16_t kair_anim_delay;
static fix16_t kshot_speed;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_HEDGEDOG);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kddy = INTTOFIX16(PALSCALE_2ND(0.167));
	kjump_str_base = INTTOFIX16(PALSCALE_1ST(-5.0));
	kjump_str_mod = INTTOFIX16(PALSCALE_1ST(-1.0));
	kdx = INTTOFIX16(PALSCALE_1ST(0.5));

	kjump_delay = PALSCALE_DURATION(72);
	kground_anim_delay = PALSCALE_DURATION(9);
	kair_anim_delay = PALSCALE_DURATION(3);
	kshot_speed = INTTOFIX16(PALSCALE_1ST(3));

	s_constants_set = 1;
}

static inline void render(Obj *o)
{
	O_Hedgedog *e = (O_Hedgedog *)o;
	uint16_t tile_offset = 0;
	int16_t sp_x, sp_y;

	if (e->phase == 0)
	{
		if (e->anim_frame == 0)
		{
			tile_offset = 16 + 9;
		}
		else if (e->anim_frame == 2)
		{
			tile_offset = 16 + 18;
		}
		else
		{
			tile_offset = 16;
		}
		obj_render_setup(o, &sp_x, &sp_y, -10, -24,
		                 map_get_x_scroll(), map_get_y_scroll());
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + tile_offset,
		                    (o->x < lyle_get()->head.x) ? OBJ_DIRECTION_RIGHT : OBJ_DIRECTION_LEFT, 0,
		                    ENEMY_PAL_LINE, 1), SPR_SIZE(3, 3));

	}
	else
	{
		tile_offset = 4 * e->anim_frame;
		obj_render_setup(o, &sp_x, &sp_y, -8, -16,
		                 map_get_x_scroll(), map_get_y_scroll());
		md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + tile_offset,
		                    (o->x < lyle_get()->head.x) ? OBJ_DIRECTION_RIGHT : OBJ_DIRECTION_LEFT, 0,
		                    ENEMY_PAL_LINE, 1), SPR_SIZE(2, 2));
	}
}

static void main_func(Obj *o)
{
	O_Hedgedog *e = (O_Hedgedog *)o;
	SYSTEM_ASSERT(e->phase >= 0 && e->phase < 3);

	if (o->hurt_stun > 0)
	{
		render(o);
		return;
	}

	switch (e->phase)
	{
		case 0:
			// Walk until the jump timer expires.
			if (e->jump_timer >= kjump_delay)
			{
				e->phase++;
				e->jump_timer = 0;
				o->dx = 0;
				o->dy = kjump_str_base +
				        (kjump_str_mod >> ((system_rand() % 4)));
				// The direction variable is used to track movement direction.
				o->direction = (o->direction == OBJ_DIRECTION_RIGHT) ?
				               OBJ_DIRECTION_LEFT : OBJ_DIRECTION_RIGHT;
			}
			else
			{
				o->dx = (o->direction == OBJ_DIRECTION_RIGHT) ? kdx : -kdx;
				e->jump_timer++;
			}

			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kground_anim_delay);

			break;

		case 1:
			o->dy += kddy;
			// At apex of jump, fire at the player.
			if (o->dy >= 0)
			{
				O_Lyle *l = lyle_get();
				e->phase++;
				if (o->x < l->head.x)
				{
					projectile_shoot_angle(o->x, o->y,
					                               PROJECTILE_TYPE_BALL2,
					                               DEGTOUINT8(-22.5),
					                               kshot_speed);
					projectile_shoot_angle(o->x, o->y,
					                               PROJECTILE_TYPE_BALL2,
					                               DEGTOUINT8(-45.0),
					                               kshot_speed);
					projectile_shoot_angle(o->x, o->y,
					                               PROJECTILE_TYPE_BALL2,
					                               DEGTOUINT8(-67.5),
					                               kshot_speed);
				}
				else
				{
					projectile_shoot_angle(o->x, o->y,
					                               PROJECTILE_TYPE_BALL2,
					                               DEGTOUINT8(180 + 22.5),
					                               kshot_speed);
					projectile_shoot_angle(o->x, o->y,
					                               PROJECTILE_TYPE_BALL2,
					                               DEGTOUINT8(180 + 45.0),
					                               kshot_speed);
					projectile_shoot_angle(o->x, o->y,
					                               PROJECTILE_TYPE_BALL2,
					                               DEGTOUINT8(180 + 67.5),
					                               kshot_speed);
				}
			}

			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kair_anim_delay);
			break;

		case 2:
			o->dy += kddy;

			if (o->y >= e->original_y)
			{
				o->y = e->original_y;
				o->dy = 0;
				e->phase = 0;
			}

			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kair_anim_delay);
			break;

	}

	obj_standard_physics(o);

	render(o);
}

void o_load_hedgedog(Obj *o, uint16_t data)
{
	O_Hedgedog *e = (O_Hedgedog *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "HedgeDog", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-10), INTTOFIX16(10), INTTOFIX16(-16), 2);
	o->main_func = main_func;
	o->cube_func = NULL;

	o->direction = OBJ_DIRECTION_LEFT;

	e->original_y = o->y;
}

void o_unload_hedgedog(void)
{
	s_vram_pos = 0;
}
