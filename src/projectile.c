#include "projectile.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"

#include "palscale.h"
#include "util/fixed.h"
#include "particle.h"

#include "map.h"
#include "lyle.h"

#include "util/trig.h"

#define PROJECTILE_MARGIN INTTOFIX32(3)

#define PROJECTILE_VRAM_TILE (PROJECTILE_VRAM_POSITION / 32)

static Projectile s_projectiles[10];

static int16_t kparticle_rate;
static int16_t kdeathorb_flash_time;
static int16_t kdeathorb_death_time;
static int16_t kflicker_2f_anim_speed;

static fix16_t kdeathorb2_dy_table[4];
static fix16_t kdeathorb_flip_dy;
static fix16_t kdeathorb_ddy;
static fix16_t kdeathorb2_ddy;

static int16_t s_particle_cnt;
static int16_t s_flicker_cnt;
static int8_t s_flicker_2f_anim;
static int8_t s_flicker_4f_anim;

static bool s_hibernate;

static void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

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

	s_constants_set = true;
}

static inline void projectile_render(Projectile *p)
{
	uint8_t size;
	uint8_t pal;
	uint16_t tile_offset;

	int16_t tx = FIX32TOINT(p->x) - 4 - map_get_x_scroll();
	int16_t ty = FIX32TOINT(p->y) - 4 - map_get_y_scroll();
	int8_t xflip = 0;
	if (tx < -32 || tx > 336 || ty < -32 || ty > 256) return;

	// Death orb flickers as it approaches end-of-life.
	if (p->type == PROJECTILE_TYPE_DEATHORB &&
	    p->frames_alive > kdeathorb_flash_time &&
	    s_flicker_4f_anim > 1)
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
			tile_offset = s_flicker_2f_anim ? 1 : 0;
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
			tile_offset = s_flicker_2f_anim ? 4 : 3;
			break;
		case PROJECTILE_TYPE_DEATHORB:
			ty -= 4;
			tx -= 4;
			size = SPR_SIZE(2, 2);
			pal = s_flicker_2f_anim ?
			      LYLE_PAL_LINE : ENEMY_PAL_LINE;
			tile_offset = s_flicker_2f_anim ? 5 : 9;
			break;
		case PROJECTILE_TYPE_DEATHORB2:
			ty -= 4;
			tx -= 4;
			size = SPR_SIZE(2, 2);
			pal = LYLE_PAL_LINE;
			if (p->dx < 0) xflip = 1;
			tile_offset = 13 + (4 * s_flicker_4f_anim);
			break;
	}

	md_spr_put(tx, ty, SPR_ATTR(PROJECTILE_VRAM_TILE + tile_offset, xflip, 0, pal, 0), size);
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

static inline void bounds_crop(Projectile *p)
{
	const int16_t tx = FIX32TOINT(p->x) - map_get_x_scroll();
	const int16_t ty = FIX32TOINT(p->y) - map_get_y_scroll();
	if (tx < -64 || tx > 384 || ty < -64 || ty > 304)
	{
		p->type = PROJECTILE_TYPE_NULL;
		return;
	}
	if (p->x > map_get_right() || p->x < 0 ||
	    p->y > map_get_bottom() || p->y < 0)
	{
		p->type = PROJECTILE_TYPE_NULL;
		return;
	}
}

static inline void lyle_touch(Projectile *p)
{
	const Obj *lh = &lyle_get()->head;
	if (!((p->x + PROJECTILE_MARGIN < lh->x + lh->left) ||
	      (p->x - PROJECTILE_MARGIN > lh->x + lh->right) ||
	      (p->y < lh->y + lh->top) ||
	      (p->y - (2 * PROJECTILE_MARGIN) > lh->y)))
	{
		lyle_get_hurt(false);
		p->type = PROJECTILE_TYPE_NULL;
	}
}

static inline void particle_effects(Projectile *p)
{
	if (s_particle_cnt == 0)
	{
		particle_spawn(p->x, p->y, PARTICLE_TYPE_SPARKLE);
	}
}

static inline void projectile_run(Projectile *p)
{
	p->frames_alive++;
	p->x += p->dx;
	p->y += p->dy;
	bounds_crop(p);
	switch (p->type)
	{
		case PROJECTILE_TYPE_NULL:
			if (basic_collision(p)) return;
			lyle_touch(p);
			particle_effects(p);
			return;
		case PROJECTILE_TYPE_BALL:
			if (ball_collision(p)) return;
			lyle_touch(p);
			particle_effects(p);
			break;
		case PROJECTILE_TYPE_BALL2:
			if (basic_collision(p)) return;
			lyle_touch(p);
			particle_effects(p);
			break;
		case PROJECTILE_TYPE_SPIKE:
			if (basic_collision(p)) return;
			lyle_touch(p);
			particle_effects(p);
			break;
		case PROJECTILE_TYPE_SPARK:
			if (basic_collision(p)) return;
			lyle_touch(p);
			break;
		case PROJECTILE_TYPE_DEATHORB:
			if (basic_collision(p)) return;
			if (p->frames_alive >= kdeathorb_death_time)
			{
				p->type = PROJECTILE_TYPE_NULL;
				return;
			}
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
			lyle_touch(p);
			particle_effects(p);

			break;
		case PROJECTILE_TYPE_DEATHORB2:
			p->dy += kdeathorb2_ddy;
			if (deathorb2_collision(p)) return;
			lyle_touch(p);
			particle_effects(p);
			break;
	}

	projectile_render(p);
}

void projectile_poll(void)
{
	if (s_hibernate) return;
	// Animation counters
	s_particle_cnt++;
	if (s_particle_cnt >= kparticle_rate) s_particle_cnt = 0;
	s_flicker_cnt++;
	if (s_flicker_cnt >= kflicker_2f_anim_speed)
	{
		s_flicker_cnt = 0;
		s_flicker_2f_anim ^= 1;
		s_flicker_4f_anim++;
		if (s_flicker_4f_anim > 3) s_flicker_4f_anim = 0;
	}

	// Run logic for each projectile in the list
	uint16_t i = ARRAYSIZE(s_projectiles);
	while (i--)
	{
		Projectile *p = &s_projectiles[i];
		if (p->type == PROJECTILE_TYPE_NULL) continue;
		projectile_run(p);
	}
}

void projectile_init(void)
{
	set_constants();

	projectile_clear();
}

void projectile_clear(void)
{
	uint16_t i = ARRAYSIZE(s_projectiles);
	while (i--)
	{
		s_projectiles[i].type = PROJECTILE_TYPE_NULL;
	}
}

static Projectile *find_slot(void)
{
	for (uint16_t i = 0; i < ARRAYSIZE(s_projectiles); i++)
	{
		Projectile *p = &s_projectiles[i];
		if (p->type == PROJECTILE_TYPE_NULL) return p;
	}
	return NULL;
}

Projectile *projectile_shoot(fix32_t x, fix32_t y, ProjectileType type,
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

Projectile *projectile_shoot_angle(fix32_t x, fix32_t y, ProjectileType type,
                                   uint8_t angle, fix16_t speed)
{
	const fix16_t dy = -FIX16MUL(speed, trig_sin(angle));
	const fix16_t dx = FIX16MUL(speed, trig_cos(angle));
	return projectile_shoot(x, y, type, dx, dy);
}

Projectile *projectile_shoot_at(fix32_t x, fix32_t y,
                                ProjectileType type,
                                fix32_t tx, fix32_t ty, fix16_t speed)
{
	const fix32_t delta_y = (ty - y);
	const fix32_t delta_x = tx - x;
	return projectile_shoot_angle(x, y, type, trig_atan(delta_y, delta_x), speed);
}

void projectile_set_hibernate(bool en)
{
	s_hibernate = en;
}
