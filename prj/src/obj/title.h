#ifndef OBJ_TITLE_H
#define OBJ_TITLE_H

#include "obj.h"

typedef struct O_Title
{
	Obj head;
	int16_t initialized;
	MdButton buttons_prev;

	fix32_t orig_y;

	int16_t v_scroll_delay_cnt;
	fix32_t v_scroll_y;
	fix16_t v_scroll_dy;
	int16_t v_scroll_complete;

	int16_t appearance_delay_cnt;
} O_Title;

void o_load_title(Obj *o, uint16_t data);
void o_unload_title(void);

// Clear the static flag that disables the title screen object.
void title_reset_hide_flag(void);

#endif  // OBJ_TITLE_H
