#include "obj/magibear.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"
#include "sfx.h"
#include "obj/lyle.h"
#include "obj/projectile_manager.h"

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_MAGIBEAR);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static fix16_t kmove_dx;
static int16_t kmouth_open_frames;
static int16_t kshot_delay;
static int16_t kanim_frame_duration;
static fix16_t kshot_speed;
static int16_t kmouth_anim_frame_duration;

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kmove_dx = INTTOFIX16(PALSCALE_1ST(0.166667));
	kmouth_open_frames = PALSCALE_DURATION(24);
	kshot_delay = PALSCALE_DURATION(120);
	kanim_frame_duration = PALSCALE_DURATION(10);
	kshot_speed = INTTOFIX16(PALSCALE_1ST(0.9));  // TODO: 0.75f was too slow. Should be MMF "Speed 8"
	kmouth_anim_frame_duration = PALSCALE_DURATION(4);

	s_constants_set = 1;
}

static inline void render(O_Magibear *m)
{
	Obj *o = &m->head;
	int16_t sp_x, sp_y;
	obj_render_setup(o, &sp_x, &sp_y, -20, -27,
	                 map_get_x_scroll(), map_get_y_scroll());

	// Magibear is big enough that one sprite won't cut it. He is split into
	// "butt" and "head" sprites.
	//
	//  ____ ______
	// |    |      |
	// |BUTT| HEAD |  *huhuhuh*
	// |    |      |               like
	// |____|______|                        cool

	// VRAM offsets for both sections, where the index is the anim frame.
	const uint16_t anim_off_butt[] = {0,  8,   0, 16, 0,  0};
	const uint16_t anim_off_head[] = {24, 36, 24, 48, 60, 72};

	if (o->direction == OBJ_DIRECTION_RIGHT)
	{
		spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + anim_off_butt[m->anim_frame],
		                    o->direction == OBJ_DIRECTION_LEFT, 0,
		                    ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
		sp_x += 16;
		spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + anim_off_head[m->anim_frame],
		                    o->direction == OBJ_DIRECTION_LEFT, 0,
		                    ENEMY_PAL_LINE, 0), SPR_SIZE(3, 4));
	}
	else
	{
		spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + anim_off_head[m->anim_frame],
		                    o->direction == OBJ_DIRECTION_LEFT, 0,
		                    ENEMY_PAL_LINE, 0), SPR_SIZE(3, 4));
		sp_x += 24;
		spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + anim_off_butt[m->anim_frame],
		                    o->direction == OBJ_DIRECTION_LEFT, 0,
		                    ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
	}

}

static void normal_state_logic(O_Magibear *m)
{
	Obj *o = &m->head;
	if (m->shot_cnt == 0)
	{
		m->mouth_cnt = kmouth_open_frames;
		m->anim_frame = 4;
		m->anim_cnt = 0;
		m->shot_cnt = kshot_delay;  // TODO: Introduce a little randomness as seen in the original?
		
		if (o->direction == OBJ_DIRECTION_LEFT)
		{
			projectile_manager_shoot(o->x + (o->left/2), o->y - INTTOFIX32(6),
			                         PROJECTILE_TYPE_DEATHORB,
			                         -kshot_speed, 0);
		}
		else
		{
			projectile_manager_shoot(o->x + (o->right/2), o->y - INTTOFIX32(6),
			                         PROJECTILE_TYPE_DEATHORB,
			                         kshot_speed, 0);
		}

		sfx_play(SFX_MAGIBEAR_SHOT, 14);
	}
	else
	{
		const O_Lyle *l = lyle_get();
		m->shot_cnt--;
		// Always face and move towards Lyle.
		o->direction = (o->x < l->head.x) ? OBJ_DIRECTION_RIGHT : OBJ_DIRECTION_LEFT;
	
		// Move forwards if there isn't a collision with the stage geometry.
		const int16_t left = FIX32TOINT(o->x + o->left);
		const int16_t right = FIX32TOINT(o->x + o->right);
		const int16_t bottom = FIX32TOINT(o->y);
		if (o->direction == OBJ_DIRECTION_RIGHT && !map_collision(right + 1, bottom - 4))
		{
			o->x += kmove_dx;
		}
		else if (o->direction == OBJ_DIRECTION_LEFT && !map_collision(left - 1, bottom - 4))
		{
			o->x -= kmove_dx;
		}

		if (m->anim_cnt >= kanim_frame_duration)
		{
			m->anim_cnt = 0;
			m->anim_frame++;
			if (m->anim_frame > 3) m->anim_frame = 0;
		}
	}
}

static void mouth_state_logic(O_Magibear *m)
{
	if (m->anim_cnt >= kmouth_anim_frame_duration)
	{
		m->anim_cnt = 0;
		m->anim_frame++;
		if (m->anim_frame > 5) m->anim_frame = 4;
	}
}

static void main_func(Obj *o)
{
	O_Magibear *m = (O_Magibear *)o;
	if (o->hurt_stun > 0)
	{
		render(m);
		return;
	}
	m->anim_cnt++;
	if (m->mouth_cnt > 0)
	{
		m->mouth_cnt--;
		mouth_state_logic(m);
	}
	else
	{
		normal_state_logic(m);
	}

	render(m);

}

void o_load_magibear(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Magibear) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-16), INTTOFIX16(16), INTTOFIX16(-27), 3);
	o->main_func = main_func;
	o->cube_func = NULL;
	O_Magibear *m = (O_Magibear *)o;
	m->shot_cnt = kshot_delay;
}

void o_unload_magibear(void)
{
	s_vram_pos = 0;
}
