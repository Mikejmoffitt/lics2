#include "objtile.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"

#include "util/fixed.h"

#include "obj/map.h"
#include "lyle.h"

#include "util/trig.h"

static ObjTile s_objtiles[32];

static uint16_t s_hibernate;

static inline void objtile_render(ObjTile *p)
{
	const int16_t tx = p->px - map_get_x_scroll();
	const int16_t ty = p->py - map_get_y_scroll();
	if (tx < -16 || tx > 320 || ty < -16 || ty > 240) return;

	p->spr.x = tx;
	p->spr.y = ty;

	md_spr_put_st(&p->spr);
	p->spr.y += 8;
	p->spr.attr += 0x10;
	md_spr_put_st(&p->spr);
	p->spr.attr -= 0x10;
}

void objtile_poll(void)
{
	if (s_hibernate) return;

	for (uint16_t i = 0; i < ARRAYSIZE(s_objtiles); i++)
	{
		ObjTile *p = &s_objtiles[i];
		if (p->flags & OBJTILE_FLAG_ACTIVE) objtile_render(p);
	}
}

void objtile_clear(void)
{
	for (uint16_t i = 0; i < ARRAYSIZE(s_objtiles); i++)
	{
		s_objtiles[i].flags = 0;
	}
}

static ObjTile *find_slot(void)
{
	for (uint16_t i = 0; i < ARRAYSIZE(s_objtiles); i++)
	{
		ObjTile *p = &s_objtiles[i];
		if (p->flags & OBJTILE_FLAG_ACTIVE) continue;
		return p;
	}
	return NULL;
}

ObjTile *objtile_place(fix32_t x, fix32_t y, uint16_t attr)
{
	ObjTile *p = find_slot();
	if (p)
	{
		p->flags = OBJTILE_FLAG_ACTIVE;
		p->px = FIX32TOINT(x);
		p->py = FIX32TOINT(y);
		p->spr.size = SPR_SIZE(2, 1);
		p->spr.attr = attr;
	}
	return p;
}

void objtile_set_hibernate(uint16_t en)
{
	s_hibernate = en;
}
