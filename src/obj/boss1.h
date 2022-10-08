#ifndef OBJ_BOSS1_H
#define OBJ_BOSS1_H

#include <stdbool.h>
#include "obj.h"

typedef enum __attribute__((packed)) Boss1State
{
	BOSS1_STATE_INIT,  // Positions or deletes the boss.
	BOSS1_STATE_APPROACH,  // Boss enters from left.
	BOSS1_STATE_ROAR,  // Delay, then roar.
	BOSS1_STATE_FALLDOWN,  // Delay, then drops down.
	BOSS1_STATE_SHAKE_DELAY,  // Delay waiting for screen shake to complete.
	// These states loop and form the fight,
	BOSS1_STATE_PRECHARGE,  // Running anim in place.
	BOSS1_STATE_CHARGE,  // Runs forwards until a wall is hit.
	BOSS1_STATE_RECOIL,  // Wall hit animation for a short bit.
	BOSS1_STATE_TURN,  // The boss changes direction and sets up cube drops.
	BOSS1_STATE_DROP_WAIT,  // Waiting for cubes to fall.
	BOSS1_STATE_PRESHOT,  // The boss delays and contemplates firing.
	BOSS1_STATE_SHOT,  // The boss fires a projectile.
	// Boss reaches this state when HP == 0.
	BOSS1_STATE_EXPLODING,
	BOSS1_STATE_EXPLODED,
} Boss1State;

typedef struct O_Boss1
{
	Obj head;
	Boss1State state;

	int16_t anim_frame;
	int16_t anim_cnt;

	int32_t state_elapsed;
	uint8_t shots_remaining;  // Set between 1 - 5 during turn.
	uint8_t metaframe;

	int16_t explode_cnt;


	// Data for the cube dropping phase set during precharge.
	struct
	{
		int16_t cnt;  // Used to space out drops.
		uint8_t remaining;
		bool shaking;
	} drop;
} O_Boss1;

void o_load_boss1(Obj *o, uint16_t data);
void o_unload_boss1(void);

#endif  // OBJ_BOSS1_H
