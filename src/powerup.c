#include "powerup.h"

#include <stdint.h>
#include "gfx.h"
#include "sfx.h"
#include "progress.h"

#include "palscale.h"
#include "particle.h"

#include "map.h"
#include "lyle.h"
#include "pause.h"
#include "obj.h"
#include "cube_manager.h"

#define POWERUP_MARGIN INTTOFIX32(3)

Powerup g_powerups[POWERUP_LIST_SIZE];

static int16_t kanim_speed;
static fix16_t kgravity;
static fix16_t kspawn_dy;
static fix16_t kbounce_dy;
static fix16_t kbounce_cube_dy;
static fix16_t kceiling_dy;
static fix16_t kmax_dy;

#define POWERUP_VRAM_TILE (POWERUP_VRAM_POSITION/32)

static bool s_hibernate;

static void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;

	kgravity = INTTOFIX16(PALSCALE_2ND( (1.0 / 5.0) * (5.0 / 6.0) ));
	kspawn_dy = INTTOFIX16(PALSCALE_1ST(-20 * (1.0 / 5.0) * (5.0 / 6.0)));
	kbounce_dy = INTTOFIX16(PALSCALE_1ST( 2 * -10 * (1.0 / 5.0) * (5.0 / 6.0) ));
	kbounce_cube_dy = INTTOFIX16(PALSCALE_1ST( -15 * (1.0 / 5.0) * (5.0 / 6.0) ));
	kceiling_dy = INTTOFIX16(PALSCALE_1ST(0.833333333));
	kanim_speed = PALSCALE_DURATION(4);
	kmax_dy = INTTOFIX16(PALSCALE_1ST(30 * (1.0 / 5.0) * (5.0 / 6.0)));

	s_constants_set = true;
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
		tile_offset = 24 + (4 * p->type);
		pal = ENEMY_PAL_LINE;
		size = SPR_SIZE(2, 2);
		tx -= 8;
		ty -= 16;
		if ((p->anim_frame / 2) == 0) ty -= 1;
		else if ((p->anim_frame / 2) == 2) ty += 1;
		if (p->anim_frame % 2)
		{
			md_pal_upload(ENEMY_CRAM_POSITION, res_pal_items1_bin, sizeof(res_pal_items1_bin) / 2);
		}
		else
		{
			md_pal_upload(ENEMY_CRAM_POSITION, res_pal_items2_bin, sizeof(res_pal_items2_bin) / 2);
		}
	}
	else if (p->type == POWERUP_TYPE_HP)
	{
		pal = BG_PAL_LINE;
		tile_offset = 16;
		size = SPR_SIZE(1, 1);
		pal = BG_PAL_LINE;
		tx -= 4;
		ty -= 8;
		tile_offset += ((p->anim_frame / 2) % 2) ? 1 : 0;
	}
	else if (p->type == POWERUP_TYPE_HP_2X)
	{
		pal = BG_PAL_LINE;
		tile_offset = 18;
		size = SPR_SIZE(1, 1);
		pal = BG_PAL_LINE;
		tx -= 4;
		ty -= 8;
		tile_offset += ((p->anim_frame / 2) % 2) ? 1 : 0;
	}
	else if (p->type == POWERUP_TYPE_CP)
	{
		pal = ENEMY_PAL_LINE;
		tile_offset = 20;
		size = SPR_SIZE(1, 1);
		tx -= 4;
		ty -= 8;
		tile_offset += ((p->anim_frame / 2) % 2) ? 1 : 0;
	}
	else if (p->type == POWERUP_TYPE_CP_2X)
	{
		pal = ENEMY_PAL_LINE;
		tile_offset = 22;
		size = SPR_SIZE(1, 1);
		tx -= 4;
		ty -= 8;
		tile_offset += ((p->anim_frame / 2) % 2) ? 1 : 0;
	}
	else if (p->type == POWERUP_TYPE_HP_ORB)
	{
		pal = BG_PAL_LINE;
		tile_offset = 0;
		size = SPR_SIZE(2, 2);
		tx -= 7;
		ty -= 14;
		tile_offset += (p->anim_cnt % 2) ? 4 : 0;
	}
	else if (p->type == POWERUP_TYPE_CP_ORB)
	{
		const uint16_t frame_offs = (p->anim_cnt % 2) ? 4 : 0;
		pal = ENEMY_PAL_LINE;
		tile_offset = 8 + frame_offs;
		size = SPR_SIZE(2, 2);
		tx -= 7;
		ty -= 14;
		// Second overlay
		md_spr_put(tx, ty, SPR_ATTR(POWERUP_VRAM_TILE + 48 + frame_offs, 0, 0, BG_PAL_LINE, 0), size);
	}
	else
	{
		return;
	}

	if (tx < -32 || tx > 336 || ty < -32 || ty > 256) return;

	md_spr_put(tx, ty, SPR_ATTR(POWERUP_VRAM_TILE + tile_offset, 0, 0, pal, 0), size);
}

void powerup_bounce(Powerup *p)
{
	p->dx = 0;
	p->dy = (p->dy / 2) + kbounce_dy;
}

void powerup_cube_bounce(Powerup *p)
{
	p->dx = 0;
	p->dy = (p->dy / 2) + kbounce_cube_dy;
}

static inline void newtonian_physics(Powerup *p)
{
	p->dy += kgravity;
	if (p->dy > kmax_dy) p->dy = kmax_dy;
	p->x += p->dx;
	p->y += physics_trunc_fix16(p->dy);
	const int16_t px = FIX32TOINT(p->x);
	const int16_t py = FIX32TOINT(p->y);

	if (p->dy > 0 && map_collision(px, py + 1)) powerup_bounce(p);
	else if (p->dy < 0 && map_collision(px, py - 12)) p->dy = kceiling_dy;
}

static inline void wall_ejection(Powerup *p)
{
	const int16_t px = FIX32TOINT(p->x);
	const int16_t py = FIX32TOINT(p->y);

	if (map_collision(px, py - 12) && map_collision(px, py + 4))
	{
		if (!map_collision(px + 4, py))
		{
			p->x += INTTOFIX32(4);
		}
		else if (!map_collision(px - 4, py))
		{
			p->x -= INTTOFIX32(4);
		}
	}
}

static inline void powerup_get(Powerup *p)
{
	O_Lyle *l = lyle_get();
	Obj *lh = &lyle_get()->head;
	ProgressSlot *prog = progress_get();

	switch (p->type)
	{
		default:
			break;
		case POWERUP_TYPE_HP_2X:
			lh->hp += 1;
			__attribute__((fallthrough));
		case POWERUP_TYPE_HP:
			particle_spawn(p->x + INTTOFIX32(4), p->y - INTTOFIX32(8),
			               PARTICLE_TYPE_SPARKLE);
			sfx_play(SFX_POWERUP_GET, 10);
			lh->hp += 1;
			if (lh->hp > prog->hp_capacity) lh->hp = prog->hp_capacity;
			break;

		case POWERUP_TYPE_CP_2X:
			l->cp += 4;
			__attribute__((fallthrough));
		case POWERUP_TYPE_CP:
			particle_spawn(p->x + INTTOFIX32(4), p->y - INTTOFIX32(8),
			               PARTICLE_TYPE_SPARKLE);
			sfx_play(SFX_POWERUP_GET, 10);
			l->cp += 4;
			if (l->cp > LYLE_MAX_CP) l->cp = LYLE_MAX_CP;
			break;

		case POWERUP_TYPE_CP_ORB:
			SYSTEM_ASSERT(p->orb_id < 16);
			prog->cp_orbs |= (1 << p->orb_id);
			prog->collected_cp_orbs++;
			pause_set_screen(PAUSE_SCREEN_CP_ORB_0 + p->orb_id);
			break;

		case POWERUP_TYPE_HP_ORB:
			SYSTEM_ASSERT(p->orb_id < 16);
			prog->hp_orbs |= (1 << p->orb_id);
			prog->hp_capacity++;
			pause_set_screen(PAUSE_SCREEN_HP_ORB_0 + p->orb_id);
			l->head.hp = prog->hp_capacity;
			break;

		case POWERUP_TYPE_MAP:
			prog->abilities |= ABILITY_MAP;
			pause_set_screen(PAUSE_SCREEN_GET_MAP);
			break;

		case POWERUP_TYPE_LIFT:
			prog->abilities |= ABILITY_LIFT;
			prog->touched_first_cube = 1;
			pause_set_screen(PAUSE_SCREEN_GET_CUBE_LIFT);
			break;

		case POWERUP_TYPE_JUMP:
			prog->abilities |= ABILITY_JUMP;
			pause_set_screen(PAUSE_SCREEN_GET_CUBE_JUMP);
			break;

		case POWERUP_TYPE_PHANTOM:
			prog->abilities |= ABILITY_PHANTOM;
			pause_set_screen(PAUSE_SCREEN_GET_PHANTOM);
			l->cp = LYLE_MAX_CP;
			break;

		case POWERUP_TYPE_KICK:
			prog->abilities |= ABILITY_KICK;
			pause_set_screen(PAUSE_SCREEN_GET_CUBE_KICK);
			break;

		case POWERUP_TYPE_ORANGE:
			prog->abilities |= ABILITY_ORANGE;
			pause_set_screen(PAUSE_SCREEN_GET_ORANGE_CUBE);
			break;
		
	}

	p->active = false;
}

static inline void cube_collision(Powerup *p)
{
	for (uint16_t i = 0; i < ARRAYSIZE(g_cubes); i++)
	{
		Cube *c = &g_cubes[i];
		if (c->status == CUBE_STATUS_NULL) continue;
		if (c->type == CUBE_TYPE_SPAWNER && c->spawn_count != 0) continue;
		if (p->y < c->y + c->top - 1) continue;
		if (p->y > c->y) continue;
		if (p->x + POWERUP_MARGIN < c->x + c->left) continue;
		if (p->x - POWERUP_MARGIN > c->x + c->right) continue;
		powerup_cube_bounce(p);
	}
}

static inline void lyle_collision(Powerup *p)
{
	const O_Lyle *l = lyle_get();
	const Obj *lh = &lyle_get()->head;
	if (lh->hp <= 0) return;
	if (l->holding_cube)
	{
		if (!((p->x + POWERUP_MARGIN < lh->x + lh->left) ||
		      (p->x - POWERUP_MARGIN > lh->x + lh->right) ||
		      (p->y < lh->y + lh->top - INTTOFIX16(16)) ||
		      (p->y - (2 * POWERUP_MARGIN) > lh->y)))
		{
			powerup_cube_bounce(p);
		}
	}
	if (!((p->x + POWERUP_MARGIN < lh->x + lh->left) ||
	      (p->x - POWERUP_MARGIN > lh->x + lh->right) ||
	      (p->y < lh->y + lh->top) ||
	      (p->y - (2 * POWERUP_MARGIN) > lh->y)))
	{
		powerup_get(p);
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
			wall_ejection(p);
			cube_collision(p);
			break;
		default:
			break;
	}

	lyle_collision(p);

	// Check for having gone OOB
	if (p->x > map_get_right() || p->x < 0 ||
	    p->y > map_get_bottom() || p->y < 0)
	{
		p->active = false;
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

	OBJ_SIMPLE_ANIM(p->anim_cnt, p->anim_frame, 8, kanim_speed);
}

void powerup_poll(void)
{
	if (s_hibernate) return;
	uint16_t i = ARRAYSIZE(g_powerups);
	while (i--)
	{
		Powerup *p = &g_powerups[i];
		if (!p->active) continue;
		powerup_run(p);
	}
}

void powerup_init(void)
{
	set_constants();

	powerup_clear();
}

void powerup_clear(void)
{
	uint16_t i = ARRAYSIZE(g_powerups);
	while (i--)
	{
		g_powerups[i].active = false;
	}
}

Powerup *powerup_spawn(fix32_t x, fix32_t y,
                       PowerupType type, int8_t orb_id)
{
	const ProgressSlot *prog = progress_get();
	switch (type)
	{
		default:
			break;
		case POWERUP_TYPE_NONE:
			return NULL;
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
		case POWERUP_TYPE_CP_ORB:
			if (prog->cp_orbs & (1 << orb_id)) return NULL;
			break;
		case POWERUP_TYPE_HP_ORB:
			if (prog->hp_orbs & (1 << orb_id)) return NULL;
			break;
		case POWERUP_TYPE_CP:
		case POWERUP_TYPE_CP_2X:
			if (!(prog->abilities & ABILITY_PHANTOM)) return NULL;
			break;
	}
	uint16_t i = ARRAYSIZE(g_powerups);
	while (i--)
	{
		Powerup *p = &g_powerups[i];
		if (p->active) continue;

		p->active = true;
		p->orb_id = orb_id;
		p->type = type;
		p->x = x;
		p->y = y;
		p->dx = 0;
		p->dy = kspawn_dy;
		p->anim_cnt = 0;
		p->anim_frame = 0;
		
		return p;
	}
	return NULL;
}

void powerup_set_hibernate(bool en)
{
	s_hibernate = en;
}
