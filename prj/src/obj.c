#include "obj.h"
#include "common.h"
#include "system.h"

#include <stdlib.h>

#include "cube.h"

#include "gfx.h"
#include "sfx.h"
#include "progress.h"

#include "obj/entrance.h"
#include "obj/metagrub.h"
#include "obj/flip.h"
#include "obj/boingo.h"
#include "obj/gaxter1.h"
#include "obj/gaxter2.h"
#include "obj/buggo.h"
#include "obj/dancyflower.h"
#include "obj/jraff.h"
#include "obj/pilla.h"
#include "obj/hedgedog.h"
#include "obj/shoot.h"
#include "obj/laser.h"
#include "obj/killzam.h"

#include "obj/teleporter.h"
#include "obj/magibear.h"

#include "obj/fissins1.h"

#include "obj/lavaanim.h"

#include "obj/scrlock.h"
#include "obj/title.h"
#include "obj/bogologo.h"

#include "obj/lyle.h"
#include "obj/cube_manager.h"
#include "obj/map.h"
#include "obj/bg.h"
#include "obj/hud.h"
#include "obj/particle_manager.h"
#include "obj/projectile_manager.h"
#include "obj/exploder.h"
#include "obj/powerup_manager.h"

#include "obj/template.h"

#include "md/megadrive.h"

#include "palscale.h"


#define OBJ_OFFSCREEN_MARGIN INTTOFIX32(64)

ObjSlot g_objects[OBJ_COUNT_MAX];
static uint16_t obj_vram_pos;

static uint16_t highest_obj_idx;

static uint16_t constants_set;
static int8_t khurt_stun_time;

static void set_constants(void)
{
	if (constants_set) return;

	khurt_stun_time = PALSCALE_DURATION(24);
	constants_set = 1;
}

// Setup and teardown functions for all object types. Leave NULL if an object
// does not require any special setup/teardown.
typedef struct SetupFuncs
{
	void (*load_func)(Obj *o, uint16_t data);  // Run when obj_spawn is called.
	void (*unload_func)(void);  // Run when obj_clear is called.
} SetupFuncs;

// Since cubes are represented in the level data as objects, those objects are
// caught and passed in to the cube manager. Then, the object slot is freed.
static void cube_spawn_stub(Obj *o, uint16_t data)
{
	o->status = OBJ_STATUS_NULL;
	Cube *c = cube_manager_spawn(o->x, o->y,
	                             (CubeType)data, CUBE_STATUS_IDLE, 0, 0);
	if (!c) return;
	c->x -= c->left;
	c->y -= c->top;
	if (c->type == CUBE_TYPE_ORANGE)
	{
		c->x -= INTTOFIX32(8);
		c->y -= INTTOFIX32(16);
	}
}

// The map data separates ability-expanding powerups and cp/hp pickups, but the
// new powerup manager unifies the two. This stub is here in order to allow for
// placement of ability-expanding powerups on the map using the old "item" obj.
static void powerup_spawn_stub(Obj *o, uint16_t data)
{
	o->status = OBJ_STATUS_NULL;
	Powerup *p = powerup_manager_spawn(o->x, o->y, data, 0);
	if (!p) return;
	p->x += INTTOFIX32(8);
	p->y += INTTOFIX32(8);
}

static const SetupFuncs setup_funcs[] =
{
	[OBJ_NULL] = {NULL, NULL},
	[OBJ_ENTRANCE] = {o_load_entrance, o_unload_entrance},
	[OBJ_CUBE] = {cube_spawn_stub, NULL},
	[OBJ_METAGRUB] = {o_load_metagrub, o_unload_metagrub},
	[OBJ_FLIP] = {o_load_flip, o_unload_flip},
	[OBJ_BOINGO] = {o_load_boingo, o_unload_boingo},
	[OBJ_ITEM] = {powerup_spawn_stub, NULL},
	[OBJ_GAXTER1] = {o_load_gaxter1, o_unload_gaxter1},
	[OBJ_GAXTER2] = {o_load_gaxter2, o_unload_gaxter2},
	[OBJ_BUGGO1] = {o_load_buggo, o_unload_buggo},
	[OBJ_BUGGO2] = {o_load_buggo, o_unload_buggo},
	[OBJ_DANCYFLOWER] = {o_load_dancyflower, o_unload_dancyflower},
	[OBJ_JRAFF] = {o_load_jraff, o_unload_jraff},
	[OBJ_PILLA] = {o_load_pilla, o_unload_pilla},
	[OBJ_HEDGEDOG] = {o_load_hedgedog, o_unload_hedgedog},
	[OBJ_SHOOT] = {o_load_shoot, o_unload_shoot},
	[OBJ_LASER] = {o_load_laser, o_unload_laser},
	[OBJ_KILLZAM] = {o_load_killzam, o_unload_killzam},

	[OBJ_TELEPORTER] = {o_load_teleporter, o_unload_teleporter},
	[OBJ_MAGIBEAR] = {o_load_magibear, o_unload_magibear},
	[OBJ_FISSINS1] = {o_load_fissins1, o_unload_fissins1},

	[OBJ_LAVAANIM] = {o_load_lavaanim, o_unload_lavaanim},

	[OBJ_SCRLOCK] = {o_load_scrlock, o_unload_scrlock},
	[OBJ_BOGOLOGO] = {o_load_bogologo, o_unload_bogologo},
	[OBJ_TITLE] = {o_load_title, o_unload_title},

	[OBJ_LYLE] = {o_load_lyle, o_unload_lyle},
	[OBJ_CUBE_MANAGER] = {o_load_cube_manager, o_unload_cube_manager},
	[OBJ_MAP] = {o_load_map, o_unload_map},
	[OBJ_BG] = {o_load_bg, o_unload_bg},
	[OBJ_HUD] = {o_load_hud, o_unload_hud},
	[OBJ_PARTICLE_MANAGER] = {o_load_particle_manager, o_unload_particle_manager},
	[OBJ_PROJECTILE_MANAGER] = {o_load_projectile_manager, o_unload_projectile_manager},
	[OBJ_EXPLODER] = {o_load_exploder, NULL},
	[OBJ_POWERUP_MANAGER] = {o_load_powerup_manager, o_unload_powerup_manager},

	[OBJ_TEMPLATE] = {o_load_template, o_unload_template},
};

// Object list execution ======================================================
int obj_init(void)
{
	SYSTEM_ASSERT(sizeof(g_objects[0]) % sizeof(uint32_t) == 0);

	obj_clear();
	set_constants();

	return 1;
}

static uint16_t obj_is_offscreen(const Obj *o)
{
	const fix32_t cam_left = INTTOFIX32(map_get_x_scroll());
	const fix32_t cam_top = INTTOFIX32(map_get_y_scroll());
	const fix32_t cam_right = INTTOFIX32(map_get_x_scroll() + GAME_SCREEN_W_PIXELS);
	const fix32_t cam_bottom = INTTOFIX32(map_get_y_scroll() + GAME_SCREEN_H_PIXELS);
	if (o->x < cam_left - OBJ_OFFSCREEN_MARGIN) return 1;
	if (o->y < cam_top - OBJ_OFFSCREEN_MARGIN) return 1;
	if (o->x > cam_right + OBJ_OFFSCREEN_MARGIN) return 1;
	if (o->y > cam_bottom + OBJ_OFFSCREEN_MARGIN) return 1;
	return 0;
}

static void obj_explode(Obj *o)
{
	const fix16_t kspawn_rate = INTTOFIX16(PALSCALE_1ST(1.0));
	exploder_spawn(o->x, o->y + (o->top / 2), o->dx, o->dy, PARTICLE_TYPE_FIZZLERED, 6, kspawn_rate);
	sfx_play(SFX_OBJ_BURST, 3);
	// TODO: (Possibly) spawn powerup.
	const uint8_t spawn_chance = system_rand() & 0x0F;
	if (spawn_chance > 0xA)
	{
		powerup_manager_spawn(o->x, o->y, system_rand() % 2 ? POWERUP_TYPE_HP : POWERUP_TYPE_CP, 0);
	}
	o->status = OBJ_STATUS_NULL;
}

static uint16_t obj_hurt_process(Obj *o)
{
	if (o->hurt_stun > 0)
	{
		o->hurt_stun--;
	}

	if (o->hurt_stun == 0 && o->hp <= 0)
	{
		obj_explode(o);
		return 1;
	}

	return 0;
}

static void obj_check_player(Obj *o)
{
	O_Lyle *l = lyle_get();
	if (!(o->flags & (OBJ_FLAG_HARMFUL | OBJ_FLAG_DEADLY |
	                  OBJ_FLAG_BOUNCE_L | OBJ_FLAG_BOUNCE_R |
	                  OBJ_FLAG_SENSITIVE)))
	{
		o->touching_player = 0;
		return;
	}
	o->touching_player = obj_touching_obj(&l->head, o);
	if (!o->touching_player) return;

	if (o->flags & OBJ_FLAG_HARMFUL)
	{
		lyle_get_hurt();
	}
	if (o->flags & OBJ_FLAG_DEADLY)
	{
		lyle_kill();
	}
	if (o->flags & OBJ_FLAG_BOUNCE_L)
	{
		l->head.direction = OBJ_DIRECTION_LEFT;
		lyle_get_bounced();
	}
	if (o->flags & OBJ_FLAG_BOUNCE_R)
	{
		l->head.direction = OBJ_DIRECTION_RIGHT;
		lyle_get_bounced();
	}
}

void obj_exec(void)
{
	ObjSlot *s = &g_objects[0];
	int16_t i = ARRAYSIZE(g_objects);
	while (i--)
	{
		Obj *o = (Obj *)s;
		s++;
		if (o->status == OBJ_STATUS_NULL) continue;

		o->offscreen = obj_is_offscreen(o);
		if (!(o->flags & OBJ_FLAG_ALWAYS_ACTIVE) && o->offscreen) continue;

		if (o->flags & OBJ_FLAG_TANGIBLE) obj_hurt_process(o);

		if (o->flags & (OBJ_FLAG_HARMFUL | OBJ_FLAG_BOUNCE_L |
		                OBJ_FLAG_BOUNCE_R | OBJ_FLAG_DEADLY |
		                OBJ_FLAG_SENSITIVE))
		{
			obj_check_player(o);
		}

		if (o->main_func) o->main_func(o);
	}
}

void obj_clear(void)
{
	ObjSlot *s = &g_objects[0];
	int16_t i = ARRAYSIZE(g_objects);
	while (i--)
	{
		Obj *o = (Obj *)s;
		s++;
		o->status = OBJ_STATUS_NULL;
	}

	for (uint16_t i = 0; i < ARRAYSIZE(setup_funcs); i++)
	{
		if (setup_funcs[i].unload_func) setup_funcs[i].unload_func();
	}

	obj_vram_pos = OBJ_TILE_VRAM_POSITION;
}

Obj *obj_spawn(int16_t x, int16_t y, ObjType type, uint16_t data)
{
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status != OBJ_STATUS_NULL) continue;
		if (!setup_funcs[type].load_func) continue;

		uint32_t *raw_mem_uint32 = (uint32_t *)g_objects[i].raw_mem;
		for (uint16_t j = 0; j < sizeof(g_objects[i]) / sizeof(uint32_t); j++)
		{
			raw_mem_uint32[j] = 0;
		}
		o->status = OBJ_STATUS_ACTIVE;
		o->type = type;
		o->x = INTTOFIX32(x);
		o->y = INTTOFIX32(y);
		setup_funcs[type].load_func(o, data);
		return o;
	}
	return NULL;
}

// VRAM load positions are reset to zero when obj_clear is called.
uint16_t obj_vram_alloc(uint16_t bytes)
{
	bytes &= 0xFFFE;
	uint16_t ret = obj_vram_pos;
	obj_vram_pos += bytes;
	return ret;
}

void obj_cube_impact(Obj *o, Cube *c)
{
	if (o->cube_func) o->cube_func(o, c);
	else obj_standard_cube_response(o, c);
}

void obj_basic_init(Obj *o, ObjFlags flags, fix16_t left, fix16_t right, fix16_t top, int16_t hp)
{
	o->left = left;
	o->right = right;
	o->top = top;

	o->x += right;
	o->y -= top;

	o->hp = hp;
	o->flags = flags;
	o->direction = OBJ_DIRECTION_RIGHT;
	o->hurt_stun = 0;
	o->offscreen = 0;
}

// Utility or commonly reused functions

void obj_get_hurt(Obj *o, int16_t damage)
{
	if (o->hurt_stun != 0) return;
	o->hp -= damage;
	o->hurt_stun = khurt_stun_time;
}

void obj_standard_cube_response(Obj *o, Cube *c)
{
	(void)c;
	if (o->hurt_stun > 0) return;

	int8_t damage = 1;
	switch (c->type)
	{
		default:
			cube_destroy(c);
			break;
		case CUBE_TYPE_RED:
			damage = 3;
			cube_destroy(c);
			break;
		case CUBE_TYPE_PHANTOM:
			if (progress_get()->abilities & ABILITY_2X_DAMAGE_PHANTOM) damage = 2;
			cube_destroy(c);
			break;
		case CUBE_TYPE_GREEN:
			cube_bounce_dx(c);
			c->dy = -c->dy;
			c->status = CUBE_STATUS_AIR;
			break;
		case CUBE_TYPE_ORANGE:
			damage = 5;
			cube_destroy(c);
			break;
	}
	sfx_play(SFX_CUBE_HIT, 5);
	obj_get_hurt(o, damage);
}
