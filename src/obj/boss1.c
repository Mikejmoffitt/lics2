#include "obj/boss1.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"
#include "obj/fakecube.h"

static uint16_t s_vram_pos;

static int16_t kapproach_start_delay;
static fix16_t kapproach_dx;
static int16_t kroar_delay;
static int16_t kroar_anim_speed;
static int16_t kroar_duration;
static int16_t kfalldown_delay;
static fix16_t kfalldown_dx;
static fix16_t kfalldown_dy;
static fix16_t kfalldown_gravity;
static int16_t kprecharge_delay;
static int16_t kprecharge_anim_speed;
static int16_t kprecharge_duration;
static fix16_t kcharge_dx;
static int16_t kcharge_anim_speed;
static int16_t krecoil_duration;
static int16_t kturn_duration;
static int16_t kpreshot_duration;
static int16_t kshot_duration;
static int16_t kexploding_duration;

static int16_t kdrop_separation;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_BOSS1);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// TODO: Set up the timings based on source.
	kapproach_start_delay = PALSCALE_DURATION(180);
	kapproach_dx = INTTOFIX16(PALSCALE_1ST(1.0));
	kroar_delay = PALSCALE_DURATION(50);
	kroar_anim_speed = PALSCALE_DURATION(2);
	kroar_duration = PALSCALE_DURATION(60);
	kfalldown_delay = PALSCALE_DURATION(100);
	kfalldown_dx = INTTOFIX16(PALSCALE_1ST(2));
	kfalldown_dy = INTTOFIX16(PALSCALE_1ST(-2));
	kfalldown_gravity = INTTOFIX16(PALSCALE_2ND(0.167));
	kprecharge_delay = PALSCALE_DURATION(60);
	kprecharge_anim_speed = PALSCALE_DURATION(4);
	kprecharge_duration = PALSCALE_DURATION(70);
	kcharge_dx = INTTOFIX32(PALSCALE_1ST(2.5));
	kcharge_anim_speed = PALSCALE_DURATION(6);
	krecoil_duration = PALSCALE_DURATION(120);
	kturn_duration = PALSCALE_DURATION(10);
	kpreshot_duration = PALSCALE_DURATION(70);
	kshot_duration = PALSCALE_DURATION(30);
	kexploding_duration = PALSCALE_DURATION(180);
	kdrop_separation = PALSCALE_DURATION(60);

	s_constants_set = 1;
}

static void render(O_Boss1 *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -24;
	static const int16_t offset_y = -32;

	const int16_t xflip = o->direction == OBJ_DIRECTION_LEFT;

	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	if (e->metaframe == 5) sp_y -= 1;
	else if (e->metaframe == 4) sp_y -= 2;

	// VRAM offsets for the back and front halves of the sprite.
	static const int16_t metaframes[] =
	{
		0, 12,  // stand
		24, 36,  // walk1
		48, 60,  // walk2
		72, 84,  // run1
		96, 108,  // run2
		120, 132,  // run3
		144, 156,  // recoil
		168, 180,  // roar1
		192, 204,  // roar2
		216, 228,  // turn
	};

	const uint16_t base_attr = SPR_ATTR(s_vram_pos, xflip, 0, ENEMY_PAL_LINE, 0);
	static const uint16_t size = SPR_SIZE(3, 4);
	spr_put(xflip ? (sp_x + 24) : sp_x, sp_y, base_attr + metaframes[e->metaframe * 2], size);
	spr_put(!xflip ? (sp_x + 24) : sp_x, sp_y, base_attr + metaframes[1 + e->metaframe * 2], size);
}

static void drop_reset(O_Boss1 *e)
{
	static const int16_t total_cubes_to_drop = 32;
	int16_t value = ARRAYSIZE(e->drop.list);
	uint16_t list_index = 0;
	for (uint16_t i = 0; i < ARRAYSIZE(e->drop.list); i++)
	{
		e->drop.list[i] = -1;
	}
	while (value > 0)
	{
		value--;
		list_index += system_rand() % 256;
		list_index = list_index % ARRAYSIZE(e->drop.list);
		while (e->drop.list[list_index] != -1)
		{
			list_index++;
			if (list_index >= ARRAYSIZE(e->drop.list)) list_index = 0;
		}
		e->drop.list[list_index] = value;
	}
	e->drop.index = 0;
	e->drop.greenblue_index = system_rand() % ARRAYSIZE(e->drop.list);
	e->drop.cnt = 0;
	e->drop.remaining = total_cubes_to_drop;
}

static void drop_process(O_Boss1 *e)
{
	if (e->drop.remaining == 0) return;
	if (e->drop.cnt > 0)
	{
		e->drop.cnt--;
		return;
	}
	e->drop.cnt = kdrop_separation;

	const CubeType cube_to_drop = (e->drop.index == e->drop.greenblue_index) ? CUBE_TYPE_GREENBLUE : CUBE_TYPE_BLUE;

	ObjSlot *s = &g_objects[0];
	int16_t i = ARRAYSIZE(g_objects);
	while (i--)
	{
		Obj *b = (Obj *)s;
		s++;
		if (b->status == OBJ_STATUS_NULL ||
		    b->type != OBJ_FAKECUBE)
		{
			continue;
		}
		O_FakeCube *f = (O_FakeCube *)b;
		if (f->id == e->drop.list[e->drop.index]) fakecube_drop_cube(f, cube_to_drop);
	}

	e->drop.remaining--;
	e->drop.index++;
	if (e->drop.index >= ARRAYSIZE(e->drop.list)) e->drop.index = 0;
}

static void main_func(Obj *o)
{
	static const fix32_t ground_y = INTTOFIX32(240 - 32);
	static const fix32_t max_x = INTTOFIX32(320 - 16 - 20);
	static const fix32_t min_x = INTTOFIX32(16 + 20);
	O_Boss1 *e = (O_Boss1 *)o;
	const Boss1State state_prev = e->state;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	switch (e->state)
	{
		default:
		case BOSS1_STATE_INIT:  // Positions or deletes the boss.
			o->x = -o->right;
			o->y = INTTOFIX32(96);
			e->state = BOSS1_STATE_APPROACH;
			e->metaframe = 0;
			break;

		case BOSS1_STATE_APPROACH:  // Boss enters from left.
			if (e->state_elapsed < kapproach_start_delay) break;
			if (o->x < INTTOFIX32(0))
			{
				const int16_t frame_prev = e->anim_frame;
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4,
				                kprecharge_anim_speed);
				o->x += kapproach_dx;
				if (e->anim_frame == 2 && frame_prev != 2)
				{
					// TODO: Play step sound
				}
				static const int16_t metaframes[4] = {0, 1, 0, 2};
				e->metaframe = metaframes[e->anim_frame];
			}
			else
			{
				e->state = BOSS1_STATE_ROAR;
				e->metaframe = 0;
			}
			break;

		case BOSS1_STATE_ROAR:  // Delay: then roar
			if (e->state_elapsed < kroar_delay)
			{
				e->metaframe = 0;
				break;
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kroar_anim_speed);
			e->metaframe = 7 + e->anim_frame;
			if (e->state_elapsed >= kroar_delay + kroar_duration)
			{
				e->state = BOSS1_STATE_FALLDOWN;
			}
			break;

		case BOSS1_STATE_FALLDOWN:  // Delay: then drops down.
			if (e->state_elapsed < kfalldown_delay)
			{
				e->metaframe = 0;
				break;
			}
			else if (e->state_elapsed == kfalldown_delay)
			{
				e->metaframe = 4;
				o->dx = kfalldown_dx;
				o->dy = kfalldown_dy;
			}
			obj_standard_physics(o);
			o->dy += kfalldown_gravity;
			if (o->y >= ground_y)
			{
				o->y = ground_y;
				o->dy = 0;
				e->state = BOSS1_STATE_PRECHARGE;  // TODO: Is it shot?
			}
			break;

		case BOSS1_STATE_PRECHARGE:  // Running anim in place.
			if (e->state_elapsed < kprecharge_delay)
			{
				e->metaframe = 0;
				e->anim_frame = 0;
				e->anim_cnt = 0;
				break;
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 3, kprecharge_anim_speed);
			e->metaframe = 3 + e->anim_frame;
			if (e->state_elapsed >= kprecharge_delay + kprecharge_duration)
			{
				e->state = BOSS1_STATE_CHARGE;
			}
			break;

		case BOSS1_STATE_CHARGE:  // Runs forwards until a wall is hit.
			o->dx = (o->direction == OBJ_DIRECTION_RIGHT) ?
			         kcharge_dx : -kcharge_dx;
			obj_standard_physics(o);
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 3, kcharge_anim_speed);
			e->metaframe = 3 + e->anim_frame;
			if (o->dx > 0 && o->x > max_x)
			{
				o->dx = 0;
				o->x = max_x;
				e->state = BOSS1_STATE_RECOIL;
			}
			else if (o->dx < 0 && o->x < min_x)
			{
				o->dx = 0;
				o->x = min_x;
				e->state = BOSS1_STATE_RECOIL;
			}
			break;

		case BOSS1_STATE_RECOIL:  // Wall hit animation for a short bit.
			if (e->state_elapsed == 0)
			{
				e->metaframe = 6;
				drop_reset(e);
			}
			else if (e->state_elapsed >= krecoil_duration)
			{
				e->state = BOSS1_STATE_TURN;
			}
			// TODO: screen shake, I guess
			break;

		case BOSS1_STATE_TURN:  // The boss changes direction.
			e->metaframe = 9;
			if (e->state_elapsed >= kturn_duration)
			{
				o->direction = (o->direction == OBJ_DIRECTION_RIGHT) ? OBJ_DIRECTION_LEFT : OBJ_DIRECTION_RIGHT;
				e->state = BOSS1_STATE_PRESHOT;
				e->shots_remaining = 1 + (system_rand() % 4);
			}
			break;

		case BOSS1_STATE_PRESHOT:  // The boss delays and contemplates firing.
			e->metaframe = 0;
			if (e->state_elapsed < kpreshot_duration) break;

			e->state = (e->shots_remaining > 0) ? BOSS1_STATE_SHOT :
			                                      BOSS1_STATE_PRECHARGE;
			break;
		case BOSS1_STATE_SHOT:  // The boss fires a projectile.
			if (e->state_elapsed == 0)
			{
				// TODO: Shoot projectile (PROJECTILE_TYPE_DEATHORB2.
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kroar_anim_speed);
			e->metaframe = 7 + e->anim_frame;
			if (e->state_elapsed < kshot_duration) break;
			e->state = BOSS1_STATE_PRESHOT;
			e->shots_remaining--;
			break;

		case BOSS1_STATE_EXPLODING:
			if (e->state_elapsed < kexploding_duration) break;
			break;
	}

	drop_process(e);

	if (e->state != state_prev) e->state_elapsed = 0;
	else e->state_elapsed++;

	render(e);
}

static void cube_func(Obj *o, Cube *c)
{
	if (c->bounce_count > 0) return;
	obj_standard_cube_response(o, c);
}

void o_load_boss1(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Boss1) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE | OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-24), INTTOFIX16(24), INTTOFIX16(-32), 3);
	// TODO: Does the boss take five hits?
	o->main_func = main_func;
	o->cube_func = NULL;

	o->x = -o->right;
	o->y = INTTOFIX32(96);
}

void o_unload_boss1(void)
{
	s_vram_pos = 0;
}
