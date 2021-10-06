#ifndef OBJ_LYLE_H
#define OBJ_LYLE_H

#include "obj.h"
#include "md/megadrive.h"
#include "cube.h"
#include <stdint.h>

#define LYLE_START_HP 5
#define LYLE_MAX_HP 15

#define LYLE_START_CP 20
#define LYLE_MAX_CP 30

// Hitboxes
#define LYLE_LEFT INTTOFIX16(-4)
#define LYLE_RIGHT INTTOFIX16(4)
#define LYLE_TOP INTTOFIX16(-19)

typedef struct O_Lyle
{
	Obj head;

	Cube *on_cube;
	CubeType holding_cube;

	int16_t hurt_cnt;
	int16_t tele_out_cnt;
	int16_t tele_in_cnt;
	int16_t invuln_cnt;
	int16_t cp_restore_cnt;
	int16_t phantom_cnt;
	int16_t throwdown_cnt;
	int16_t throw_cnt;
	int16_t kick_cnt;
	int16_t lift_cnt;

	int8_t anim_cnt;
	int8_t anim_frame;
	int8_t metaframe;

	int8_t action_cnt;
	int8_t grounded;

	int8_t ext_disable;
	int8_t cube_jump_disable_cnt;

	int8_t cp;
	int8_t scroll_disable_h;
	int8_t scroll_disable_v;
	int8_t priority;

	int8_t full_disable;
} O_Lyle;

extern O_Lyle *g_lyle;

void o_load_lyle(Obj *o, uint16_t data);
void o_unload_lyle(void);

// Public functions

static inline O_Lyle *lyle_get(void) { return g_lyle; }

// Slightly optimized collision function that hardcodes Lyle's dimensions.
static inline int16_t lyle_touching_obj(Obj *o)
{
	if (g_lyle->head.x + LYLE_RIGHT < o->x + o->left) return 0;
	if (g_lyle->head.x + LYLE_LEFT > o->x + o->right) return 0;
	if (g_lyle->head.y < o->y + o->top) return 0;
	if (g_lyle->head.y + LYLE_TOP > o->y) return 0;
	return 1;
}

void lyle_get_bounced(void);
void lyle_get_hurt(int16_t bypass_invuln);

fix32_t lyle_get_x(void);
fix32_t lyle_get_y(void);
void lyle_set_pos(fix32_t x, fix32_t y);
void lyle_set_direction(ObjDirection d);

int16_t lyle_get_hp(void);
void lyle_set_hp(int16_t hp);
int16_t lyle_get_cp(void);
static inline void lyle_kill(void) { lyle_set_hp(0); }

void lyle_set_scroll_h_en(int16_t en);
void lyle_set_scroll_v_en(int16_t en);
void lyle_set_control_en(int16_t en);
void lyle_set_master_en(int16_t en);

#endif  // OBJ_LYLE_H
