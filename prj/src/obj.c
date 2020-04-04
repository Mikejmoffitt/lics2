#include "obj.h"
#include "common.h"
#include "system.h"

#include <stdlib.h>

#include "cube.h"


#include "obj/entrance.h"

#include "obj/cube_manager.h"
#include "obj/template.h"
#include "obj/lyle.h"
#include "obj/map.h"

#include "md/megadrive.h"

#define OBJ_OFFSCREEN_MARGIN INTTOFIX32(64)

// TODO: Place this in a more dynamic / less shitty way.
#define OBJ_VRAM_BASE 0x2000

ObjSlot g_objects[OBJ_COUNT_MAX];
static uint16_t obj_vram_pos;

// Setup and teardown functions for all object types. Leave NULL if an object
// does not require any special setup/teardown.
typedef struct SetupFuncs
{
	void (*load_func)(Obj *o, uint16_t data);  // Run when obj_spawn is called.
	void (*unload_func)(void);  // Run when obj_clear is called.
} SetupFuncs;

static const SetupFuncs setup_funcs[] =
{
	[OBJ_ENTRANCE] = {o_load_entrance, o_unload_entrance},

	[OBJ_LYLE] = {o_load_lyle, o_unload_lyle},
	[OBJ_CUBE_MANAGER] = {o_load_cube_manager, o_unload_cube_manager},
	[OBJ_MAP] = {o_load_map, o_unload_map},
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
	// TODO: Explosion particles.
	// TODO: (Possibly) spawn powerup.
	// TODO: Explosion sound.
	o->status = OBJ_STATUS_NULL;
}

static inline uint16_t obj_hurt_process(Obj *o)
{
	if (o->hurt_stun > 0)
	{
		o->hurt_stun--;
		if (o->hurt_stun == 0)
		{
			o->hp--;
			if (o->hp == 0)
			{
				obj_explode(o);
				// TODO: Spawn explosion particle
			}
		}
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
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status == OBJ_STATUS_NULL) continue;
		pal_set(0, PALRGB(0, i % 8, 4));  // TODO: Remove profiling
		
		if (!(o->flags & OBJ_FLAG_ALWAYS_ACTIVE) &&
		    (o->offscreen = obj_is_offscreen(o)))
		{
			continue;
		}
		if (o->flags & OBJ_FLAG_TANGIBLE &&
		    obj_hurt_process(o))
		{
			continue;
		}

		{
			obj_check_player(o);
		}

		if (o->main_func) o->main_func(o);
	}
	pal_set(0, PALRGB(0, 0, 0));  // TODO: Remove profiling
}

void obj_clear(void)
{
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status == OBJ_STATUS_NULL) continue;
		o->status = OBJ_STATUS_NULL;
		if (!setup_funcs[o->type].unload_func) continue;
		setup_funcs[o->type].unload_func();
	}

	obj_vram_pos = OBJ_VRAM_BASE;
}

Obj *obj_spawn(int16_t x, int16_t y, ObjType type, uint16_t data)
{
	// This is a hack to work around an old wart the old codebase, which
	// represented a cube on the map as an "enemy" object in the file.
	if (type == OBJ_CUBE)
	{
		cube_manager_spawn(INTTOFIX32(x), INTTOFIX32(y + 16), (CubeType)data, CUBE_STATUS_IDLE, 0, 0);
		return NULL;
	}
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status != OBJ_STATUS_NULL) continue;
		memset(o, 0, sizeof(g_objects[i]));
		o->status = OBJ_STATUS_ACTIVE;
		o->type = type;
		o->x = INTTOFIX32(x);
		o->y = INTTOFIX32(y);
		if (setup_funcs[type].load_func) setup_funcs[type].load_func(o, data);
		return o;
	}
	return NULL;
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

void obj_standard_cube_response(Obj *o, Cube *c)
{
	(void)c;
	if (o->hurt_stun != 0) return;

	// TODO: Switch on cube type.

	// TODO: Phantom: if player has double phantom, subtract 1 extra hp
	// TODO: Red: subtract 2 extra hp
	// TODO: Green: Bounce the cube.
	
	// TODO: If none of the above, and not exploding or fizzling, destroy cube.
}

void obj_get_hurt(Obj *o, int16_t damage)
{
	if (o->hurt_stun != 0) return;
	o->hp -= damage;
	if (o->hp <= 0)
	{
		obj_explode(o);
		o->hp = 0;
	}
	o->hurt_stun = system_is_ntsc() ? 24 : 30;
}
