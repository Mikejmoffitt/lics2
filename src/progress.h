#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdint.h>

#define PROGRESS_NUM_SLOTS 3

#define PROGRESS_MAP_W 25
#define PROGRESS_MAP_H 12

typedef enum ProgressAbility
{
	ABILITY_MAP = 0x01,
	ABILITY_LIFT = 0x02,
	ABILITY_JUMP = 0x04,
	ABILITY_PHANTOM = 0x08,
	ABILITY_KICK = 0x10,
	ABILITY_ORANGE = 0x20,
	ABILITY_FAST_PHANTOM = 0x40,
	ABILITY_CHEAP_PHANTOM = 0x80,
	ABILITY_2X_DAMAGE_PHANTOM = 0x100,
	ABILITY_MASK = 0x1FF,
} ProgressAbility;

typedef struct ProgressSlot
{
	uint32_t magic_0;
	char name[16];
	uint8_t map_explored[PROGRESS_MAP_H][PROGRESS_MAP_W];

	uint16_t cp_orbs;  // Bitfield for collected CP orb IDs.
	int16_t pending_cp_orbs;  // CP orbs Lyle hasn't deposited.
	int16_t registered_cp_orbs;  // CP orbs Lyle has deposited.

	uint16_t hp_orbs;  // Bitfield for collected HP orb IDs.
	int16_t hp_capacity;  // Lyle's health capacity (max 15).

	int16_t touched_first_cube;
	int16_t killed_dancyflower;

	ProgressAbility abilities;

	uint32_t magic_1;
} ProgressSlot;

// Initialize progress slots, and load from SRAM.
int progress_init(void);

// Returns 1 if SRAM tested positively during progress_init.
// Returns 0 if non-volatile progress storage is unavailable.
uint8_t progress_is_sram_working(void);

// Selects the progress slot to use for the following functions.
void progress_select_slot(uint16_t slot);

// Commit progress data to non-volatile RAM.
void progress_save(void);

// Empty out the current progress slot. Does not commit to SRAM.
void progress_erase(void);

// Get a handle to the current progress slot.
ProgressSlot *progress_get(void);

#endif  // PROGRESS_H
