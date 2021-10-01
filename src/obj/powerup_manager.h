#ifndef OBJ_POWERUP_MANAGER_H
#define OBJ_POWERUP_MANAGER_H

// If you can pick it up, it lives here.

#include "obj.h"

#include <stdlib.h>
#include "md/megadrive.h"

typedef enum PowerupType
{
	// Items found on the map. These values must align with map data.
	POWERUP_TYPE_MAP = 0,
	POWERUP_TYPE_LIFT = 1,
	POWERUP_TYPE_JUMP = 2,
	POWERUP_TYPE_PHANTOM = 3,
	POWERUP_TYPE_KICK = 4,
	POWERUP_TYPE_ORANGE = 5,
	// Items which come from boxes or enemies.
	POWERUP_TYPE_HP = 6,
	POWERUP_TYPE_HP_2X = 7,
	POWERUP_TYPE_CP = 8,
	POWERUP_TYPE_CP_2X = 9,
	POWERUP_TYPE_CP_ORB = 0x0A,
	POWERUP_TYPE_HP_ORB = 0x0B,

	POWERUP_TYPE_NONE = 0xFF,
} PowerupType;

typedef struct Powerup
{
	int8_t active;
	int8_t orb_id;
	int16_t anim_cnt;
	int16_t anim_frame;
	PowerupType type;
	fix32_t x, y;
	fix16_t dx, dy;
} Powerup;

typedef struct O_PowerupManager
{
	Obj head;
	int16_t particle_cnt;
	int16_t flicker_cnt;
	int8_t flicker_2f_anim;
	int8_t flicker_4f_anim;
} O_PowerupManager;

extern Powerup g_powerups[10];

void o_load_powerup_manager(Obj *o, uint16_t data);
void o_unload_powerup_manager(void);

void powerup_manager_clear(void);
Powerup *powerup_manager_spawn(fix32_t x, fix32_t y, PowerupType type, int8_t orb_id);
void powerup_bounce(Powerup *p);

#endif  // OBJ_POWERUP_MANAGER_H
