#ifndef OBJ_KEDDUMS_H
#define OBJ_KEDDUMS_H

#include "obj.h"

typedef enum KeddumsState
{
	KEDDUMS_FLOAT,       // Floating in Psychowave glass vessel.
	KEDDUMS_SHAKE,       // Shaking when Psychowave is active.
	KEDDUMS_FLY,         // Flying out of the broken glass vessel.
	KEDDUMS_FOLLOW_LYLE  // In Lyle's hands.
} KeddumsState;

typedef struct O_Keddums
{
	Obj head;
	KeddumsState state;
	uint16_t anim_cnt;
	uint16_t anim_frame;
} O_Keddums;

void o_load_keddums(Obj *o, uint16_t data);
void o_unload_keddums(void);

void keddums_set_state(KeddumsState state);

#endif  // OBJ_KEDDUMS_H
