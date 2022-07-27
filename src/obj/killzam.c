#include "obj/killzam.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "util/fixed.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "obj/lyle.h"
#include "obj/projectile_manager.h"


static uint16_t s_vram_pos;

static fix16_t kdy_hysteresis;
static fix16_t kddy;
static int16_t kanim_delay;
static int16_t ksequence[4];
static fix16_t kshot_speed;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_KILLZAM);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kdy_hysteresis = INTTOFIX16(PALSCALE_1ST(2.083));
	kddy = INTTOFIX16(PALSCALE_2ND(0.2084));
	kanim_delay = PALSCALE_DURATION(8);

	ksequence[0] = PALSCALE_DURATION(48);  // Begin flickering
	ksequence[1] = PALSCALE_DURATION(96);  // Go solid
	ksequence[2] = PALSCALE_DURATION(144);  // Begin flickering again
	ksequence[3] = PALSCALE_DURATION(192);  // Disappear

	kshot_speed = INTTOFIX16(PALSCALE_1ST(3.0));

	s_constants_set = 1;
}

static void movement(Obj *o)
{
	O_Killzam *e = (O_Killzam *)o;
	if (e->moving_down)
	{
		o->dy += kddy;
		if (o->dy > kdy_hysteresis)
		{
			e->moving_down = 0;
		}
	}
	else
	{
		o->dy -= kddy;
		if (o->dy < -kdy_hysteresis)
		{
			e->moving_down = 1;
		}
	}

	obj_standard_physics(o);
}

static void run_timer(Obj *o)
{
	O_Killzam *e = (O_Killzam *)o;
	O_Lyle *l = lyle_get();

	// Set harmful and tangible flags based on state timer
	if (e->timer >= ksequence[1] && e->timer < ksequence[2])
	{
		o->flags |= OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE;
	}
	else
	{
		o->flags &= ~(OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE);
	}

	// Right at the start of the flicker sequence, choose a position
	if (e->timer == 1)
	{
		uint16_t rando = system_rand() % 512;
		if (rando > 300) rando -= 300;
		o->x = INTTOFIX32(map_get_x_scroll() + 10 + rando);

		rando = system_rand() % 256;
		if (rando > 170) rando -= 170;
		o->y = INTTOFIX32(map_get_y_scroll() + 32 + rando);
	}
	else if (e->timer < ksequence[0])
	{
		// If we landed in a backdrop or on top of the player, go back.
		if (map_collision(FIX32TOINT(o->x + o->right), FIX32TOINT(o->y)) ||
		    map_collision(FIX32TOINT(o->x + o->left), FIX32TOINT(o->y)) ||
		    map_collision(FIX32TOINT(o->x + o->right), FIX32TOINT(o->y + o->top)) ||
		    map_collision(FIX32TOINT(o->x + o->left), FIX32TOINT(o->y + o->top)) ||
		    o->touching_player)
		{
			e->timer = 0;
		}
	}
	else
	{
		// Look at the player during all moments of visibility
		o->direction = (o->x < l->head.x) ?
		               OBJ_DIRECTION_RIGHT :
		               OBJ_DIRECTION_LEFT;
	}

	if (e->timer == ksequence[0] ||
	    e->timer == ksequence[2])
	{
		// TODO: Warp sfx
	}

	if (e->timer == ksequence[2])
	{
		// TODO: Figure out actual shot speed.
		projectile_manager_shoot_at(o->x, o->y - INTTOFIX32(8), PROJECTILE_TYPE_BALL2,
		                            l->head.x, l->head.y - INTTOFIX32(10), kshot_speed);
		// TODO: Shot sfx
	}

	// Advance timer
	e->timer++;
	if (e->timer >= ksequence[3])
	{
		e->timer = 0;
	}
}

static void animate(Obj *o)
{
	O_Killzam *e = (O_Killzam *)o;

	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_delay);

	e->flicker = !e->flicker;
}

static inline void render(Obj *o)
{
	O_Killzam *e = (O_Killzam *)o;
	const uint16_t tile_offset = e->anim_frame == 0 ? 0 : 6;

	if (e->timer < ksequence[0]) return;
	if ((e->timer < ksequence[1] || e->timer > ksequence[2]) &&
	    e->flicker)
	{
		return;
	}
	
	int16_t sp_x, sp_y;
	obj_render_setup(o, &sp_x, &sp_y, -8, -23,
	                 map_get_x_scroll(), map_get_y_scroll());

	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + tile_offset,
	                    o->direction == OBJ_DIRECTION_LEFT, 0,
	                    ENEMY_PAL_LINE, 1), SPR_SIZE(2, 3));
}

static void main_func(Obj *o)
{
	if (o->hurt_stun > 0)
	{
		render(o);
		return;
	}

	movement(o);
	run_timer(o);
	animate(o);
	render(o);
}

void o_load_killzam(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_Killzam) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Killzam", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-7), INTTOFIX16(7), INTTOFIX16(-24), 3);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_killzam(void)
{
	s_vram_pos = 0;
}
