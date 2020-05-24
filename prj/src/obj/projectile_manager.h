#ifndef OBJ_PROJECTILE_MANAGER_H
#define OBJ_PROJECTILE_MANAGER_H

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

typedef struct O_ProjectileManager
{
	Obj head;
	int16_t particle_cnt;
	int16_t flicker_cnt;
	int8_t flicker_2f_anim;
	int8_t flicker_4f_anim;
} O_ProjectileManager;

void o_load_projectile_manager(Obj *o, uint16_t data);
void o_unload_projectile_manager(void);

void projectile_manager_clear(void);
Projectile *projectile_manager_shoot(fix32_t x, fix32_t y, ProjectileType type,
                                     fix16_t dx, fix16_t dy);
Projectile *projectile_manager_shoot_at(fix32_t x, fix32_t y,
                                        ProjectileType type,
                                         int32_t tx, int32_t ty);

#endif  // OBJ_PROJECTILE_MANAGER_H
