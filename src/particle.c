#include "particle.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "game.h"

#include "palscale.h"
#include "util/fixed.h"


#include "map.h"

static Particle s_particles[16];

#define PARTICLE_VRAM_TILE (PARTICLE_VRAM_POSITION / 32)

static int16_t ksparkle_life;
static int16_t kfizzle_life;
static int16_t kexplosion_life;
static int16_t ksand_life;
static int16_t kanim_speed;
static int16_t kanim_speed_explosion;
static int16_t kanim_speed_sand;

static fix16_t kcrumbly_gravity;

static int16_t s_spawn_start_index;

static uint16_t s_hibernate;

static uint16_t kpwave_life;

static void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	ksparkle_life = PALSCALE_DURATION(14);
	kfizzle_life = PALSCALE_DURATION(12);
	kexplosion_life = PALSCALE_DURATION(21);
	kanim_speed = PALSCALE_DURATION(3.4);
	kanim_speed_explosion = PALSCALE_DURATION(3.4);
	kanim_speed_sand = PALSCALE_DURATION(3.3);
	ksand_life = kanim_speed_sand * 5;
	kpwave_life = PALSCALE_DURATION(86);
	kcrumbly_gravity = INTTOFIX16(PALSCALE_1ST(0.1190476));
	s_constants_set = 1;
}

static const uint16_t sparkle_anim[] =
{
	12, 0, 4, 8, 12, 12,
};

static const uint16_t fizzle_anim[] =
{
	16, 20, 24, 28, 28, 28, 28
};

static const uint16_t sand_anim[] =
{
	77, 78, 77, 79, 80, 80, 80, 80, 80
};

static inline void animate(Particle *p, int16_t speed_check)
{
	p->anim_cnt++;
	if (p->anim_cnt >= speed_check)
	{
		p->anim_cnt = 0;
		p->anim_frame++;
	}
}

static inline void particle_run(Particle *p, int16_t map_x, int16_t map_y)
{
	// Delete off-screen particles.
	const int16_t margin = 16;
	int16_t px = FIX32TOINT(p->x) - map_x;
	int16_t py = FIX32TOINT(p->y) - map_y;
	if (py < -margin) goto delete_particle;
	if (py > GAME_SCREEN_H_PIXELS + margin) goto delete_particle;
	if (px < -margin) goto delete_particle;
	if (px > GAME_SCREEN_W_PIXELS + margin) goto delete_particle;

	p->x += p->dx;
	p->y += p->dy;

	p->life--;
	if (p->life <= 0) goto delete_particle;

	// Render
	switch (p->type)
	{
		default:
			goto delete_particle;
		case PARTICLE_TYPE_SPARKLE:
			animate(p, kanim_speed);
			p->spr.attr = SPR_ATTR(PARTICLE_VRAM_TILE + sparkle_anim[p->anim_frame],
			                       0, 0, BG_PAL_LINE, 1);
			break;
		case PARTICLE_TYPE_FIZZLE:
			animate(p, kanim_speed);
			p->spr.attr = SPR_ATTR(PARTICLE_VRAM_TILE + fizzle_anim[p->anim_frame],
			                       0, 0, BG_PAL_LINE, 1);
			break;
		case PARTICLE_TYPE_FIZZLERED:
			animate(p, kanim_speed);
			p->spr.attr = SPR_ATTR(PARTICLE_VRAM_TILE + fizzle_anim[p->anim_frame] + 16,
			                       0, 0, LYLE_PAL_LINE, 1);
			break;
		case PARTICLE_TYPE_EXPLOSION:
			animate(p, kanim_speed_explosion);
			if (p->anim_frame == 0 || p->anim_frame == 2)
			{
				px -= 12;
				py -= 12;
				p->spr.attr = SPR_ATTR(PARTICLE_VRAM_TILE + 52,
				                       0, 0, LYLE_PAL_LINE, 1);
				p->spr.size = SPR_SIZE(3, 3);
			}
			else if (p->anim_frame == 1 || p->anim_frame == 4)
			{
				px -= 8;
				py -= 8;
				p->spr.attr = SPR_ATTR(PARTICLE_VRAM_TILE + 48,
				                       0, 0, LYLE_PAL_LINE, 1);
				p->spr.size = SPR_SIZE(2, 2);
			}
			else
			{
				px -= 16;
				py -= 16;
				p->spr.attr = SPR_ATTR(PARTICLE_VRAM_TILE + 61,
				                       0, 0, LYLE_PAL_LINE, 1);
				p->spr.size = SPR_SIZE(4, 4);
			}
			break;
		case PARTICLE_TYPE_SAND:
			animate(p, kanim_speed_sand);
			p->spr.attr = SPR_ATTR(PARTICLE_VRAM_TILE + sand_anim[p->anim_frame],
			                       0, 0, LYLE_PAL_LINE, 1);
			break;
		case PARTICLE_TYPE_CRUMBLY:
			p->dy += kcrumbly_gravity;
			break;

		case PARTICLE_TYPE_PSYCHOWAVE:
			if (p->life & 1) return;  // Flicker
			// Two-sided sprite; the opposite side is drawn here.
			p->spr.attr ^= SPR_ATTR(0, 1, 0, 0, 0);
			p->spr.x = px + 24;
			p->spr.y = py;
			// The params are then put back and the normal draw routine handles the other half.
			md_spr_put_st(&p->spr);
			p->spr.attr ^= SPR_ATTR(0, 1, 0, 0, 0);
			break;

		case PARTICLE_TYPE_Z:
			animate(p, kanim_speed * 2);
			if (p->anim_frame >= 4)
			{
				p->anim_frame = 0;
			}
			if (p->anim_cnt == 0)
			{
				p->x += (p->anim_frame < 2) ? INTTOFIX32(1) : INTTOFIX32(-1);
			}
			if ((p->life < PALSCALE_DURATION((6.0 / 5.0) * 30)) && (p->life % 2 == 0))
			{
				return;  // flicker
			}
			break;
			
	}

	p->spr.x = px;
	p->spr.y = py;
	md_spr_put_st(&p->spr);

	return;

delete_particle:
	p->type = PARTICLE_TYPE_NULL;
}

void particle_poll(void)
{
	if (s_hibernate) return;
	uint16_t i = ARRAYSIZE(s_particles);
	const int16_t map_x = map_get_x_scroll();
	const int16_t map_y = map_get_y_scroll();
	while (i--)
	{
		Particle *p = &s_particles[i];
		if (p->type == PARTICLE_TYPE_NULL) continue;
		particle_run(p, map_x, map_y);
	}
}

void particle_init(void)
{
	s_hibernate = false;

	set_constants();

	particle_clear();
}


void particle_clear(void)
{
	uint16_t i = ARRAYSIZE(s_particles);
	while (i--)
	{
		s_particles[i].type = PARTICLE_TYPE_NULL;
	}
}

static void set_standard_velocity(Particle *p)
{
	// TODO: These don't take palscale into account.
	p->dy = INTTOFIX16(((system_rand() % 64) - 32)) / 16;
	p->dx = INTTOFIX16(((system_rand() % 64) - 32)) / 16;
	// Don't let dx/dy be zero.
	if (p->dx == 0) p->dx = INTTOFIX16(2);
	if (p->dy == 0) p->dy = INTTOFIX16(2);

	p->dx /= 2;
}

Particle *particle_spawn(fix32_t x, fix32_t y, ParticleType type)
{
	// Particles are offset by their width/height divided by two, so as to avoid
	// applying offsets at runtime while rendering.
	static const fix32_t position_offset_tbl[] =
	{
		[PARTICLE_TYPE_SPARKLE] = INTTOFIX32(8),
		[PARTICLE_TYPE_FIZZLE] = INTTOFIX32(8),
		[PARTICLE_TYPE_FIZZLERED] = INTTOFIX32(8),
		[PARTICLE_TYPE_EXPLOSION] = 0,  // Handled at runtime.
		[PARTICLE_TYPE_SAND] = INTTOFIX32(4),
		[PARTICLE_TYPE_CRUMBLY] = INTTOFIX32(4),
		[PARTICLE_TYPE_PSYCHOWAVE] = 0,  // Handled at runtime.
		[PARTICLE_TYPE_Z] = INTTOFIX32(4),
	};

	Particle *ret = NULL;
	if (type == PARTICLE_TYPE_NULL) return NULL;
	uint16_t i = ARRAYSIZE(s_particles);
	uint16_t seek_idx = s_spawn_start_index;
	while (i--)
	{
		Particle *p = &s_particles[seek_idx];
		seek_idx++;
		if (seek_idx >= ARRAYSIZE(s_particles)) seek_idx = 0;
		if (p->type != PARTICLE_TYPE_NULL) continue;

		p->type = type;
		p->x = x - position_offset_tbl[p->type];
		p->y = y - position_offset_tbl[p->type];
		p->dx = 0;
		p->dy = 0;
		p->spr.size = SPR_SIZE(2, 2);
		switch (type)
		{
			default:
				return NULL;
			case PARTICLE_TYPE_SPARKLE:
				p->life = ksparkle_life;
				p->x += INTTOFIX32((system_rand() % 16) - 8);
				p->y += INTTOFIX32((system_rand() % 16) - 8);
				break;

			case PARTICLE_TYPE_FIZZLE:
			case PARTICLE_TYPE_FIZZLERED:
				p->life = kfizzle_life;
				set_standard_velocity(p);
				break;

			case PARTICLE_TYPE_EXPLOSION:
				p->life = kexplosion_life;
				set_standard_velocity(p);
				break;

			case PARTICLE_TYPE_SAND:
				p->spr.size = SPR_SIZE(1, 1);
				p->life = ksand_life;
				set_standard_velocity(p);
				p->dy -= INTTOFIX32(2);
				break;

			case PARTICLE_TYPE_CRUMBLY:
				p->life = kfizzle_life * 4;
				p->spr.size = SPR_SIZE(1, 1);
				p->spr.attr = SPR_ATTR(0x0060, 0, 0, MAP_PAL_LINE, 1);
				break;

			case PARTICLE_TYPE_PSYCHOWAVE:
				p->life = kpwave_life;
				p->spr.size = SPR_SIZE(3, 1);
				p->spr.attr = SPR_ATTR(PARTICLE_VRAM_TILE + 81, 0, 0, LYLE_PAL_LINE, 0);
				p->x -= INTTOFIX32(24);
				p->y -= INTTOFIX32(4);
				p->dy = INTTOFIX16(PALSCALE_1ST(1.0 * 5.0/6.0));
				break;

			case PARTICLE_TYPE_Z:
				p->life = PALSCALE_DURATION((6.0 / 5.0) * 60);
				p->spr.size = SPR_SIZE(1, 1);
				p->spr.attr = SPR_ATTR(PARTICLE_VRAM_TILE + 84, 0, 0, LYLE_PAL_LINE, 1);
				p->dx = 0;
				p->dy = INTTOFIX16(PALSCALE_1ST(-0.25));
				break;
		}

		p->anim_cnt = 0;
		p->anim_frame = 0;
		ret = p;
		break;
	}
	s_spawn_start_index = seek_idx;
	return ret;
}

void particle_set_hibernate(uint16_t en)
{
	s_hibernate = en;
}
