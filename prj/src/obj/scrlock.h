#ifndef OBJ_SCRLOCK_H
#define OBJ_SCRLOCK_H

#include "obj.h"

typedef struct O_ScrLock
{
	Obj head;
} O_ScrLock;

void o_load_scrlock(Obj *o, uint16_t data);
void o_unload_scrlock(void);

#endif  // OBJ_SCRLOCK_H
