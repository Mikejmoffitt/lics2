#include "obj/teleporter.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "palscale.h"
#include "obj/lyle.h"
#include "obj/map.h"
#include "obj/particle_manager.h"
#include "game.h"

// Constants.

static int8_t kactive_len;
static int16_t kanim_len;
static int8_t ktele_sound_trigger;
static int8_t kanim_speed;

static uint16_t constants_set;

static void set_constants(void)
{
	if (constants_set) return;

	kactive_len = PALSCALE_DURATION(90);
	kanim_len = PALSCALE_DURATION(120);
	ktele_sound_trigger = PALSCALE_DURATION(75);
	kanim_speed = PALSCALE_DURATION(2.5);

	constants_set = 1;
}

// VRAM.

static uint16_t vram_pos;

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g = gfx_get(GFX_TELEPORTER);
	vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Main.

static inline void render(O_Teleporter *t)
{
	Obj *o = &t->head;
	int16_t sp_x, sp_y;

	// The base.
	obj_render_setup(o, &sp_x, &sp_y, -16, 15,
	                 map_get_x_scroll(), map_get_y_scroll());
	const uint16_t base_tile = vram_pos + ((t->disabled || t->anim_frame >= 2) ? 24 : 16);
	spr_put(sp_x, sp_y, SPR_ATTR(base_tile, 0, 0,
	                    ENEMY_PAL_LINE, 0), SPR_SIZE(4, 2));

	// The aura.
	if (t->disabled || t->anim_frame == 1 || t->anim_frame == 3) return;
	const uint16_t aura_tile = vram_pos + ((t->anim_frame == 2) ? 8 : 0);
	obj_render_setup(o, &sp_x, &sp_y, -16, -1,
	                 map_get_x_scroll(), map_get_y_scroll());
	spr_put(sp_x, sp_y, SPR_ATTR(aura_tile, 0, 0,
	                    ENEMY_PAL_LINE, 0), SPR_SIZE(4, 2));
}

static void main_func(Obj *o)
{
	O_Teleporter *t = (O_Teleporter *)o;
	O_Lyle *l = lyle_get();

	if (t->active_cnt > 0)
	{
		t->active_cnt--;
		if (t->active_cnt == 0)
		{
			t->disabled = 0;
		}

		// Center the player on the pad.
		if (l->head.x < o->x - INTTOFIX16(0.5))
		{
			l->head.x += INTTOFIX16(0.5);
		}
		else if (l->head.x > o->x + INTTOFIX16(0.5))
		{
			l->head.x -= INTTOFIX16(0.5);
		}
		else
		{
			l->head.x = o->x;
		}

		if (g_elapsed % 2)
		{
			particle_manager_spawn(o->x + INTTOFIX32(-8 + (system_rand() % 16)),
			                       o->y + INTTOFIX32(-16 + (system_rand() % 32)),
			                       PARTICLE_TYPE_SPARKLE);
		}
	}

	if (!t->disabled && t->active_cnt == 0 && l->grounded &&
	    o->touching_player && l->tele_out_cnt == 0)
	{
		// TODO: Mark in SRAM that teleporter[t->id] has been discovered.

		// If the player is teleporting IN, disable the teleporter once the anim is over.
		if (l->tele_in_cnt > 0)
		{
			if (l->tele_in_cnt == ktele_sound_trigger)
			{
				// TODO: Play teleport / bogologo sound
			}
			if (g_elapsed % 2)
			{
				particle_manager_spawn(o->x + INTTOFIX32(-8 + (system_rand() % 16)),
				                       o->y + INTTOFIX32(-16 + (system_rand() % 32)),
				                       PARTICLE_TYPE_SPARKLE);
			}
			
			if (l->tele_in_cnt == 8) t->disabled = 1;
		}
		// Normal active player is ready to teleport.
		else if (l->tele_in_cnt == 0)
		{
			l->head.dx = 0;
			l->head.dy = 0;
			l->tele_out_cnt = kanim_len;
			l->tele_in_cnt = kanim_len;
			t->active_cnt = kactive_len;
			// TODO: Play teleport / bogologo sound
		}
	}

	// Animate.

	if (t->anim_cnt == kanim_speed)
	{
		if (t->anim_frame < 3) t->anim_frame++;
		else t->anim_frame = 0;
		t->anim_cnt = 0;
	}
	else
	{
		t->anim_cnt++;
	}

	render(t);
}

// Boilerplate.

void o_load_teleporter(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Teleporter) <= sizeof(ObjSlot));
	O_Teleporter *t = (O_Teleporter *)o;
	(void)data;
	vram_load();
	set_constants();

	obj_basic_init(o, OBJ_FLAG_SENSITIVE,
	               INTTOFIX16(-1), INTTOFIX16(1), INTTOFIX16(-1), 127);
	o->x += INTTOFIX32(15);
	o->main_func = main_func;
	o->cube_func = NULL;

	t->id = data & 0xFF;
	t->activator = (data & 0xFF00) >> 8;
}

void o_unload_teleporter(void)
{
	vram_pos = 0;
}
