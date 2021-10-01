#ifndef OBJ_TECHNOBGH_H
#define OBJ_TECHNOBGH_H

#include "obj.h"

typedef struct O_TechnoBg
{
	Obj head;
	const uint8_t *source;  // Pointer to tile data.
	uint16_t last_index;

} O_TechnoBg;

void o_load_technobg(Obj *o, uint16_t data);

#endif  // OBJ_TECHNOBGH_H
