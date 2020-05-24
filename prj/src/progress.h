#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdint.h>

#define PROGRESS_NUM_SLOTS 3

#define PROGRESS_MAP_W 25
#define PROGRESS_MAP_H 12

typedef struct ProgressSlot
{
	uint16_t magic_0;
	char name[16];
	uint8_t map_explored[PROGRESS_MAP_H][PROGRESS_MAP_W];
	// Bitfields for item collection.
	uint16_t cp_orbs;
	uint16_t hp_orbs;

	uint16_t magic_1;
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
