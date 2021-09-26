#ifndef OBJ_RADIO_H
#define OBJ_RADIO_H

#include "obj.h"

typedef struct O_Radio
{
	Obj head;

	int16_t anim_frame;
	int16_t anim_cnt;

	uint16_t storage;  // Item stored.
} O_Radio;

void o_load_radio(Obj *o, uint16_t data);
void o_unload_radio(void);

#endif  // OBJ_RADIO_H
