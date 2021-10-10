#ifndef OBJ_VYLE1_H
#define OBJ_VYLE1_H

#include "obj.h"

typedef enum Vyle1State
{
	VYLE1_STATE_INIT,  // Waiting for Lyle to approach x = 64px
	VYLE1_STATE_INTRO,  // Slowly entering
	VYLE1_STATE_ACTIVE,
	VYLE1_STATE_RECOIL,
	VYLE1_STATE_RETREAT,
} Vyle1State;

typedef struct O_Vyle1
{
	Obj head;

	Vyle1State state;
	int16_t state_elapsed;

	int16_t airborn;

	int16_t anim_cnt;
	int16_t anim_frame;
	int16_t metaframe;
	int16_t shot_anim_active;

	int16_t jump_cnt;
	int16_t direction_flip_cnt;
	int16_t pre_shot_cnt;

	fix32_t cloak_x;
	fix32_t cloak_y;
	fix16_t cloak_dy;

	fix32_t original_y;

} O_Vyle1;

void o_load_vyle1(Obj *o, uint16_t data);
void o_unload_vyle1(void);

#endif  // OBJ_VYLE1_H
