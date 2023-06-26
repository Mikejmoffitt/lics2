#ifndef OBJ_VYLE2_H
#define OBJ_VYLE2_H

#include "obj.h"

typedef enum Vyle2State
{
	VYLE2_STATE_INIT,
	// Introduction states.
	VYLE2_STATE_LYLE_WALK_ANGRY,
	VYLE2_STATE_CAMERA_PAN_TO_MACHINE,
	VYLE2_STATE_KEDDUMS_MEOW,
	VYLE2_STATE_CAMERA_PAN_TO_LYLE,
	VYLE2_STATE_LYLE_APPROACH_MACHINE,
	VYLE2_STATE_VYLE_ACTIVATE_MACHINE,
	VYLE2_STATE_VYLE_APPROACH_MACHINE,
	VYLE2_STATE_VYLE_GROW_1,
	VYLE2_STATE_VYLE_GROW_2,
	VYLE2_STATE_VYLE_GROW_3,
	VYLE2_STATE_VYLE_GROW_4,
	VYLE2_STATE_ENTER_ARENA,
	VYLE2_STATE_START_DELAY,
	// Fight states.
	VYLE2_STATE_PRE_JUMP,
	VYLE2_STATE_JUMP,
	VYLE2_STATE_LAND,  // --> PRE_JUMP, or SHOOTING
	VYLE2_STATE_SHOOTING,
	VYLE2_STATE_EDGE_PRE_JUMP,
	VYLE2_STATE_EDGE_JUMP,
	VYLE2_STATE_EDGE_LAND,  // --> EDGE_PRE_JUMP or PRE_BELCH
	VYLE2_STATE_PRE_BELCH,
	VYLE2_STATE_BELCH,  // --> PRE_BELCH or PRE_CHARGE
	VYLE2_STATE_PRE_CHARGE,
	VYLE2_STATE_CHARGE,
	VYLE2_STATE_ZAP,
	VYLE2_STATE_ZAP_RECOIL,
	VYLE2_STATE_ZAP_RECOIL_BOUNCED,
	VYLE2_STATE_VULNERABLE,
	VYLE2_STATE_CENTER_PRE_JUMP,
	VYLE2_STATE_CENTER_JUMP,
	VYLE2_STATE_CENTER_LAND,  // --> JUMP_TO_CENTER or PRE_SUPERJUMP
	VYLE2_STATE_PRE_SUPERJUMP,
	VYLE2_STATE_SUPERJUMP_UP,
	VYLE2_STATE_SUPERJUMP_HOVER,
	VYLE2_STATE_SUPERJUMP_DOWN,  // --> VYLE2_STATE_LAND or VYLE2_STATE_SUPERJUMP_EXIT
	VYLE2_STATE_SUPERJUMP_EXIT,
	// Ending (part 1)
	VYLE2_STATE_END1_FALL_REPEAT,
	VYLE2_STATE_END1_FALL_DOWN,
	VYLE2_STATE_END1_LYLE_LANDED,
	VYLE2_STATE_END1_EXPLODING,
	VYLE2_STATE_END1_EXPLODED,
	// Ending (part 2)
	VYLE2_STATE_END2_LYLE_RISE_UP,
	VYLE2_STATE_END2_LYLE_LANDED,
	VYLE2_STATE_END2_LYLE_ESCAPE,

} Vyle2State;

typedef struct O_Vyle2
{
	Obj head;

	// Position for the camera.
	fix32_t xscroll;
	fix32_t yscroll;

	int16_t lyle_anim_cnt;
	int16_t lyle_anim_frame;

	Vyle2State first_state;
	Vyle2State state;
	int16_t state_elapsed;

	uint16_t anim_cnt;
	uint16_t anim_frame;
	int16_t metaframe;

	// jump logic
	int16_t jump_count;
	bool shot_at_lyle;
	fix32_t jump_tx;
	// shot phase
	int16_t shots_remaining;
	int16_t shot_cnt;
	// superjump phase
	int16_t crumble_cnt;
	int16_t ground_slams;

	bool shaking;

	// endinng pt1
	int16_t fall_cycles;
} O_Vyle2;

void o_load_vyle2(Obj *o, uint16_t data);
void o_unload_vyle2(void);

#endif  // OBJ_VYLE2_H
