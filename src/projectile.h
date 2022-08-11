#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "obj.h"

#include <stdlib.h>
#include "md/megadrive.h"

typedef enum ProjectileType
{
	PROJECTILE_TYPE_NULL = 0,
	PROJECTILE_TYPE_BALL,
	PROJECTILE_TYPE_BALL2,
	PROJECTILE_TYPE_SPIKE,  // Ceiling buggo fires these downwards.
	PROJECTILE_TYPE_SPARK,  // Ground buggo shoots these sideways.
	PROJECTILE_TYPE_DEATHORB,  // Sinusoidal orb from the magibear.
	PROJECTILE_TYPE_DEATHORB2,  // Bouncing oblong thing from Boss 1.
} ProjectileType;

typedef struct Projectile
{
	ProjectileType type;
	fix32_t x, y;
	fix16_t dx, dy;
	int16_t moving_up;
	int16_t frames_alive;
} Projectile;

void projectile_load(void);
void projectile_poll(void);
void projectile_clear(void);
Projectile *projectile_shoot(fix32_t x, fix32_t y, ProjectileType type,
                             fix16_t dx, fix16_t dy);
Projectile *projectile_shoot_angle(fix32_t x, fix32_t y, ProjectileType type,
                                   uint8_t angle, fix16_t speed);
Projectile *projectile_shoot_at(fix32_t x, fix32_t y,
                                ProjectileType type,
                                fix32_t tx, fix32_t ty, fix16_t speed);

#endif  // PROJECTILE_H
