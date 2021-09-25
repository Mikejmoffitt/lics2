#include "obj/lyle.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "sfx.h"

#include "game.h"
#include "palscale.h"
#include "cube.h"
#include "obj/cube_manager.h"
#include "obj/particle_manager.h"
#include "obj/map.h"
#include "obj/pause.h"

#include "progress.h"

#include "res.h"

// Access for Lyle singleton.
O_Lyle *g_lyle;

static MdButton buttons;
static MdButton buttons_prev;

// Integer constants that are too small to meaningfully scale.
#define LYLE_ACTION_THROW_TIME 2
#define LYLE_ACTION_LIFT_TIME 2
#define LYLE_ACTION_KICK_TIME 3

#define LYLE_CUBEJUMP_DISABLE_TIME 2

// Tolerance to the bottom check for collisions so Lyle can "step up" on horizontal collisions
#define LYLE_STEP_UP INTTOFIX32(-4)

// TODO: Find real numbers from the MMF code.
#define LYLE_CP_SPAWN_PRICE 4
#define LYLE_CP_SPAWN_CHEAP 2

#define LYLE_DRAW_LEFT -8
#define LYLE_DRAW_TOP -23

// Constants!
static fix16_t kdx_max;
static fix16_t kdy_max;
static fix16_t kx_accel;
static fix16_t kgravity;
static fix16_t kgravity_weak;
static fix16_t kjump_dy;
static fix16_t kceiling_dy;
static fix16_t khurt_dx;
static fix16_t kdx_snap;  // 0.1

static fix16_t ktoss_cube_dx_short;
static fix16_t ktoss_cube_dy_short;

static fix16_t ktoss_cube_dy_up;

static fix16_t ktoss_cube_dx_strong;
static fix16_t ktoss_cube_dy_strong;

static fix16_t ktoss_cube_dx_normal;
static fix16_t ktoss_cube_dy_normal;

static fix16_t ktoss_cube_dy_down;
static fix16_t kbounce_cube_dx;
static fix16_t kbounce_cube_dy;

static fix16_t kdrop_cube_dx;
static fix16_t kdrop_cube_dy;

static fix16_t kcube_kick_dx;

static int16_t kthrow_anim_len;
static int16_t kkick_anim_len;
static int16_t kcubejump_anim_len;
static int16_t klift_time;

static int16_t khurt_time;
static int16_t khurt_timeout;
static int16_t kinvuln_time;
static int16_t kcp_restore_period;
static int16_t kcp_restore_period_fast;
static int16_t kcp_spawn_fast;
static int16_t kcp_spawn_slow;
static int16_t kcube_fx;
static int16_t kanim_speed;
static int16_t ktele_anim;

static void set_constants(void)
{
	static int16_t s_constants_set = 0;
	if (s_constants_set) return;

	kdx_max = INTTOFIX16(PALSCALE_1ST(1.41666666667));
	kdy_max = INTTOFIX16(PALSCALE_1ST(6.66666666667));
	kx_accel = INTTOFIX16(PALSCALE_2ND(0.10416666667));
	kgravity = INTTOFIX16(PALSCALE_2ND(0.15972222223));  // Was 0.21 : 0.3024
	kgravity_weak = INTTOFIX16(PALSCALE_2ND(0.0903777777777));  // Was 0.10 : 0.144
	kjump_dy = INTTOFIX16(PALSCALE_1ST(-2.94));  // was -2.94 : -3.58
	kceiling_dy = INTTOFIX16(PALSCALE_1ST(-0.416666667));  // was -0.42 : -0.5
	khurt_dx = INTTOFIX16(PALSCALE_1ST(-1.91666667));  // was -1.92 : -2.3
	kdx_snap = INTTOFIX16(PALSCALE_1ST(0.1));

	ktoss_cube_dx_short = INTTOFIX16(PALSCALE_1ST(0.83333333333));
	ktoss_cube_dy_short = INTTOFIX16(PALSCALE_1ST(-2.5));
	ktoss_cube_dy_up = INTTOFIX16(PALSCALE_1ST(-4.16666666667));
	ktoss_cube_dx_strong = INTTOFIX16(PALSCALE_1ST(3.333333333));
	ktoss_cube_dy_strong = INTTOFIX16(PALSCALE_1ST(-1.666666667));
	ktoss_cube_dx_normal = INTTOFIX16(PALSCALE_1ST(1.666666667));
	ktoss_cube_dy_normal = INTTOFIX16(PALSCALE_1ST(-2.5));
	ktoss_cube_dy_down = INTTOFIX16(PALSCALE_1ST(3.333333333333));
	kbounce_cube_dx = INTTOFIX16(PALSCALE_1ST(0.8333333333333));
	kbounce_cube_dy = INTTOFIX16(PALSCALE_1ST(-1.833));

	kdrop_cube_dx = INTTOFIX16(PALSCALE_1ST(0.8333333333333));
	kdrop_cube_dy = INTTOFIX16(PALSCALE_1ST(-2.5));

	kcube_kick_dx = INTTOFIX16(PALSCALE_1ST(2.5));

	kthrow_anim_len = PALSCALE_DURATION(10);  // was 10 : 8
	kkick_anim_len = PALSCALE_DURATION(10);
	kcubejump_anim_len = PALSCALE_DURATION(24);  // was 24 : 20
	klift_time = PALSCALE_DURATION(18) + 1;  // was 18 : 15

	khurt_time = PALSCALE_DURATION(35);  // was 35 : 29
	khurt_timeout = PALSCALE_DURATION(24);  // was 24 : 20
	kinvuln_time = PALSCALE_DURATION(95);  // was 95 : 79

	kcp_restore_period = PALSCALE_DURATION(300);  // was 300 : 250
	kcp_restore_period_fast = PALSCALE_DURATION(150);  // was 150 : 125
	kcp_spawn_fast = PALSCALE_DURATION(36);  // was 36 : 30
	kcp_spawn_slow = PALSCALE_DURATION(72);  // was 72 : 60

	kcube_fx = PALSCALE_DURATION(6.3);
	kanim_speed = PALSCALE_DURATION(6.8);
	ktele_anim = PALSCALE_DURATION(75);  // was 75 : 62

	s_constants_set = 1;
}

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_LYLE);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline uint16_t is_control_disabled(O_Lyle *l)
{
	return (l->ext_disable || l->hurt_cnt > 0 || l->head.hp == 0 ||
	        l->tele_in_cnt > 0 || l->tele_out_cnt > 0);
}

static inline void walking_sound(O_Lyle *l)
{
	if (!l->grounded && !l->on_cube) return;
	if (l->anim_cnt == kanim_speed)
	{
		sfx_stop(SFX_WALK2);
		sfx_play(SFX_WALK1, 8);
	}
	else if (l->anim_cnt == kanim_speed * 3)
	{
		sfx_stop(SFX_WALK1);
		sfx_play(SFX_WALK2, 8);
	}
}

static inline void eval_grounded(O_Lyle *l)
{
	if (l->ext_disable)
	{
		l->grounded = 1;
		return;
	}
	if (l->head.dy < 0)
	{
		l->grounded = 0;
		return;
	}

	// Check the left and right bottom pixels below Lyle's hitbox
	const uint16_t px_r = FIX32TOINT(l->head.x + l->head.right);
	const uint16_t px_l = FIX32TOINT(l->head.x + l->head.left);
	const uint16_t py = FIX32TOINT(l->head.y) + 1;
	l->grounded = (map_collision(px_r, py) || map_collision(px_l, py));
}

static inline void teleport_seq(O_Lyle *l)
{
	if (l->tele_out_cnt > 0)
	{
		if (l->holding_cube != CUBE_TYPE_NULL)
		{
			// Destroy the cube that Lyle is holding.
			Cube *c = cube_manager_spawn(l->head.x, l->head.y - INTTOFIX32(32),
			                             l->holding_cube, CUBE_STATUS_AIR,
			                             0, 0);
			cube_destroy(c);
			l->holding_cube = CUBE_TYPE_NULL;
		}
		l->tele_out_cnt--;
		if (l->tele_out_cnt == 0) map_set_exit_trigger(MAP_EXIT_TELEPORT);
	}
	else if (l->tele_in_cnt > 0)
	{
		l->tele_in_cnt--;
	}
}

static inline void decelerate_with_ddx(O_Lyle *l, fix16_t ddx)
{
	if (l->head.dx > 0)
	{
		l->head.dx = l->head.dx - ddx;
		if (l->head.dx < 0) l->head.dx = 0;
	}
	else if (l->head.dx < 0)
	{
		l->head.dx = l->head.dx + ddx;
		if (l->head.dx > 0) l->head.dx = 0;
	}
}

static inline void x_acceleration(O_Lyle *l)
{
	if (is_control_disabled(l) || l->lift_cnt > 0)
	{
		decelerate_with_ddx(l, kx_accel / 2);
		return;
	}

	// deceleration
	if (!(buttons & (BTN_RIGHT | BTN_LEFT)))
	{
		decelerate_with_ddx(l, kx_accel);
	}

	// going left and right
	if (buttons & BTN_RIGHT)
	{
		l->head.dx += kx_accel;
		l->head.direction = OBJ_DIRECTION_RIGHT;
		walking_sound(l);
	}
	else if (buttons & BTN_LEFT)
	{
		l->head.dx -= kx_accel;
		l->head.direction = OBJ_DIRECTION_LEFT;
		walking_sound(l);
	}

	// snap dx to 0 if it is near
	// TODO: Is this needed?
	if (l->head.dx > -kdx_snap &&
	    l->head.dx < kdx_snap && !(buttons & (BTN_RIGHT | BTN_LEFT)))
	{
		l->head.dx = 0;
	}

	// limit top speed
	fix16_t dx_max = kdx_max;
	if (l->holding_cube == CUBE_TYPE_ORANGE) dx_max /= 2;
	if (l->head.dx > dx_max) l->head.dx = dx_max;
	else if (l->head.dx < -dx_max) l->head.dx = -dx_max;
}

// Push the cube out of nearby walls. This is used when spawning a nwe cube,
// to prevent the player from accidentally wasting a new cube.
static inline void align_cube_to_touching_wall(Cube *c)
{
	const int16_t cx_left = FIX32TOINT(c->x + c->left);
	const int16_t cx_right = FIX32TOINT(c->x + c->right);
	const int16_t cy_bottom = FIX32TOINT(c->y);
	const int16_t cy_top = FIX32TOINT(c->y + c->top);
	uint16_t gnd_chk[2];
	gnd_chk[0] = map_collision(cx_left, cy_bottom + 1);
	gnd_chk[1] = map_collision(cx_right, cy_bottom + 1);
	if (gnd_chk[0] && !gnd_chk[1])
	{
		const int16_t touching_tile_x = 8 + ((cx_left - 1) / 8) * 8;
		c->x = INTTOFIX32(touching_tile_x) - c->left + 1;
	}
	else if (gnd_chk[1] && !gnd_chk[0])
	{
		const int16_t touching_tile_x = ((cx_right + 1) / 8) * 8;
		c->x = INTTOFIX32(touching_tile_x) - c->right - 1;
	}

	gnd_chk[0] = map_collision(cx_left, cy_top - 1);
	gnd_chk[1] = map_collision(cx_right, cy_top - 1);
	if (gnd_chk[0] || gnd_chk[1])
	{
		uint8_t i = 16;
		while (i-- &&
		       (map_collision(FIX32TOINT(c->x + c->left), FIX32TOINT(c->y + c->top)) ||
		        map_collision(FIX32TOINT(c->x + c->right), FIX32TOINT(c->y + c->top))))
		{
			c->y += INTTOFIX32(2);
		}
	}
}

static inline void toss_cubes(O_Lyle *l)
{
	if (is_control_disabled(l)) return;
	if (!l->holding_cube) return;
	if ((buttons & BTN_B) && !(buttons_prev & BTN_B))
	{
		fix16_t c_dx = 0;
		fix16_t c_dy = 0;
		// Holding down; short toss
		if ((buttons & BTN_DOWN) && (l->grounded || l->on_cube))
		{
			c_dx = ktoss_cube_dx_short;
			c_dy = ktoss_cube_dy_short;
		}
		else if (buttons & BTN_UP)
		{
			c_dx = 0;
			c_dy = ktoss_cube_dy_up;
		}
		else if (buttons & (BTN_RIGHT | BTN_LEFT))
		{
			c_dx = ktoss_cube_dx_strong;
			c_dy = ktoss_cube_dy_strong;
		}
		else
		{
			c_dx = ktoss_cube_dx_normal;
			c_dy = ktoss_cube_dy_normal;
		}

		if (l->head.direction == OBJ_DIRECTION_LEFT) c_dx = -c_dx;

		fix32_t c_y = l->head.y - INTTOFIX32(23);
		fix32_t c_x = l->head.x;

		// Convert a greenblue cube to blue.
		if (l->holding_cube == CUBE_TYPE_GREENBLUE)
		{
			l->holding_cube = CUBE_TYPE_BLUE;
		}

		Cube *c = cube_manager_spawn(c_x, c_y, l->holding_cube, CUBE_STATUS_AIR,
		                             c_dx, c_dy);
		align_cube_to_touching_wall(c);

		sfx_play(SFX_CUBE_TOSS, 5);

		l->holding_cube = CUBE_TYPE_NULL;
		l->action_cnt = LYLE_ACTION_THROW_TIME;
		l->throw_cnt = kthrow_anim_len;
	}
}

static inline void lift_cubes(O_Lyle *l)
{
	if (l->action_cnt > 0 || is_control_disabled(l)) return;

	if (l->on_cube && l->lift_cnt == 0 &&
	    buttons & BTN_B && !(buttons_prev & BTN_B))
	{
		l->lift_cnt = klift_time;
		if (l->on_cube->type == CUBE_TYPE_ORANGE) l->lift_cnt *= 2;
		l->action_cnt = LYLE_ACTION_LIFT_TIME;
		l->head.dx = 0;
	}
	if (l->lift_cnt == 1 && l->on_cube)
	{
		ProgressSlot *progress = progress_get();
		if (!(progress->touched_first_cube))
		{
			progress->touched_first_cube = 1;
			pause_set_screen(PAUSE_SCREEN_LYLE_WEAK);
		}
		if (!(progress->abilities & ABILITY_LIFT)) return;
		Cube *c = l->on_cube;
		if (c->type == CUBE_TYPE_ORANGE && !(progress->abilities & ABILITY_ORANGE)) return;
		l->holding_cube = c->type;
		c->status = CUBE_STATUS_NULL;

		// Repro of MMF1 version bug where you can jump while lifting.
		if (buttons & BTN_C)
		{
			sfx_play(SFX_JUMP, 17);
			l->head.dy = kjump_dy;
		}
		l->action_cnt = LYLE_ACTION_LIFT_TIME;
		sfx_play(SFX_CUBE_LIFT, 10);
	}
}

static inline void jump(O_Lyle *l)
{
	// Small timeout to keep Lyle from cube-jumping as he steps off a ledge, so
	// a player who hits jump late doesn't accidentally cube jump.
	if (l->grounded || l->on_cube)
	{
		l->cube_jump_disable_cnt = LYLE_CUBEJUMP_DISABLE_TIME;
	}

	if (l->lift_cnt || is_control_disabled(l)) return;

	// C button pressed down.
	if ((buttons & BTN_C) && !(buttons_prev & BTN_C))
	{
		if (l->grounded || l->on_cube)
		{
			l->head.dy = kjump_dy;
			sfx_play(SFX_JUMP, 9);
		}
		else if (l->holding_cube && !l->cube_jump_disable_cnt &&
		         (progress_get()->abilities & ABILITY_JUMP))
		{
			l->throwdown_cnt = kcubejump_anim_len;
			fix32_t c_x = l->head.x;
			fix32_t c_y = l->head.y;

			// Convert greenblue to blue
			if (l->holding_cube == CUBE_TYPE_GREENBLUE)
			{
				l->holding_cube = CUBE_TYPE_BLUE;
			}

			Cube *c = cube_manager_spawn(c_x, c_y, l->holding_cube, CUBE_STATUS_AIR,
			                             0, ktoss_cube_dy_down);

			align_cube_to_touching_wall(c);

			l->holding_cube = CUBE_TYPE_NULL;

			sfx_play(SFX_CUBE_TOSS, 5);
			l->head.dy = kjump_dy;
		}
	}
}

static inline void bg_vertical_collision(O_Lyle *l)
{
	l->head.x -= l->head.dx;

	const int16_t px_r = FIX32TOINT(l->head.x + l->head.right);
	const int16_t px_l = FIX32TOINT(l->head.x + l->head.left);
	const int16_t py_top = FIX32TOINT(l->head.y + l->head.top);
	const int16_t py_bottom = FIX32TOINT(l->head.y);
	if (l->head.dy > 0 && INTTOFIX32(py_bottom + 1) < map_get_bottom())
	{
		if (map_collision(px_r, py_bottom + 1) ||
		    map_collision(px_l, py_bottom + 1))
		{
			const int16_t touching_tile_y = ((py_bottom + 1) / 8) * 8;

			l->head.y = INTTOFIX32(touching_tile_y) - 1;
			l->head.dy = 0;
			eval_grounded(l);
		}
		// Make sure we aren't stuck in a wall, and push out if needed.
		uint8_t i = 3;
		while (i-- && (map_collision(px_r, FIX32TOINT(l->head.y)) ||
		               map_collision(px_l, FIX32TOINT(l->head.y))))
		{
			l->head.y -= INTTOFIX32(8);
		}
	}
	else if (l->head.dy < 0 && INTTOFIX32(py_top - 1) > 0)
	{
		if (map_collision(px_r, py_top - 1) ||
		    map_collision(px_l, py_top - 1))
		{
			const int16_t touching_tile_y = 8 + ((py_top - 1) / 8) * 8;
			l->head.y = INTTOFIX32(touching_tile_y) - l->head.top + 1;
			if (l->head.dy < kceiling_dy) l->head.dy = kceiling_dy;
		}
	}

	l->head.x += l->head.dx;
}

static inline void bg_horizontal_collision(O_Lyle *l)
{
	const int16_t px_r = FIX32TOINT(l->head.x + l->head.right);
	const int16_t px_l = FIX32TOINT(l->head.x + l->head.left);
	const int16_t py_top = FIX32TOINT(l->head.y + l->head.top);
	const int16_t py_mid = FIX32TOINT(l->head.y + ((l->head.top) / 2));
	const int16_t py_bot = FIX32TOINT(l->head.y + LYLE_STEP_UP);
	if (l->head.dx > 0 && INTTOFIX32(px_r + 1) < map_get_right())
	{
		if (map_collision(px_r + 1, py_top) ||
		    map_collision(px_r + 1, py_mid) ||
		    map_collision(px_r + 1, py_bot))
		{
			// X position (in pixels) of wall being touched.
			const int16_t touching_tile_x = ((px_r + 1) / 8) * 8;
			l->head.x = INTTOFIX32(touching_tile_x) - l->head.right - 1;
			l->head.dx = 0;
		}
	}
	else if (l->head.dx < 0 && INTTOFIX32(px_l - 1) > 0)
	{
		if (map_collision(px_l - 1, py_top) ||
		    map_collision(px_l - 1, py_mid) ||
		    map_collision(px_l - 1, py_bot))
		{
			// X position (in pixels) of wall being touched. We want the right
			// side of the tile, so 7 is added.
			const int16_t touching_tile_x = 8 + ((px_l - 1) / 8) * 8;
			l->head.x = INTTOFIX32(touching_tile_x) - l->head.left + 1;
			l->head.dx = 0;
		}
	}
}

// This function is called to handle vertical collision events once it is
// confirmed that Lyle is already touching the Cube.
static inline void cube_vertical_collision(O_Lyle *l, Cube *c)
{
	if (l->head.dy == 0) return;
	if (c->status == CUBE_STATUS_AIR || c->type == CUBE_TYPE_SPAWNER) return;

	if (l->head.x + l->head.right >= c->x + c->left &&
	    l->head.x + l->head.left <= c->x + c->right)
	{
		if (l->head.dy > 0 &&
		    l->head.y + 1 >= c->y + c->top &&
		    l->head.y < c->y + (c->top / 2))
		{
			l->head.y = c->y + c->top - 1;
			l->head.dy = 0;
		}
		else if (l->head.dy < 0 &&
		         l->head.y + l->head.top - 1 <= c->y &&
		         l->head.y + l->head.top > c->y + (c->top / 2))
		{
			l->head.y = c->y - l->head.top + 1;
			if (l->head.dy < kceiling_dy) l->head.dy = kceiling_dy;
		}
	}
}

// This function is called to handle horizontal collision events once it is
// confirmed that Lyle is already touching the Cube.
static inline void cube_horizontal_collision(O_Lyle *l, Cube *c)
{
	if (l->head.dx == 0) return;
	if (c->status == CUBE_STATUS_AIR || c->type == CUBE_TYPE_SPAWNER) return;

	l->head.y -= l->head.dy;

	if (l->head.y >= c->y + c->top &&
	    l->head.y + l->head.top <= c->y)
	{
		if (l->head.dx > 0 &&
		    l->head.x + l->head.right + 1 >= c->x + c->left &&
		    l->head.x < c->x)
		{
			l->head.x = c->x + c->left - l->head.right - 1;
			l->head.dx = 0;
		}
		else if (l->head.dx < 0 &&
		         l->head.x + l->head.left - 1 <= c->x + c->right &&
		         l->head.x > c->x)
		{
			l->head.x = c->x + c->right - l->head.left + 1;
			l->head.dx = 0;
		}
	}

	l->head.y += l->head.dy;
}

static inline void cube_eval_standing(O_Lyle *l, Cube *c)
{
	if (c->status == CUBE_STATUS_AIR || c->type == CUBE_TYPE_SPAWNER) return;

	const int16_t px_r = FIX32TOINT(l->head.x + l->head.right);
	const int16_t px_l = FIX32TOINT(l->head.x + l->head.left);
	const int16_t py_top = FIX32TOINT(l->head.y + l->head.top);
	if (l->head.y > c->y + c->top / 2) return;
	if (map_collision(px_r, py_top) || map_collision(px_l, py_top)) return;
	
	if (l->head.x + l->head.right > c->x + c->left &&
	    l->head.x + l->head.left < c->x + c->right)
	{
		if (l->head.y + 1 >= c->y + c->top &&
		    l->head.y < c->y + (c->top / 2))
		{
			// If we are already standing on a cube, evaluate against another.
			if (l->on_cube)
			{
				fix16_t old_x_distance = l->head.x - l->on_cube->x;
				fix16_t new_x_distance = l->head.x - c->x;
				if (old_x_distance < 0) old_x_distance = -old_x_distance;
				if (new_x_distance < 0) new_x_distance = -new_x_distance;
				// If the new cube is closer, adopt it as the standing cube.
				if (new_x_distance < old_x_distance) l->on_cube = c;
			}
			else
			{
				l->on_cube = c;
			}
		}
	}
}

static inline void cube_kick(O_Lyle *l, Cube *c)
{
	const ProgressSlot *prog = progress_get();
	if (!(prog->abilities & ABILITY_KICK)) return;
	if (c->status != CUBE_STATUS_IDLE || c->type == CUBE_TYPE_SPAWNER || c->type == CUBE_TYPE_ORANGE) return;
	if (!(l->grounded || l->on_cube)) return;
	if (l->action_cnt > 0) return;
	if (!((buttons & BTN_B) && !(buttons_prev & BTN_B))) return;

	if (l->head.y >= c->y + c->top &&
	    l->head.y + l->head.top / 2 <= c->y)
	{
		if (l->head.direction == OBJ_DIRECTION_RIGHT &&
		    l->head.x + l->head.right + 1 >= c->x + c->left &&
		    l->head.x < c->x)
		{
			l->kick_cnt = kkick_anim_len;
			c->status = CUBE_STATUS_KICKED;
			c->dx = kcube_kick_dx;
			l->action_cnt = LYLE_ACTION_KICK_TIME;
			sfx_play(SFX_CUBE_BOUNCE, 13);
		}
		else if (l->head.direction == OBJ_DIRECTION_LEFT &&
		         l->head.x + l->head.left - 1 <= c->x + c->right &&
		         l->head.x > c->x)
		{
			l->kick_cnt = kkick_anim_len;
			c->status = CUBE_STATUS_KICKED;
			c->dx = -kcube_kick_dx;
			l->action_cnt = LYLE_ACTION_KICK_TIME;
			sfx_play(SFX_CUBE_BOUNCE, 13);
		}
	}
}

static inline void cube_collision(O_Lyle *l)
{
	int i = ARRAYSIZE(g_cubes);
	l->on_cube = NULL;
	while (i--)
	{
		Cube *c = &g_cubes[i];
		if (c->status == CUBE_STATUS_NULL) continue;

		// Simple collision checking
		if (l->head.y < c->y + c->top - 1) continue;
		if (l->head.x + l->head.right + 1 < c->x + c->left) continue;
		if (l->head.x + l->head.left - 1 > c->x + c->right) continue;

		// Bounce cubes off of ones lyle is holding
		if (l->holding_cube)
		{
			if (c->status == CUBE_STATUS_AIR &&
			    c->x + c->left <= l->head.x + l->head.right + 1 &&
			    c->x + c->right >= l->head.x + l->head.left - 1 &&
			    c->y + c->top <= l->head.y - INTTOFIX32(24) &&
			    c->y >= l->head.y + l->head.top - INTTOFIX32(15))
			{
				if (c->dx == 0)
				{
					c->dx = (g_elapsed % 2) ?
					        kbounce_cube_dx :
					        -kbounce_cube_dx;
				}
				c->dy = kbounce_cube_dy;
				sfx_play(SFX_CUBE_BOUNCE, 13);
			}
		}

		// Gross vertical bounds check (bottom is done after holding cube)
		if (l->head.y + l->head.top > c->y + 1) continue;

		if (c->type == CUBE_TYPE_SPAWNER)
		{
			cube_restrict_spawn(c);
		}

		if (c->status == CUBE_STATUS_IDLE)
		{
			cube_horizontal_collision(l, c);
			cube_vertical_collision(l, c);
			cube_eval_standing(l, c);
			cube_kick(l, c);
		}
		else if (obj_touching_cube(&l->head, c))
		{
			if (c->status == CUBE_STATUS_AIR && l->throw_cnt == 0 &&
			         l->kick_cnt == 0 && l->throwdown_cnt == 0)
			{
				if (l->hurt_cnt < khurt_time - khurt_timeout &&
				    l->tele_out_cnt == 0)
				{
					if ((c->type != CUBE_TYPE_GREEN &&
					    c->type != CUBE_TYPE_GREENBLUE) ||
					    (l->head.y >= c->y))
					{
						lyle_get_hurt();
					}
					else
					{
						lyle_get_bounced();
						l->head.dy = l->head.dy / 2;
					}
				}
			}
		}
	}
}

static inline void exit_check(O_Lyle *l)
{
	const fix32_t x_margin = INTTOFIX32(6);
	const fix32_t bottom_margin = INTTOFIX32(2);
	const fix32_t top_margin = INTTOFIX32(6);
	if (l->head.x < x_margin && l->head.dx < 0)
	{
		map_set_exit_trigger(MAP_EXIT_LEFT);
	}
	else if (l->head.x > map_get_right() - x_margin && l->head.dx > 0)
	{
		map_set_exit_trigger(MAP_EXIT_RIGHT);
	}
	else if (l->head.y > map_get_bottom() - bottom_margin && l->head.dy > 0)
	{
		map_set_exit_trigger(MAP_EXIT_BOTTOM);
	}
	else if (l->head.y + l->head.top < top_margin && l->head.dy < 0)
	{
		map_set_exit_trigger(MAP_EXIT_TOP);
	}
}

static inline void gravity(O_Lyle *l)
{
	if (l->grounded || l->on_cube) return;
	// TODO: Alternate gravity when dying. Or, just use another object...

	if ((buttons & BTN_C) && l->head.dy < 0)
	{
		l->head.dy += kgravity_weak;
	}
	else
	{
		l->head.dy += kgravity;
	}

	if (l->head.dy > kdy_max) l->head.dy = kdy_max;
}

static inline void head_pushout(O_Lyle *l)
{
	if (!l->grounded && !l->on_cube) return;

	const int16_t px_r = FIX32TOINT(l->head.x + l->head.right);
	const int16_t px_l = FIX32TOINT(l->head.x + l->head.left);
	const int16_t py_top = FIX32TOINT(l->head.y + l->head.top);
	if (map_collision(px_r, py_top))
	{
		l->head.x -= INTTOFIX32(1);
	}
	else if (map_collision(px_l, py_top))
	{
		l->head.x += INTTOFIX32(1);
	}
}

static inline void check_spikes(O_Lyle *l)
{
	if (l->hurt_cnt > 0) return;
	const int16_t px = FIX32TOINT(l->head.x);
	const int16_t py_bottom = FIX32TOINT(l->head.y);
	if (map_is_tile_harmful(map_data_at(px, py_bottom)))
	{
		lyle_get_hurt();
	}
	return;
	const int16_t px_r = FIX32TOINT(l->head.x + l->head.right);
	const int16_t px_l = FIX32TOINT(l->head.x + l->head.left);
	if (map_is_tile_harmful(map_data_at(px_r, py_bottom)) ||
	    map_is_tile_harmful(map_data_at(px_l, py_bottom)))
	{
		lyle_get_hurt();
	}
}

static inline void cp(O_Lyle *l)
{
	const ProgressSlot *prog = progress_get();
	if (!(prog->abilities & ABILITY_PHANTOM)) return;

	l->cp_restore_cnt++;
	// Periodic restoration of CP.
	if (l->cp_restore_cnt >= kcp_restore_period)
	{
		l->cp_restore_cnt = 0;
		if (l->cp < LYLE_MAX_CP) l->cp++;
	}
	// Bail out if in the middle of something that voids this ability
	if (l->lift_cnt > 0 || l->hurt_cnt > 0 || l->action_cnt > 0 ||
	    is_control_disabled(l))
	{
		return;
	}

	const uint16_t cube_price = (prog->abilities & ABILITY_CHEAP_PHANTOM) ?
	                            LYLE_CP_SPAWN_CHEAP : LYLE_CP_SPAWN_PRICE;
	if (!l->holding_cube && l->cp >= cube_price)
	{
		if (buttons & BTN_B)
		{
			l->phantom_cnt++;
			if (l->phantom_cnt == kcube_fx + 1)
			{
				sfx_play(SFX_CUBE_SPAWN, 6);
			}
			else if (l->phantom_cnt == kcp_spawn_fast + 1)
			{
				sfx_stop(SFX_CUBE_SPAWN);
				sfx_play(SFX_CUBE_SPAWN, 6);
			}
		}
		else
		{
			if (l->phantom_cnt > kcube_fx)
			{
				sfx_stop(SFX_CUBE_SPAWN);
			}
			l->phantom_cnt = 0;
		}
		const uint16_t spawn_period = (prog->abilities & ABILITY_FAST_PHANTOM) ?
		                              kcp_spawn_fast : kcp_spawn_slow;
		if (l->phantom_cnt >= spawn_period)
		{
			l->holding_cube = CUBE_TYPE_PHANTOM;
			l->phantom_cnt = 0;
			l->cp -= cube_price;
		}
	}

	if (l->phantom_cnt > kcube_fx && g_elapsed % 2)
	{
		// TODO: Adjust particle spawn rate not based on elapsed, but a time
		// scalable constant.
		particle_manager_spawn(l->head.x, l->head.y - INTTOFIX32(32),
		                       PARTICLE_TYPE_SPARKLE);
	}
}

static void calc_anim_frame(O_Lyle *l)
{
	if (l->ext_disable)
	{
		return;
	}
	if (l->grounded || l->on_cube)
	{
		l->anim_cnt++;
		if (l->anim_cnt == kanim_speed * 4) l->anim_cnt = 0;
	}
	else
	{
		l->anim_cnt = 0;
	}

	// TODO: Shouldn't this skip the render instead?
	if (l->invuln_cnt && (g_elapsed % 2))
	{
		return;
	}
	// TODO: Dying sequence
	
	if (l->tele_out_cnt > 0 || l->tele_in_cnt > 0)  // Teleporting
	{
		l->anim_frame = 0x00;
	}
	else if (l->throw_cnt > 0)  // Throwing
	{
		l->anim_frame = 0x16;
	}
	else if (l->throwdown_cnt > 0)  // Throwing downwards
	{
		l->anim_frame = 0x07;
	}
	else if (l->kick_cnt > 0)  // Kicking
	{
		l->anim_frame = 0x17;
	}
	else if (l->lift_cnt > 0)  // Lifting something
	{
		l->anim_frame = 0x05;
	}
	else if (l->hurt_cnt > 0)  // Recoil from damage
	{
		l->anim_frame = 0x06;
	}
	else if (l->grounded || l->on_cube)  // Grounded
	{
		if (!(buttons & (BTN_LEFT | BTN_RIGHT))) // standing
		{
			l->anim_frame = 0x00;
		}
		else  // Walk cycle
		{
			if (l->anim_cnt < kanim_speed)
			{
				l->anim_frame = 0x02;
			}
			else if (l->anim_cnt < kanim_speed * 2)
			{
				l->anim_frame = 0x03;
			}
			else if (l->anim_cnt < kanim_speed * 3)
			{
				l->anim_frame = 0x02;
			}
			else
			{
				l->anim_frame = 0x01;
			}
		}
	}
	else  // In air
	{
		l->anim_frame = 0x04;
	}
	
	// Offset frame to an "arms up" variant
	if ((l->phantom_cnt > kcube_fx || l->holding_cube) &&
	    l->anim_frame < 0x08 && l->throwdown_cnt == 0)
	{
		l->anim_frame += 0x08;
	}
}

static inline void counters(O_Lyle *l)
{
	if (l->throwdown_cnt > 0) l->throwdown_cnt--;
	if (l->throw_cnt > 0) l->throw_cnt--;
	if (l->kick_cnt > 0) l->kick_cnt--;
	if (l->lift_cnt > 0) l->lift_cnt--;
	if (l->hurt_cnt > 0) l->hurt_cnt--;
	if (l->invuln_cnt > 0) l->invuln_cnt--;
	if (l->action_cnt > 0) l->action_cnt--;
	if (l->cube_jump_disable_cnt > 0) l->cube_jump_disable_cnt--;
}

static inline void draw(O_Lyle *l)
{
	const uint16_t odd_frame = g_elapsed % 2;
	if (l->holding_cube)
	{
		const int16_t x_offset = (l->holding_cube == CUBE_TYPE_ORANGE) ?
		                         -16 : -8;
		const int16_t y_offset = (l->holding_cube == CUBE_TYPE_ORANGE) ?
		                         LYLE_DRAW_TOP - 31 : LYLE_DRAW_TOP - 15;
		cube_manager_draw_cube(FIX32TOINT(l->head.x) + x_offset,
		                       FIX32TOINT(l->head.y) + y_offset,
		                       l->holding_cube);
	}
	if (l->invuln_cnt && odd_frame) return;

	// Teleporter in flashing
	if (l->tele_out_cnt > 0)
	{
		if ((g_elapsed % 8 > 3) && l->tele_out_cnt >= ktele_anim) return;
		if (l->tele_out_cnt < ktele_anim) return;
	}
	else if (l->tele_in_cnt > 0)
	{
		if ((g_elapsed % 8 > 3) && l->tele_in_cnt <= ktele_anim) return;
		if (l->tele_in_cnt > ktele_anim) return;
	}
	
	// Now change anim_frame to reflect the offset in VRAM
	// TODO: Make a fucking table
	const uint16_t tile_offset = (l->anim_frame < 0x14) ?
	                              l->anim_frame * 6 :
	                              (120 + (9 * (l->anim_frame - 0x14)));
	uint8_t size;
	int16_t yoff;
	int16_t xoff;

	// Set sprite size and Y offset based on frame
	if (l->anim_frame < 0x10)
	{
		size = SPR_SIZE(2, 3);
		yoff = LYLE_DRAW_TOP;
		xoff = LYLE_DRAW_LEFT + ((l->lift_cnt > 0) ?
		                         (l->lift_cnt / 2) % 2 : 0);
	}
	else if (l->anim_frame < 0x14)
	{
		size = SPR_SIZE(3, 2);
		yoff = LYLE_DRAW_TOP + 8;
		xoff = LYLE_DRAW_LEFT -4;
	}
	else
	{
		size = SPR_SIZE(3, 3);
		yoff = LYLE_DRAW_TOP;
		xoff = LYLE_DRAW_LEFT + (l->head.direction == OBJ_DIRECTION_LEFT ? -8 : 0);
	}

	int16_t sp_x = FIX32TOINT(l->head.x) + xoff - map_get_x_scroll();
	int16_t sp_y = FIX32TOINT(l->head.y) + yoff - map_get_y_scroll();

	// TODO: Dying sequence stuff
	if (sp_x < -32 || sp_x > GAME_SCREEN_W_PIXELS) return;
	if (sp_y < -32 || sp_y > GAME_SCREEN_H_PIXELS) return;

	spr_put(sp_x, sp_y,
	        SPR_ATTR(s_vram_pos + tile_offset,
	                 l->head.direction == OBJ_DIRECTION_LEFT, 0,
	                 LYLE_PAL_LINE, l->priority),
	        size);
}

static inline void set_map_scroll(const O_Lyle *l)
{
	if (!l->scroll_disable_h)
	{
		int16_t px = FIX32TOINT(l->head.x);
		const int16_t left_bound = GAME_SCREEN_W_PIXELS / 2;
		px -= left_bound;
		map_set_x_scroll(px);
	}
	if (!l->scroll_disable_v)
	{
		int16_t py;
		if (map_get_bottom() <= INTTOFIX32(240))
		{
			py = (system_is_ntsc() ? 8 : 0);
		}
		else
		{
			py = FIX32TOINT(l->head.y);
			const int16_t top_bound = GAME_SCREEN_H_PIXELS / 2;
			py -= top_bound;
		}

		map_set_y_scroll(py);
	}
}

static inline void update_exploration(O_Lyle *l)
{
	if (l->ext_disable || l->head.hp == 0) return;
	ProgressSlot *prog = progress_get();
	const int16_t px = FIX32TOINT(l->head.x);
	const int16_t py = FIX32TOINT(l->head.y);
	const uint16_t x_index = map_get_world_x_tile() + (px / GAME_SCREEN_W_PIXELS);
	const uint16_t y_index = map_get_world_y_tile() + (py / GAME_SCREEN_H_PIXELS);
	if (y_index >= ARRAYSIZE(prog->map_explored)) return;
	if (x_index >= ARRAYSIZE(prog->map_explored[0])) return;
	prog->map_explored[y_index][x_index] = 1;
}

static void main_func(Obj *o)
{
	O_Lyle *l = (O_Lyle *)o;
	if (!l->full_disable)
	{
		buttons_prev = buttons;
		buttons = io_pad_read(0);

		teleport_seq(l);
		x_acceleration(l);
		toss_cubes(l);
		lift_cubes(l);
		cp(l);
		jump(l);

		system_profile(PALRGB(7, 0, 0));
		obj_standard_physics(&l->head);
		bg_vertical_collision(l);
		bg_horizontal_collision(l);
		head_pushout(l);
		eval_grounded(l);
		system_profile(PALRGB(0, 4, 3));
		cube_collision(l);
		system_profile(PALRGB(3, 1, 1));
		exit_check(l);
		gravity(l);

		system_profile(PALRGB(3, 1, 5));
		check_spikes(l);
		system_profile(PALRGB(6, 6, 7));
		calc_anim_frame(l);
		system_profile(PALRGB(1, 1, 1));
		counters(l);
		set_map_scroll(l);
		system_profile(PALRGB(4, 4, 0));
	}
	draw(l);
	system_profile(PALRGB(0, 0, 0));

	update_exploration(l);
}

void o_load_lyle(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Lyle) <= sizeof(ObjSlot));
	(void)data;

	if (g_lyle)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}

	g_lyle = (O_Lyle *)o;

	vram_load();
	set_constants();

	// Lyle is not marked tangible because he does his own cube detection.
	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE,
	                  LYLE_LEFT, LYLE_RIGHT, LYLE_TOP, 5);

	o->main_func = main_func;
	o->cube_func = NULL;

	pal_upload(LYLE_CRAM_POSITION, res_pal_lyle_bin, sizeof(res_pal_lyle_bin) / 2);
}

void o_unload_lyle(void)
{
	if (!s_vram_pos) return;

	g_lyle = NULL;

	s_vram_pos = 0;
}

// Public functions that act on the Lyle singleton
void lyle_get_bounced(void)
{
	if (!g_lyle) return;
	g_lyle->head.dy = kjump_dy;
	g_lyle->head.dx = (g_lyle->head.direction == OBJ_DIRECTION_RIGHT) ?
	                khurt_dx : -khurt_dx;
}

void lyle_get_hurt(void)
{
	if (!g_lyle) return;
	if (g_lyle->tele_out_cnt > 0) return;
	if (g_lyle->invuln_cnt != 0) return;
	if (g_lyle->hurt_cnt != 0) return;
	lyle_get_bounced();
	g_lyle->hurt_cnt = khurt_time;
	g_lyle->invuln_cnt = kinvuln_time;
	g_lyle->phantom_cnt = 0;

	if (g_lyle->head.hp > 0) g_lyle->head.hp--;

	// Cubes that are held get dropped
	if (g_lyle->holding_cube != CUBE_TYPE_NULL)
	{
		cube_manager_spawn(g_lyle->head.x, g_lyle->head.y - INTTOFIX32(12),
		                   g_lyle->holding_cube, CUBE_STATUS_AIR,
		                   g_lyle->head.direction == OBJ_DIRECTION_RIGHT ?
		                   kdrop_cube_dx : -kdrop_cube_dx, kdrop_cube_dy);
		g_lyle->holding_cube = CUBE_TYPE_NULL;
	}

	sfx_stop(SFX_CUBE_SPAWN);
	sfx_play(SFX_HURT, 4);
}

fix32_t lyle_get_x(void)
{
	if (!g_lyle) return 0;
	return g_lyle->head.x;
}

fix32_t lyle_get_y(void)
{
	if (!g_lyle) return 0;
	return g_lyle->head.y;
}

int16_t lyle_get_hp(void)
{
	if (!g_lyle) return 0;
	return g_lyle->head.hp;
}

void lyle_set_hp(int16_t hp)
{
	if (!g_lyle) return;
	g_lyle->head.hp = hp;
}

int16_t lyle_get_cp(void)
{
	if (!g_lyle) return 0;
	return g_lyle->cp;
}

void lyle_set_pos(fix32_t x, fix32_t y)
{
	if (!g_lyle) return;
	g_lyle->head.x = x;
	g_lyle->head.y = y;
}

void lyle_set_direction(ObjDirection d)
{
	if (!g_lyle) return;
	g_lyle->head.direction = d;
}

void lyle_set_scroll_h_en(int16_t en)
{
	if (!g_lyle) return;
	g_lyle->scroll_disable_h = !en;
}

void lyle_set_scroll_v_en(int16_t en)
{
	if (!g_lyle) return;
	g_lyle->scroll_disable_v = !en;
}

void lyle_set_control_en(int16_t en)
{
	if (!g_lyle) return;
	g_lyle->ext_disable = !en;
}

void lyle_set_master_en(int16_t en)
{
	if (!g_lyle) return;
	g_lyle->full_disable = !en;
}
