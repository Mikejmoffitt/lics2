#ifndef OBJ_PSYCHOWAVE_H
#define OBJ_PSYCHOWAVE_H

#include "obj.h"

typedef enum PsychowaveState
{
	PWAVE_STATE_OFF,
	PWAVE_STATE_ON,
	PWAVE_STATE_BROKEN
} PsychowaveState;

typedef struct O_Psychowave
{
	Obj head;
	PsychowaveState state;
	uint16_t anim_cnt;
	uint16_t anim_frame;
} O_Psychowave;

void o_load_psychowave(Obj *o, uint16_t data);
void o_unload_psychowave(void);

void psychowave_set_state(PsychowaveState state);

#endif  // OBJ_PSYCHOWAVE_H
