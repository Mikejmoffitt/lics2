#ifndef OBJ_PARTICLE_MANAGER_H
#define OBJ_PARTICLE_MANAGER_H

#include "cube.h"
#include "obj.h"

#include <stdlib.h>
#include "common.h"
#include "md/megadrive.h"
#include "game.h"

typedef enum ParticleType
{
	PARTICLE_TYPE_NULL = 0,
	PARTICLE_TYPE_SPARKLE,
	PARTICLE_TYPE_FIZZLE,
	PARTICLE_TYPE_FIZZLERED,
	PARTICLE_TYPE_EXPLOSION,
	PARTICLE_TYPE_SAND,
} ParticleType;

typedef struct Particle
{
	ParticleType type;
	fix32_t x, y;
	fix16_t dx, dy;
	int16_t life;  // Sets type to null on reaching zero.
} Particle;

typedef struct O_ParticleManager
{
	Obj head;
} O_ParticleManager;

void o_load_particle_manager(Obj *o, uint16_t data);
void o_unload_particle_manager(void);

void particle_manager_clear(void);
void particle_manager_spawn(int32_t x, int32_t y, ParticleType type);

#endif  // OBJ_PARTICLE_MANAGER_H
