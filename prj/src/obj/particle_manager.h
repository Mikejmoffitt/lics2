#ifndef OBJ_PARTICLE_MANAGER_H
#define OBJ_PARTICLE_MANAGER_H

#include "obj.h"

#include <stdlib.h>
#include "md/megadrive.h"

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
	int16_t delay;
	int16_t anim_cnt;
	int16_t anim_frame;
} Particle;

typedef struct O_ParticleManager
{
	Obj head;
	int16_t spawn_start_index;
} O_ParticleManager;

void o_load_particle_manager(Obj *o, uint16_t data);
void o_unload_particle_manager(void);

void particle_manager_clear(void);
Particle *particle_manager_spawn(fix32_t x, fix32_t y, ParticleType type);

#endif  // OBJ_PARTICLE_MANAGER_H
