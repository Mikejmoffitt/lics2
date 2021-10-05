#ifndef OBJ_BOSS2_H
#define OBJ_BOSS2_H

#include "obj.h"

typedef struct Brick
{
	int16_t x;
	int16_t y;
	int8_t type;  // 0 = inactive, 1 = blue, 2 = green (2 hits);
	int8_t method;  // 0 = use sprite; 1 = use BG
	int8_t previous_type;
} Brick;

typedef enum Boss2State
{
	BOSS2_STATE_INTRO_IDLE,  // Just sits still.
	BOSS2_STATE_INTRO_FLYING_UP,  // Flies upwards to the top of the screen.
	BOSS2_STATE_INTRO_GROWTH_1,  // Flickering between small and medium
	BOSS2_STATE_INTRO_GROWTH_2,  // Flickering between medium and large
	BOSS2_STATE_INTRO_GROWTH_3,  // Flickering between large and full
	BOSS2_STATE_ROAM,  // Flies left/right; periodic spread shot
	BOSS2_STATE_SHOOTING,  // Shot animation, shoots ball
	BOSS2_STATE_RECOIL,  // Flies up slowly preparing to dive
	BOSS2_STATE_DIVING,  // Goes down, aims horizontally at Lyle (can be hit)
	BOSS2_STATE_DIVING_HIT,  // Goes down, aims horizontally at Lyle (can be hit)
	BOSS2_STATE_RETREAT,  // Flying back up
	BOSS2_STATE_EXPLODING,  // Continues path of DIVING.
	BOSS2_STATE_EXPLODED,
} Boss2State;

typedef enum BallState
{
	BALL_STATE_NONE,
	BALL_STATE_PENDING,
	BALL_STATE_ACTIVE,
	BALL_STATE_ACTIVE_TOUCHED
} BallState;

typedef struct O_Boss2
{
	Obj head;

	Boss2State state;
	int16_t state_elapsed;

	int16_t anim_cnt;
	int16_t anim_frame;
	int16_t metaframe;

	int16_t hover_phase;  // Flag 0 in MMF2
	fix16_t hover_d;  // Value G in MMF2

	int16_t spread_shot_cnt;  // Incremented when roaming.

	BallState ball_state;
	fix32_t ball_x;
	fix32_t ball_y;
	fix16_t ball_speed;
	uint8_t ball_angle;

	int8_t hit_pending;

	int16_t brick_draw_cnt;
	uint16_t brick_draw_index;
	const Brick *brick_list;
	uint16_t brick_list_size;
	int16_t brick_pal_cnt;

	int16_t explode_cnt;
} O_Boss2;

void o_load_boss2(Obj *o, uint16_t data);
void o_unload_boss2(void);

#endif  // OBJ_BOSS2_H
