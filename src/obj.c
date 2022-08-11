#include "obj.h"

#include "system.h"
#include <stdlib.h>
#include "cube.h"
#include "gfx.h"
#include "sfx.h"
#include "progress.h"
#include "md/megadrive.h"
#include "palscale.h"
#include "powerup.h"

#define OBJ_OFFSCREEN_MARGIN 64

ObjSlot g_objects[OBJ_COUNT_MAX];
static uint16_t obj_vram_pos;
static uint16_t s_powerup_drop_index;

static uint16_t constants_set;
static int8_t khurt_stun_time;
static fix16_t kcube_bounce_offset_dy;
static fix16_t kcube_bounce_base_dx;

static void set_constants(void)
{
	if (constants_set) return;

	khurt_stun_time = PALSCALE_DURATION(24);
	kcube_bounce_offset_dy = INTTOFIX16(PALSCALE_1ST(0));  // Was 0.8333333
	kcube_bounce_base_dx = INTTOFIX16(PALSCALE_1ST(0.8333333333));
	constants_set = 1;
}

// The object dispatch table and includes are generated by a python script.
// Object routines are named by convention, and a NULL entry is placed in
// the table if the setup or teardown functions do not exist. They will not
// be called, and will not cause harm. However, an object without a load
// function isn't so useful, so...
typedef struct SetupFuncs
{
	void (*load_func)(Obj *o, uint16_t data);
	void (*unload_func)(void);
} SetupFuncs;

#include "obj_dispatch.inc"  // setup_funcs

static const PowerupType powerup_drop_order[32] =
{
	POWERUP_TYPE_NONE,	POWERUP_TYPE_HP,	POWERUP_TYPE_CP,	POWERUP_TYPE_NONE,
	POWERUP_TYPE_HP,	POWERUP_TYPE_HP,	POWERUP_TYPE_NONE,	POWERUP_TYPE_CP,
	POWERUP_TYPE_CP,	POWERUP_TYPE_NONE,	POWERUP_TYPE_HP_2X,	POWERUP_TYPE_NONE,
	POWERUP_TYPE_HP,	POWERUP_TYPE_CP,	POWERUP_TYPE_NONE,	POWERUP_TYPE_HP,
	POWERUP_TYPE_NONE,	POWERUP_TYPE_NONE,	POWERUP_TYPE_CP,	POWERUP_TYPE_HP,
	POWERUP_TYPE_NONE,	POWERUP_TYPE_HP,	POWERUP_TYPE_CP,	POWERUP_TYPE_HP,
	POWERUP_TYPE_CP_2X,	POWERUP_TYPE_NONE,	POWERUP_TYPE_HP,	POWERUP_TYPE_CP,
	POWERUP_TYPE_NONE,	POWERUP_TYPE_CP,	POWERUP_TYPE_NONE,	POWERUP_TYPE_NONE,
};

// Object list execution ======================================================
int obj_init(void)
{
	SYSTEM_ASSERT(sizeof(g_objects[0]) % sizeof(uint32_t) == 0);

	obj_clear();
	set_constants();

	s_powerup_drop_index = system_rand() % ARRAYSIZE(powerup_drop_order);

	return 1;
}

static inline uint16_t obj_is_offscreen(const Obj *o)
{
	const int16_t obj_x = FIX32TOINT(o->x);
	const int16_t obj_y = FIX32TOINT(o->y);
	const int16_t cam_left = map_get_x_scroll();
	const int16_t cam_top = map_get_y_scroll();
	const int16_t cam_right = map_get_x_scroll() + GAME_SCREEN_W_PIXELS;
	const int16_t cam_bottom = map_get_y_scroll() + GAME_SCREEN_H_PIXELS;
	if (obj_x < cam_left - OBJ_OFFSCREEN_MARGIN) return 1;
	if (obj_y < cam_top - OBJ_OFFSCREEN_MARGIN) return 1;
	if (obj_x > cam_right + OBJ_OFFSCREEN_MARGIN) return 1;
	if (obj_y > cam_bottom + OBJ_OFFSCREEN_MARGIN) return 1;
	return 0;
}

static void obj_explode(Obj *o)
{
	const fix16_t kspawn_rate = INTTOFIX16(PALSCALE_1ST(1.0));
	exploder_spawn(o->x, o->y + (o->top / 2), o->dx, o->dy, PARTICLE_TYPE_FIZZLERED, 6, kspawn_rate);
	sfx_play(SFX_OBJ_BURST, 3);
	sfx_play(SFX_OBJ_BURST_HI, 3);

	ProgressSlot *progress = progress_get();

	PowerupType powerup_to_drop = powerup_drop_order[s_powerup_drop_index];
	if (!(progress->abilities & ABILITY_PHANTOM))
	{
		if (powerup_to_drop == POWERUP_TYPE_CP)
		{
			powerup_to_drop = POWERUP_TYPE_HP;
		}
		else if (powerup_to_drop == POWERUP_TYPE_CP_2X)
		{
			powerup_to_drop = POWERUP_TYPE_HP_2X;
		}
	}
	s_powerup_drop_index++;
	if (s_powerup_drop_index >= ARRAYSIZE(powerup_drop_order))
	{
		s_powerup_drop_index = 0;
	}

	powerup_spawn(o->x, o->y, powerup_to_drop, 0);
	o->status = OBJ_STATUS_NULL;

}

static inline uint16_t obj_hurt_process(Obj *o)
{
	if (o->hurt_stun > 0)
	{
		o->hurt_stun--;
	}

	if (o->hurt_stun == 0 && o->hp <= 0)
	{
		obj_explode(o);
		return 1;
	}

	return 0;
}

static inline void obj_check_player(Obj *o)
{
	O_Lyle *l = lyle_get();
	if (!(o->touching_player = lyle_touching_obj(o))) return;

	if (o->flags & OBJ_FLAG_HARMFUL)
	{
		lyle_get_hurt((o->flags & OBJ_FLAG_ALWAYS_HARMFUL) ? 1 : 0);
	}
	if (o->flags & OBJ_FLAG_DEADLY)
	{
		lyle_kill();
	}
	if (o->flags & OBJ_FLAG_BOUNCE_L ||
	    (o->flags & OBJ_FLAG_BOUNCE_ANY && o->x > l->head.x))
	{
		l->head.direction = OBJ_DIRECTION_RIGHT;
		lyle_get_bounced();
	}
	if (o->flags & OBJ_FLAG_BOUNCE_R ||
	    (o->flags & OBJ_FLAG_BOUNCE_ANY && o->x < l->head.x))
	{
		l->head.direction = OBJ_DIRECTION_LEFT;
		lyle_get_bounced();
	}
}


void obj_exec(void)
{
	ObjSlot *s = &g_objects[0];
	int16_t i = ARRAYSIZE(g_objects);
	while (i--)
	{
		Obj *o = (Obj *)s++;
		if (o->status != OBJ_STATUS_ACTIVE) continue;

		o->offscreen = obj_is_offscreen(o);
		if (!(o->flags & OBJ_FLAG_ALWAYS_ACTIVE) && o->offscreen) continue;

		if (o->flags & OBJ_FLAG_TANGIBLE) obj_hurt_process(o);

		if (o->flags & (OBJ_FLAG_HARMFUL | OBJ_FLAG_BOUNCE_L |
		                OBJ_FLAG_BOUNCE_R | OBJ_FLAG_DEADLY |
		                OBJ_FLAG_SENSITIVE))
		{
			obj_check_player(o);
		}
		else
		{
			o->touching_player = 0;
		}

		if (o->main_func) o->main_func(o);
		sfx_poll();
	}
}

void obj_clear(void)
{
	ObjSlot *s = &g_objects[0];
	int16_t i = ARRAYSIZE(g_objects);
	while (i--)
	{
		Obj *o = (Obj *)s;
		s++;
		obj_erase(o);
	}

	for (uint16_t i = 0; i < ARRAYSIZE(setup_funcs); i++)
	{
		if (setup_funcs[i].unload_func) setup_funcs[i].unload_func();
	}

	obj_vram_pos = OBJ_TILE_VRAM_POSITION;
}

Obj *obj_spawn(int16_t x, int16_t y, ObjType type, uint16_t data)
{
	if (!setup_funcs[type].load_func) return NULL;
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status != OBJ_STATUS_NULL) continue;

		uint32_t *raw_mem_uint32 = (uint32_t *)g_objects[i].raw_mem;
		for (uint16_t j = 0; j < sizeof(g_objects[i]) / sizeof(uint32_t); j++)
		{
			raw_mem_uint32[j] = 0;
		}
		o->status = OBJ_STATUS_ACTIVE;
		o->type = type;
		o->x = INTTOFIX32(x);
		o->y = INTTOFIX32(y);
		setup_funcs[type].load_func(o, data);
		return o;
	}
	return NULL;
}

void obj_erase(Obj *o)
{
	o->status = OBJ_STATUS_NULL;
	o->name[0] = '<';
	o->name[1] = '-';
	o->name[2] = 'N';
	o->name[3] = 'U';
	o->name[4] = 'L';
	o->name[5] = 'L';
	o->name[6] = '-';
	o->name[7] = '>';
}

// VRAM load positions are reset to zero when obj_clear is called.
uint16_t obj_vram_alloc(uint16_t bytes)
{
	bytes &= 0xFFFE;
	uint16_t ret = obj_vram_pos;
	obj_vram_pos += bytes;
	return ret;
}

uint16_t obj_get_vram_pos(void)
{
	return obj_vram_pos;
}

void obj_cube_impact(Obj *o, Cube *c)
{
	if (o->cube_func) o->cube_func(o, c);
	else obj_standard_cube_response(o, c);
}

void obj_basic_init(Obj *o, const char *name, ObjFlags flags, fix16_t left, fix16_t right, fix16_t top, int16_t hp)
{
	uint16_t name_idx = 0;
	while (*name && name_idx < 8)
	{
		o->name[name_idx] = *name++;
		name_idx++;
	}
	o->left = left;
	o->right = right;
	o->top = top;

	o->x += right;
	o->y -= top;

	o->hp = hp;
	o->flags = flags;
	o->direction = OBJ_DIRECTION_RIGHT;
	o->hurt_stun = 0;
	o->offscreen = 0;
}

// Utility or commonly reused functions

void obj_get_hurt(Obj *o, int16_t damage)
{
	if (o->hurt_stun != 0) return;
	o->hp -= damage;
	o->hurt_stun = khurt_stun_time;
}

void obj_standard_cube_response(Obj *o, Cube *c)
{
	(void)c;
	if (o->hurt_stun > 0) return;

	int8_t damage = 1;
	switch (c->type)
	{
		default:
			cube_destroy(c);
			break;
		case CUBE_TYPE_RED:
			damage = 3;
			cube_destroy(c);
			break;
		case CUBE_TYPE_PHANTOM:
			if (progress_get()->abilities & ABILITY_2X_DAMAGE_PHANTOM) damage = 2;
			cube_destroy(c);
			break;
		case CUBE_TYPE_GREEN:
			if (c->dx == 0)
			{
				c->dx = (system_rand()) & 1 ?
				        kcube_bounce_base_dx :
				        kcube_bounce_base_dx;
			}
			cube_bounce_dx(c);
			c->dy = (-c->dy / 2) - kcube_bounce_offset_dy;
			c->status = CUBE_STATUS_AIR;
			break;
		case CUBE_TYPE_ORANGE:
			damage = 5;
			cube_destroy(c);
			break;
	}
	sfx_play(SFX_CUBE_HIT, 5);
	obj_get_hurt(o, damage);
}
