#include "obj/cube_manager.h"

#include "obj.h"
#include "system.h"
#include "gfx.h"

void o_load_cube_manager(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_CubeManager) <= sizeof(*o));
	(void)data;
}

void o_unload_cube_manager(void)
{
}


