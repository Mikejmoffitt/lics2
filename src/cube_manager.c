#include "cube_manager.h"

#include <stdlib.h>
#include "system.h"
#include "gfx.h"

#include "palscale.h"
#include "util/fixed.h"
#include "cube.h"

uint16_t g_cube_vram_pos;
static uint16_t s_vram_pos;

static uint8_t phantom_anim_counter;
uint8_t g_cube_phantom_anim_frame;
static uint8_t kphantom_anim_counter_max;

Cube g_cubes[CUBE_COUNT_MAX];

static bool s_hibernate;

SprParam g_cube_spr;

static void vram_load(void)
{
	const Gfx *g = gfx_get(GFX_CUBES);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
	g_cube_vram_pos = s_vram_pos;
}

void cube_manager_set_hibernate(bool hibernate)
{
	s_hibernate = hibernate;
}

void cube_manager_poll(void)
{
	if (s_hibernate) return;
	uint16_t i = ARRAYSIZE(g_cubes);
	while (i--)
	{
		Cube *c = &g_cubes[i];
		if (c->status == CUBE_STATUS_NULL) continue;
		cube_run(c);
	}

	phantom_anim_counter++;
	if (phantom_anim_counter >= kphantom_anim_counter_max)
	{
		phantom_anim_counter = 0;
		g_cube_phantom_anim_frame++;
		if (g_cube_phantom_anim_frame >= 4) g_cube_phantom_anim_frame = 0;
	}
}

void cube_manager_init(void)
{
	g_cube_spr.size = SPR_SIZE(2, 2);
	phantom_anim_counter = 0;
	g_cube_phantom_anim_frame = 0;

	cube_set_constants();

	vram_load();

	cube_manager_set_hibernate(false);

	uint16_t i = ARRAYSIZE(g_cubes);
	while (i--)
	{
		Cube *c = &g_cubes[i];
		c->status = CUBE_STATUS_NULL;
	}

	kphantom_anim_counter_max = PALSCALE_DURATION(6);
}
