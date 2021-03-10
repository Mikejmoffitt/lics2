#ifndef OBJ_COLUMNS_H
#define OBJ_COLUMNS_H

#include "obj.h"

typedef struct O_Columns
{
	Obj head;
} O_Columns;

void o_load_columns(Obj *o, uint16_t data);

#endif  // OBJ_COLUMNS_H
