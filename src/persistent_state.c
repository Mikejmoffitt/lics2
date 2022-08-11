#include "persistent_state.h"
#include "progress.h"

#include "lyle.h"

PersistentState g_persistent_state;

void persistent_state_init(void)
{
	// TODO: How do we not have fucking memset in this toolchain??
	uint8_t *raw = (uint8_t *)(&g_persistent_state);
	for (uint16_t i = 0; i < sizeof(g_persistent_state); i++)
	{
		raw[i] = 0;
	}
	g_persistent_state.lyle_hp = LYLE_START_HP;
	g_persistent_state.lyle_cp = LYLE_START_CP;
}
