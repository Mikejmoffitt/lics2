#include "obj/powerup_manager.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "sfx.h"

#include "palscale.h"
#include "util/fixed.h"
#include "obj/particle_manager.h"
#include "common.h"
#include "obj/map.h"
#include "obj/lyle.h"

#define POWERUP_MARGIN INTTOFIX32(3)

static O_PowerupManager *powerup_manager;
static Powerup powerups[10];


static fix16_t kgravity;
static fix16_t kspawn_dy;
// TODO: Anim speeds

static uint16_t vram_pos;

static void set_constants(void)
{
	static int16_t constants_set;
	if (constants_set) return;

	// TODO: Get the real values for these.
	kgravity = INTTOFIX16(PALSCALE_2ND(0.25));
	kspawn_dy = INTTOFIX16(PALSCALE_1ST(-2.0));

	constants_set = 1;
}

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g = gfx_get(GFX_EX_POWERUPS);
	vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void powerup_render(Powerup *p)
{
	const uint8_t size = SPR_SIZE(2, 2);
	uint8_t pal;
	uint16_t tile_offset;

	// TODO: This.

	int16_t tx = FIX32TOINT(p->x) - 4 - map_get_x_scroll();
	int16_t ty = FIX32TOINT(p->y) - 4 - map_get_y_scroll();
	if (tx < -32 || tx > 336 || ty < -32 || ty > 256) return;

	spr_put(tx, ty, SPR_ATTR(vram_pos + tile_offset, 0, 0, pal, 0), size);
}

static inline void newtonian_physics(Powerup *p)
{
	p->y += p->dy;
	p->dy += kgravity;
	const int16_t px = FIX32TOINT(p->x);
	const int16_t py = FIX32TOINT(p->y);

	if (p->dy > 0 && map_collision(px, py + 1)) p->dy = p->dy * -1;
	else if (p->dy < 0 && map_collision(px, py - 8)) p->dy = 0;
}

static inline void powerup_get(Powerup *p)
{
	O_Lyle *l = lyle_get();
	Obj *lh = &lyle_get()->head;
	p->active = 0;
	switch (p->type)
	{
		case POWERUP_TYPE_HP:
			lh->hp++;
			if (lh->hp > LYLE_MAX_HP) lh->hp = LYLE_MAX_HP;
			sfx_play(SFX_POWERUP_GET, 10);
			break;
		case POWERUP_TYPE_CP:
			l->cp++;
			if (l->cp > LYLE_MAX_CP) l->cp = LYLE_MAX_CP;
			sfx_play(SFX_POWERUP_GET, 10);
			break;
		case POWERUP_TYPE_HP_2X:
			lh->hp += 2;
			if (lh->hp > LYLE_MAX_HP) lh->hp = LYLE_MAX_HP;
			sfx_play(SFX_POWERUP_GET, 10);
			break;
		case POWERUP_TYPE_CP_2X:
			l->cp += 2;
			if (l->cp > LYLE_MAX_CP) l->cp = LYLE_MAX_CP;
			sfx_play(SFX_POWERUP_GET, 10);
			return;
		case POWERUP_TYPE_CP_ORB:
			// TODO: Mark progress.
			sfx_play(SFX_POWERUP_GET, 10);
			return;
		case POWERUP_TYPE_HP_ORB:
			// TODO: Mark progress.
			sfx_play(SFX_POWERUP_GET, 10);
			return;
		default:
			SYSTEM_ASSERT(p->type >= POWERUP_TYPE_HP);
			return;
		
	}
}

static inline void powerup_run(Powerup *p)
{
	switch (p->type)
	{
		case POWERUP_TYPE_HP:
		case POWERUP_TYPE_CP:
		case POWERUP_TYPE_HP_2X:
		case POWERUP_TYPE_CP_2X:
		case POWERUP_TYPE_CP_ORB:
		case POWERUP_TYPE_HP_ORB:
			newtonian_physics(p);
			return;
		default:
			break;
	}

	// Check for having gone OOB
	if (p->x > map_get_right() || p->x < 0 ||
	    p->y > map_get_bottom() || p->y < 0)
	{
		p->active = 0;
		return;
	}

	const Obj *lh = &lyle_get()->head;

	// Check for collision with player
	if (!((p->x + POWERUP_MARGIN < lh->x + lh->left) ||
	      (p->x - POWERUP_MARGIN > lh->x + lh->right) ||
	      (p->y < lh->y + lh->top) ||
	      (p->y - (2 * POWERUP_MARGIN) > lh->y)))
	{
		powerup_get(p);
		return;
	}

	// Don't render off-screen.
	const int16_t tx = FIX32TOINT(p->x) - map_get_x_scroll();
	const int16_t ty = FIX32TOINT(p->y) - map_get_y_scroll();
	if (tx < -64 || tx > 384 || ty < -64 || ty > 304)
	{
		return;
	}

	powerup_render(p);
}

static void main_func(Obj *o)
{
	system_profile(PALRGB(7, 0, 7));
	O_PowerupManager *p = (O_PowerupManager *)o;

	uint16_t i = ARRAYSIZE(powerups);
	while (i--)
	{
		system_profile(PALRGB(3, i % 2 ? 0 : 4, 3));
		Powerup *p = &powerups[i];
		if (!p->active) continue;
		powerup_run(p);
	}
	system_profile(PALRGB(0, 0, 0));
}

void o_load_powerup_manager(Obj *o, uint16_t data)
{
	(void)data;
	SYSTEM_ASSERT(sizeof(O_PowerupManager) <= sizeof(ObjSlot));

	if (powerup_manager || vram_pos)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}

	powerup_manager = (O_PowerupManager *)o;

	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE, 0, 0, 0, 127);
	o->main_func = main_func;

	powerup_manager_clear();
}

void o_unload_powerup_manager(void)
{
	vram_pos = 0;
	powerup_manager = NULL;
}

void powerup_manager_clear(void)
{
	if (!powerup_manager) return;
	uint16_t i = ARRAYSIZE(powerups);
	while (i--)
	{
		powerups[i].active = 0;
	}
}

Powerup *powerup_manager_spawn(fix32_t x, fix32_t y, PowerupType type, int8_t orb_id)
{
	return NULL;
	/*
	if (!powerup_manager) return NULL;i
	uint16_t i = ARRAYSIZE(powerups);
	while (i--)
	{
		Powerup *p = &powerups[i];
		if (p->active) continue;

		p->active = 1;
		p->orb_id = orb_id;
		p->type = type;
		p->x = x;
		p->y = y;
		p->dy = kspawn_dy;
		
		return p;
	}
	return NULL;*/
}
