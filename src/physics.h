#ifndef PHYSICS_H
#define PHYSICS_H
#include "md/macro.h"

#include "util/fixed.h"

extern fix16_t g_fix_trunc_table_pos[10];
extern fix16_t g_fix_trunc_table_neg[10];

void physics_init(void);

// Truncate movement vectors the way the MMF2 game did.
static fix16_t physics_trunc_fix16(fix16_t v)
{
	if (v == 0) return 0;
	else if (v > 0)
	{
		for (uint16_t i = 1; i < ARRAYSIZE(g_fix_trunc_table_pos); i++)
		{
			if (v < g_fix_trunc_table_pos[i]) return g_fix_trunc_table_pos[i - 1];
		}
		return g_fix_trunc_table_pos[ARRAYSIZE(g_fix_trunc_table_pos) - 1];
	}
	else if (v < 0)
	{
		for (uint16_t i = 1; i < ARRAYSIZE(g_fix_trunc_table_neg); i++)
		{
			if (v > g_fix_trunc_table_neg[i]) return g_fix_trunc_table_neg[i - 1];
		}
		return g_fix_trunc_table_pos[ARRAYSIZE(g_fix_trunc_table_neg) - 1];
	}
	return 0;
}


#endif  // PHYSICS_H
