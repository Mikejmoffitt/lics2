#include "obj/lyle.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"

#include "game.h"
#include "palscale.h"
#include "cube.h"
#include "obj/cube_manager.h"
#include "obj/map.h"

#include "res.h"

// Access for Lyle singleton.
static O_Lyle *lyle;

// Integer constants that are too small to meaningfully scale.
#define LYLE_ACTION_THROW_TIME 2
#define LYLE_ACTION_LIFT_TIME 2
#define LYLE_ACTION_KICK_TIME 3

#define LYLE_CUBEJUMP_DISABLE_TIME 2

#define LYLE_START_CP 5
#define LYLE_MAX_CP 30

// Tolerance to the bottom check for collisions so Lyle can "step up" on cubes
#define LYLE_STEP_UP INTTOFIX32(-3)

// TODO: Find real numbers from the MMF code.
#define LYLE_CP_SPAWN_PRICE 4
#define LYLE_CP_SPAWN_CHEAP 2

#define LYLE_DRAW_LEFT -7
#define LYLE_DRAW_TOP -23

#define LYLE_PALETTE 3

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

static fix16_t ktoss_cube_dx_short;  // 0.83333333333 : 1.0
static fix16_t ktoss_cube_dy_short;  // -1.77 : -2.0  should be -1.6666666667

static fix16_t ktoss_cube_dy_up;  // -4.2 : -5.0 should be -4.1666666667

static fix16_t ktoss_cube_dx_strong;  // 3.33333333 : 4
static fix16_t ktoss_cube_dx_normal;  // 1.66666667 : 2
static fix16_t ktoss_cube_dy_normal;  // -0.833333333333 : -1.0

static fix16_t ktoss_cube_dy_down;
static fix16_t kbounce_cube_dx;
static fix16_t kbounce_cube_dy;

static fix16_t kdrop_cube_dx;
static fix16_t kdrop_cube_dy;

static fix16_t kcube_kick_dx;

static uint16_t kthrow_anim_len;
static uint16_t kkick_anim_len;
static uint16_t kcubejump_anim_len;
static uint16_t klift_time;

static uint16_t khurt_time;
static uint16_t khurt_timeout;
static uint16_t kinvuln_time;
static uint16_t kcp_restore_period;
static uint16_t kcp_restore_period_fast;
static uint16_t kcp_spawn_fast;
static uint16_t kcp_spawn_slow;
static uint16_t kcube_fx;
static uint16_t kanim_speed;
static uint16_t ktele_anim;

static void set_constants(void)
{
	static int16_t constants_set = 0;
	if (constants_set) return;

	kdx_max = INTTOFIX16(PALSCALE_1ST(1.5));  // Was 1.54 : 1.8
	kdy_max = INTTOFIX16(PALSCALE_1ST(6.666667));  // Was just 6.67 : 8.0
	kx_accel = INTTOFIX16(PALSCALE_2ND(0.125));  // Was 0.125 : 0.18
	kgravity = INTTOFIX16(PALSCALE_2ND(0.21));  // Was 0.21 : 0.3024
	kgravity_weak = INTTOFIX16(PALSCALE_2ND(0.10));  // Was 0.10 : 0.144
	kjump_dy = INTTOFIX16(PALSCALE_1ST(-2.9833333));  // was -2.94 : -3.58
	kceiling_dy = INTTOFIX16(PALSCALE_1ST(-0.416666667));  // was -0.42 : -0.5
	khurt_dx = INTTOFIX16(PALSCALE_1ST(-1.91666667));  // was -1.92 : -2.3
	kdx_snap = INTTOFIX16(PALSCALE_1ST(0.1));

	ktoss_cube_dx_short = INTTOFIX16(PALSCALE_1ST(0.83333333333));
	ktoss_cube_dy_short = INTTOFIX16(PALSCALE_1ST(-1.666666667));
	ktoss_cube_dy_up = INTTOFIX16(PALSCALE_1ST(-4.16666666667));
	ktoss_cube_dx_strong = INTTOFIX16(PALSCALE_1ST(3.333333333));
	ktoss_cube_dx_normal = INTTOFIX16(PALSCALE_1ST(1.666666667));
	ktoss_cube_dy_normal = INTTOFIX16(PALSCALE_1ST(-0.833333333));
	ktoss_cube_dy_down = INTTOFIX16(PALSCALE_1ST(3.333333333333));
	kbounce_cube_dx = INTTOFIX16(PALSCALE_1ST(0.8333333333333));
	kbounce_cube_dy = INTTOFIX16(PALSCALE_1ST(-1.833));

	kdrop_cube_dx = INTTOFIX16(PALSCALE_1ST(0.8333333333333));
	kdrop_cube_dy = INTTOFIX16(PALSCALE_1ST(-2.5));

	kcube_kick_dx = INTTOFIX16(2.5);

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

	kcube_fx = PALSCALE_DURATION(6);
	kanim_speed = PALSCALE_DURATION(6);
	ktele_anim = PALSCALE_DURATION(75);  // was 75 : 62

	constants_set = 1;
}


static uint16_t vram_pos;

static void vram_load(void)
{
	if (vram_pos) return;

	const Gfx *g = gfx_get(GFX_LYLE);
	vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline uint16_t is_control_disabled(O_Lyle *l)
{
	return (l->ext_disable || l->hurt_cnt > 0 || l->head.hp == 0 ||
	        l->tele_in_cnt > 0 || l->tele_out_cnt > 0);
}

static inline void walking_sound(O_Lyle *l)
{
	// TODO: Cue walkg sounds.
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
	const uint16_t px_r = FIX32TOINT(l->head.x + l->head.right) - 1;
	const uint16_t px_l = FIX32TOINT(l->head.x + l->head.left) + 1;
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
			l->holding_cube = NULL;
		}
		l->tele_out_cnt--;
		// if (l->tele_out_cnt == 0)  // TODO: Trigger room transition.
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
	if (l->control_disabled || l->lift_cnt > 0)
	{
		decelerate_with_ddx(l, kx_accel / 2);
		return;
	}

	// deceleration
	if (!(l->buttons & (BTN_RIGHT | BTN_LEFT)))
	{
		decelerate_with_ddx(l, kx_accel);
	}

	// going left and right
	if (l->buttons & BTN_RIGHT)
	{
		l->head.dx += kx_accel;
		l->head.direction = OBJ_DIRECTION_RIGHT;
		walking_sound(l);
	}
	else if (l->buttons & BTN_LEFT)
	{
		l->head.dx -= kx_accel;
		l->head.direction = OBJ_DIRECTION_LEFT;
		walking_sound(l);
	}

	// snap dx to 0 if it is near
	// TODO: Is this needed?
	if (l->head.dx > -kdx_snap &&
	    l->head.dx < kdx_snap && !(l->buttons & (BTN_RIGHT | BTN_LEFT)))
	{
		l->head.dx = 0;
	}

	// limit top speed
	if (l->head.dx > kdx_max) l->head.dx = kdx_max;
	else if (l->head.dx < -kdx_max) l->head.dx = -kdx_max;
}

static inline void toss_cubes(O_Lyle *l)
{
	if (l->control_disabled) return;
	if (!l->holding_cube) return;
	if ((l->buttons & BTN_B) && !(l->buttons_prev & BTN_B))
	{
		fix16_t c_dx = 0;
		fix16_t c_dy = 0;
		// Holding down; short toss
		if ((l->buttons & BTN_DOWN) && (l->grounded || l->on_cube))
		{
			c_dx = ktoss_cube_dx_short;
			c_dy = ktoss_cube_dy_short;
		}
		else if (l->buttons & BTN_UP)
		{
			c_dx = 0;
			c_dy = ktoss_cube_dy_up;
		}
		else if (l->buttons & (BTN_RIGHT | BTN_LEFT))
		{
			c_dx = ktoss_cube_dx_strong;
			c_dy = ktoss_cube_dy_normal;
		}
		else
		{
			c_dx = ktoss_cube_dx_normal;
			c_dy = ktoss_cube_dy_normal;
		}

		if (l->head.direction == OBJ_DIRECTION_LEFT) c_dx = -c_dx;

		fix32_t c_y = l->head.y - INTTOFIX32(23);
		fix32_t c_x = l->head.x;

		// TODO: Check if the newly created cube would be in a wall, and push
		// it down if needed by two tiles at most.

		// Convert a greenblue cube to blue.
		if (l->holding_cube == CUBE_TYPE_GREENBLUE)
		{
			l->holding_cube = CUBE_TYPE_BLUE;
		}

		cube_manager_spawn(c_x, c_y, l->holding_cube, CUBE_STATUS_AIR,
		                   c_dx, c_dy);

		// TODO: cue cube toss sound

		l->holding_cube = CUBE_TYPE_NULL;
		l->action_cnt = LYLE_ACTION_THROW_TIME;
		l->throw_cnt = kthrow_anim_len;
	}
}

static inline void lift_cubes(O_Lyle *l)
{
	// TODO: Check if the player has the lift ability.
	// if (!save_have_lift()) l->lift_fail = 1;
	if (l->action_cnt > 0 || l->control_disabled) return;

	if (l->on_cube && l->lift_cnt == 0 &&
	    l->buttons & BTN_B && !(l->buttons_prev & BTN_B))
	{
		l->lift_cnt = klift_time;
		l->action_cnt = LYLE_ACTION_LIFT_TIME;
		l->head.dx = 0;
	}
	if (l->lift_cnt == 1 && l->on_cube)
	{
		if (l->lift_fail) l->lift_fail = 0;
		else
		{
			Cube *c = l->on_cube;
			l->holding_cube = c->type;
			c->status = CUBE_STATUS_NULL;

			// Repro of MMF1 version bug where you can jump while lifting.
			if (l->buttons & BTN_C) l->head.dy = kjump_dy;
			l->action_cnt = LYLE_ACTION_LIFT_TIME;
			// TODO: Cue cube lift sound
		}
	}
}

static inline void jump(O_Lyle *l)
{
	// Small timeout to keep Lyle from cube-jumping as he steps off a ledge, so
	// a player who hits jump late doesn't accidentally cube jump.
	if (l->grounded || l->on_cube)
	{
		l->cubejump_disable = LYLE_CUBEJUMP_DISABLE_TIME;
	}

	if (l->lift_cnt || l->control_disabled) return;

	// C button pressed down.
	if ((l->buttons & BTN_C) && !(l->buttons_prev & BTN_C))
	{
		if (l->grounded || l->on_cube)
		{
			// TODO: Cue jump sound.
			l->head.dy = kjump_dy;
		}
		// TODO: Add "player has cube jump" check once save data is available
		else if (l->holding_cube && !l->cubejump_disable)
		{
			l->throwdown_cnt = kcubejump_anim_len;

			// If the wall behind the player is solid, snap the cube's X to it
			// so the tube does not fizzle immediately on throw.
			fix32_t c_x = l->head.x;
			fix32_t c_y = l->head.y;
			fix32_t back_x = (l->head.direction == OBJ_DIRECTION_LEFT) ?
			                 (c_x + l->head.right + INTTOFIX16(4)) :
			                 (c_x + l->head.left - INTTOFIX16(4));

			// Align the cube
			if (map_collision(FIX32TOINT(back_x), FIX32TOINT(l->head.y)))
			{
				c_x = INTTOFIX32(8 * ((FIX32TOINT(c_x) + 4 ) / 8));
			}
			

			// Convert greenblue to blue
			if (l->holding_cube == CUBE_TYPE_GREENBLUE)
			{
				l->holding_cube = CUBE_TYPE_BLUE;
			}

			cube_manager_spawn(c_x, c_y, l->holding_cube, CUBE_STATUS_AIR,
			                   0, ktoss_cube_dy_down);

			l->holding_cube = CUBE_TYPE_NULL;
			// TODO: cue cube toss sound
			l->head.dy = kjump_dy;
		}
	}
}

static inline void bg_vertical_collision(O_Lyle *l)
{
	int16_t py = FIX32TOINT(l->head.y);

	l->head.x -= l->head.dx;

	const int16_t px_r_corner = FIX32TOINT(l->head.x + l->head.right) - 1;
	const int16_t px_l_corner = FIX32TOINT(l->head.x + l->head.left) + 1;
	const int16_t py_top = FIX32TOINT(l->head.y + l->head.top);
	if (l->ext_disable) return;
	if (l->head.dy > 0)
	{
		if (map_collision(px_r_corner, py) ||
		    map_collision(px_l_corner, py))
		{
			l->head.dy = 0;
			// Snap to nearest 8px boundary
			// TODO: This logic looks a little fucky, play with it more
			py = 8 * (py / 8) - 1;

			// Loop to check if we are stuck.
			uint8_t i = 8;
			while (i--)
			{
				if (map_collision(px_r_corner, py) ||
			        map_collision(px_l_corner, py))
				{
					py -= 1;
				}
				else
				{
					break;
				}
			}

			eval_grounded(l);
			// If we somehow popped above the ground, rectify it
			// TODO: Remove, shouldn't this be pointless
			if (!l->grounded)
			{
				py += 8;
				eval_grounded(l);
			}
			l->head.y = INTTOFIX32(py);
		}
	}
	else if (l->head.dy < 0)
	{
		if (map_collision(px_r_corner, py_top) ||
		    map_collision(px_l_corner, py_top))
		{
			// Snap to nearest 8px boundary
			// TODO: This logic looks a little fucky, play with it more
			py = 8 * ((py + 4) / 8) + 3;
			if (l->head.dy < kceiling_dy)
			{
				l->head.dy = kceiling_dy;
			}
			l->head.y = INTTOFIX32(py);
		}
	}

	l->head.x += l->head.dx;
}

static inline void bg_horizontal_collision(O_Lyle *l)
{
	int16_t px = FIX32TOINT(l->head.x);
	const int16_t px_r = FIX32TOINT(l->head.x + l->head.right);
	const int16_t px_l = FIX32TOINT(l->head.x + l->head.left);
	const int16_t py_top = FIX32TOINT(l->head.y + l->head.top) + 1;
	const int16_t py_mid = FIX32TOINT(l->head.y + (l->head.top) / 2);
	const int16_t py_bot = FIX32TOINT(l->head.y + LYLE_STEP_UP);
	if (l->ext_disable) return;
	if (l->head.dx > 0)
	{
		if (map_collision(px_r, py_top) ||
		    map_collision(px_r, py_mid) ||
		    map_collision(px_r, py_bot))
		{
			px = (8 * (px / 8)) + 3;
			l->head.x = INTTOFIX32(px);
			l->head.dx = 0;
		}
	}
	else if (l->head.dx < 0)
	{
		if (map_collision(px_l, py_top) ||
		    map_collision(px_l, py_mid) ||
		    map_collision(px_l, py_bot))
		{
			px = (8 * (px / 8)) + 5;
			l->head.x = INTTOFIX32(px);
			l->head.dx = 0;
		}
	}

}

// TODO: Are we going to have to do collisions in the pixel domain?

// This function is called to handle vertical collision events once it is
// confirmed that Lyle is already touching the Cube.
static inline void cube_vertical_collision(O_Lyle *l, Cube *c)
{
	if (c->type == CUBE_TYPE_SPAWNER) return;
	
	// TODO: Originally I subtracted dx preemptively...
	const fix32_t preemptive_x = l->head.x - l->head.dx;

	if (c->x + c->left > preemptive_x + l->head.right ||
	    c->x + c->right < preemptive_x + l->head.left)
	{
		return;
	}

	// We are in the correct horizontal range...
	if (l->head.dy > 0)
	{
		// Feet stuck in cube?
		if (l->head.y > c->y + c->top - 1&& l->head.y + l->head.top < c->y)
		{
			// Snap to cube
			l->head.y = c->y + c->top - 1;
			l->head.dy = 0;
		}
	}
	else if (l->head.dy < 0)
	{
		// Head stuck in cube?
		if (l->head.y + l->head.top < c->y && l->head.y > c->y)
		{
			l->head.y = c->y - l->head.top;
			if (l->head.dy < kceiling_dy)
			{
				l->head.dy = kceiling_dy;
			}
		}
	}
}

// This function is called to handle horizontal collision events once it is
// confirmed that Lyle is already touching the Cube.
static inline void cube_horizontal_collision(O_Lyle *l, Cube *c)
{
	if (c->status == CUBE_STATUS_AIR || c->type == CUBE_TYPE_SPAWNER) return;

	if (!(c->y + c->top <= l->head.y - 1 &&
	      c->y >= l->head.y + l->head.top + 1))
	{
		return;
	}
	// If the cube is to the right of the player...
	if (l->head.dx > 0 && c->x + c->left > l->head.x)
	{
		l->head.x = c->x + c->left - l->head.right - 1;
		l->head.dx = 0;
	}
	// and to the left
	else if (l->head.dx < 0 && c->x + c->right < l->head.x)
	{
		l->head.x = c->x + c->right - l->head.left + 1;
		l->head.dx = 0;
	}
}

static inline void cube_eval_standing(O_Lyle *l, Cube *c)
{
	if (c->type == CUBE_TYPE_SPAWNER) return;
	if (c->x + c->left <= l->head.x + l->head.right - 1 &&
	    c->x + c->right >= l->head.x + l->head.left + 1 &&
	    l->head.y + 1 >= c->y + c->top &&
	    l->head.y + l->head.top < c->y + c->top)  // TODO: Suspicious
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

static inline void cube_kick(O_Lyle *l, Cube *c)
{
	if (c->status != CUBE_STATUS_IDLE) return;
	if (!(l->grounded || l->on_cube)) return;
	if (l->action_cnt > 0) return;

	if ((l->buttons & BTN_B) && !(l->buttons_prev & BTN_B) &&
	    c->y + c->top <= l->head.y - 1 &&
	    c->y >= l->head.y + l->head.top / 2)
	{
		if (l->head.x == c->x + c->left - l->head.right - 1 &&
		    l->head.direction == OBJ_DIRECTION_RIGHT)
		{
			l->kick_cnt = kkick_anim_len;
			c->status = CUBE_STATUS_KICKED;
			c->dx = kcube_kick_dx;
			l->action_cnt = LYLE_ACTION_KICK_TIME;
			// TODO: Play cube bounce sound
			// TODO: Do we care about a cube's "direction"?
		}
		else if (l->head.x == c->x + c->right - l->head.left + 1 &&
		         l->head.direction == OBJ_DIRECTION_LEFT)
		{
			l->kick_cnt = kkick_anim_len;
			c->status = CUBE_STATUS_KICKED;
			c->dx = -kcube_kick_dx;
			l->action_cnt = LYLE_ACTION_KICK_TIME;
			// TODO: Play cube bounce sound
			// TODO: Do we care about a cube's "direction"?
		}
	}
}

static inline void cube_collision(O_Lyle *l)
{
	pal_set(0, 0);
	if (l->ext_disable) return;
	int i = ARRAYSIZE(g_cubes);
	while (i--)
	{
		Cube *c = &g_cubes[i];
		if (c->status == CUBE_STATUS_NULL) continue;
		if (obj_touching_cube(&l->head, c))
		{
			// TODO: If SRAM says we have't touched a cube, show the message.
			if (c->type == CUBE_TYPE_SPAWNER)
			{
				cube_restrict_spawn(c);
			}
			else if (c->status == CUBE_STATUS_IDLE)
			{
				cube_vertical_collision(l, c);
				cube_horizontal_collision(l, c);
				cube_eval_standing(l, c);
				cube_kick(l, c);
			}
			else if (c->status == CUBE_STATUS_AIR && l->throw_cnt == 0 &&
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
						l->head.dy = l->head.dy << 1;
					}
				}
			}

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
						c->dx = g_elapsed % 2 ? kbounce_cube_dx : -kbounce_cube_dx;
					}
					c->dy = kbounce_cube_dy;
					// TODO: Cue cube bounce sound
				}
			}
		}
	}
}

static inline void move(O_Lyle *l)
{
	l->on_cube = NULL;
	bg_vertical_collision(l);
	bg_horizontal_collision(l);
	eval_grounded(l);
	cube_collision(l);

	// Variable gravity based on the jump button.
	if (!l->grounded && !l->ext_disable)
	{
		// TODO: Alternate gravity when dying. Or, just use another object...

		if (((l->buttons & BTN_C) && !l->control_disabled) && l->head.dy < 0)
		{
			l->head.dy += kgravity_weak;
		}
		else
		{
			l->head.dy += kgravity;
		}

		if (l->head.dy > kdy_max) l->head.dy = kdy_max;
	}
}

static inline void check_spikes(O_Lyle *l)
{
	// TODO: Check agains BG for tiles in spikes range
	// TODO: Would be nice to instead have a LUT applied to the tileset.
}

static inline void cp(O_Lyle *l)
{
	// TODO: Bail out if Phantom power has not been acquired.

	l->cp_restore_cnt++;
	// Periodic restoration of CP.
	if (l->cp_restore_cnt >= kcp_restore_period)
	{
		l->cp_restore_cnt = 0;
		if (l->cp < LYLE_MAX_CP) l->cp++;
	}
	// Bail out if in the middle of something that voids this ability
	if (l->lift_cnt > 0 || l->hurt_cnt > 0 || l->action_cnt > 0 ||
	    l->control_disabled)
	{
		return;
	}

	// TODO: Change price to LYLE_CP_SPAWN_CHEAP for cheap phantom powerup.
	const uint16_t cube_price = LYLE_CP_SPAWN_PRICE;
	if (!l->holding_cube && l->cp >= cube_price)
	{
		if (l->buttons & BTN_B)
		{
			l->cp_cnt++;
			if (l->cp_cnt == kcube_fx + 1)
			{
				// TODO: Cue cube spawn sound
			}
			else if (l->cp_cnt == kcp_spawn_fast + 1)
			{
				// TODO: Cue cube spawn sound
			}
		}
		else
		{
			if (l->cp_cnt > kcube_fx)
			{
				// TODO: Stop cube spawn sound
			}
			l->cp_cnt = 0;
		}
		const uint16_t spawn_period = kcp_spawn_slow;  // TODO: kcp_spawn_fast for fast phantom
		if (l->cp_cnt >= spawn_period)
		{
			l->holding_cube = CUBE_TYPE_PHANTOM;
			l->cp_cnt = 0;
			l->cp -= cube_price;
		}
	}

	if (l->cp_cnt > kcube_fx && g_elapsed % 2)
	{
		// TODO: Spawn sparkle particles
	}
}

static void calc_anim_frame(O_Lyle *l)
{
	if (l->ext_disable)
	{
		l->anim_frame = (l->holding_cube ? 0x08 : 0x00);
		l->anim_cnt = 0;
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
		if (!(l->buttons & (BTN_LEFT | BTN_RIGHT))) // standing
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
	if ((l->cp_cnt > kcube_fx || l->holding_cube) &&
	    l->anim_frame < 0x08)
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
	if (l->cubejump_disable > 0) l->cubejump_disable--;
}

static inline void draw(O_Lyle *l)
{
	const uint16_t odd_frame = g_elapsed % 2;
	if (l->holding_cube)
	{
		// TODO: Position cube differently if it is orange.
		cube_manager_draw_cube(FIX32TOINT(l->head.x) + LYLE_DRAW_LEFT,
		                       FIX32TOINT(l->head.y) + LYLE_DRAW_TOP - 15,
		                       l->holding_cube);
	}
	if (l->invuln_cnt && odd_frame) return;

	// Teleporter in flashing
	if (l->tele_out_cnt > 0)
	{
		if (odd_frame && l->tele_out_cnt >= ktele_anim) return;
		if (l->tele_out_cnt < ktele_anim) return;
	}
	else if (l->tele_in_cnt > 0)
	{
		if (odd_frame && l->tele_in_cnt <= ktele_anim) return;
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
		xoff = LYLE_DRAW_LEFT + (l->lift_cnt > 0 ? (l->lift_cnt / 2) % 2 : 0);
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

	if (sp_x < -32 || sp_x > GAME_SCREEN_W_PIXELS) return;
	if (sp_y < -32 || sp_y > GAME_SCREEN_H_PIXELS) return;

	// TODO: Dying sequence stuff
	spr_put(sp_x, sp_y,
	        SPR_ATTR(vram_pos + tile_offset,
	                 l->head.direction == OBJ_DIRECTION_LEFT, 0,
	                 LYLE_PALETTE, 0),
	        size);


}

static inline void set_map_scroll(const O_Lyle *l)
{
	int16_t px = FIX32TOINT(l->head.x);
	int16_t py = FIX32TOINT(l->head.y);
	const int16_t left_bound = GAME_SCREEN_W_PIXELS / 2;
	const int16_t top_bound = GAME_SCREEN_H_PIXELS / 2;
	px -= left_bound;
	py -= top_bound;
	map_set_scroll(px, py);
}

static void main_func(Obj *o)
{
	pal_set(0, 0);
	O_Lyle *l = (O_Lyle *)o;
	lyle = l;

	l->buttons_prev = l->buttons;
	l->buttons = io_pad_read(0);

	l->control_disabled = is_control_disabled(l);
	teleport_seq(l);
	x_acceleration(l);
	toss_cubes(l);
	lift_cubes(l);
	jump(l);
	obj_standard_physics(o);
	move(l);
	cp(l);
	eval_grounded(l);
	calc_anim_frame(l);
	// entrance_collision(l);  // TODO: Remove this, because entrances can
	                           // be objects that collide normally
	counters(l);
	set_map_scroll(l);
	draw(l);
}

void o_load_lyle(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Lyle) <= sizeof(ObjSlot));
	(void)data;

	if (lyle)
	{
		o->status = OBJ_STATUS_NULL;
		return;
	}

	lyle = (O_Lyle *)o;

	vram_load();
	set_constants();

	// Lyle is not marked tangible because he does his own cube detection.
	obj_basic_init(o, OBJ_FLAG_ALWAYS_ACTIVE, INTTOFIX16(-5),
	               INTTOFIX16(5), INTTOFIX16(-18), 5);

	o->main_func = main_func;
	o->cube_func = NULL;

	pal_upload(LYLE_CRAM_POSITION, res_pal_lyle_bin, sizeof(res_pal_lyle_bin) / 2);
}

void o_unload_lyle(void)
{
	if (!vram_pos) return;

	lyle = NULL;

	vram_pos = 0;
}

// Public functions that act on the Lyle singleton
void lyle_get_bounced(void)
{
	if (!lyle) return;
	lyle->head.dy = kjump_dy;
	lyle->head.dx = (lyle->head.direction == OBJ_DIRECTION_RIGHT) ?
	                khurt_dx : -khurt_dx;
}

void lyle_get_hurt(void)
{
	if (!lyle) return;
	if (lyle->tele_out_cnt > 0) return;
	if (lyle->invuln_cnt != 0) return;
	lyle_get_bounced();
	lyle->hurt_cnt = khurt_time;
	lyle->invuln_cnt = kinvuln_time;
	lyle->cp_cnt = 0;

	if (lyle->head.hp > 0) lyle->head.hp--;

	// Cubes that are held get dropped
	if (lyle->holding_cube != CUBE_TYPE_NULL)
	{
		cube_manager_spawn(lyle->head.x, lyle->head.y - INTTOFIX32(12),
		                   lyle->holding_cube, CUBE_STATUS_AIR,
		                   lyle->head.direction == OBJ_DIRECTION_RIGHT ?
		                   kdrop_cube_dx : -kdrop_cube_dx, kdrop_cube_dy);
		lyle->holding_cube = CUBE_TYPE_NULL;
	}

		// TODO: Play hurt sound
}

void lyle_kill(void)
{
	if (!lyle) return;
	lyle->head.hp = 0;
}

O_Lyle *lyle_get(void)
{
	return lyle;
}

fix32_t lyle_get_x(void)
{
	if (!lyle) return 0;
	return lyle->head.x;
}

fix32_t lyle_get_y(void)
{
	if (!lyle) return 0;
	return lyle->head.y;
}

int16_t lyle_get_hp(void)
{
	if (!lyle) return 0;
	return lyle->head.hp;
}

int16_t lyle_get_cp(void)
{
	if (!lyle) return 0;
	return lyle->cp;
}
