#ifndef OBJ_FALSEBLOCK_H
#define OBJ_FALSEBLOCK_H

#include "obj.h"

typedef struct O_Falseblock
{
	Obj head;

	uint8_t base_tile_id;
} O_Falseblock;

void o_load_falseblock(Obj *o, uint16_t data);
void o_unload_falseblock(void);

#endif  // OBJ_FALSEBLOCK_H
