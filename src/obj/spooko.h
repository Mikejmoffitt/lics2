#ifndef OBJ_SPOOKO_H
#define OBJ_SPOOKO_H

#include "obj.h"

typedef struct O_Spooko
{
	Obj head;
	uint16_t data;
} O_Spooko;

void o_load_spooko(Obj *o, uint16_t data);
void o_unload_spooko(void);

#endif  // OBJ_SPOOKO_H
