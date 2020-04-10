#ifndef OBJ_TELEPORTER_H
#define OBJ_TELEPORTER_H

#include "obj.h"
#include <md/megadrive.h>

typedef struct O_Teleporter
{
	Obj head;
	int8_t id;
	int8_t activator;
	int8_t anim_cnt;
	int8_t anim_frame;
	int8_t active_cnt;  // Transition from 1 --> 0 disables teleporter.
	int8_t disabled;
} O_Teleporter;

void o_load_teleporter(Obj *o, uint16_t data);
void o_unload_teleporter(void);

#endif  // OBJ_TELEPORTER_H
