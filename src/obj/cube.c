#include "obj/cube.h"

void o_load_cube(Obj *o, uint16_t data)
{
	Cube *c = cube_manager_spawn(o->x, o->y,
	                             (CubeType)data, CUBE_STATUS_IDLE, 0, 0);
	if (!c) return;
	c->x -= c->left;
	c->y -= c->top;

	if (c->type == CUBE_TYPE_SPAWNER)
	{
		Cube *cb = cube_manager_spawn(o->x, o->y, CUBE_TYPE_BLUE,
		                              CUBE_STATUS_IDLE, 0, 0);
		if (cb)
		{
			c->spawned_cube = cb;
			cb->blue_cube_flash = true;
			cb->x = c->x;
			cb->y = c->y;
		}
	}

	// TODO: What's up here? Shouldn't orange have a correct c->left?
	if (c->type == CUBE_TYPE_ORANGE)
	{
		c->x -= INTTOFIX32(8);
		c->y -= INTTOFIX32(16);
	}
	obj_erase(o);
}
