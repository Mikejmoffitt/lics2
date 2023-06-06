#ifndef PARTICLE_H
#define PARTICLE_H

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
	PARTICLE_TYPE_CRUMBLY,  // final boss ground bits
} ParticleType;

typedef struct Particle Particle;
struct Particle
{
	ParticleType type;
	fix32_t x, y;
	fix16_t dx, dy;
	int16_t life;  // Sets type to null on reaching zero.
	int16_t delay;
	int16_t anim_cnt;
	int16_t anim_frame;
	SprParam spr;
} __attribute__((aligned(32)));

void particle_load(void);
void particle_poll(void);

void particle_clear(void);
Particle *particle_spawn(fix32_t x, fix32_t y, ParticleType type);

void particle_set_hibernate(uint16_t en);

#endif  // PARTICLE_H
