#include "obj/particle_manager.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "game.h"

#include "palscale.h"
#include "util/fixed.h"
#include "common.h"

#include "obj/map.h"

static O_ParticleManager *particle_manager;

static uint16_t s_vram_pos;

static int16_t ksparkle_life;
static int16_t kfizzle_life;
static int16_t kexplosion_life;
static int16_t ksand_life;
static int16_t kanim_speed;
static int16_t kanim_speed_explosion;
static int16_t kanim_speed_sand;

static Particle particles[16];

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
	s_constants_set = 1;
}

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_EX_PARTICLES);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
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

	uint16_t attr;

	// Render
	switch (p->type)
	{
		default:
			goto delete_particle;
		case PARTICLE_TYPE_SPARKLE:
			animate(p, kanim_speed);
			attr = SPR_ATTR(s_vram_pos + sparkle_anim[p->anim_frame],
			        0, 0, BG_PAL_LINE, 1);
			md_spr_put(px, py, attr, SPR_SIZE(2, 2));
			break;
		case PARTICLE_TYPE_FIZZLE:
			animate(p, kanim_speed);
			attr = SPR_ATTR(s_vram_pos + fizzle_anim[p->anim_frame],
			        0, 0, BG_PAL_LINE, 1);
			md_spr_put(px, py, attr, SPR_SIZE(2, 2));
			break;
		case PARTICLE_TYPE_FIZZLERED:
			animate(p, kanim_speed);
			attr = SPR_ATTR(s_vram_pos + fizzle_anim[p->anim_frame] + 16,
			        0, 0, LYLE_PAL_LINE, 1);
			md_spr_put(px, py, attr, SPR_SIZE(2, 2));
			break;
		case PARTICLE_TYPE_EXPLOSION:
			animate(p, kanim_speed_explosion);
			if (p->anim_frame == 0 || p->anim_frame == 2)
			{
				px -= 12;
				py -= 12;
				attr = SPR_ATTR(s_vram_pos + 52,
				        0, 0, LYLE_PAL_LINE, 1);
				md_spr_put(px, py, attr, SPR_SIZE(3, 3));
			}
			else if (p->anim_frame == 1 || p->anim_frame == 4)
			{
				px -= 8;
				py -= 8;
				attr = SPR_ATTR(s_vram_pos + 48,
				        0, 0, LYLE_PAL_LINE, 1);
				md_spr_put(px, py, attr, SPR_SIZE(2, 2));
			}
			else
			{
				px -= 16;
				py -= 16;
				attr = SPR_ATTR(s_vram_pos + 61,
				        0, 0, LYLE_PAL_LINE, 1);
				md_spr_put(px, py, attr, SPR_SIZE(4, 4));
			}
			break;
		case PARTICLE_TYPE_SAND:
			animate(p, kanim_speed_sand);
			attr = SPR_ATTR(s_vram_pos + sand_anim[p->anim_frame],
			        0, 0, LYLE_PAL_LINE, 1);
			md_spr_put(px, py, attr, SPR_SIZE(1, 1));
			break;
	}

	return;

delete_particle:
	p->type = PARTICLE_TYPE_NULL;
}

static void main_func(Obj *o)
{
	(void)o;
	uint16_t i = ARRAYSIZE(particles);
	const int16_t map_x = map_get_x_scroll();
	const int16_t map_y = map_get_y_scroll();
	while (i--)
	{
		Particle *p = &particles[i];
		if (p->type == PARTICLE_TYPE_NULL) continue;
		particle_run(p, map_x, map_y);
	}
}

void o_load_particle_manager(Obj *o, uint16_t data)
{
	(void)data;
	SYSTEM_ASSERT(sizeof(O_ParticleManager) <= sizeof(ObjSlot));

	if (particle_manager || s_vram_pos)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}

	particle_manager = (O_ParticleManager *)o;

	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE, 0, 0, 0, 127);
	o->main_func = main_func;

	particle_manager_clear();
}

void o_unload_particle_manager(void)
{
	s_vram_pos = 0;
	particle_manager = 0;
}

void particle_manager_clear(void)
{
	if (!particle_manager) return;
	uint16_t i = ARRAYSIZE(particles);
	while (i--)
	{
		particles[i].type = PARTICLE_TYPE_NULL;
	}
}

Particle *particle_manager_spawn(fix32_t x, fix32_t y, ParticleType type)
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
	};

	Particle *ret = NULL;
	if (!particle_manager) return NULL;
	if (type == PARTICLE_TYPE_NULL) return NULL;
	uint16_t i = ARRAYSIZE(particles);
	uint16_t seek_idx = particle_manager->spawn_start_index;
	while (i--)
	{
		Particle *p = &particles[seek_idx];
		seek_idx++;
		if (seek_idx >= ARRAYSIZE(particles)) seek_idx = 0;
		if (p->type != PARTICLE_TYPE_NULL) continue;

		p->type = type;
		p->x = x - position_offset_tbl[p->type];
		p->y = y - position_offset_tbl[p->type];
		p->dx = 0;
		p->dy = 0;
		switch (type)
		{
			default:
				return NULL;
			case PARTICLE_TYPE_SPARKLE:
				p->life = ksparkle_life;
				break;
			case PARTICLE_TYPE_FIZZLE:
			case PARTICLE_TYPE_FIZZLERED:
				p->life = kfizzle_life;
				break;
			case PARTICLE_TYPE_EXPLOSION:
				p->life = kexplosion_life;
				break;
			case PARTICLE_TYPE_SAND:
				p->life = ksand_life;
				break;
		}

		if (type == PARTICLE_TYPE_SPARKLE)
		{
			p->x += INTTOFIX32((system_rand() % 16) - 8);
			p->y += INTTOFIX32((system_rand() % 16) - 8);
		}
		else
		{
			// TODO: These don't take palscale into account.
			p->dy = INTTOFIX16(((system_rand() % 64) - 32)) / 16;
			p->dx = INTTOFIX16(((system_rand() % 64) - 32)) / 16;
			// Don't let dx/dy be zero.
			if (p->dx == 0) p->dx = INTTOFIX16(2);
			if (p->dy == 0) p->dy = INTTOFIX16(2);

			if (type == PARTICLE_TYPE_SAND)
			{
				p->dy -= INTTOFIX32(2);
			}

			p->dx /= 2;
			p->dy /= 2;
		}

		p->anim_cnt = 0;
		p->anim_frame = 0;
		ret = p;
		break;
	}
	particle_manager->spawn_start_index = seek_idx;
	return ret;
}
