#include "obj/item.h"

void o_load_item(Obj *o, uint16_t data)
{
	o->status = OBJ_STATUS_NULL;
	const PowerupType type = data & 0xFF;
	const uint8_t orb_id = data >> 8;
	Powerup *p = powerup_manager_spawn(o->x, o->y, type, orb_id);
	if (!p) return;
	p->x += INTTOFIX32(8);
	p->y += INTTOFIX32(8);
}
