#include "obj.h"
#include "common.h"
#include "system.h"

#include <stdlib.h>

#include "cube.h"

#include "obj/cube_manager.h"
#include "obj/template.h"

// TODO: Place this in a more dynamic / less shitty way.
#define OBJ_VRAM_BASE 0x400

static ObjSlot objects[OBJ_COUNT_MAX];
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
	[OBJ_CUBE_MANAGER] = {o_load_cube_manager, o_unload_cube_manager},
	[OBJ_TEMPLATE] = {o_load_template, o_unload_template},
};

// Object list execution ======================================================
int obj_init(void)
{
	for (uint16_t i = 0; i < ARRAYSIZE(objects); i++)
	{
		Obj *o = &objects[i].obj;
		o->status = OBJ_STATUS_NULL;
		o->type = OBJ_NULL;
	}

	obj_clear();

	return 1;
}

static inline uint16_t obj_is_offscreen(const Obj *o)
{
	(void)o;
	// TODO: Implement once camera is done.
	return 0;
}

static inline void obj_explode(Obj *o)
{
	// TODO: Explosion particles.
	// TODO: (Possibly) spawn powerup.
	// TODO: Explosion sound.
	o->status = OBJ_STATUS_NULL;
	o->type = OBJ_NULL;
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
	// TODO: Check for player, set/clear touching player bit in o->flags
	if (!(o->flags & OBJ_FLAG_TOUCHING_PLAYER)) return;
	
	if (o->flags & OBJ_FLAG_DEADLY)
	{
		// TODO: Kill the player.
	}
	else if (o->flags & OBJ_FLAG_HARMFUL)
	{
		// TODO: Handle dealing damage to player
	}
	else if (o->flags & OBJ_FLAG_BOUNCE_L)
	{
		// TODO: Handle bouncing player left
	}
	else if (o->flags & OBJ_FLAG_BOUNCE_R)
	{
		// TODO: Handle bouncing player right
	}
}

void obj_exec(void)
{
	for (uint16_t i = 0; i < ARRAYSIZE(objects); i++)
	{
		Obj *o = &objects[i].obj;
		if (o->status == OBJ_STATUS_NULL || o->type == OBJ_NULL) continue;
		if (!(o->flags & OBJ_FLAG_ALWAYS_ACTIVE) &&
		    obj_is_offscreen(o))
		{
			continue;
		}
		if (!(o->flags & OBJ_FLAG_TANGIBLE) &&
		    obj_hurt_process(o))
		{
			continue;
		}

		if (o->main_func) o->main_func(o);

		obj_check_player(o);
	}
}
void obj_clear(void)
{
	for (uint16_t i = 0; i < ARRAYSIZE(objects); i++)
	{
		Obj *o = &objects[i].obj;
		if (o->status == OBJ_STATUS_NULL) continue;
		o->status = OBJ_STATUS_NULL;
		if (!setup_funcs[o->type].unload_func) continue;
		setup_funcs[o->type].unload_func();
	}

	obj_vram_pos = OBJ_VRAM_BASE;
}

Obj *obj_spawn(int16_t x, int16_t y, ObjType type, uint16_t data)
{
	for (uint16_t i = 0; i < ARRAYSIZE(objects); i++)
	{
		Obj *o = &objects[i].obj;
		if (o->status != OBJ_STATUS_NULL) continue;
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
uint16_t obj_vram_alloc(uint16_t words)
{
	uint16_t ret = obj_vram_pos;
	obj_vram_pos += words;
	return ret;
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
