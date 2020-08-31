#include "obj/powerup_manager.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "sfx.h"
#include "progress.h"

#include "palscale.h"
#include "util/fixed.h"
#include "obj/particle_manager.h"
#include "common.h"
#include "obj/map.h"
#include "obj/lyle.h"

#define POWERUP_MARGIN INTTOFIX32(3)

static O_PowerupManager *powerup_manager;
static Powerup powerups[10];

static int16_t kanim_speed;
static fix16_t kgravity;
static fix16_t kspawn_dy;
static fix16_t kbounce_dy;
// TODO: Anim speeds

static uint16_t vram_pos;

static void set_constants(void)
{
	static int16_t constants_set;
	if (constants_set) return;

	// TODO: Get the real values for these. They are all fabricated.
	kgravity = INTTOFIX16(PALSCALE_2ND(0.25));
	kspawn_dy = INTTOFIX16(PALSCALE_1ST(-3.0));
	kbounce_dy = INTTOFIX16(PALSCALE_1ST(-2.0));
	kanim_speed = PALSCALE_DURATION(6);

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
	uint8_t size;
	uint8_t pal;
	uint16_t tile_offset;

	int16_t tx = FIX32TOINT(p->x) - map_get_x_scroll();
	int16_t ty = FIX32TOINT(p->y) - map_get_y_scroll();

	if (p->type == POWERUP_TYPE_CP ||
	    p->type == POWERUP_TYPE_CP_2X)
	{
		pal = ENEMY_PAL_LINE;
	}
	else if (p->type == POWERUP_TYPE_CP_ORB)
	{
		pal = LYLE_PAL_LINE;
	}
	else
	{
		pal = BG_PAL_LINE;
	}

	if (p->type < POWERUP_TYPE_HP)
	{
		tile_offset = 24 + (8 * p->type);
		pal = LYLE_PAL_LINE;
		size = SPR_SIZE(2, 2);
		tx -= 8;
		ty -= 16;
		if (p->anim_frame == 0) ty -= 1;
		else if (p->anim_frame == 2) ty += 1;
		tile_offset += (p->anim_frame % 2) ? 4 : 0;
	}
	else if (p->type == POWERUP_TYPE_HP)
	{
		pal = BG_PAL_LINE;
		tile_offset = 16;
		size = SPR_SIZE(1, 1);
		pal = BG_PAL_LINE;
		tx -= 4;
		ty -= 8;
		tile_offset += (p->anim_frame % 2) ? 1 : 0;
	}
	else if (p->type == POWERUP_TYPE_HP_2X)
	{
		pal = BG_PAL_LINE;
		tile_offset = 18;
		size = SPR_SIZE(1, 1);
		pal = BG_PAL_LINE;
		tx -= 4;
		ty -= 8;
		tile_offset += (p->anim_frame % 2) ? 1 : 0;
	}
	else if (p->type == POWERUP_TYPE_CP)
	{
		pal = ENEMY_PAL_LINE;
		tile_offset = 20;
		size = SPR_SIZE(1, 1);
		tx -= 4;
		ty -= 8;
		tile_offset += (p->anim_frame % 2) ? 1 : 0;
	}
	else if (p->type == POWERUP_TYPE_CP_2X)
	{
		pal = ENEMY_PAL_LINE;
		tile_offset = 22;
		size = SPR_SIZE(1, 1);
		tx -= 4;
		ty -= 8;
		tile_offset += (p->anim_frame % 2) ? 1 : 0;
	}
	else if (p->type == POWERUP_TYPE_HP_ORB)
	{
		pal = BG_PAL_LINE;
		tile_offset = 0;
		size = SPR_SIZE(2, 2);
		tx -= 8;
		ty -= 16;
		tile_offset += (p->anim_frame % 2) ? 4 : 0;
	}
	else if (p->type == POWERUP_TYPE_CP_ORB)
	{
		pal = LYLE_PAL_LINE;
		tile_offset = 8;
		size = SPR_SIZE(2, 2);
		tx -= 8;
		ty -= 16;
		tile_offset += (p->anim_frame % 2) ? 4 : 0;
	}
	else
	{
		return;
	}

	if (tx < -32 || tx > 336 || ty < -32 || ty > 256) return;

	spr_put(tx, ty, SPR_ATTR(vram_pos + tile_offset, 0, 0, pal, 0), size);
}

static inline void newtonian_physics(Powerup *p)
{
	p->y += p->dy;
	p->dy += kgravity;
	const int16_t px = FIX32TOINT(p->x);
	const int16_t py = FIX32TOINT(p->y);

	if (p->dy > 0 && map_collision(px, py + 1)) p->dy = kbounce_dy;
	else if (p->dy < 0 && map_collision(px, py - 12)) p->dy = 0;
}

static inline void powerup_get(Powerup *p)
{
	O_Lyle *l = lyle_get();
	Obj *lh = &lyle_get()->head;
	ProgressSlot *prog = progress_get();

	switch (p->type)
	{
		case POWERUP_TYPE_HP:
			lh->hp++;
			if (lh->hp > prog->hp_capacity) lh->hp = prog->hp_capacity;
			break;
		case POWERUP_TYPE_CP:
			l->cp++;
			if (l->cp > LYLE_MAX_CP) l->cp = LYLE_MAX_CP;
			break;
		case POWERUP_TYPE_HP_2X:
			lh->hp += 2;
			if (lh->hp > prog->hp_capacity) lh->hp = prog->hp_capacity;
			break;
		case POWERUP_TYPE_CP_2X:
			l->cp += 2;
			if (l->cp > LYLE_MAX_CP) l->cp = LYLE_MAX_CP;
			break;
		case POWERUP_TYPE_CP_ORB:
			SYSTEM_ASSERT(p->orb_id < 16);
			prog->cp_orbs |= (1 << p->orb_id);
			// TODO: Trigger indication screen.
			return;
		case POWERUP_TYPE_HP_ORB:
			SYSTEM_ASSERT(p->orb_id < 16);
			prog->hp_orbs |= (1 << p->orb_id);
			prog->hp_capacity++;
			// TODO: Trigger indication screen.
			return;
		case POWERUP_TYPE_MAP:
			prog->abilities |= ABILITY_MAP;
			break;
		case POWERUP_TYPE_LIFT:
			prog->abilities |= ABILITY_LIFT;
			break;
		case POWERUP_TYPE_JUMP:
			prog->abilities |= ABILITY_JUMP;
			break;
		case POWERUP_TYPE_PHANTOM:
			prog->abilities |= ABILITY_PHANTOM;
			break;
		case POWERUP_TYPE_KICK:
			prog->abilities |= ABILITY_KICK;
			break;
		case POWERUP_TYPE_ORANGE:
			prog->abilities |= ABILITY_ORANGE;
			break;
		
	}

	sfx_play(SFX_POWERUP_GET, 10);
	p->active = 0;
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
			break;
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

	p->anim_cnt++;
	if (p->anim_cnt >= kanim_speed)
	{
		p->anim_cnt = 0;
		p->anim_frame++;
		if (p->anim_frame >= 4) p->anim_frame = 0;
	}
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
	if (!powerup_manager) return NULL;
	const ProgressSlot *prog = progress_get();
	switch (type)
	{
		default:
			break;
		case POWERUP_TYPE_MAP:
		if (prog->abilities & ABILITY_MAP) return NULL;
			break;
		case POWERUP_TYPE_LIFT:
		if (prog->abilities & ABILITY_LIFT) return NULL;
			break;
		case POWERUP_TYPE_JUMP:
		if (prog->abilities & ABILITY_JUMP) return NULL;
			break;
		case POWERUP_TYPE_PHANTOM:
		if (prog->abilities & ABILITY_PHANTOM) return NULL;
			break;
		case POWERUP_TYPE_KICK:
		if (prog->abilities & ABILITY_KICK) return NULL;
			break;
		case POWERUP_TYPE_ORANGE:
		if (prog->abilities & ABILITY_ORANGE) return NULL;
			break;
	}
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
	return NULL;
}
