#include "obj/particle_manager.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"

#include "palscale.h"
#include "util/fixed.h"
#include "cube.h"

#include "obj/map.h"

static uint16_t vram_pos;

static int16_t ksparkle_life;
static int16_t kfizzle_life;
static int16_t kexplosion_life;
static int16_t ksand_life;

static Particle particles[16];

static void set_constants(void)
{
	static int16_t constants_set;
	if (constants_set) return;

	ksparkle_life = PALSCALE_DURATION(20);
	kfizzle_life = PALSCALE_DURATION(16);
	kexplosion_life = PALSCALE_DURATION(21);
	ksand_life = PALSCALE_DURATION(20);

	constants_set = 1;
}

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g = gfx_get(GFX_EX_PARTICLES);
	vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void particle_run(Particle *p)
{
	// Move
	if (p->type != PARTICLE_TYPE_SPARKLE)
	{
		p->x += p->dx;
		p->y += p->dy;
	}

	// Animate
	int8_t center;
	uint8_t size;
	uint8_t pal;
	uint16_t tile_offset;

	switch (p->type)
	{
		default:
			return;
		case PARTICLE_TYPE_SPARKLE:
			center = 8;
			size = SPR_SIZE(2, 2);
			pal = BG_PAL_LINE;
			if (p->life > 16) tile_offset = 12;
			else if (p->life > 12) tile_offset = 0;
			else if (p->life > 8) tile_offset = 4;
			else if (p->life > 4) tile_offset = 8;
			else tile_offset = 12;
			break;
		case PARTICLE_TYPE_FIZZLE:
		case PARTICLE_TYPE_FIZZLERED:
			center = 8;
			size = SPR_SIZE(2, 2);
			if (p->life > 12) tile_offset = 16;
			else if (p->life > 8) tile_offset = 20;
			else if (p->life > 4) tile_offset = 24;
			else tile_offset = 28;
			if (p->type == PARTICLE_TYPE_FIZZLERED)
			{
				tile_offset += 16;
				pal = LYLE_PAL_LINE;
			}
			else
			{
				pal = BG_PAL_LINE;
			}
			break;
		case PARTICLE_TYPE_EXPLOSION:
			pal = LYLE_PAL_LINE;
			if ((p->life > 18) || (p->life < 16 && p->life > 12))
			{
				center = 12;
				size = SPR_SIZE(3, 3);
				tile_offset = 52;
			}
			else if ((p->life > 15 && p->life < 19) || (p->life > 6 && p->life < 10))
			{
				center = 8;
				size = SPR_SIZE(2, 2);
				tile_offset = 48;
			}
			else
			{
				center = 16;
				size = SPR_SIZE(4, 4);
				tile_offset = 61;
			}
			break;
		case PARTICLE_TYPE_SAND:
			center = 4;
			size = SPR_SIZE(1, 1);
			pal = LYLE_PAL_LINE;
			if (p->life >= 8) tile_offset = 77;
			else if (p->life >= 6) tile_offset = 78;
			else if (p->life >= 4) tile_offset = 77;
			else if (p->life >= 2) tile_offset = 79;
			else tile_offset = 80;
			break;
	}

	const int16_t tx = FIX32TOINT(p->x) - center - map_get_x_scroll();
	const int16_t ty = FIX32TOINT(p->y) - center - map_get_y_scroll();
	if (tx < -32 || tx > 336 || ty < -32 || ty > 256) return;

	spr_put(tx, ty, SPR_ATTR(vram_pos + tile_offset, 0, 0, pal, 0), size);

	p->life--;
	if (p->life <= 0) p->type = PARTICLE_TYPE_NULL;
}

static void main_func(Obj *o)
{
	(void)o;
	uint16_t i = ARRAYSIZE(particles);
	while (i--)
	{
		Particle *p = &particles[i];
		if (p->type == PARTICLE_TYPE_NULL) continue;
		particle_run(p);
	}
}

void o_load_particle_manager(Obj *o, uint16_t data)
{
	(void)data;
	SYSTEM_ASSERT(sizeof(O_ParticleManager) <= sizeof(ObjSlot));

	// If VRAM is already loaded, then a particle manager already is present.
	if (vram_pos)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}

	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE, 0, 0, 0, 127);
	o->main_func = main_func;

	particle_manager_clear();
}

void o_unload_particle_manager(void)
{
	vram_pos = 0;
}

void particle_manager_clear(void)
{
	uint16_t i = ARRAYSIZE(particles);
	while (i--)
	{
		particles[i].type = PARTICLE_TYPE_NULL;
	}
}

void particle_manager_spawn(int32_t x, int32_t y, ParticleType type)
{
	if (type == PARTICLE_TYPE_NULL) return;
	uint16_t i = ARRAYSIZE(particles);
	while (i--)
	{
		Particle *p = &particles[i];
		if (p->type != PARTICLE_TYPE_NULL) continue;

		p->type = type;
		p->x = x;
		p->y = y;
		p->dx = 0;
		p->dy = 0;
		switch (type)
		{
			default:
				return;
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
			p->dy = INTTOFIX16((system_rand() % 4) - 2);
			p->dx = INTTOFIX16((system_rand() % 4) - 2);
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
		return;
	}
}
