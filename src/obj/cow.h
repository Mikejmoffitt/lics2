#ifndef OBJ_COW_H
#define OBJ_COW_H

#include "obj.h"

typedef enum CowState
{
	COW_EATING,
	COW_PREPARING,
	COW_WALKING,
	COW_JUMPING,
	COW_FINISHED,
	COW_ANGRY
} CowState;

typedef struct O_Cow
{
	Obj head;

	CowState state;

	int16_t state_elapsed;

	int16_t anim_frame;
	int16_t anim_cnt;
	int16_t shot_cnt;

	int16_t hit_cnt;

	int16_t orb_id;

	fix32_t max_x;  // Determined by scanning for Boundary object.
	fix32_t max_y;
} O_Cow;

void o_load_cow(Obj *o, uint16_t data);
void o_unload_cow(void);

#endif  // OBJ_COW_H
