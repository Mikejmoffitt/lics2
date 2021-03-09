#ifndef OBJ_PURPLETREE_H
#define OBJ_PURPLETREE_H

#include "obj.h"

typedef struct O_PurpleTree
{
	Obj head;
	int16_t tile;
} O_PurpleTree;

void o_load_purpletree(Obj *o, uint16_t data);
void o_unload_purpletree(void);

#endif  // OBJ_PURPLETREE_H
