#ifndef OBJ_H
#define OBJ_H

#include <stdint.h>

#include "system.h"
#include "util/fixed.h"
#include "obj_types.h"

#include "cube.h"

#define OBJ_COUNT_MAX 64
#define OBJ_BYTES 80

#define OBJ_ACTIVE_DISTANCE 32

// Possible states for an object to be in.
typedef enum ObjStatus
{
	OBJ_STATUS_NULL,
	OBJ_STATUS_ACTIVE,
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
	void (*main_func)(Obj *o);
	void (*cube_func)(Obj *o, Cube *c);

	ObjStatus status;
	ObjFlags flags;
	ObjType type;
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

typedef union ObjSlot
{
	Obj obj;
	uint8_t raw_mem[OBJ_BYTES];
} ObjSlot;

// Object list is public so it may be scanned.
extern ObjSlot g_objects[OBJ_COUNT_MAX];

// Get the index of the highest-active object + 1. This can be treated as the
// number of active objects, except that within the bound, there may be some
// marked as OBJ_TYPE_NULL (inactive).
uint16_t obj_get_upper_bound(void);

int obj_init(void);
void obj_exec(void);
void obj_clear(void);
Obj *obj_spawn(int16_t x, int16_t y, ObjType type, uint16_t data);

// VRAM load positions are reset to zero when obj_clear is called.
uint16_t obj_vram_alloc(uint16_t bytes);

// Called by cubes.c when a collision against an object is detected.
// Calls an object's cube handler, or the default if there is none.
void obj_cube_impact(Obj *o, Cube *c);

// Utility or commonly reused functions =======================================
void obj_basic_init(Obj *o, ObjFlags flags, fix16_t left, fix16_t right, fix16_t top, int16_t hp);
static inline void obj_standard_physics(Obj *o)
{
	o->x += (fix32_t)o->dx;
	o->y += (fix32_t)o->dy;
}
void obj_standard_cube_response(Obj *o, Cube *c);

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
		*sp_x += (system_rand() % 8) - 3;
		*sp_y += (system_rand() % 8) - 3;
	}
}

void obj_get_hurt(Obj *o, int16_t damage);

static inline uint16_t obj_touching_obj(const Obj *a, const Obj *b)
{
	return !((a->x + a->right < b->x + b->left) ||
	         (a->x + a->left > b->x + b->right) ||
	         (a->y < b->y + b->top) ||
	         (a->y + a->top > b->y));
}

static inline int obj_touching_cube(const Obj *o, const Cube *c)
{
	const fix32_t margin = INTTOFIX32(1);
	if (c->y < o->y + o->top - margin) return 0;
	if (c->y + c->top > o->y + margin) return 0;
	if (c->x + c->left > o->x + o->right + margin) return 0;
	if (c->x + c->right < o->x + o->left - margin) return 0;
	return 1;
}



#endif  // OBJ_H
