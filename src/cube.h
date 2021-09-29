#ifndef CUBE_H
#define CUBE_H

// Cubes are managed by obj/cube_manager.h

#include "util/fixed.h"
#include <stdint.h>

typedef enum CubeStatus
{
	CUBE_STATUS_NULL,
	CUBE_STATUS_IDLE,
	CUBE_STATUS_AIR,
	CUBE_STATUS_KICKED,
	CUBE_STATUS_FIZZLE,
	CUBE_STATUS_EXPLODE,
} CubeStatus;

typedef enum CubeType
{
	CUBE_TYPE_NULL = 0,
	CUBE_TYPE_BLUE = 0x0100,
	CUBE_TYPE_PHANTOM = 0x0200,
	CUBE_TYPE_GREEN = 0x0300,
	CUBE_TYPE_GREENBLUE = 0x0301,
	CUBE_TYPE_RED = 0x0400,
	CUBE_TYPE_YELLOW_HPUP = 0x0800,
	CUBE_TYPE_YELLOW_HPUP2 = 0x0810,
	CUBE_TYPE_YELLOW_CPUP = 0x0820,
	CUBE_TYPE_YELLOW_CPUP2 = 0x0830,
	CUBE_TYPE_YELLOW_CPORB0 = 0x0840,
	CUBE_TYPE_YELLOW_CPORB1 = 0x0841,
	CUBE_TYPE_YELLOW_CPORB2 = 0x0842,
	CUBE_TYPE_YELLOW_CPORB3 = 0x0843,
	CUBE_TYPE_YELLOW_CPORB4 = 0x0844,
	CUBE_TYPE_YELLOW_CPORB5 = 0x0845,
	CUBE_TYPE_YELLOW_CPORB6 = 0x0846,
	CUBE_TYPE_YELLOW_CPORB7 = 0x0847,
	CUBE_TYPE_YELLOW_CPORB8 = 0x0848,
	CUBE_TYPE_YELLOW_CPORB9 = 0x0849,
	CUBE_TYPE_YELLOW_CPORB10 = 0x084A,
	CUBE_TYPE_YELLOW_CPORB11 = 0x084B,
	CUBE_TYPE_YELLOW_CPORB12 = 0x084C,
	CUBE_TYPE_YELLOW_CPORB13 = 0x084D,
	CUBE_TYPE_YELLOW_CPORB14 = 0x084E,
	CUBE_TYPE_YELLOW_CPORB15 = 0x084F,
	CUBE_TYPE_YELLOW_HPORB0 = 0x0880,
	CUBE_TYPE_YELLOW_HPORB1 = 0x0881,
	CUBE_TYPE_YELLOW_HPORB2 = 0x0882,
	CUBE_TYPE_YELLOW_HPORB3 = 0x0883,
	CUBE_TYPE_YELLOW_HPORB4 = 0x0884,
	CUBE_TYPE_YELLOW_HPORB5 = 0x0885,
	CUBE_TYPE_YELLOW_HPORB6 = 0x0886,
	CUBE_TYPE_YELLOW_HPORB7 = 0x0887,
	CUBE_TYPE_YELLOW_HPORB8 = 0x0888,
	CUBE_TYPE_YELLOW_HPORB9 = 0x0889,
	CUBE_TYPE_YELLOW_HPORB10 = 0x088A,
	CUBE_TYPE_YELLOW_HPORB11 = 0x088B,
	CUBE_TYPE_YELLOW_HPORB12 = 0x088C,
	CUBE_TYPE_YELLOW_HPORB13 = 0x088D,
	CUBE_TYPE_YELLOW_HPORB14 = 0x088E,
	CUBE_TYPE_YELLOW_HPORB15 = 0x088F,
	CUBE_TYPE_ORANGE = 0x1000,
	CUBE_TYPE_SPAWNER = 0x2000,

} CubeType;

typedef struct Cube Cube;

struct Cube
{
	CubeStatus status;
	Cube *spawned_cube;
	CubeType type;
	fix32_t x, y;
	fix16_t dx, dy;
	fix16_t left, right;
	fix16_t top;
	uint8_t bounce_count;
	uint8_t collision_timeout;
	uint8_t spawn_count;
	uint8_t fizzle_count;
	uint8_t lyle_spawn_check;
	// TODO: anim vars for spawner flashing.
};

// If needed, initialize constants used by cubes.
void cube_set_constants(void);

// Execute logic for a single cube.
void cube_run(Cube *c);

// Reverses dx and sets it to kcube_on_cube_dx.
void cube_bounce_dx(Cube *c);

void cube_destroy(Cube *c);

// Constrains cube'x DX
void cube_clamp_dx(Cube *c);

// Called when touching a spawner cube.
void cube_restrict_spawn(Cube *c);


#endif  // CUBE_H
