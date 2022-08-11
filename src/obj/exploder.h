#ifndef OBJ_EXPLODER_H
#define OBJ_EXPLODER_H

// This invisible object just sits there yarfing up explosions until it stops.

#include "obj.h"
#include "md/megadrive.h"
#include "particle.h"

typedef struct O_Exploder
{
	Obj head;
	ParticleType type;
	int count;
	fix16_t accumulator;
	fix16_t rate;
} O_Exploder;

// Spawn an exploder.
void exploder_spawn(fix32_t x, fix32_t y, fix16_t dx, fix16_t dy,
                    ParticleType type, uint16_t count, fix16_t rate);

void o_load_exploder(Obj *o, uint16_t data);

#endif  // OBJ_EXPLODER_H
