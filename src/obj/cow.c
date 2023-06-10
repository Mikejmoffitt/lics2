#include "obj/cow.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"

#include "lyle.h"
#include "projectile.h"
#include "res.h"
#include "sfx.h"
#include "progress.h"
#include "obj/lava.h"

static const int16_t kAngryHits = 10;

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_COW);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Store static constants here.
static int16_t keating_anim_speed;
static int16_t kwalk_anim_speed;
static int16_t kjump_anim_speed;
static int16_t kprepare_frames;
static int16_t kshot_frames;
static fix16_t kjump_str;
static fix16_t kgravity;
static fix16_t kwalk_dx;
static fix16_t kshot_speed;

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;

	keating_anim_speed = PALSCALE_DURATION(14);
	kwalk_anim_speed = PALSCALE_DURATION(12);
	kjump_anim_speed = PALSCALE_DURATION(3);
	kprepare_frames = PALSCALE_DURATION(48);
	kshot_frames = PALSCALE_DURATION(12);
	kjump_str = INTTOFIX16(PALSCALE_1ST(4.16666667));
	kgravity = INTTOFIX16(PALSCALE_1ST(0.10416666667));
	kwalk_dx = INTTOFIX16(PALSCALE_1ST(0.416666667));
	kshot_speed = INTTOFIX16(PALSCALE_1ST(3.0));

	s_constants_set = true;
}

static void render(O_Cow *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -20;
	static const int16_t offset_y = -24;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	const int16_t flip = (o->direction == OBJ_DIRECTION_LEFT);

	const int16_t head_x = (o->direction == OBJ_DIRECTION_RIGHT) ?
	                       16 : 0;
	const int16_t butt_x = (o->direction == OBJ_DIRECTION_RIGHT) ?
	                       0 : 24;

	static const int16_t kbutt_walk_anim[] = {0, 6, 0, 12};
	static const int16_t khead_walk_anim[] = {45, 54, 45, 63};

	int16_t head_offset, butt_offset;
	switch (e->state)
	{
		default:
			butt_offset = 0;
			head_offset = 0;
			break;

		case COW_EATING:
		case COW_FINISHED:
			butt_offset = 0;
			head_offset = e->anim_frame ? 27 : 18;
			break;

		case COW_PREPARING:
			butt_offset = 0;
			head_offset = 45;
			break;

		case COW_WALKING:
			butt_offset = kbutt_walk_anim[e->anim_frame];
			head_offset = khead_walk_anim[e->anim_frame];
			break;

		case COW_JUMPING:
			butt_offset = kbutt_walk_anim[e->anim_frame];
			head_offset = khead_walk_anim[e->anim_frame] + 27;
			break;
			
		case COW_ANGRY:
			butt_offset = 0;
			head_offset = 36;
			break;
	}

	// Caboose.
	md_spr_put(sp_x + butt_x, sp_y, SPR_ATTR(s_vram_pos + butt_offset, flip, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 3));

	// Head.
	md_spr_put(sp_x + head_x, sp_y, SPR_ATTR(s_vram_pos + head_offset, flip, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(3, 3));

}

static void cube_func(Obj *o, Cube *c)
{
	O_Cow *e = (O_Cow *)o;
	if (o->hurt_stun > 0) return;
	e->hit_cnt++;
	obj_standard_cube_response(o, c);
	o->hp = 127;
}

static void main_func(Obj *o)
{
	O_Cow *e = (O_Cow *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	const CowState state_prev = e->state;

	switch (e->state)
	{
		case COW_EATING:
			if (e->state_elapsed == 0)
			{
				e->max_x = o->x;
				const Obj *bounds = obj_find_by_type(OBJ_BOUNDS);
				if (bounds) e->max_x = bounds->x;
			}
			if (e->hit_cnt > 0)
			{
				e->hit_cnt = 0;
				e->state = COW_PREPARING;
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, keating_anim_speed);
			break;

		case COW_PREPARING:
			if (e->state_elapsed >= kprepare_frames)
			{
				o->dx = kwalk_dx;
				e->state = COW_WALKING;
				e->hit_cnt = 0;
			}
			break;

		case COW_WALKING:
			o->dy = 0;
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kwalk_anim_speed);
			if (e->hit_cnt > 0)
			{
				e->hit_cnt = 0;
				e->state = COW_JUMPING;
			}
			if (o->x >= e->max_x)
			{
				e->state = COW_FINISHED;
				o->dx = 0;
				e->hit_cnt = 0;
			}
			break;

		case COW_JUMPING:
			if (e->state_elapsed == 0)
			{
				o->dx = 0;
				o->dy = -kjump_str;
				sfx_play(SFX_MOO, 0);
			}
			o->dy += kgravity;
			if (o->dy > 0 && o->y >= e->max_y)
			{
				e->state = COW_WALKING;
				o->y = e->max_y;
				o->dx = kwalk_dx;
				o->dy = 0;
				e->hit_cnt = 0;
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4, kjump_anim_speed);
			break;

		case COW_FINISHED:
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, keating_anim_speed);
			if (e->hit_cnt >= kAngryHits) e->state = COW_ANGRY;
			break;

		case COW_ANGRY:
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, keating_anim_speed);
			obj_face_towards_obj(o, &lyle_get()->head);
			e->shot_cnt++;
			if (e->shot_cnt >= kshot_frames)
			{
				const O_Lyle *l = lyle_get();
				e->shot_cnt = 0;
				projectile_shoot_at(o->x, o->y - INTTOFIX32(9), PROJECTILE_TYPE_BALL2,
				                            l->head.x, l->head.y - INTTOFIX32(10), kshot_speed);
			}
			
			break;
	}

	if (e->state != state_prev)
	{
		e->anim_frame = 0;
		e->anim_cnt = 0;
		e->state_elapsed = 0;
	}
	else e->state_elapsed++;

	// Destroy lava spawners if relevant orb has been collected
	const ProgressSlot *prog = progress_get();
	if (((e->orb_id & 0xFFF0) == 0x840 &&
	     (prog->cp_orbs & (1 << (e->orb_id & 0xF)))) ||
	    ((e->orb_id & 0xFFF0) == 0x880 &&
	     (prog->hp_orbs & (1 << (e->orb_id & 0xF)))))
	{
		ObjSlot *s = &g_objects[0];
		while (s < &g_objects[OBJ_COUNT_MAX])
		{
			Obj *o = (Obj *)s;
			s++;
			if (o->status != OBJ_STATUS_ACTIVE) continue;
			if (o->type != OBJ_LAVA) continue;
			O_Lava *l = (O_Lava *)o;
			if (l->is_generator) obj_erase(o);
		}
	}

	obj_mixed_physics_h(o);

	render(e);

	md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_cow_bin,
	           sizeof(res_pal_enemy_cow_bin) / 2);
}

void o_load_cow(Obj *o, uint16_t data)
{
	O_Cow *e = (O_Cow *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	set_constants();
	vram_load();

	obj_basic_init(o, "Cow", OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-20), INTTOFIX16(20), INTTOFIX16(-24), 127);
	o->top = INTTOFIX16(-16);
	o->left = INTTOFIX16(-14);
	o->right = INTTOFIX16(14);
	o->main_func = main_func;
	o->cube_func = cube_func;

	e->max_y = o->y;
	e->orb_id = data;
}

void o_unload_cow(void)
{
	s_vram_pos = 0;
}
