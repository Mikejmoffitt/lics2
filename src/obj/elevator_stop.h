#ifndef OBJ_ELEVATOR_STOP_H
#define OBJ_ELEVATOR_STOP_H

#include "obj.h"

typedef struct O_ElevatorStop
{
	Obj head;
	uint16_t id;
} O_ElevatorStop;

void o_load_elevator_stop(Obj *o, uint16_t data);
void o_unload_elevator_stop(void);

#endif  // OBJ_ELEVATOR_STOP_H
