#include "obj/projectile_manager.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"

#include "palscale.h"
#include "util/fixed.h"
#include "obj/particle_manager.h"
#include "common.h"
#include "obj/map.h"
#include "obj/lyle.h"

#include "trig.h"

#define PROJECTILE_MARGIN INTTOFIX32(3)

static O_ProjectileManager *projectile_manager;
static Projectile projectiles[10];

static int16_t kparticle_rate;
static int16_t kdeathorb_flash_time;
static int16_t kdeathorb_death_time;
static int16_t kflicker_2f_anim_speed;

static fix16_t kdeathorb2_dy_table[4];
static fix16_t kdeathorb_flip_dy;
static fix16_t kdeathorb_ddy;
static fix16_t kdeathorb2_ddy;

static uint16_t vram_pos;

static void set_constants(void)
{
	static int16_t constants_set;
	if (constants_set) return;

	kparticle_rate = PALSCALE_DURATION(8);
	kdeathorb_flash_time = PALSCALE_DURATION(109);
	kdeathorb_death_time = PALSCALE_DURATION(181);
	kflicker_2f_anim_speed = PALSCALE_DURATION(2.4);

	kdeathorb2_dy_table[0] = INTTOFIX16(PALSCALE_1ST(-5.2083));
	kdeathorb2_dy_table[1] = INTTOFIX16(PALSCALE_1ST(-5.0));
	kdeathorb2_dy_table[2] = INTTOFIX16(PALSCALE_1ST(-5.0));
	kdeathorb2_dy_table[3] = INTTOFIX16(PALSCALE_1ST(-4.54));
	kdeathorb_flip_dy = INTTOFIX16(PALSCALE_1ST(3.0));
	kdeathorb_ddy = INTTOFIX16(PALSCALE_2ND(0.194444444448));
	kdeathorb2_ddy = INTTOFIX16(PALSCALE_2ND(0.17361111111113));

	constants_set = 1;
}

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g = gfx_get(GFX_EX_PROJECTILES);
	vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void projectile_render(Projectile *p)
{
	uint8_t size;
	uint8_t pal;
	uint16_t tile_offset;

	int16_t tx = FIX32TOINT(p->x) - 4 - map_get_x_scroll();
	int16_t ty = FIX32TOINT(p->y) - 4 - map_get_y_scroll();
	if (tx < -32 || tx > 336 || ty < -32 || ty > 256) return;

	// Death orb flickers as it approaches end-of-life.
	if (p->type == PROJECTILE_TYPE_DEATHORB &&
	    p->frames_alive > kdeathorb_flash_time &&
	    projectile_manager->flicker_4f_anim > 1)
	{
		return;
	}

	switch (p->type)
	{
		default:
			return;
		case PROJECTILE_TYPE_BALL:
		case PROJECTILE_TYPE_BALL2:
			size = SPR_SIZE(1, 1);
			pal = LYLE_PAL_LINE;
			tile_offset = projectile_manager->flicker_2f_anim ? 1 : 0;
			break;
		case PROJECTILE_TYPE_SPIKE:
			size = SPR_SIZE(1, 1);
			pal = BG_PAL_LINE;
			tile_offset = 2;
			break;
		case PROJECTILE_TYPE_SPARK:
			ty -= 3;
			size = SPR_SIZE(1, 1);
			pal = LYLE_PAL_LINE;
			tile_offset = projectile_manager->flicker_2f_anim ? 4 : 3;
			break;
		case PROJECTILE_TYPE_DEATHORB:
			ty -= 4;
			tx -= 4;
			size = SPR_SIZE(2, 2);
			pal = projectile_manager->flicker_2f_anim ?
			      LYLE_PAL_LINE : ENEMY_PAL_LINE;
			tile_offset = projectile_manager->flicker_2f_anim ? 5 : 9;
			break;
		case PROJECTILE_TYPE_DEATHORB2:
			ty -= 4;
			tx -= 4;
			size = SPR_SIZE(2, 2);
			pal = LYLE_PAL_LINE;
			tile_offset = 13 + (4 * projectile_manager->flicker_4f_anim);
			break;

	}

	spr_put(tx, ty, SPR_ATTR(vram_pos + tile_offset, 0, 0, pal, 0), size);
}

static inline int16_t basic_collision(Projectile *p)
{
	const int16_t px = FIX32TOINT(p->x);
	const int16_t py = FIX32TOINT(p->y);

	if (map_collision(px, py))
	{
		p->type = PROJECTILE_TYPE_NULL;
		return 1;
	}
	return 0;
}

static inline int16_t ball_collision(Projectile *p)
{
	const int16_t px = FIX32TOINT(p->x);
	const int16_t py = FIX32TOINT(p->y);

	if (map_collision(px, py))
	{
		if (p->dy > 0)
		{
			p->dy = p->dy * -1;
		}
		else
		{
			p->type = PROJECTILE_TYPE_NULL;
			return 1;
		}

	}
	return 0;
}

static inline int16_t deathorb2_collision(Projectile *p)
{
	const int16_t kx_check = 9;
	const int16_t ky_check = 8;
	const int16_t px = FIX32TOINT(p->x);
	const int16_t py = FIX32TOINT(p->y);

	if ((px > 304) || (px <= 16) ||
	    (p->dx > 0 && map_collision(px + kx_check, py - ky_check)) ||
	    (p->dx < 0 && map_collision(px - kx_check, py - ky_check)))
	{
		p->type = PROJECTILE_TYPE_NULL;
		return 1;
	}
	else if (p->dy > 0 && map_collision(px, py + ky_check))
	{

		p->dy = kdeathorb2_dy_table[system_rand() %
		                            ARRAYSIZE(kdeathorb2_dy_table)];
	}

	return 0;
}

static inline void projectile_run(Projectile *p)
{
	// Projectile lifetime
	p->frames_alive++;
	if (p->type == PROJECTILE_TYPE_DEATHORB &&
	    p->frames_alive >= kdeathorb_death_time)
	{
		p->type = PROJECTILE_TYPE_NULL;
		return;
	}

	// Projectiles off-screen get cropped.
	const int16_t tx = FIX32TOINT(p->x) - map_get_x_scroll();
	const int16_t ty = FIX32TOINT(p->y) - map_get_y_scroll();
	if (tx < -64 || tx > 384 || ty < -64 || ty > 304)
	{
		p->type = PROJECTILE_TYPE_NULL;
		return;
	}

	// Move
	p->x += p->dx;
	p->y += p->dy;
	if (p->type == PROJECTILE_TYPE_DEATHORB)
	{
		if (p->moving_up)
		{
			p->dy -= kdeathorb_ddy;
			if (p->dy <= -kdeathorb_flip_dy) p->moving_up = 0;
		}
		else
		{
			p->dy += kdeathorb_ddy;
			if (p->dy >= kdeathorb_flip_dy) p->moving_up = 1;
		}
	}
	else if (p->type == PROJECTILE_TYPE_DEATHORB2)
	{
		p->dy += kdeathorb2_ddy;
	}

	// Check for having gone OOB
	if (p->x > map_get_right() || p->x < 0 ||
	    p->y > map_get_bottom() || p->y < 0)
	{
		p->type = PROJECTILE_TYPE_NULL;
		return;
	}

	const Obj *lh = &lyle_get()->head;

	// Check for collision with player
	if (!((p->x + PROJECTILE_MARGIN < lh->x + lh->left) ||
	      (p->x - PROJECTILE_MARGIN > lh->x + lh->right) ||
	      (p->y < lh->y + lh->top) ||
	      (p->y - (2 * PROJECTILE_MARGIN) > lh->y)))
	{
		lyle_get_hurt();
		p->type = PROJECTILE_TYPE_NULL;
		return;
	}

	// Background collisions. Collision functions return 1 if the projectile
	// has been deactivated as a result of a collision.
	switch (p->type)
	{
		default:
			if (basic_collision(p)) return;
			break;
		case PROJECTILE_TYPE_BALL:
			if (ball_collision(p)) return;
			break;
		case PROJECTILE_TYPE_DEATHORB2:
			if (deathorb2_collision(p)) return;
			break;
	}

	// Particle effects
	if (p->type != PROJECTILE_TYPE_SPARK &&
	    projectile_manager->particle_cnt == 0)
	{
		particle_manager_spawn(p->x, p->y, PARTICLE_TYPE_SPARKLE);
	}

	// Render
	projectile_render(p);
}

static void main_func(Obj *o)
{
	system_profile(PALRGB(7, 0, 7));
	O_ProjectileManager *p = (O_ProjectileManager *)o;

	p->particle_cnt++;
	if (p->particle_cnt >= kparticle_rate) p->particle_cnt = 0;
	p->flicker_cnt++;
	if (p->flicker_cnt >= kflicker_2f_anim_speed)
	{
		p->flicker_cnt = 0;
		p->flicker_2f_anim ^= 1;
		p->flicker_4f_anim++;
		if (p->flicker_4f_anim > 3) p->flicker_4f_anim = 0;
	}

	uint16_t i = ARRAYSIZE(projectiles);
	while (i--)
	{
		system_profile(PALRGB(3, i % 2 ? 0 : 4, 3));
		Projectile *p = &projectiles[i];
		if (p->type == PROJECTILE_TYPE_NULL) continue;
		projectile_run(p);
	}
	system_profile(PALRGB(0, 0, 0));
}

void o_load_projectile_manager(Obj *o, uint16_t data)
{
	(void)data;
	SYSTEM_ASSERT(sizeof(O_ProjectileManager) <= sizeof(ObjSlot));

	if (projectile_manager || vram_pos)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}

	projectile_manager = (O_ProjectileManager *)o;

	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE, 0, 0, 0, 127);
	o->main_func = main_func;

	projectile_manager_clear();
}

void o_unload_projectile_manager(void)
{
	vram_pos = 0;
	projectile_manager = NULL;
}

void projectile_manager_clear(void)
{
	if (!projectile_manager) return;
	uint16_t i = ARRAYSIZE(projectiles);
	while (i--)
	{
		projectiles[i].type = PROJECTILE_TYPE_NULL;
	}
}

static Projectile *find_slot(void)
{
	if (!projectile_manager) return NULL;
	for (uint16_t i = 0; i < ARRAYSIZE(projectiles); i++)
	{
		Projectile *p = &projectiles[i];
		if (p->type == PROJECTILE_TYPE_NULL) return p;
	}
	return NULL;
}

Projectile *projectile_manager_shoot(fix32_t x, fix32_t y, ProjectileType type,
                                     fix16_t dx, fix16_t dy)
{
	if (type == PROJECTILE_TYPE_NULL) return NULL;
	Projectile *p = find_slot();
	if (p)
	{
		p->type = type;
		p->x = x;
		p->y = y;
		p->dx = dx;
		p->dy = dy;
		p->moving_up = 1;
		p->frames_alive = 0;
	}
	return p;
}

Projectile *projectile_manager_shoot_angle(fix32_t x, fix32_t y, ProjectileType type,
                                           uint8_t angle, fix16_t speed)
{
	const fix16_t dy = -FIX16MUL(speed, fix_sin(angle));
	const fix16_t dx = FIX16MUL(speed, fix_cos(angle));
	return projectile_manager_shoot(x, y, type, dx, dy);
}

Projectile *projectile_manager_shoot_at(fix32_t x, fix32_t y,
                                        ProjectileType type,
                                        fix32_t tx, fix32_t ty, fix16_t speed)
{
	const fix32_t rise = ty - y;
	const fix32_t run = tx - x;
//	const fix32_t rise = -8;
//	const fix32_t run = 11000;
	if (run == 0)
	{
		return projectile_manager_shoot_angle(x, y, type, rise < 0 ? DEGTOUINT8(270) : DEGTOUINT8(90), speed);
	}
	else
	{
		if (run >= 0)
		{
			if (rise >= 0)
			{
				const fix32_t ratio = FIX32DIV(rise, run);
				const uint8_t angle = fix_atan(ratio);
				return projectile_manager_shoot_angle(x, y, type, angle, speed);
			}
			else
			{
				const fix32_t ratio = FIX32DIV(-rise, run);
				const uint8_t angle = fix_atan(ratio);
				return projectile_manager_shoot_angle(x, y, type, DEGTOUINT8(270) + (DEGTOUINT8(90) - angle), speed);
			}
		}
		else
		{
			if (rise >= 0)
			{
				const fix32_t ratio = FIX32DIV(rise, -run);
				const uint8_t angle = fix_atan(ratio);
				return projectile_manager_shoot_angle(x, y, type, DEGTOUINT8(180) - angle, speed);
			}
			else
			{
				const fix32_t ratio = FIX32DIV(-rise, -run);
				const uint8_t angle = fix_atan(ratio);
				return projectile_manager_shoot_angle(x, y, type, DEGTOUINT8(180) + angle, speed);
			}
		}
	}
	return NULL;
}
