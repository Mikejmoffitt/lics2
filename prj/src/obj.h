#ifndef OBJ_H
#define OBJ_H

#include <stdint.h>

#include "util/fixed.h"
#include "obj_types.h"

#include "cube.h"

#define OBJ_BYTES 64
#define OBJ_COUNT_MAX 32

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

	OBJ_FLAG_TANGIBLE = 0x0800,  // Can be hit by a cube and get hurt.

	OBJ_FLAG_ALWAYS_ACTIVE = 0x0400,  // Active even if off-screen.

	OBJ_FLAG_TOUCHING_PLAYER = 0x0001,
} ObjFlags;

typedef enum ObjDirection
{
	OBJ_DIR_RIGHT,
	OBJ_DIR_LEFT
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
	fix16_t w;  // Actually half-width.
	fix16_t h;

	int16_t hp;
	uint16_t hurt_stun; // Decrements; decreases HP on zero.
};

typedef union ObjSlot
{
	Obj obj;
	uint8_t raw_mem[OBJ_BYTES];
} ObjSlot;

int obj_init(void);
void obj_exec(void);
void obj_clear(void);
Obj *obj_spawn(int16_t x, int16_t y, ObjType type, uint16_t data);

// VRAM load positions are reset to zero when obj_clear is called.
uint16_t obj_vram_alloc(uint16_t words);

// Utility or commonly reused functions =======================================
void obj_standard_physics(Obj *o);
void obj_standard_cube_response(Obj *o, Cube *c);

void obj_get_hurt(Obj *o, int16_t damage);

static inline uint16_t obj_touching_obj(Obj *a, Obj *b)
{
	return !((a->x + a->w < b->x - b->w) ||
	         (a->x - a->w < b->x + b->w) ||
	         (a->y < b->y - b->h) ||
	         (a->y + a->h > b->y));
}

#endif  // OBJ_H
