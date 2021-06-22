#ifndef OBJ_METAGRUB_H
#define OBJ_METAGRUB_H

#include "obj.h"
#include <stdint.h>

typedef struct O_Metagrub
{
	Obj head;
	int16_t move_cnt; // When == 1, metagrub lurches forwards.
} O_Metagrub;

void o_load_metagrub(Obj *o, uint16_t data);
void o_unload_metagrub(void);
#endif  // OBJ_METAGRUB_H
