#include "obj.h"
#include "common.h"
#include "system.h"

#include <stdlib.h>

#include "cube.h"

#include "obj/entrance.h"
#include "obj/metagrub.h"
#include "obj/flip.h"
#include "obj/boingo.h"
#include "obj/gaxter1.h"
#include "obj/gaxter2.h"

#include "obj/teleporter.h"

#include "obj/lyle.h"
#include "obj/cube_manager.h"
#include "obj/map.h"

#include "obj/particle_manager.h"

#include "obj/template.h"

#include "md/megadrive.h"

#include "palscale.h"

#define OBJ_OFFSCREEN_MARGIN INTTOFIX32(64)

// TODO: Place this in a more dynamic / less shitty way.
#define OBJ_VRAM_BASE 0x2000

ObjSlot g_objects[OBJ_COUNT_MAX];
static uint16_t obj_vram_pos;

static uint16_t highest_obj_idx;

static uint16_t constants_set;
static int8_t khurt_stun_time;

static inline void set_constants(void)
{
	if (constants_set) return;

	khurt_stun_time = PALSCALE_DURATION(24);
	constants_set = 1;
}

// Setup and teardown functions for all object types. Leave NULL if an object
// does not require any special setup/teardown.
typedef struct SetupFuncs
{
	void (*load_func)(Obj *o, uint16_t data);  // Run when obj_spawn is called.
	void (*unload_func)(void);  // Run when obj_clear is called.
} SetupFuncs;

// Since cubes are represented in the level data as objects, those objects are
// caught and passed in to the cube manager. Then, the object slot is freed.
static void cube_spawn_stub(Obj *o, uint16_t data)
{
	Cube *c = cube_manager_spawn(o->x, o->y,
	                             (CubeType)data, CUBE_STATUS_IDLE, 0, 0);
	if (!c) return;
	c->x -= c->left;
	c->x += INTTOFIX32(0.5);
	c->y -= c->top;
	o->status = OBJ_STATUS_NULL;
}

static const SetupFuncs setup_funcs[] =
{
	[OBJ_NULL] = {NULL, NULL},
	[OBJ_ENTRANCE] = {o_load_entrance, o_unload_entrance},
	[OBJ_CUBE] = {cube_spawn_stub, NULL},
	[OBJ_METAGRUB] = {o_load_metagrub, o_unload_metagrub},
	[OBJ_FLIP] = {o_load_flip, o_unload_flip},
	[OBJ_BOINGO] = {o_load_boingo, o_unload_boingo},
	[OBJ_GAXTER1] = {o_load_gaxter1, o_unload_gaxter1},
	[OBJ_GAXTER2] = {o_load_gaxter2, o_unload_gaxter2},

	[OBJ_TELEPORTER] = {o_load_teleporter, o_unload_teleporter},

	[OBJ_LYLE] = {o_load_lyle, o_unload_lyle},
	[OBJ_CUBE_MANAGER] = {o_load_cube_manager, o_unload_cube_manager},
	[OBJ_MAP] = {o_load_map, o_unload_map},

	[OBJ_PARTICLE_MANAGER] = {o_load_particle_manager, o_unload_particle_manager},

	[OBJ_TEMPLATE] = {o_load_template, o_unload_template},
};

// Object list execution ======================================================
int obj_init(void)
{
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		o->status = OBJ_STATUS_NULL;
	}

	obj_clear();
	set_constants();

	return 1;
}

static inline uint16_t obj_is_offscreen(const Obj *o)
{
	const fix32_t cam_left = INTTOFIX32(map_get_x_scroll());
	const fix32_t cam_top = INTTOFIX32(map_get_y_scroll());
	const fix32_t cam_right = INTTOFIX32(map_get_x_scroll() + GAME_SCREEN_W_PIXELS);
	const fix32_t cam_bottom = INTTOFIX32(map_get_y_scroll() + GAME_SCREEN_H_PIXELS);
	if (o->x < cam_left - OBJ_OFFSCREEN_MARGIN) return 1;
	if (o->y < cam_top - OBJ_OFFSCREEN_MARGIN) return 1;
	if (o->x > cam_right + OBJ_OFFSCREEN_MARGIN) return 1;
	if (o->y > cam_bottom + OBJ_OFFSCREEN_MARGIN) return 1;
	return 0;
}

static inline void obj_explode(Obj *o)
{
	particle_manager_spawn(o->x, o->y, PARTICLE_TYPE_FIZZLERED);
	particle_manager_spawn(o->x + o->right, o->y, PARTICLE_TYPE_FIZZLERED);
	particle_manager_spawn(o->x + o->left, o->y, PARTICLE_TYPE_FIZZLERED);
	particle_manager_spawn(o->x, o->y + o->top, PARTICLE_TYPE_FIZZLERED);
	particle_manager_spawn(o->x + o->right, o->y + o->top, PARTICLE_TYPE_FIZZLERED);
	particle_manager_spawn(o->x + o->left, o->y + o->top, PARTICLE_TYPE_FIZZLERED);
	// TODO: (Possibly) spawn powerup.
	// TODO: Explosion sound.
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
	if (!(o->flags & (OBJ_FLAG_HARMFUL | OBJ_FLAG_DEADLY |
	                  OBJ_FLAG_BOUNCE_L | OBJ_FLAG_BOUNCE_R |
	                  OBJ_FLAG_SENSITIVE)))
	{
		o->touching_player = 0;
		return;
	}
	o->touching_player = obj_touching_obj(&l->head, o);
	if (!o->touching_player) return;

	if (o->flags & OBJ_FLAG_HARMFUL)
	{
		lyle_get_hurt();
	}
	if (o->flags & OBJ_FLAG_DEADLY)
	{
		lyle_kill();
	}
	if (o->flags & OBJ_FLAG_BOUNCE_L)
	{
		l->head.direction = OBJ_DIRECTION_LEFT;
		lyle_get_bounced();
	}
	if (o->flags & OBJ_FLAG_BOUNCE_R)
	{
		l->head.direction = OBJ_DIRECTION_RIGHT;
		lyle_get_bounced();
	}
}

void obj_exec(void)
{
	uint16_t highest_obj = 0;
	for (uint16_t i = 0; i < highest_obj_idx + 1; i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status == OBJ_STATUS_NULL) continue;

		if (i > highest_obj) highest_obj = i;

		if (!(o->flags & OBJ_FLAG_ALWAYS_ACTIVE) &&
		    (o->offscreen = obj_is_offscreen(o)))
		{
			continue;
		}
		if (o->flags & OBJ_FLAG_TANGIBLE)
		{
		    obj_hurt_process(o);
		}

		if (o->flags & (OBJ_FLAG_HARMFUL | OBJ_FLAG_BOUNCE_L |
		                OBJ_FLAG_BOUNCE_R | OBJ_FLAG_DEADLY |
		                OBJ_FLAG_SENSITIVE))
		{
			obj_check_player(o);
		}

		if (o->main_func) o->main_func(o);
	}

	if (highest_obj_idx > highest_obj) highest_obj_idx = highest_obj;
}

void obj_clear(void)
{
	highest_obj_idx = 0;
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status == OBJ_STATUS_NULL) continue;
		o->status = OBJ_STATUS_NULL;
	}

	for (uint16_t i = 0; i < ARRAYSIZE(setup_funcs); i++)
	{
		if (!setup_funcs[i].unload_func) continue;
		setup_funcs[i].unload_func();
	}

	obj_vram_pos = OBJ_VRAM_BASE;
}

Obj *obj_spawn(int16_t x, int16_t y, ObjType type, uint16_t data)
{
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status != OBJ_STATUS_NULL) continue;
		if (!setup_funcs[type].load_func) continue;

		if (i > highest_obj_idx) highest_obj_idx = i;
		memset(o, 0, sizeof(g_objects[i]));
		o->status = OBJ_STATUS_ACTIVE;
		o->type = type;
		o->x = INTTOFIX32(x);
		o->y = INTTOFIX32(y);
		setup_funcs[type].load_func(o, data);
		return o;
	}
	return NULL;
}

uint8_t obj_max_index(void)
{
	return highest_obj_idx;
}

// VRAM load positions are reset to zero when obj_clear is called.
uint16_t obj_vram_alloc(uint16_t bytes)
{
	bytes &= 0xFFFE;
	uint16_t ret = obj_vram_pos;
	obj_vram_pos += bytes;
	return ret;
}

void obj_cube_impact(Obj *o, Cube *c)
{
	if (o->cube_func) o->cube_func(o, c);
	else obj_standard_cube_response(o, c);
}

void obj_basic_init(Obj *o, ObjFlags flags, fix16_t left, fix16_t right, fix16_t top, int16_t hp)
{
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
void obj_standard_physics(Obj *o)
{
	o->x += o->dx;
	o->y += o->dy;
}

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
			damage = 2; // TODO: Gate this if player has double phantom.
			cube_destroy(c);
			break;
		case CUBE_TYPE_GREEN:
			cube_bounce_dx(c);
			c->dy = -c->dy;
			c->status = CUBE_STATUS_AIR;
			break;
		case CUBE_TYPE_ORANGE:
			damage = 5;
			cube_destroy(c);
			break;
	}
	
	obj_get_hurt(o, damage);
}
