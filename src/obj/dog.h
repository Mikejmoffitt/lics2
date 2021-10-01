#ifndef OBJ_DOG_H
#define OBJ_DOG_H

#include "obj.h"

typedef enum DogState
{
	DOG_STATE_NORMAL,
	DOG_STATE_CHEWING,
	DOG_STATE_HAPPY,
	DOG_STATE_FLICKER,
} DogState;

typedef struct O_Dog
{
	Obj head;

	int16_t anim_cnt;
	int16_t anim_frame;

	DogState state;
	int16_t state_elapsed;

	int16_t eggs_eaten;

	int16_t metaframe;
} O_Dog;

void o_load_dog(Obj *o, uint16_t data);
void o_unload_dog(void);

#endif  // OBJ_DOG_H
