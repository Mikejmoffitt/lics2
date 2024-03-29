#ifndef OBJ_H
#define OBJ_H

#include <stdint.h>

#include "system.h"
#include "util/fixed.h"
#include "obj_types.h"
#include "physics.h"
#include "game.h"

#include "cube.h"

#include <stdlib.h>

#define OBJ_SIMPLE_ANIM(_counter, _frame, _anim_length, _frame_delay) \
{ \
	_counter++; \
	if ((_counter) >= (_frame_delay)) \
	{ \
		_counter = 0; \
		_frame++; \
		if ((_frame) >= (_anim_length)) \
		{ \
			_frame = 0; \
		} \
	} \
}

#define OBJ_COUNT_MAX 40
#define OBJ_BYTES 128

#define OBJ_ACTIVE_DISTANCE 16

// Possible states for an object to be in.
typedef enum ObjStatus
{
	OBJ_STATUS_NULL,
	OBJ_STATUS_ACTIVE,
	OBJ_STATUS_HIBERNATE,
} ObjStatus;

// Flags containing some basic properties about the object.
typedef enum ObjFlags
{
	// Flags that indicate what happens when the player touches an object.
	OBJ_FLAG_HARMFUL = 0x8000,  // Hurts the player on contact.
	OBJ_FLAG_BOUNCE_L = 0x4000,  // Pushes the player to the left on contact.
	OBJ_FLAG_BOUNCE_R = 0x2000,  // Pushes the player to the right on contact.
	OBJ_FLAG_DEADLY = 0x1000,  // Kills the player immediately on contact.
	OBJ_FLAG_SENSITIVE = 0x0800,  // Innocuous player contact.
	OBJ_FLAG_BOUNCE_ANY = 0x0400,  // Picks BOUNCE_L or BOUNCE_R based on side.
	OBJ_FLAG_ALWAYS_HARMFUL = 0x0200,  // Hurts Lyle even if he is still flashing.

	OBJ_FLAG_TANGIBLE = 0x0040,  // Can be hit by a cube and get hurt.
	OBJ_FLAG_ALWAYS_ACTIVE = 0x0010,  // Active even if off-screen.
} ObjFlags;

typedef enum ObjDirection
{
	OBJ_DIRECTION_RIGHT,
	OBJ_DIRECTION_LEFT
} ObjDirection;

typedef struct Obj Obj;
struct Obj
{
	char name[8];  // This is NOT null-terminated!
	void (*main_func)(Obj *o);
	void (*cube_func)(Obj *o, Cube *c);

	ObjType type;
	ObjFlags flags;
	ObjStatus status;
	ObjDirection direction;

	// Positions and dimensions, in game-world coordinates.
	fix32_t x, y;
	fix16_t dx, dy;
	fix16_t left, right;
	fix16_t top;

	int8_t hp;
	int8_t hurt_stun; // Decrements; decreases HP on zero.
	int8_t offscreen;
	int8_t touching_player;
};

typedef union ObjSlot ObjSlot;
union ObjSlot
{
	Obj obj;
	uint8_t raw_mem[OBJ_BYTES];
} __attribute__((aligned(OBJ_BYTES)));

// Object list is public so it may be scanned.
extern ObjSlot g_objects[OBJ_COUNT_MAX];

void obj_init(void);
void obj_exec(void);
void obj_clear(void);
Obj *obj_spawn(int16_t x, int16_t y, ObjType type, uint16_t data);
void obj_erase(Obj *o);

void obj_scramble_powerup_spawn_index(void);

void obj_vram_set_base(uint16_t addr);

// VRAM load positions are reset to zero when obj_clear is called.
uint16_t obj_vram_alloc(uint16_t bytes);

uint16_t obj_get_vram_pos(void);

// Called by cube manager when a collision against an object is detected.
// Calls an object's cube handler, or the default if there is none.
void obj_cube_impact(Obj *o, Cube *c);

// Utility or commonly reused functions =======================================
// name is a null-terminated string. At most 8 characters will be copied.
void obj_basic_init(Obj *o, const char *name, ObjFlags flags, fix16_t left, fix16_t right, fix16_t top, int16_t hp);

// A recreation of the strange physics seen in the MMF original.
static inline void obj_standard_physics(Obj *o)
{
	o->x += physics_trunc_fix16(o->dx);
	o->y += physics_trunc_fix16(o->dy);
}

// Accurate precise physics, used really only by Lyle.
static inline void obj_accurate_physics(Obj *o)
{
	o->x += (fix32_t)o->dx;
	o->y += (fix32_t)o->dy;
}

// Accurate H movement, MMF behavior V movement.
static inline void obj_mixed_physics_h(Obj *o)
{
	o->x += (fix32_t)o->dx;
	o->y += physics_trunc_fix16(o->dy);
}

// Accurate V movement, MMF behavior H movement.
static inline void obj_mixed_physics_v(Obj *o)
{
	o->x += physics_trunc_fix16(o->dx);
	o->y += (fix32_t)o->dy;
}

void obj_standard_cube_response(Obj *o, Cube *c);
void obj_get_hurt(Obj *o, int16_t damage);

// Sets up sprite rendering position based on sprite's coordinates, stun, and
// map scroll.
static inline void obj_render_setup(Obj *o, int16_t *sp_x, int16_t *sp_y,
                                    int16_t x_offset, int16_t y_offset,
                                    int16_t x_scroll, int16_t y_scroll)
{
	*sp_x = FIX32TOINT(o->x) + x_offset - x_scroll;
	*sp_y = FIX32TOINT(o->y) + y_offset - y_scroll;
	if (o->hurt_stun > 0)
	{
		if (o->hurt_stun % 2 == 0)
		{
			*sp_x = GAME_SCREEN_W_PIXELS + 64;
			*sp_y = GAME_SCREEN_H_PIXELS + 64;
		}
		else
		{
			*sp_x += (system_rand() % 8) - 3;
			*sp_y += (system_rand() % 8) - 3;
		}
	}
}

// Same as above, but for objects which do not need hurt stun.
static inline void obj_render_setup_simple(Obj *o, int16_t *sp_x, int16_t *sp_y,
                                           int16_t x_offset, int16_t y_offset,
                                           int16_t x_scroll, int16_t y_scroll)
{
	*sp_x = FIX32TOINT(o->x) + x_offset - x_scroll;
	*sp_y = FIX32TOINT(o->y) + y_offset - y_scroll;
}


static inline bool obj_touching_obj(const Obj *a, const Obj *b)
{
	if (a->x + a->right < b->x + b->left) return false;
	if (a->x + a->left > b->x + b->right) return false;
	if (a->y < b->y + b->top) return false;
	if (a->y + a->top > b->y) return false;
	return true;
}

static inline bool obj_touching_cube(const Obj *o, const Cube *c)
{
	static const fix32_t margin = INTTOFIX32(1);
	if (c->y < o->y + o->top - margin) return false;
	if (c->y + c->top > o->y + margin) return false;
	if (c->x + c->left > o->x + o->right + margin) return false;
	if (c->x + c->right < o->x + o->left - margin) return false;
	return true;
}

static inline void obj_face_towards_obj(Obj *o, const Obj *e)
{
	o->direction = (o->x < e->x) ? OBJ_DIRECTION_RIGHT : OBJ_DIRECTION_LEFT;
}

static inline Obj *obj_find_by_type(ObjType type)
{
	ObjSlot *s = &g_objects[0];
	while (s < &g_objects[OBJ_COUNT_MAX])
	{
		Obj *o = (Obj *)s;
		s++;
		if (o->status != OBJ_STATUS_ACTIVE) continue;
		if (o->type == type) return o;
	}
	return NULL;
}



#endif  // OBJ_H
