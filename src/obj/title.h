#ifndef OBJ_TITLE_H
#define OBJ_TITLE_H

#include "obj.h"
#include "input.h"

typedef enum TitleState
{
	TITLE_STATE_INIT,
	TITLE_STATE_INTRO,
	TITLE_STATE_CUTSCENE,
	TITLE_STATE_MENU,
	TITLE_STATE_BEGIN
} TitleState;

typedef struct O_Title
{
	Obj head;
	LyleBtn buttons_prev;

	TitleState state;
	TitleState state_prev;

	fix32_t orig_y;

	fix32_t v_scroll_y;
	fix16_t v_scroll_dy;

	fix32_t cloakdude_x;
	fix16_t cloakdude_dx;


	int16_t cloakdude_anim_cnt;
	int8_t cloakdude_anim_frame;
	int8_t cloakdude_anim_state;
	// 0 = walking forwards
	// 1 = looking around
	// 2 = running away (with kitty)
	// 3 = invisible

	int16_t kitty_anim_cnt;
	int8_t kitty_anim_frame;
	int8_t kitty_anim_state;
	// 0 = sleeping
	// 1 = standing
	// 2 = invisible (combined with cloakdude)

	int16_t state_elapsed;

	int16_t lyle_anim_cnt;
	int16_t lyle_anim_frame;

	int8_t menu_choice;
	int16_t menu_flash_cnt;
} O_Title;

uint16_t title_get_vram_pos(void);

void o_load_title(Obj *o, uint16_t data);
void o_unload_title(void);

#endif  // OBJ_TITLE_H
