#include "cube.h"
#include "obj.h"
#include "util/fixed.h"
#include "palscale.h"
#include "common.h"
#include "system.h"
#include "obj/cube_manager.h"
#include "obj/map.h"
#include "game.h"

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
	kcube_on_cube_dx = INTTOFIX16(PALSCALE_1ST(1));
	kbounce_coef = INTTOFIX16(0.35);
	kbounce_cutoff = INTTOFIX16(PALSCALE_1ST(-1.04));  // TODO: Check this one - it was imbalanced in the first port; pal was -1.3
	kceiling_dy = INTTOFIX16(PALSCALE_1ST(2.5));
	kdx_degrade = INTTOFIX16(PALSCALE_1ST(0.8333333333));
	kdy_degrade = INTTOFIX16(0.42);
	kspawn_seq[0] = PALSCALE_DURATION(72);
	kspawn_seq[1] = PALSCALE_DURATION(120);
	kfizzle_duration = PALSCALE_DURATION(7);
	kcollision_timeout = PALSCALE_DURATION(8);
	
	constants_set = 1;
}

static void cube_scan_objects(Cube *c)
{
	uint16_t i = ARRAYSIZE(g_objects);
	while (i--)
	{
		Obj *o = &g_objects[i].obj;
		if (o->status == OBJ_STATUS_NULL) continue;
//		if (!(o->flags & OBJ_FLAG_TANGIBLE)) continue;
//		if (o->offscreen) continue;
		if (obj_touching_cube(o, c))
		{
			obj_cube_impact(o, c);
		}
	}
}

void cube_destroy(Cube *c)
{
	c->dx = 0;
	c->fizzle_count = kfizzle_duration;
	if (c->status == CUBE_STATUS_EXPLODE ||
	    c->status == CUBE_STATUS_FIZZLE)
	{
		return;
	}
	if (c->type == CUBE_TYPE_RED)
	{
		c->status = CUBE_STATUS_EXPLODE;
		// TODO: Queue explosion sound
	}
	else if (c->type == CUBE_TYPE_ORANGE)
	{
		c->status = CUBE_STATUS_EXPLODE;
		// TODO: Queue explosion sound
		// TODO: I think this is supposed to do something more as well.
	}
	else
	{
		c->status = CUBE_STATUS_FIZZLE;
		// TODO: Queue regular cube burst sound
	}

	switch (c->type)
	{
		case CUBE_TYPE_RED:
		case CUBE_TYPE_ORANGE:
			c->status = CUBE_STATUS_EXPLODE;
			break;
		default:
			c->status = CUBE_STATUS_FIZZLE;
			break;
		case CUBE_TYPE_YELLOW_HPUP:
			// TODO: Powerup manager spawn HPUP
			break;
		case CUBE_TYPE_YELLOW_HPUP2:
			// TODO: Powerup manager spawn HPUP2
			break;
		case CUBE_TYPE_YELLOW_CPUP:
			// TODO: Powerup manager spawn CPUP
			break;
		case CUBE_TYPE_YELLOW_CPUP2:
			// TODO: Powerup manager spawn CPUP2
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
			// TODO: Powerup manager spawn CP orb based on low nybble
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
			// TODO: Powerup manager spawn HP orb based on low nybble
			break;
	}
}

static inline void clamp_dx(Cube *c)
{
	if (c->dx > 0) c->dx = kcube_on_cube_dx;
	else if (c->dx < 0) c->dx = -kcube_on_cube_dx;
}

static inline void cube_bg_bounce_sides(Cube *c)
{
	const int16_t cx_r = FIX32TOINT(c->x + c->right);
	const int16_t cx_l = FIX32TOINT(c->x + c->left);
	const int16_t cy_top = FIX32TOINT(c->y + c->top);
	const int16_t cy_bot = FIX32TOINT(c->y);
	if (c->dy > 0 && map_collision(FIX32TOINT(c->x), cy_bot + 2)) return;

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
			// TODO: Queue cube bounce sound.
		}
	}
	else if (c->dx < 0)
	{
		if (map_collision(cx_l - 1, cy_top) ||
		    map_collision(cx_l - 1, cy_bot))
		{
			// X position (in pixels) of wall being touched. We want the right
			// side of the tile, so 7 is added.
			const int16_t touching_tile_x = ((cx_l + 1) / 8) * 8;
			c->x = INTTOFIX32(touching_tile_x) - c->left + 1;
			cube_bounce_dx(c);
			c->bounce_count = 1;
			if (c->status == CUBE_STATUS_KICKED)
			{
				c->status = CUBE_STATUS_AIR;
				c->bounce_count = CUBE_INITIAL_BOUNCE_COUNT;
				c->dy = kcube_on_cube_dy;
			}
			// TODO: Queue cube bounce sound.
		}
	}
}

static inline void cube_bg_bounce_top(Cube *c)
{
	const int16_t cx_left = FIX32TOINT(c->x + c->left);
	const int16_t cx_right = FIX32TOINT(c->x + c->right);
	const int16_t cy_top = FIX32TOINT(c->y + c->top);
	uint16_t gnd_chk[2];
	gnd_chk[0] = map_collision(cx_left, cy_top - 1);
	gnd_chk[1] = map_collision(cx_right, cy_top - 1);
	// Both left and right tests count as a collison.
	if (gnd_chk[0] && gnd_chk[1])
	{
		c->dy = kceiling_dy;
		// TODO: play cube bounce sound.
	}
	else if (gnd_chk[0] || gnd_chk[1])
	{
		// One edge is missing. Check for the center.
		const uint16_t center_chk = map_collision(FIX32TOINT(c->x), cy_top - 1);

		if (center_chk)
		{
			// Center checks out, bounce.
			c->dy = kceiling_dy;
			// TODO: cube bounce sound
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
				cx = ((cx - 1) / 8) * 8;
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
	}
	else if (c->bounce_count == 0)
	{
		c->bounce_count = 1;
	}

	if (c->bounce_count > 0 && c->dx == 0 && c->dy > kbounce_cutoff)
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


	// TODO: Play bounce sound

}

static inline void cube_bg_bounce_ground(Cube *c)
{
	if (c->dy < 0) return;
	int16_t cx = FIX32TOINT(c->x);
	const int16_t cx_left = FIX32TOINT(c->x + c->left);
	const int16_t cx_right = FIX32TOINT(c->x + c->right);
	const int16_t cy_bottom = FIX32TOINT(c->y);
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
				cx = ((cx + 8) / 8) * 8;
			}
			else
			{
				cx = ((cx - 2) / 8) * 8;
			}
			c->x = INTTOFIX32(cx);
		}
	}
}

static inline void cube_bg_collision(Cube *c)
{
	const int16_t cx_left = FIX32TOINT(c->x + c->left);
	const int16_t cx_right = FIX32TOINT(c->x + c->right);
	const int16_t cy_top = FIX32TOINT(c->y + c->top);
	const int16_t cy_bottom = FIX32TOINT(c->y);
	if (c->type == CUBE_TYPE_GREEN || c->type == CUBE_TYPE_GREENBLUE)
	{
		if (c->dy > 0) cube_bg_bounce_ground(c);
		else if (c->dy < 0) cube_bg_bounce_top(c);
		if (c->dx != 0) cube_bg_bounce_sides(c);
	}
	else
	{
		if (map_collision(cx_left, cy_bottom) ||
		    map_collision(cx_right, cy_bottom) ||
		    map_collision(cx_left, cy_top) ||
		    map_collision(cx_right, cy_top))
		{
			cube_destroy(c);
		}
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

	int16_t cx = FIX32TOINT(c->x);
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
			c->dx = 0;
			c->status = CUBE_STATUS_AIR;
			c->dy = kgravity;
			// Lock to the grid.
			// TODO: I don't know about that +4
			cx = ((cx + 4) / 8) * 8;
			c->x = INTTOFIX32(cx);
		}
	}

	// Check for cube out of bounds.
	// TODO: Implement once map is in, so we have actual bounds.
	if (cx_right > map_get_right() ||
	    cx_left < 0 ||
	    cy_bottom > map_get_bottom() ||
	    cy_top < 0)
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
			clamp_dx(c);
		}
		c->dy = kcube_on_cube_dy;
		// TODO: Cue sound for cube bounce
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

		if (c->x + c->left <= d->x + c->right &&
		    c->x + c->right >= d->x + c->left &&
		    c->y + c->top <= d->y &&
		    c->y >= d->y + c->top)
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

// Simplified collision for spawner cubes that reset the spawn count if a blue
// cube exists in the same location.
static inline void spawn_touch_check(Cube *c)
{
	uint16_t i = ARRAYSIZE(g_cubes);
	while (i--)
	{
		Cube *d = &g_cubes[i];
		if (d->status == CUBE_STATUS_NULL) continue;
		if (d->type != CUBE_TYPE_BLUE) continue;
		if (d->x != c->x || d->y != c->y) continue;
		c->spawn_count = 0;
		d->spawn_count = 2;  // spawn_count is used to make blue cubes flash.
		break;
	}
}

static inline void cube_render(Cube *c)
{
	CubeType render_type = c->type;
	// Render the cube.
	if (c->type == CUBE_TYPE_SPAWNER)
	{
		if (c->spawn_count < kspawn_seq[0] && c->spawn_count > 0) return;
		if ((g_elapsed >> 2) % 2 == 0) return;
	}
	else if (c->type == CUBE_TYPE_BLUE)
	{
		// Blue cubes flash the spawner colors if there is a spawner below them.
		if ((g_elapsed % 8) > 4 && c->spawn_count > 0 && (g_elapsed >> 2) % 2 == 0)
		{
			render_type = CUBE_TYPE_SPAWNER;
		}
	}

	if (render_type == CUBE_TYPE_SPAWNER && (g_elapsed % 8) < 4)
	{
		render_type = CUBE_TYPE_BLUE;
	}

	if (system_is_debug_enabled() && io_pad_read(0) & BTN_A)
	{
		int16_t sp_x = FIX32TOINT(c->x) - map_get_x_scroll();
		int16_t sp_y = FIX32TOINT(c->y) - map_get_y_scroll();
		spr_put(sp_x, sp_y, SPR_ATTR(1, 0, 0, 0, 0), SPR_SIZE(1, 1));
		spr_put(sp_x + FIX32TOINT(c->right), sp_y, SPR_ATTR(1, 0, 0, 3, 0), SPR_SIZE(1, 1));
		spr_put(sp_x + FIX32TOINT(c->left),  sp_y, SPR_ATTR(1, 0, 0, 3, 0), SPR_SIZE(1, 1));
		spr_put(sp_x + FIX32TOINT(c->right), sp_y + FIX32TOINT(c->top), SPR_ATTR(1, 0, 0, 3, 0), SPR_SIZE(1, 1));
		spr_put(sp_x + FIX32TOINT(c->left),  sp_y + FIX32TOINT(c->top), SPR_ATTR(1, 0, 0, 3, 0), SPR_SIZE(1, 1));
		return;
	}
	cube_manager_draw_cube(FIX32TOINT(c->x + c->left),
	                       FIX32TOINT(c->y + c->top), render_type);



}

void cube_run(Cube *c)
{
	if (c->type == CUBE_TYPE_BLUE)
	{
		if (c->spawn_count > 0) c->spawn_count--;
	}
	else if (c->type == CUBE_TYPE_SPAWNER)
	{
		if (c->spawn_count < kspawn_seq[1]) c->spawn_count++;
		if (c->spawn_count == kspawn_seq[1])
		{
			cube_manager_spawn(c->x, c->y, CUBE_TYPE_BLUE, CUBE_STATUS_IDLE,
			                   0, 0);
		}
		spawn_touch_check(c);
	}

	if (c->status == CUBE_STATUS_FIZZLE || c->status == CUBE_STATUS_EXPLODE)
	{
		if (c->fizzle_count > 0)
		{
			c->fizzle_count--;
			cube_scan_objects(c);
		}
		else
		{
			c->status = CUBE_STATUS_NULL;
		}
	}
	else if (c->status != CUBE_STATUS_IDLE)
	{
		// Collision is processed one frame late to mimic the original
		// game's behavior... so says the old code.
		cube_scan_objects(c);
		cube_bg_collision(c);
		cube_movement(c);
		if (c->status == CUBE_STATUS_AIR ||
		    c->status == CUBE_STATUS_KICKED)
		{
			cube_on_cube_collisions(c);
		}
	}

	// Spawn particles if needed.
	if (c->status == CUBE_STATUS_FIZZLE)
	{
		// TODO: Spawn fizzle particle in center of cube.
		return;
	}
	else if (c->status == CUBE_STATUS_EXPLODE)
	{
		// TODO: Spawn explosion particle in center of cube.
		return;
	}

	cube_render(c);
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
	if (c->spawn_count == kspawn_seq[1] - 1)
	{
		c->spawn_count--;
	}
}


