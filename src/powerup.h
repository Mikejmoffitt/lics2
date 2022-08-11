#ifndef POWERUP_H
#define POWERUP_H

#include <stdint.h>
#include "util/fixed.h"

#define POWERUP_LIST_SIZE 8

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
	uint16_t anim_cnt;
	uint16_t anim_frame;
	PowerupType type;
	fix32_t x, y;
	fix16_t dx, dy;
} Powerup;

extern Powerup g_powerups[POWERUP_LIST_SIZE];

void powerup_load(void);
// TODO: Maybe split poll and render
void powerup_poll(void);

void powerup_clear(void);
Powerup *powerup_spawn(fix32_t x, fix32_t y, PowerupType type, int8_t orb_id);
void powerup_bounce(Powerup *p);
uint16_t powerup_get_vram_pos(void);

void powerup_set_hibernate(uint16_t en);

#endif  // POWERUP_H
