#ifndef OBJ_PLANT_H
#define OBJ_PLANT_H

#include "obj.h"

typedef enum PlantState
{
	PLANT_STATE_IDLE,
	PLANT_STATE_CHARGE,
	PLANT_STATE_COOLDOWN
} PlantState;

typedef struct O_Plant
{
	Obj head;

	uint16_t anim_frame;
	uint16_t anim_cnt;

	PlantState state;
	uint16_t state_elapsed;
} O_Plant;

void o_load_plant(Obj *o, uint16_t data);
void o_unload_plant(void);

#endif  // OBJ_PLANT_H
