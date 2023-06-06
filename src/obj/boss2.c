#include "obj/boss2.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"

#include "progress.h"
#include "lyle.h"
#include "game.h"
#include "sfx.h"
#include "util/trig.h"
#include "particle.h"
#include "projectile.h"
#include "cube_manager.h"
#include "res.h"
#include "music.h"
#include "obj/rockman_door.h"

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_BOSS2);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static int16_t kintro_idle_anim_speed;
static int16_t kintro_flying_up_anim_speed;
static int16_t kintro_growth_anim_speed;
static fix16_t kintro_flying_up_dy;

static int16_t kintro_growth_time;

static int16_t kroam_anim_speed;
static fix16_t kroam_dx;

static int16_t krecoil_time;

static int16_t kshot_stuck_anim_speed;

static fix16_t khover_d_max;
static fix16_t khover_d_min;
static fix16_t khover_accel;

static fix16_t kball_speed[3];

static fix16_t kshot_dx;
static fix16_t kshot_dy;
static int16_t kshot_anim_speed;

static fix16_t kdive_dx;
static fix16_t kdive_dy;

static int16_t kbrick_draw_speed;
static int16_t kbrick_pal_speed;

static int16_t kexplosion_separation;
static int16_t kexploding_duration;

static fix16_t kspread_shot_speed;
static int16_t kspread_shot_separation;

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;

	kintro_idle_anim_speed = PALSCALE_DURATION(8);
	kintro_flying_up_anim_speed = PALSCALE_DURATION(2);
	kintro_growth_anim_speed = PALSCALE_DURATION(2);
	kintro_flying_up_dy = INTTOFIX16(PALSCALE_1ST(-0.83333333));

	kintro_growth_time = PALSCALE_DURATION(120);

	kroam_anim_speed = PALSCALE_DURATION(2);
	kroam_dx = INTTOFIX16(PALSCALE_1ST(0.41666666667));

	krecoil_time = PALSCALE_DURATION(24);

	kshot_stuck_anim_speed = PALSCALE_DURATION(5);

	khover_d_max = INTTOFIX16(PALSCALE_1ST(2.22222222));
	khover_d_min = INTTOFIX16(PALSCALE_1ST(-2.22222222));
	khover_accel = INTTOFIX16(PALSCALE_2ND(0.2777777778));

	kball_speed[2] = INTTOFIX16(PALSCALE_1ST(1.5));
	kball_speed[1] = INTTOFIX16(PALSCALE_1ST(1.75));
	kball_speed[0] = INTTOFIX16(PALSCALE_1ST(2.0));

	kshot_dx = INTTOFIX16(PALSCALE_1ST(0.833333));
	kshot_dy = INTTOFIX16(PALSCALE_1ST(-5));
	kshot_anim_speed = PALSCALE_DURATION(5);

	kdive_dx = INTTOFIX16(PALSCALE_1ST(1.6666667));
	kdive_dy = INTTOFIX16(PALSCALE_1ST(3.3333333));

	kbrick_pal_speed = PALSCALE_DURATION(6);
	kbrick_draw_speed = PALSCALE_DURATION(5);  // TODO: This is fabricated

	kexplosion_separation = PALSCALE_DURATION(12);
	kexploding_duration = kexplosion_separation * 20;

	kspread_shot_speed = INTTOFIX16(PALSCALE_1ST(3));
	kspread_shot_separation = PALSCALE_DURATION(120);

	s_constants_set = true;
}

// ----------------------------------------------------------------------------

static Brick s_bricks[46];

static const Brick brick_mapping_1[] =
{
	{16,56,1,1,0},{40,56,1,1,0},{64,56,1,1,0},{88,56,1,1,0},{112,56,1,1,0},{136,56,1,1,0},{160,56,1,1,0},{184,56,1,1,0},{208,56,1,1,0},{232,56,1,1,0},{256,56,1,1,0},{280,56,1,1,0},
	{28,64,2,0,0},{52,64,2,0,0},{76,64,2,0,0},{100,64,2,0,0},{124,64,2,0,0},{148,64,2,0,0},{172,64,2,0,0},{196,64,2,0,0},{220,64,2,0,0},{244,64,2,0,0},{268,64,2,0,0},
};

static const Brick brick_mapping_2[]=
{
	{16,56,1,1,0},{40,56,1,1,0},{64,56,1,1,0},{88,56,1,1,0},{112,56,1,1,0},{136,56,1,1,0},{160,56,1,1,0},{184,56,1,1,0},{208,56,1,1,0},{232,56,1,1,0},{256,56,1,1,0},{280,56,1,1,0},
	{28,64,2,0,0},{52,64,2,0,0},{76,64,2,0,0},{100,64,2,0,0},{124,64,2,0,0},{148,64,2,0,0},{172,64,2,0,0},{196,64,2,0,0},{220,64,2,0,0},{244,64,2,0,0},{268,64,2,0,0},
	{16,72,1,1,0},{40,72,1,1,0},{64,72,1,1,0},{88,72,1,1,0},{112,72,1,1,0},{136,72,1,1,0},{160,72,1,1,0},{184,72,1,1,0},{208,72,1,1,0},{232,72,1,1,0},{256,72,1,1,0},{280,72,1,1,0},
};

static const Brick brick_mapping_3[]=
{
	{16,56,1,1,0},{40,56,1,1,0},{64,56,1,1,0},{88,56,1,1,0},{112,56,1,1,0},{136,56,1,1,0},{160,56,1,1,0},{184,56,1,1,0},{208,56,1,1,0},{232,56,1,1,0},{256,56,1,1,0},{280,56,1,1,0},
	{28,64,2,0,0},{52,64,2,0,0},{76,64,2,0,0},{100,64,2,0,0},{124,64,2,0,0},{148,64,2,0,0},{172,64,2,0,0},{196,64,2,0,0},{220,64,2,0,0},{244,64,2,0,0},{268,64,2,0,0},
	{16,72,1,1,0},{40,72,1,1,0},{64,72,1,1,0},{88,72,1,1,0},{112,72,1,1,0},{136,72,1,1,0},{160,72,1,1,0},{184,72,1,1,0},{208,72,1,1,0},{232,72,1,1,0},{256,72,1,1,0},{280,72,1,1,0},
	{52,80,1,0,0},{76,80,1,0,0},{124,80,1,0,0},{148,80,1,0,0},{172,80,1,0,0},{220,80,1,0,0},{244,80,1,0,0},
	{40,104,1,1,0},{136,104,1,1,0},{160,104,1,1,0},{256,104,1,1,0},
};

// Brick functions ------------------------------------------------------------

static inline void draw_brick(Brick *b)
{
	const uint16_t attr =  SPR_ATTR(s_vram_pos + (b->type == 1 ? 0 : 4), 0, 0, ENEMY_PAL_LINE, 0);
	if (b->method == 0 && b->type != 0)
	{
		md_spr_put(b->x, b->y - map_get_y_scroll(), attr, SPR_SIZE(3, 1));
	}
	else if (b->method == 1 && b->type != b->previous_type)
	{
		b->previous_type = b->type;
		md_vdp_set_autoinc(2);
		uint16_t addr = md_vdp_get_plane_base(VDP_PLANE_A);
		addr += 2 * (b->x / 8);
		addr += (GAME_PLANE_W_CELLS * 2) * (b->y / 8);
		if (b->type != 0)
		{
			md_vdp_poke(addr, attr);
			md_vdp_write(attr + 1);
			md_vdp_write(attr + 2);
		}
		else
		{
			md_vdp_poke(addr, 0);
			md_vdp_write(0);
			md_vdp_write(0);
		}
	}
}

// Draws all bricks as sprites.
static void bricks_render(O_Boss2 *e)
{
	for (uint16_t i = 0; i < e->brick_list_size; i++)
	{
		Brick *b = &s_bricks[i];
		draw_brick(b);
	}
}

static void bricks_reset(void)
{
	for (uint16_t i = 0; i < ARRAYSIZE(s_bricks); i++)
	{
		s_bricks[i].type = 0;
	}
}

// Sets up a brick mapping for bricks_process to handle
static void bricks_init(O_Boss2 *e, int16_t phase)
{
	switch (phase)
	{
		default:
			e->brick_list = brick_mapping_1;
			e->brick_list_size = 0;
			break;

		case 3:
			e->brick_list = brick_mapping_1;
			e->brick_list_size = ARRAYSIZE(brick_mapping_1);
			break;

		case 2:
			e->brick_list = brick_mapping_2;
			e->brick_list_size = ARRAYSIZE(brick_mapping_2);
			break;

		case 1:
			e->brick_list = brick_mapping_3;
			e->brick_list_size = ARRAYSIZE(brick_mapping_3);
			break;
	}

	e->brick_draw_cnt = 0;
	e->brick_draw_index = 0;
}

// Activates bricks based on the current brick mapping.
static void bricks_process(O_Boss2 *e)
{
	if (e->brick_draw_index < e->brick_list_size)
	{
		if (e->brick_draw_cnt >= kbrick_draw_speed)
		{
			s_bricks[e->brick_draw_index].x = e->brick_list[e->brick_draw_index].x;
			s_bricks[e->brick_draw_index].y = e->brick_list[e->brick_draw_index].y + 16;
			s_bricks[e->brick_draw_index].type = e->brick_list[e->brick_draw_index].type;
			s_bricks[e->brick_draw_index].method = e->brick_list[e->brick_draw_index].method;
			s_bricks[e->brick_draw_index].previous_type = -1;  // Force redraw
			e->brick_draw_index++;
			e->brick_draw_cnt = 0;
		}
		else
		{
			e->brick_draw_cnt++;
		}
	}
	bricks_render(e);
}

// Ball functions --------------------------------------------
static inline void ball_brick_collisions(O_Boss2 *e, int16_t px, int16_t py)
{
	static const int16_t ball_size = 3;
	static const int16_t brick_width = 24;
	static const int16_t brick_height = 8;

	if (e->ball_state == BALL_STATE_ACTIVE_TOUCHED)
	{
		for (uint16_t i = 0; i < e->brick_list_size; i++)
		{
			Brick *b = &s_bricks[i];
			if (b->type == 0) continue;
			if (px + ball_size < b->x) continue;
			if (px - ball_size > b->x + brick_width) continue;
			if (py + ball_size < b->y) continue;
			if (py - ball_size > b->y + brick_height) continue;

			// Downgrade/delete the brick.
			b->type = (b->type == 2) ? 1 : 0;

			// Bounce ball off the brick; fairly direct translation of the MMF code.
			// TODO: My angles might be flipped compared to this
			if (e->ball_angle >= 152 && e->ball_angle <= 232)
			{
				if (py < b->y) e->ball_angle = 0 - e->ball_angle;
				else e->ball_angle = 128 - e->ball_angle;
			}
			else if (e->ball_angle >= 8 && e->ball_angle <= 120)
			{
				if (py < b->y + brick_height) e->ball_angle = 128 - e->ball_angle;
				else e->ball_angle = 0 - e->ball_angle;
			}
			return;  // Only check one brick per frame.
		}
	}
}

static inline void ball_wall_collisions(O_Boss2 *e, int16_t px, int16_t py, fix16_t ball_dx, fix16_t ball_dy)
{
	static const int16_t ball_size = 3;
	static const int16_t min_px = 16 + ball_size;
	static const int16_t max_px = 304 - ball_size;
	static const int16_t max_py = 224 - ball_size;
	static const int16_t min_py = 32 + ball_size;
	if ((ball_dx > 0 && px >= max_px) ||
	    (ball_dx < 0 && px <= min_px))
	{
		e->ball_angle = 128 - e->ball_angle;
	}
	if (ball_dy < 0 && py <= min_py)
	{
		e->ball_angle = 0 - e->ball_angle;
	}
	// Upon hitting the ground, the ball is deactivated, and two bouncing
	// projectiles are emitted.
	if (py >= max_py)
	{
		e->ball_state = BALL_STATE_NONE;
		projectile_shoot(e->ball_x, e->ball_y,
		                         PROJECTILE_TYPE_DEATHORB2, kshot_dx, kshot_dy);
		projectile_shoot(e->ball_x, e->ball_y,
		                         PROJECTILE_TYPE_DEATHORB2, -kshot_dx, kshot_dy);
	}
}

// Used when the ball touches a cube, or lyle while holding a cube.
static inline void ball_touching_cube(O_Boss2 *e, fix32_t cx)
{
	static const fix32_t center_margin = INTTOFIX32(2);
	if (e->ball_state == BALL_STATE_ACTIVE)
	{
		e->ball_state = BALL_STATE_ACTIVE_TOUCHED;
	}

	// Another direct translation of MMF code.
	// TODO: Again, check the orientation of my angles compared to MMF.
	if (e->ball_angle == 192)  // straight down; angle 24 in MMF
	{
		e->ball_angle = 56;  // angle 7 in MMF
	}
	else if (e->ball_angle >= 128)
	{
		if (e->ball_angle < 232 && e->ball_x > cx + center_margin)
		{
			e->ball_angle = (0 - e->ball_angle) - 16;
		}
		else if (e->ball_angle > 152 && e->ball_x < cx - center_margin)
		{
			e->ball_angle = (0 - e->ball_angle) + 16;
		}
		else
		{
			e->ball_angle = 0 - e->ball_angle;
		}
	}
}

static inline void ball_cube_collisions(O_Boss2 *e)
{
	static const fix32_t ball_size = INTTOFIX32(3);
	uint16_t i = ARRAYSIZE(g_cubes);
	if (e->ball_angle < 152 || e->ball_angle > 232) return;
	while (i--)
	{
		Cube *c = &g_cubes[i];
		if (c->status == CUBE_STATUS_NULL) continue;
		if (e->ball_y + ball_size < c->y + c->top) continue;
		if (e->ball_y - ball_size > c->y) continue;
		if (e->ball_x + ball_size < c->x + c->left) continue;
		if (e->ball_x - ball_size > c->x + c->right) continue;

		ball_touching_cube(e, c->x);
	}
}

static inline void ball_lyle_collisions(O_Boss2 *e)
{
	const O_Lyle *l = lyle_get();
	if (e->ball_angle < 152 || e->ball_angle > 232) return;
	if (!l->holding_cube) return;

	// Ball half-width plus cube half-width.
	const fix32_t adj_x = INTTOFIX32(4 + 8);
	if (e->ball_x + adj_x < l->head.x) return;
	if (e->ball_x - adj_x > l->head.x) return;
	if (e->ball_y + INTTOFIX32(4) < l->head.y - INTTOFIX32(35)) return;
	if (e->ball_y - INTTOFIX32(4) > l->head.y - INTTOFIX32(35 - 16)) return;

	ball_touching_cube(e, l->head.x);
}

static inline void ball_boss_collisions(O_Boss2 *e)
{
	static const fix32_t ball_size = INTTOFIX32(3);
	if (e->state != BOSS2_STATE_ROAM || e->state != BOSS2_STATE_SHOOTING) return;
	if (e->ball_y - ball_size > e->head.y) return;
	if (e->ball_y + ball_size < e->head.y + e->head.top) return;
	if (e->ball_x + ball_size < e->head.x + e->head.left) return;
	if (e->ball_x - ball_size > e->head.x + e->head.right) return;

	e->state = BOSS2_STATE_RECOIL;
	e->ball_state = BALL_STATE_NONE;
}

static inline void ball_render(int16_t px, int16_t py)
{
	md_spr_put(px - 4, py - 4 - map_get_y_scroll(),
	        SPR_ATTR(s_vram_pos + 3, 0, 0, ENEMY_PAL_LINE, 0),
	        SPR_SIZE(1, 1));
}

static void ball_process(O_Boss2 *e)
{
	if (e->ball_state < BALL_STATE_ACTIVE) return;
	// Speed/angle based movement.
	const fix16_t ball_dx = FIX16MUL(e->ball_speed, trig_cos(e->ball_angle));
	const fix16_t ball_dy = -FIX16MUL(e->ball_speed, trig_sin(e->ball_angle));
	e->ball_x += ball_dx;
	e->ball_y += ball_dy;

	// Ball position as pixel integers.
	const int16_t px = FIX32TOINT(e->ball_x);
	const int16_t py = FIX32TOINT(e->ball_y);

	if (e->ball_state == BALL_STATE_ACTIVE_TOUCHED)
	{
		ball_brick_collisions(e, px, py);
		ball_boss_collisions(e);
	}
	ball_wall_collisions(e, px, py, ball_dx, ball_dy);
	ball_cube_collisions(e);
	ball_lyle_collisions(e);
	ball_render(px, py);
}

static void ball_launch(O_Boss2 *e)
{
	e->ball_state = BALL_STATE_ACTIVE;
	e->ball_x = e->head.x;
	e->ball_y = e->head.y;
	e->ball_angle = 192;
	const int16_t speed_index = (e->head.hp > 2) ? 2 : e->head.hp;
	e->ball_speed = kball_speed[speed_index];
}

// Main logic ------------------------------------------------

static void render(O_Boss2 *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	obj_render_setup(o, &sp_x, &sp_y, 0, -8,
	                 map_get_x_scroll(), map_get_y_scroll());

	if (o->hurt_stun == 0 && e->state == BOSS2_STATE_EXPLODING)
	{
		if (e->state_elapsed % 2 == 1) return;
		sp_x += (system_rand() % 8) - 3;
		sp_y += (system_rand() % 8) - 3;
	}

	static const int16_t pal = ENEMY_PAL_LINE;

	typedef struct SprDef
	{
		int16_t x;
		int16_t y;
		uint16_t attr;  // minus s_vram_pos
		int16_t size;
	} SprDef;

	static const SprDef metaframes[] =
	{
		// tiny fly 1
		{-2, -8, SPR_ATTR(0x07, 0, 0, pal, 0), SPR_SIZE(1, 1)},  // fly sprite
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		// tiny fly 2
		{-3, -8, SPR_ATTR(0x07, 0, 0, pal, 0), SPR_SIZE(1, 1)},  // fly sprite
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		// tiny fly upward 1 (wings down)
		{-3, -8, SPR_ATTR(0x08, 0, 0, pal, 0), SPR_SIZE(1, 1)},  // fly sprite
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		// tiny fly upward 2 (wings up)
		{-3, -8, SPR_ATTR(0x09, 0, 0, pal, 0), SPR_SIZE(1, 1)},  // fly sprite
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		// growth small 1 (wings up)
		{-7, -12, SPR_ATTR(0x0A, 0, 0, pal, 0), SPR_SIZE(2, 2)},  // fly sprite
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		// growth small 2 (wings down)
		{-7, -12, SPR_ATTR(0x0E, 0, 0, pal, 0), SPR_SIZE(2, 2)},  // fly sprite
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		// growth med 1 (wings up)
		{-11, -16, SPR_ATTR(0x12, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // fly sprite
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		// growth med 2 (wings down)
		{-11, -16, SPR_ATTR(0x1B, 0, 0, pal, 0), SPR_SIZE(3, 3)},  // fly sprite
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
		// flying 1 / growth large 1
		{-11, -20, SPR_ATTR(0x24, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -17, SPR_ATTR(0x48, 1, 0, pal, 0), SPR_SIZE(2, 2)},  // right wing
		{-19, -17, SPR_ATTR(0x48, 0, 0, pal, 0), SPR_SIZE(2, 2)},  // left wing
		{-1, -1, -1, -1},
		// flying 2
		{-11, -20, SPR_ATTR(0x24, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -17, SPR_ATTR(0x4C, 1, 0, pal, 0), SPR_SIZE(1, 2)},  // right wing
		{-11, -17, SPR_ATTR(0x4C, 0, 0, pal, 0), SPR_SIZE(1, 2)},  // left wing
		{-1, -1, -1, -1},
		// flying 3 / growth large 2
		{-11, -20, SPR_ATTR(0x24, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -1, SPR_ATTR(0x48, 1, 1, pal, 0), SPR_SIZE(2, 2)},  // right wing
		{-19, -1, SPR_ATTR(0x48, 0, 1, pal, 0), SPR_SIZE(2, 2)},  // left wing
		{-1, -1, -1, -1},
		// flying 4
		{-11, -20, SPR_ATTR(0x24, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -1, SPR_ATTR(0x4C, 1, 1, pal, 0), SPR_SIZE(1, 2)},  // right wing
		{-11, -1, SPR_ATTR(0x4C, 0, 1, pal, 0), SPR_SIZE(1, 2)},  // left wing
		{-1, -1, -1, -1},
		// stuck struggling 1
		{-12, -20, SPR_ATTR(0x30, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -17, SPR_ATTR(0x48, 1, 0, pal, 0), SPR_SIZE(2, 2)},  // right wing
		{-19, -17, SPR_ATTR(0x48, 0, 0, pal, 0), SPR_SIZE(2, 2)},  // left wing
		{-1, -1, -1, -1},
		// stuck struggling 2
		{-12, -20, SPR_ATTR(0x30, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -17, SPR_ATTR(0x4C, 1, 0, pal, 0), SPR_SIZE(1, 2)},  // right wing
		{-11, -17, SPR_ATTR(0x4C, 0, 0, pal, 0), SPR_SIZE(1, 2)},  // left wing
		{-1, -1, -1, -1},
		// stuck struggling 3
		{-12, -20, SPR_ATTR(0x30, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -1, SPR_ATTR(0x48, 1, 1, pal, 0), SPR_SIZE(2, 2)},  // right wing
		{-19, -1, SPR_ATTR(0x48, 0, 1, pal, 0), SPR_SIZE(2, 2)},  // left wing
		{-1, -1, -1, -1},
		// stuck struggling 4
		{-12, -20, SPR_ATTR(0x30, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -1, SPR_ATTR(0x4C, 1, 1, pal, 0), SPR_SIZE(1, 2)},  // right wing
		{-11, -1, SPR_ATTR(0x4C, 0, 1, pal, 0), SPR_SIZE(1, 2)},  // left wing
		{-1, -1, -1, -1},
		// stuck struggling 5
		{-10, -20, SPR_ATTR(0x30, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -17, SPR_ATTR(0x48, 1, 0, pal, 0), SPR_SIZE(2, 2)},  // right wing
		{-19, -17, SPR_ATTR(0x48, 0, 0, pal, 0), SPR_SIZE(2, 2)},  // left wing
		{-1, -1, -1, -1},
		// stuck struggling 6
		{-10, -20, SPR_ATTR(0x30, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -17, SPR_ATTR(0x4C, 1, 0, pal, 0), SPR_SIZE(1, 2)},  // right wing
		{-11, -17, SPR_ATTR(0x4C, 0, 0, pal, 0), SPR_SIZE(1, 2)},  // left wing
		{-1, -1, -1, -1},
		// stuck struggling 7
		{-10, -20, SPR_ATTR(0x30, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -1, SPR_ATTR(0x48, 1, 1, pal, 0), SPR_SIZE(2, 2)},  // right wing
		{-19, -1, SPR_ATTR(0x48, 0, 1, pal, 0), SPR_SIZE(2, 2)},  // left wing
		{-1, -1, -1, -1},
		// stuck struggling 8
		{-10, -20, SPR_ATTR(0x30, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -1, SPR_ATTR(0x4C, 1, 1, pal, 0), SPR_SIZE(1, 2)},  // right wing
		{-11, -1, SPR_ATTR(0x4C, 0, 1, pal, 0), SPR_SIZE(1, 2)},  // left wing
		{-1, -1, -1, -1},
		// stuck still
		{-11, -20, SPR_ATTR(0x3C, 0, 0, pal, 0), SPR_SIZE(3, 4)},  // body
		{4, -17, SPR_ATTR(0x48, 1, 0, pal, 0), SPR_SIZE(2, 2)},  // right wing
		{-19, -17, SPR_ATTR(0x48, 0, 0, pal, 0), SPR_SIZE(2, 2)},  // left wing
		{-1, -1, -1, -1},
	};

	for (int16_t i = 0; i < 4; i++)
	{
		const SprDef *def = &metaframes[(e->metaframe * 4) + i];
		if (def->size == -1) continue;
		md_spr_put(sp_x + def->x, sp_y + def->y, s_vram_pos + def->attr, def->size);
	}
}

static inline void hover_osc(O_Boss2 *e)
{
	if (e->hover_phase == 0)
	{
		e->hover_d += khover_accel;
		if (e->hover_d >= khover_d_max)
		{
			e->hover_phase = 1;
		}
	}
	else
	{
		e->hover_d -= khover_accel;
		if (e->hover_d <= khover_d_min)
		{
			e->hover_phase = 0;
		}
	}
}

static void horizontal_hover(O_Boss2 *e)
{
	hover_osc(e);
	e->head.x += e->hover_d;
}

static void vertical_hover(O_Boss2 *e)
{
	hover_osc(e);
	e->head.y += e->hover_d;
}

static void main_func(Obj *o)
{
	O_Boss2 *e = (O_Boss2 *)o;

	const Boss2State state_prev = e->state;

	if (o->hurt_stun == 0)
	{
		const O_Lyle *l = lyle_get();
		static const fix32_t ball_size = INTTOFIX32(3);
		static const fix32_t ground_y = INTTOFIX32(223);
		static const fix32_t growth_activation_y = INTTOFIX32(52);
		static const fix32_t side_margin = INTTOFIX32(24);
		static const fix32_t center_x = INTTOFIX32(GAME_SCREEN_W_PIXELS / 2);
		static const fix32_t center_margin = INTTOFIX32(1);
		switch (e->state)
		{
			default:
				break;

			case BOSS2_STATE_INTRO_IDLE:
				if (e->state_elapsed == 0)
				{
					e->anim_frame = 0;
					e->anim_cnt = 0;
				}

				if (l->head.x + l->head.right < INTTOFIX32(304))
				{
					rockman_door_set_closed(1);
				}

				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame,
				                2, kintro_idle_anim_speed);
				if (o->touching_player) e->state = BOSS2_STATE_INTRO_FLYING_UP;
				e->metaframe = e->anim_frame;
				break;

			case BOSS2_STATE_INTRO_FLYING_UP:
				if (e->state_elapsed == 0)
				{
					e->anim_frame = 0;
					e->anim_cnt = 0;
					e->hover_phase = 0;
					e->hover_d = 0;
					o->dy = kintro_flying_up_dy;
					e->hover_d = khover_d_max - khover_accel;
					e->hover_phase = 0;

					o->right = INTTOFIX16(12);
					o->left = INTTOFIX16(-12);
					o->top = INTTOFIX16(-24);
					// TODO: begin buzzing sound
				}

				if (o->y <= growth_activation_y)
				{
					o->dy = 0;
					e->state = BOSS2_STATE_INTRO_GROWTH_1;
				}

				obj_accurate_physics(o);
				horizontal_hover(e);
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2,
				                kintro_flying_up_anim_speed);
				e->metaframe = e->anim_frame + 2;
				break;

			case BOSS2_STATE_INTRO_GROWTH_1:
				if (e->state_elapsed == 0)
				{
					e->anim_frame = 0;
					e->anim_cnt = 0;
					// TODO: stop buzzing sound
					sfx_play(SFX_TELEPORT, 0);
					sfx_play(SFX_TELEPORT_2, 1);
				}
				else if (e->state_elapsed >= kintro_growth_time)
				{
					e->state = BOSS2_STATE_INTRO_GROWTH_2;
				}
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4,
				                kintro_growth_anim_speed);
				{
					static const int16_t metaframes[] =
					{
						3, 1, 2, 4
					};
					e->metaframe = metaframes[e->anim_frame];
				}
				break;

			case BOSS2_STATE_INTRO_GROWTH_2:
				if (e->state_elapsed == 0)
				{
					e->anim_frame = 0;
					e->anim_cnt = 0;
					sfx_play(SFX_TELEPORT, 0);
					sfx_play(SFX_TELEPORT_2, 1);
				}
				else if (e->state_elapsed >= kintro_growth_time)
				{
					e->state = BOSS2_STATE_INTRO_GROWTH_3;
				}
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4,
				                kintro_growth_anim_speed);
				{
					static const int16_t metaframes[] =
					{
						4, 6, 5, 7
					};
					e->metaframe = metaframes[e->anim_frame];
				}
				break;

			case BOSS2_STATE_INTRO_GROWTH_3:
				if (e->state_elapsed == 0)
				{
					e->anim_frame = 0;
					e->anim_cnt = 0;
					sfx_play(SFX_TELEPORT, 0);
					sfx_play(SFX_TELEPORT_2, 1);
				}
				else if (e->state_elapsed >= kintro_growth_time)
				{
					e->state = BOSS2_STATE_ROAM;
					e->hover_phase = 0;
					e->hover_d = 0;
					bricks_init(e, o->hp);
					music_play(10);
				}
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 4,
				                kintro_growth_anim_speed);
				{
					static const int16_t metaframes[] =
					{
						6, 8, 7, 10
					};
					e->metaframe = metaframes[e->anim_frame];
				}
				break;

			case BOSS2_STATE_ROAM:
				if (e->state_elapsed == 0)
				{
					e->anim_frame = 0;
					e->anim_cnt = 0;
				}

				obj_accurate_physics(o);
				if (o->direction == OBJ_DIRECTION_RIGHT &&
				    o->x > map_get_right() - side_margin)
				{
					o->direction = OBJ_DIRECTION_LEFT;
					if (e->ball_state == BALL_STATE_NONE)
					{
						e->ball_state = BALL_STATE_PENDING;
					}
				}
				else if (o->direction == OBJ_DIRECTION_LEFT &&
				         o->x < side_margin)
				{
					o->direction = OBJ_DIRECTION_RIGHT;
					if (e->ball_state == BALL_STATE_NONE)
					{
						e->ball_state = BALL_STATE_PENDING;
					}
				}

				if (e->spread_shot_cnt >= kspread_shot_separation)
				{
					e->spread_shot_cnt = 0;
					projectile_shoot_angle(o->x, o->y,
					                               PROJECTILE_TYPE_BALL2,
					                               192,
					                               kspread_shot_speed);
					projectile_shoot_angle(o->x, o->y,
					                               PROJECTILE_TYPE_BALL2,
					                               216,
					                               kspread_shot_speed);
					projectile_shoot_angle(o->x, o->y,
					                               PROJECTILE_TYPE_BALL2,
					                               168,
					                               kspread_shot_speed);
				}
				else
				{
					e->spread_shot_cnt++;
				}

				if (e->ball_state == BALL_STATE_PENDING &&
				    o->x > center_x - center_margin &&
				    o->x < center_x + center_margin)
				{
					e->state = BOSS2_STATE_SHOOTING;
				}

				o->dx = (o->direction == OBJ_DIRECTION_RIGHT) ?
				        kroam_dx : -kroam_dx;

				vertical_hover(e);
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame,
				                4, kroam_anim_speed);
				e->metaframe = e->anim_frame + 8;
				if (e->ball_state >= BALL_STATE_ACTIVE &&
				    e->ball_x - ball_size <= o->x + o->right &&
				    e->ball_x + ball_size >= o->x + o->left &&
				    e->ball_y - ball_size <= o->y &&
				    e->ball_y + ball_size >= o->y + o->top)
				{
					e->ball_state = BALL_STATE_NONE;
					e->state = BOSS2_STATE_RECOIL;
				}
				break;

			case BOSS2_STATE_SHOOTING:
				if (e->state_elapsed == 0)
				{
					e->anim_frame = 0;
					e->anim_cnt = 0;
					ball_launch(e);
					sfx_play(SFX_MAGIBEAR_SHOT, 0);
					o->y += INTTOFIX32(-2);
				}
				else if (e->state_elapsed == kshot_stuck_anim_speed * 1)
				{
					o->y += INTTOFIX32(4);
				}
				else if (e->state_elapsed == kshot_stuck_anim_speed * 2)
				{
					o->y += INTTOFIX32(-4);
				}
				else if (e->state_elapsed == kshot_stuck_anim_speed * 3)
				{
					o->y += INTTOFIX32(3);
				}
				else if (e->state_elapsed == kshot_stuck_anim_speed * 4)
				{
					o->y += INTTOFIX32(-2);
				}
				else if (e->state_elapsed == kshot_stuck_anim_speed * 5)
				{
					o->y += INTTOFIX32(1);
				}
				else if (e->state_elapsed >= kshot_stuck_anim_speed * 6)
				{
					e->state = BOSS2_STATE_ROAM;
				}
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame,
				                4, kroam_anim_speed);
				e->metaframe = e->anim_frame + 8;
				break;

			case BOSS2_STATE_RECOIL:
				if (e->state_elapsed == 0)
				{
					o->dy = kintro_flying_up_dy;
				}
				if (e->state_elapsed >= krecoil_time)
				{
					o->dy = kdive_dy;
					e->state = BOSS2_STATE_DIVING;
					o->flags = OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE | OBJ_FLAG_SENSITIVE;
				}
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame,
				                4, kroam_anim_speed);
				e->metaframe = e->anim_frame + 8;
				obj_accurate_physics(o);
				break;

			case BOSS2_STATE_EXPLODING:
				if (e->state_elapsed == 0)
				{
					e->explode_cnt = 0;
				}
				__attribute__((fallthrough));
			case BOSS2_STATE_DIVING_HIT:
				e->hit_pending = 0;
				if (o->y >= ground_y && o->hp < 127)
				{
					e->state = BOSS2_STATE_RETREAT;
				}
				__attribute__((fallthrough));
			case BOSS2_STATE_DIVING:
				if (o->dy > 0)
				{
					o->dx = (o->x > lyle_get()->head.x) ? -kdive_dx : kdive_dx;
					if (o->y >= ground_y)
					{
						o->y = ground_y;
						o->dy = 0;
						o->dx = 0;
					}
					OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame,
					                2, kroam_anim_speed);
					e->metaframe = e->anim_frame + 8;
				}
				else
				{
					o->dx = 0;
					OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame,
					                64, kshot_stuck_anim_speed / 2);
					static const int16_t metaframes[] =
					{
						12, 13, 14, 15, 16, 17, 18, 19,
						12, 13, 14, 15, 16, 17, 18, 19,
						12, 13, 14, 15, 16, 17, 18, 19,
						12, 13, 14, 15, 16, 17, 18, 19,
						20, 20, 20, 20, 20, 20, 20, 20,
						20, 20, 20, 20, 20, 20, 20, 20,
						20, 20, 20, 20, 20, 20, 20, 20,
						20, 20, 20, 20, 20, 20, 20, 20,
					};
					e->metaframe = metaframes[e->anim_frame];
				}

				obj_accurate_physics(o);

				if (e->hit_pending)
				{
					e->state = BOSS2_STATE_DIVING_HIT;
					o->flags = OBJ_FLAG_HARMFUL | OBJ_FLAG_SENSITIVE;
				}

				if (e->state == BOSS2_STATE_EXPLODING)
				{
					if (e->explode_cnt >= kexplosion_separation)
					{
						e->explode_cnt = 0;
						particle_spawn(o->x, o->y + (o->top / 2), PARTICLE_TYPE_EXPLOSION);
						sfx_play(SFX_EXPLODE, 0);
					}
					else
					{
						e->explode_cnt++;
					}
					if (e->state_elapsed >= kexploding_duration)
					{
						o->flags = OBJ_FLAG_TANGIBLE;
						o->hp = 0;
						o->hurt_stun = 0;
						progress_get()->boss_defeated[1] = 1;
						e->state = BOSS2_STATE_EXPLODED;
						music_stop();
					}
				}
				break;

			case BOSS2_STATE_RETREAT:
				if (e->state_elapsed == 0)
				{
					e->anim_frame = 0;
					e->anim_cnt = 0;
					e->hover_phase = 0;
					e->hover_d = 0;
					o->dy = kintro_flying_up_dy;
				}
				horizontal_hover(e);
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame,
				                4, kroam_anim_speed);
				e->metaframe = e->anim_frame + 8;
				obj_accurate_physics(o);

				if (o->y <= growth_activation_y)
				{
					o->flags = OBJ_FLAG_SENSITIVE;
					e->state = BOSS2_STATE_ROAM;
					e->hover_phase = 0;
					e->hover_d = 0;
					o->dy = 0;
					o->y = growth_activation_y;
					bricks_init(e, o->hp);
				}
				break;
			case BOSS2_STATE_EXPLODED:
				rockman_door_set_closed(0);
				bricks_reset();
				bricks_process(e);
				return;
		}

		if (o->hp >= 127)
		{
			e->state = BOSS2_STATE_EXPLODING;
		}

		if (e->state != state_prev) e->state_elapsed = 0;
		else e->state_elapsed++;
	}

	if (e->brick_pal_cnt >= kbrick_pal_speed * 2) e->brick_pal_cnt = 0;
	else e->brick_pal_cnt++;

	if (e->brick_pal_cnt < kbrick_pal_speed)
	{
		md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_boss2_1_bin,
		           sizeof(res_pal_enemy_boss2_1_bin) / 2);
	}
	else
	{
		md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_boss2_2_bin,
		           sizeof(res_pal_enemy_boss2_2_bin) / 2);
	}

	render(e);
	ball_process(e);
	bricks_process(e);
}

static void cube_func(Obj *o, Cube *c)
{
	c->type = CUBE_TYPE_BLUE;
	obj_standard_cube_response(o, c);
	if (o->hp <= 0)
	{
		o->hp = 127;
	}

	O_Boss2 *e = (O_Boss2 *)o;
	e->hit_pending = 1;
}

void o_load_boss2(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_Boss2) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");

	(void)data;

	const ProgressSlot *prog = progress_get();
	if (prog->boss_defeated[1])
	{
		obj_erase(o);
		return;
	}

	set_constants();
	vram_load();

	obj_basic_init(o, "Boss 2", OBJ_FLAG_SENSITIVE,
	               INTTOFIX16(-4), INTTOFIX16(4), INTTOFIX16(-8), 3);
	o->main_func = main_func;
	o->cube_func = cube_func;

	bricks_reset();

}

void o_unload_boss2(void)
{
	s_vram_pos = 0;
}
