#ifndef OBJ_CP_GIVER_H
#define OBJ_CP_GIVER_H

#include "obj.h"
#include "progress.h"

// The end of the pipe above the cube power pad that takes Lyle's CP orbs and
// emits cube power level-up items.

typedef enum CpGiverState
{
	CP_GIVER_STATE_IDLE,
	CP_GIVER_STATE_PRETAKE,
	CP_GIVER_STATE_TAKING,
	CP_GIVER_STATE_TOOK,
	CP_GIVER_STATE_GIVING,
} CpGiverState;

typedef struct O_CpGiver
{
	Obj head;

	CpGiverState state;
	int16_t state_elapsed;

	int16_t metaframe;

	int16_t anim_frame;
	int16_t anim_cnt;

	// CP orb that comes out of lyle.
	fix32_t orb_y;
	fix16_t orb_dy;
	int16_t orb_flicker_cnt;
	int16_t orb_anim_cnt;
	int16_t orb_anim_frame;

	// Cube powerup that is given to Lyle.
	fix32_t powerup_y;
	fix16_t powerup_dy;
	ProgressAbility powerup_ability;
} O_CpGiver;

void o_load_cp_giver(Obj *o, uint16_t data);
void o_unload_cp_giver(void);

#endif  // OBJ_CP_GIVER_H
