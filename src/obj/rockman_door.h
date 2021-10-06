#ifndef OBJ_ROCKMAN_DOOR_H
#define OBJ_ROCKMAN_DOOR_H

#include "obj.h"

typedef struct O_RockmanDoor
{
	Obj head;
	int16_t closed;
	int16_t state;
} O_RockmanDoor;

void o_load_rockman_door(Obj *o, uint16_t data);
void o_unload_rockman_door(void);

void rockman_door_set_closed(int16_t closed);

#endif  // OBJ_ROCKMAN_DOOR_H
