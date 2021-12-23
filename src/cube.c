#include "cube.h"
#include "obj.h"
#include "util/fixed.h"
#include "palscale.h"
#include "common.h"
#include "system.h"
#include "obj/cube_manager.h"
#include "obj/particle_manager.h"
#include "obj/powerup_manager.h"
#include "obj/map.h"
#include "game.h"
#include "sfx.h"
#include "obj/lyle.h"

#include "md/megadrive.h"

#define CUBE_INITIAL_BOUNCE_COUNT 1

static fix16_t kgravity;
static fix16_t kcube_on_cube_dy;
static fix16_t kcube_on_cube_dx;
static fix16_t kbounce_coef;
static fix16_t kbounce_cutoff;
static fix16_t kceiling_dy;
static fix16_t kdx_degrade;
static fix16_t kdy_degrade;
static uint8_t kcollision_timeout;
static uint8_t kfizzle_duration;
static uint16_t kspawn_seq[2];

void cube_set_constants(void)
{
	static int16_t constants_set = 0;
	if (constants_set) return;
	kgravity = INTTOFIX16(PALSCALE_2ND(0.1388888888f));
	kcube_on_cube_dy = INTTOFIX16(PALSCALE_1ST(-1.833));
	kbounce_coef = INTTOFIX16(0.35);
	kbounce_cutoff = INTTOFIX16(PALSCALE_1ST(-0.7)); // -1.04));  // TODO: Check this one - it was imbalanced in the first port; pal was -1.3
	kceiling_dy = INTTOFIX16(PALSCALE_1ST(2.5));
	kdx_degrade = INTTOFIX16(PALSCALE_1ST(0.8333333333));
	kcube_on_cube_dx = kdx_degrade;
	kdy_degrade = INTTOFIX16(0.42);
	kspawn_seq[0] = PALSCALE_DURATION(72);
	kspawn_seq[1] = PALSCALE_DURATION(120);
	kfizzle_duration = PALSCALE_DURATION(7);
	kcollision_timeout = PALSCALE_DURATION(8);
	
	constants_set = 1;
}

static inline void cube_scan_objects(Cube *c)
{
	for (uint16_t i = 0; i < ARRAYSIZE(g_objects); i++)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status == OBJ_STATUS_NULL) continue;
		if (o->offscreen) continue;
		if (!(o->flags & OBJ_FLAG_TANGIBLE)) continue;
		if (obj_touching_cube(o, c)) obj_cube_impact(o, c);
	}
}

void cube_destroy(Cube *c)
{
	c->dx = 0;
	if (c->status == CUBE_STATUS_EXPLODE ||
	    c->status == CUBE_STATUS_FIZZLE)
	{
		return;
	}
	c->fizzle_count = kfizzle_duration;
	if (c->type == CUBE_TYPE_RED)
	{
		c->status = CUBE_STATUS_EXPLODE;
		sfx_play(SFX_EXPLODE, 14);
	}
	else if (c->type == CUBE_TYPE_ORANGE)
	{
		c->status = CUBE_STATUS_EXPLODE;
		sfx_play(SFX_EXPLODE, 14);
		// TODO: I think this is supposed to do something more as well.
	}
	else
	{
		c->status = CUBE_STATUS_FIZZLE;
		sfx_play(SFX_CUBE_FIZZLE, 14);
	}

	if (c->type >= CUBE_TYPE_YELLOW_HPUP)
	{
		sfx_play(SFX_CUBE_FIZZLE, 14);
		switch (c->type)
		{
			case CUBE_TYPE_YELLOW_HPUP:
				powerup_manager_spawn(c->x, c->y, POWERUP_TYPE_HP, 0);
				break;
			case CUBE_TYPE_YELLOW_HPUP2:
				powerup_manager_spawn(c->x, c->y, POWERUP_TYPE_HP_2X, 0);
				break;
			case CUBE_TYPE_YELLOW_CPUP:
				powerup_manager_spawn(c->x, c->y, POWERUP_TYPE_CP, 0);
				break;
			case CUBE_TYPE_YELLOW_CPUP2:
				powerup_manager_spawn(c->x, c->y, POWERUP_TYPE_CP_2X, 0);
				break;
			case CUBE_TYPE_YELLOW_CPORB0:
			case CUBE_TYPE_YELLOW_CPORB1:
			case CUBE_TYPE_YELLOW_CPORB2:
			case CUBE_TYPE_YELLOW_CPORB3:
			case CUBE_TYPE_YELLOW_CPORB4:
			case CUBE_TYPE_YELLOW_CPORB5:
			case CUBE_TYPE_YELLOW_CPORB6:
			case CUBE_TYPE_YELLOW_CPORB7:
			case CUBE_TYPE_YELLOW_CPORB8:
			case CUBE_TYPE_YELLOW_CPORB9:
			case CUBE_TYPE_YELLOW_CPORB10:
			case CUBE_TYPE_YELLOW_CPORB11:
			case CUBE_TYPE_YELLOW_CPORB12:
			case CUBE_TYPE_YELLOW_CPORB13:
			case CUBE_TYPE_YELLOW_CPORB14:
			case CUBE_TYPE_YELLOW_CPORB15:
				powerup_manager_spawn(c->x, c->y, POWERUP_TYPE_CP_ORB, (c->type & 0x000F));
				break;
			case CUBE_TYPE_YELLOW_HPORB0:
			case CUBE_TYPE_YELLOW_HPORB1:
			case CUBE_TYPE_YELLOW_HPORB2:
			case CUBE_TYPE_YELLOW_HPORB3:
			case CUBE_TYPE_YELLOW_HPORB4:
			case CUBE_TYPE_YELLOW_HPORB5:
			case CUBE_TYPE_YELLOW_HPORB6:
			case CUBE_TYPE_YELLOW_HPORB7:
			case CUBE_TYPE_YELLOW_HPORB8:
			case CUBE_TYPE_YELLOW_HPORB9:
			case CUBE_TYPE_YELLOW_HPORB10:
			case CUBE_TYPE_YELLOW_HPORB11:
			case CUBE_TYPE_YELLOW_HPORB12:
			case CUBE_TYPE_YELLOW_HPORB13:
			case CUBE_TYPE_YELLOW_HPORB14:
			case CUBE_TYPE_YELLOW_HPORB15:
				powerup_manager_spawn(c->x, c->y, POWERUP_TYPE_HP_ORB, (c->type & 0x000F));
				break;
			default:
				break;
		}
	}
}

void cube_clamp_dx(Cube *c)
{
	if (c->dx > 0) c->dx = kcube_on_cube_dx;
	else if (c->dx < 0) c->dx = -kcube_on_cube_dx;
}

static inline void cube_bg_bounce_sides(Cube *c)
{
	int16_t cx_r = FIX32TOINT(c->x + c->right);
	int16_t cx_l = FIX32TOINT(c->x + c->left);
	const int16_t cy_top = FIX32TOINT(c->y + c->top);
	const int16_t cy_bot = FIX32TOINT(c->y);

	const int16_t map_right_px = map_get_right_px();
	if (cx_r + 1 >= map_right_px) cx_r = map_right_px - 2;
	if (cx_l - 1 < 0) cx_l = 1;

	if (c->dy > 0 && map_collision((cx_r + cx_l) / 2, cy_bot + 2)) return;

	if (c->dx > 0)
	{
		if (map_collision(cx_r + 1, cy_top) ||
		    map_collision(cx_r + 1, cy_bot))
		{
			// X position (in pixels) of wall being touched.
			const int16_t touching_tile_x = ((cx_r + 1) / 8) * 8;
			c->x = INTTOFIX32(touching_tile_x) - c->right - 1;
			cube_bounce_dx(c);
			c->bounce_count = 1;
			if (c->status == CUBE_STATUS_KICKED)
			{
				c->status = CUBE_STATUS_AIR;
				c->bounce_count = CUBE_INITIAL_BOUNCE_COUNT;
				c->dy = kcube_on_cube_dy;
			}
			sfx_play(SFX_CUBE_BOUNCE, 15);
		}
	}
	else if (c->dx < 0)
	{
		if (map_collision(cx_l - 1, cy_top) ||
		    map_collision(cx_l - 1, cy_bot))
		{
			// X position (in pixels) of wall being touched. We want the right
			// side of the tile, so 7 is added.
			const int16_t touching_tile_x = 8 + ((cx_l - 1) / 8) * 8;
			c->x = INTTOFIX32(touching_tile_x) - c->left + 1;
			cube_bounce_dx(c);
			c->bounce_count = 1;
			if (c->status == CUBE_STATUS_KICKED)
			{
				c->status = CUBE_STATUS_AIR;
				c->bounce_count = CUBE_INITIAL_BOUNCE_COUNT;
				c->dy = kcube_on_cube_dy;
			}
			sfx_play(SFX_CUBE_BOUNCE, 15);
		}
	}
}

static inline void cube_bg_bounce_top(Cube *c)
{
	int16_t cx_left = FIX32TOINT(c->x + c->left);
	int16_t cx_right = FIX32TOINT(c->x + c->right);
	int16_t cy_top = FIX32TOINT(c->y + c->top);
	uint16_t gnd_chk[2];
	if (cy_top <= 0) cy_top = 1;

	const int16_t map_right_px = map_get_right_px();
	if (cx_left <= 0) cx_left = 1;
	if (cx_right >= map_right_px - 1) cx_right = map_right_px - 2;

	gnd_chk[0] = map_collision(cx_left, cy_top - 1);
	gnd_chk[1] = map_collision(cx_right, cy_top - 1);
	// Both left and right tests count as a collison.
	if (gnd_chk[0] && gnd_chk[1])
	{
		c->dy = kceiling_dy;
		sfx_play(SFX_CUBE_BOUNCE, 15);
	}
	else if (gnd_chk[0] || gnd_chk[1])
	{
		// One edge is missing. Check for the center.
		const uint16_t center_chk = map_collision(FIX32TOINT(c->x), cy_top - 1);

		if (center_chk)
		{
			// Center checks out, bounce.
			c->dy = kceiling_dy;
			sfx_play(SFX_CUBE_BOUNCE, 15);
		}
		else
		{
			// TODO: Make this more precise, I do not trust it.
			// If the center doesn't have a collision, align it to the wall.
			int16_t cx = FIX32TOINT(c->x);
			if (gnd_chk[0])
			{
				cx = ((cx + 8) / 8) * 8;
			}
			else
			{
				cx = ((cx) / 8) * 8;
			}
			c->x = INTTOFIX32(cx);
		}
	}
}

static inline void cube_do_ground_recoil(Cube *c)
{
	// Align cube vertically to the ground.
	const int16_t cy_bottom = FIX32TOINT(c->y);
	const int16_t touching_tile_y = ((cy_bottom + 1) / 8) * 8;

	c->y = INTTOFIX32(touching_tile_y) - INTTOFIX32(1);

	// Bounce - about 35% of original dy.
	c->dy = -FIX16MUL(c->dy, kdy_degrade);

	// See if the cube should stop moving.
	if (c->bounce_count <= 0 && c->dx == 0 && c->dy >= kbounce_cutoff)
	{
		c->dy = 0;
		c->status = CUBE_STATUS_IDLE;
		if (c->type == CUBE_TYPE_GREENBLUE) c->type = CUBE_TYPE_BLUE;
	}
	else if (c->bounce_count == 0)
	{
		c->bounce_count = 1;
	}

	if (c->bounce_count > 0 && c->dx == 0 && c->dy >= kbounce_cutoff)
	{
		c->bounce_count--;
	}
	else
	{
		c->bounce_count = 1;
	}

	// Degrade dx
	if (c->dx > kdx_degrade) c->dx -= kdx_degrade;
	else if (c->dx < -kdx_degrade) c->dx += kdx_degrade;
	else c->dx = 0;

	if (c->dx != 0 && c->dy >= kbounce_cutoff) c->dy = 0;


	sfx_play(SFX_CUBE_BOUNCE, 15);

}

static inline void cube_bg_bounce_ground(Cube *c)
{
	const int16_t cx = FIX32TOINT(c->x);
	int16_t cx_left = FIX32TOINT(c->x + c->left);
	int16_t cx_right = FIX32TOINT(c->x + c->right);
	int16_t cy_bottom = FIX32TOINT(c->y);

	const int16_t map_bottom_px = map_get_bottom_px();
	const int16_t map_right_px = map_get_right_px();
	if (cy_bottom >= map_bottom_px - 1) cy_bottom = map_bottom_px - 2;
	if (cx_left <= 0) cx_left = 1;
	if (cx_right >= map_right_px - 1) cx_right = map_right_px - 2;
	uint16_t gnd_chk[2];
	gnd_chk[0] = map_collision(cx_left, cy_bottom + 1);
	gnd_chk[1] = map_collision(cx_right, cy_bottom + 1);
	// Both left and right tests count as a collison.
	if (gnd_chk[0] && gnd_chk[1])
	{
		cube_do_ground_recoil(c);
	}
	else if (gnd_chk[0] || gnd_chk[1])
	{
		// One edge is missing. Check for the center.
		const uint16_t center_chk = map_collision(cx, cy_bottom + 1);

		if (center_chk)
		{
			cube_do_ground_recoil(c);
		}
		else
		{
			// If the center doesn't have a collision, align it to the wall.
			if (gnd_chk[0])
			{
				const int new_cx = ((cx + 8) / 8) * 8;
				c->x = INTTOFIX32(new_cx);
			}
			else
			{
				const int new_cx = ((cx) / 8) * 8;
				c->x = INTTOFIX32(new_cx);
			}
		}
	}
}

static inline void cube_bg_collision(Cube *c)
{
	int16_t cx_left = FIX32TOINT(c->x + c->left);
	int16_t cx_right = FIX32TOINT(c->x + c->right);
	int16_t cy_top = FIX32TOINT(c->y + c->top);
	int16_t cy_bottom = FIX32TOINT(c->y);
	if (c->type == CUBE_TYPE_GREEN || c->type == CUBE_TYPE_GREENBLUE)
	{
		if (c->dx != 0) cube_bg_bounce_sides(c);
		if (c->dy > 0) cube_bg_bounce_ground(c);
		else if (c->dy < 0) cube_bg_bounce_top(c);
	}
	else
	{
		const int16_t map_bottom_px = map_get_bottom_px();
		const int16_t map_right_px = map_get_right_px();
		if (cy_top <= 0) cy_top = 1;
		if (cy_bottom >= map_bottom_px - 1) cy_bottom = map_bottom_px - 2;
		if (cx_left <= 0) cx_left = 1;
		if (cx_right >= map_right_px - 1) cx_right = map_right_px - 2;
		if ((c->dx > 0 || c->dy > 0) && map_collision(cx_right, cy_bottom)) cube_destroy(c);
		else if ((c->dx < 0 || c->dy > 0) && map_collision(cx_left, cy_bottom)) cube_destroy(c);
		else if ((c->dx > 0 || c->dy < 0) && map_collision(cx_right, cy_top)) cube_destroy(c);
		else if ((c->dx < 0 || c->dy < 0) && map_collision(cx_left, cy_top)) cube_destroy(c);
	}
}

static inline void cube_movement(Cube *c)
{
	if (c->status == CUBE_STATUS_AIR)
	{
		c->y += c->dy;
		c->dy += kgravity;
	}
	c->x += c->dx;

	const int16_t cy_top = FIX32TOINT(c->y + c->top);
	const int16_t cy_bottom = FIX32TOINT(c->y);
	const int16_t cx_left = FIX32TOINT(c->x + c->left);
	const int16_t cx_right = FIX32TOINT(c->x + c->right);

	// Allow the cube to free-drop
	if (c->status == CUBE_STATUS_KICKED)
	{
		// In free air, change into an air cube falling straight down.
		if (!map_collision(cx_left, cy_bottom + 1) &&
		    !map_collision(cx_right, cy_bottom + 1))
		{
			c->x = c->dx < 0 ? INTTOFIX32(-1 + ((cx_left + 4) / 8) * 8) - c->left:
			                   INTTOFIX32((((cx_right) / 8) * 8)) - c->right;
			c->dx = 0;
			c->status = CUBE_STATUS_AIR;
			c->dy = kgravity;
			// Lock to the grid.
			// TODO: I don't know about that +4
		}
	}

	// Check for cube out of bounds.
	if (cx_left > FIX32TOINT(map_get_right()) ||
	    cx_right < 0 ||
	    cy_top > FIX32TOINT(map_get_bottom()) ||
	    cy_bottom < 0)
	{
		c->status = CUBE_STATUS_NULL;
	}
}

static inline void normal_cube_col(Cube *c, Cube *d)
{
	if (d->type == CUBE_TYPE_SPAWNER) return;
	if (c->status != CUBE_STATUS_IDLE) cube_destroy(c);
	else if (c->dy != 0) c->dy = 0;

	if (d->type != CUBE_TYPE_GREEN && d->type != CUBE_TYPE_GREENBLUE &&
	    d->status != CUBE_STATUS_IDLE)
	{
		cube_destroy(d);
	}
	else if (d->dy != 0) d->dy = 0;
}

static inline void green_cube_col(Cube *c, Cube *d)
{
	if (d->type == CUBE_TYPE_SPAWNER) return;
	if (c->status != CUBE_STATUS_IDLE)
	{
		if (c->status == CUBE_STATUS_KICKED)
		{
			cube_bounce_dx(c);
			c->status = CUBE_STATUS_AIR;
			c->bounce_count = CUBE_INITIAL_BOUNCE_COUNT;
		}
		if (c->dx == 0)
		{
			c->dx = INTTOFIX16(system_rand() % 2 ? 1 : -1);
		}
		else
		{
			cube_clamp_dx(c);
		}
		c->dy = kcube_on_cube_dy;
		sfx_play(SFX_CUBE_BOUNCE, 15);
		c->collision_timeout = kcollision_timeout;
	}
	else if (c->dy != 0) c->dy = 0;

	// Destroy an active non-bouncy other cube.
	if (d->type != CUBE_TYPE_GREEN && d->type != CUBE_TYPE_GREENBLUE &&
	    d->status != CUBE_STATUS_IDLE)
	{
		cube_destroy(d);
	}
	else if (d->dy != 0) d->dy = 0;
}

static inline void cube_on_cube_collisions(Cube *c)
{
	if (c->collision_timeout)
	{
		c->collision_timeout--;
		return;
	}

	uint16_t i = ARRAYSIZE(g_cubes);
	while (i--)
	{
		Cube *d = &g_cubes[i];
		if (d == c ||
		    d->status == CUBE_STATUS_NULL ||
		    d->status == CUBE_STATUS_FIZZLE ||
		    d->status == CUBE_STATUS_EXPLODE)
		{
			continue;
		}

		if (c->x + c->left <= d->x + d->right &&
		    c->x + c->right >= d->x + d->left &&
		    c->y + c->top <= d->y &&
		    c->y >= d->y + d->top)
		{
			if (c->type == CUBE_TYPE_GREEN || c->type == CUBE_TYPE_GREENBLUE)
			{
				green_cube_col(c, d);
			}
			else
			{
				normal_cube_col(c, d);
			}
		}
	}
}

void cube_run(Cube *c)
{
	if (c->type == CUBE_TYPE_ORANGE && c->status == CUBE_STATUS_IDLE && !c->lyle_spawn_check)
	{
		static const fix32_t margin = INTTOFIX32(64);
		const O_Lyle *l = lyle_get();
		if (!(c->x + margin < l->head.x ||
		      c->x - margin > l->head.x ||
		      c->y + margin < l->head.y ||
		      c->y - margin > l->head.y))
		{
			c->status = CUBE_STATUS_NULL;
			return;
		}
		c->lyle_spawn_check = 1;
	}
	if (c->status == CUBE_STATUS_AIR || c->status == CUBE_STATUS_KICKED)
	{
		system_profile(PALRGB(6, 6, 7));
		// Collision is processed one frame late to mimic the original
		// game's behavior... so says the old code.
		cube_scan_objects(c);
		system_profile(PALRGB(6, 3, 7));
		if (c->status == CUBE_STATUS_AIR ||
		    c->status == CUBE_STATUS_KICKED)
		{
			system_profile(PALRGB(4, 0, 7));
			cube_bg_collision(c);
			system_profile(PALRGB(4, 2, 5));
			cube_movement(c);
			system_profile(PALRGB(3, 5, 3));
			cube_on_cube_collisions(c);
		}
	}
	
	system_profile(PALRGB(0, 1, 0));

	// Check if the cube is off-screen, and skip some things if so.
	const fix32_t margin = 32;
	const int16_t x_scroll = map_get_x_scroll();
	const int16_t y_scroll = map_get_y_scroll();
	const int16_t cx = FIX32TOINT(c->x + c->left);
	const int16_t cy = FIX32TOINT(c->y + c->top);
	if (cy > y_scroll + GAME_SCREEN_H_PIXELS + margin) goto off_screen;
	if (cy < y_scroll - margin) goto off_screen;
	if (cx > x_scroll + GAME_SCREEN_W_PIXELS + margin) goto off_screen;
	if (cx < x_scroll - margin) goto off_screen;
	
	// Things that only happen when cubes are on-screen:
	system_profile(PALRGB(7, 3, 0));

	if (c->type == CUBE_TYPE_SPAWNER)
	{
		if (c->spawn_count < kspawn_seq[1]) c->spawn_count++;
		if (c->spawned_cube)
		{
			if (c->spawned_cube->status == CUBE_STATUS_IDLE)
			{
				c->spawn_count = 0;
			}
			else
			{
				c->spawned_cube = NULL;
			}
		}
		if (c->spawn_count == kspawn_seq[1])
		{
			c->spawned_cube = cube_manager_spawn(c->x, c->y, CUBE_TYPE_BLUE,
			                                     CUBE_STATUS_IDLE, 0, 0);
			c->spawned_cube->spawn_count = 1;  // Mark blue cube to flash.
		}
	}

	if (c->status == CUBE_STATUS_FIZZLE || c->status == CUBE_STATUS_EXPLODE)
	{
		if (c->fizzle_count > 0)
		{
			c->fizzle_count--;
			cube_scan_objects(c);
		}
		if (c->fizzle_count <= 0)
		{
			c->status = CUBE_STATUS_NULL;
			return;
		}
	}

	// Spawn particles.
	if (c->status == CUBE_STATUS_FIZZLE)
	{
		particle_manager_spawn(c->x, c->y + (c->top / 2), PARTICLE_TYPE_FIZZLE);
		return;
	}
	else if (c->status == CUBE_STATUS_EXPLODE)
	{
		particle_manager_spawn(c->x, c->y + (c->top / 2), PARTICLE_TYPE_EXPLOSION);
		return;
	}
	system_profile(PALRGB(3, 0, 7));

	// Render the cube.
	CubeType render_type = c->type;
	// Render the cube.
	if (c->type == CUBE_TYPE_SPAWNER)
	{
		if (c->spawn_count < kspawn_seq[0]) return;
		if ((g_elapsed >> 2) % 2 == 0) return;
	}
	else if (c->type == CUBE_TYPE_BLUE)
	{
		// Blue cubes flash the spawner colors if there is a spawner below them.
		// TODO: Anim counter instead of dividing elapsed time.
		if (c->spawn_count > 0 && (g_elapsed >> 2) % 2 == 0)
		{
			render_type = CUBE_TYPE_SPAWNER;
		}
	}

	system_profile(PALRGB(7, 7, 7));
	cube_manager_draw_cube(cx, cy, render_type);
	system_profile(PALRGB(0, 0, 0));
	return;

off_screen:
	if (c->fizzle_count > 0)
	{
		c->fizzle_count = 0;
		c->status = CUBE_STATUS_NULL;
	}
	if (c->type == CUBE_TYPE_SPAWNER)
	{
		if (c->spawn_count < kspawn_seq[1] - 1) c->spawn_count = kspawn_seq[1] - 1;
	}

	system_profile(PALRGB(0, 0, 0));
}

void cube_bounce_dx(Cube *c)
{
	c->dx = c->dx * -1;
	if (c->dx > 0) c->dx = kcube_on_cube_dx;
	else if (c->dx < 0) c->dx = -kcube_on_cube_dx;
}

// Called externally when Lyle touches a cube.
void cube_restrict_spawn(Cube *c)
{
	if (c->spawn_count == kspawn_seq[1] - 2)
	{
		c->spawn_count--;
	}
}


