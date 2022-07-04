#include "obj/cube.h"

void o_load_cube(Obj *o, uint16_t data)
{
	Cube *c = cube_manager_spawn(o->x, o->y,
	                             (CubeType)data, CUBE_STATUS_IDLE, 0, 0);
	if (!c) return;
	c->x -= c->left;
	c->y -= c->top;
	// TODO: What's up here? Shouldn't orange have a correct c->left?
	if (c->type == CUBE_TYPE_ORANGE)
	{
		c->x -= INTTOFIX32(8);
		c->y -= INTTOFIX32(16);
	}
	obj_erase(o);
}
