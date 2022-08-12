#include "physics.h"
#include "palscale.h"

void physics_init(void)
{
	for (uint16_t i = 0; i < ARRAYSIZE(g_fix_trunc_table_pos); i++)
	{
		g_fix_trunc_table_pos[i] = INTTOFIX16(PALSCALE_1ST(i));
		g_fix_trunc_table_neg[i] = -g_fix_trunc_table_pos[i];
	}
}
