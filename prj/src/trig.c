#include "trig.h"
#include "util/fixed.h"

uint8_t fix_atan(fix32_t ratio)
{
	if (ratio >= INTTOFIX16(TRIG_TAB_ATAN_INPUT_RANGE))
	{
		return trig_tab_atan[ARRAYSIZE(trig_tab_atan) - 1];
	}
	if (ratio < 0) return 0;
	ratio = ratio >> 2;
	return trig_tab_atan[ratio];
}
