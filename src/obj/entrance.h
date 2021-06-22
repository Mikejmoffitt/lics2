#ifndef OBJ_ENTRANCE_H
#define OBJ_ENTRANCE_H

#include "obj.h"

typedef struct O_Entrance
{
	Obj head;
	uint8_t entrance_num;
	uint8_t to_room_id;
	uint8_t to_entrance_num;
} O_Entrance;

void o_load_entrance(Obj *o, uint16_t data);
void o_unload_entrance(void);

#endif  // OBJ_ENTRANCE_H
