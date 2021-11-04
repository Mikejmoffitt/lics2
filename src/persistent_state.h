#ifndef PERSISTENT_STATE_H
#define PERSISTENT_STATE_H

// State that is preserved across room transitions and object list clears.

#include "util/fixed.h"

typedef struct PersistentState
{
	uint8_t next_room_id;
	uint8_t next_room_entrance;
	int16_t lyle_tele_in_cnt;
	int16_t lyle_phantom_cnt;
	int16_t lyle_cp;
	int16_t lyle_hp;
	fix16_t lyle_entry_dx;
	fix16_t lyle_entry_dy;
} PersistentState;

extern PersistentState g_persistent_state;

void persistent_state_init(void);
static inline PersistentState *persistent_state_get(void)
{
	return &g_persistent_state;
}

#endif  // PERSISTENT_STATE_H
