#ifndef OBJ_LYLE_H
#define OBJ_LYLE_H

#include "obj.h"
#include "md/megadrive.h"
#include "cube.h"
#include <stdint.h>

#define LYLE_START_HP 5

typedef struct O_Lyle
{
	Obj head;

	Cube *on_cube;
	CubeType holding_cube;

	MdButton buttons;
	MdButton buttons_prev;

	int16_t hurt_cnt;
	int16_t tele_out_cnt;
	int16_t tele_in_cnt;
	int16_t invuln_cnt;
	int16_t cp_restore_cnt;
	int16_t cp_cnt;
	int8_t throwdown_cnt;
	int8_t throw_cnt;
	int8_t kick_cnt;
	int8_t lift_cnt;

	int16_t death_counter;
	int16_t dying_seq;

	int16_t cp;

	uint8_t anim_cnt;
	uint8_t anim_frame;

	uint8_t action_cnt;
	uint8_t grounded;

	uint8_t ext_disable;
	uint8_t cubejump_disable;
	uint8_t lift_fail;
	uint8_t control_disabled;  // Set at the start of lyle's main function.
} O_Lyle;

void o_load_lyle(Obj *o, uint16_t data);
void o_unload_lyle(void);

// Public functions

O_Lyle *lyle_get(void);

void lyle_get_bounced(void);
void lyle_get_hurt(void);
void lyle_kill(void);

fix32_t lyle_get_x(void);
fix32_t lyle_get_y(void);

int16_t lyle_get_hp(void);
int16_t lyle_get_cp(void);

#endif  // OBJ_LYLE_H
