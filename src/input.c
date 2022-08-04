#include "input.h"

#include "md/megadrive.h"

static LyleBtn s_button_cache;

LyleBtn input_read(void)
{
	return s_button_cache;
}

void input_poll(void)
{
	s_button_cache = 0;
#ifdef MDK_TARGET_C2
	const MdIoCPlayerInput ply = md_ioc_get_player_input(0);
	if (ply & SYSC_PL_UP) s_button_cache |= LYLE_BTN_UP;
	if (ply & SYSC_PL_DOWN) s_button_cache |= LYLE_BTN_DOWN;
	if (ply & SYSC_PL_LEFT) s_button_cache |= LYLE_BTN_LEFT;
	if (ply & SYSC_PL_RIGHT) s_button_cache |= LYLE_BTN_RIGHT;
	if (ply & SYSC_PL_B) s_button_cache |= LYLE_BTN_JUMP;
	if (ply & SYSC_PL_A) s_button_cache |= LYLE_BTN_CUBE;
	if (ply & SYSC_PL_C) s_button_cache |= LYLE_BTN_CUBE;
	const MdIoCSystemInput sys = md_ioc_get_system_input();
	if (sys & SYSC_SYSTEM_START1) s_button_cache |= LYLE_BTN_START;
#else
	const uint16_t buttons = g_md_pad[0];
	if (buttons & BTN_UP) s_button_cache |= LYLE_BTN_UP;
	if (buttons & BTN_DOWN) s_button_cache |= LYLE_BTN_DOWN;
	if (buttons & BTN_LEFT) s_button_cache |= LYLE_BTN_LEFT;
	if (buttons & BTN_RIGHT) s_button_cache |= LYLE_BTN_RIGHT;
	if (buttons & BTN_START) s_button_cache |= LYLE_BTN_START;
	if (buttons & BTN_A) s_button_cache |= LYLE_BTN_JUMP;
	if (buttons & BTN_B) s_button_cache |= LYLE_BTN_CUBE;
	if (buttons & BTN_C) s_button_cache |= LYLE_BTN_JUMP;
#endif
}
