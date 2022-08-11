#include "obj/item.h"
#include "powerup.h"

void o_load_item(Obj *o, uint16_t data)
{
	obj_erase(o);
	const PowerupType type = data & 0xFF;
	const uint8_t orb_id = data >> 8;
	Powerup *p = powerup_spawn(o->x, o->y, type, orb_id);
	if (!p) return;
	p->x += INTTOFIX32(8);
	p->y += INTTOFIX32(8);
}
