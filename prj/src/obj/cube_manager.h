#ifndef OBJ_CUBE_MANAGER_H
#define OBJ_CUBE_MANAGER_H

#include "cube.h"
#include "obj.h"

typedef struct O_CubeManager
{
	Obj head;
} O_CubeManager;

void o_load_cube_manager(Obj *o, uint16_t data);
void o_unload_cube_manager(void);

#endif  // OBJ_CUBE_MANAGER_H
