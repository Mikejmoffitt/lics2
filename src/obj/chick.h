#ifndef OBJ_CHICK_H
#define OBJ_CHICK_H

#include "obj.h"

typedef enum ChickState
{
	CHICK_STATE_NORMAL,
	CHICK_STATE_BELCHING,
	CHICK_STATE_FLYING,
} ChickState;

typedef struct O_Chick
{
	Obj head;

	ChickState state;
	int16_t state_elapsed;
	int16_t reverse_cnt;

	int16_t belched;

	int16_t metaframe;

	int16_t anim_cnt;
	int16_t anim_frame;
} O_Chick;

void o_load_chick(Obj *o, uint16_t data);
void o_unload_chick(void);

#endif  // OBJ_CHICK_H
