#include "persistent_state.h"
#include "progress.h"
#include <string.h>

#include "lyle.h"

PersistentState g_persistent_state;

void persistent_state_init(void)
{
	memset(&g_persistent_state, 0, sizeof(g_persistent_state));
	g_persistent_state.lyle_hp = LYLE_START_HP;
	g_persistent_state.lyle_cp = LYLE_START_CP;
}
