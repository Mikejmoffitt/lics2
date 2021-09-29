#include "persistent_state.h"
#include "progress.h"

static PersistentState s_persistent_state;

void persistent_state_init(void)
{
	// TODO: How do we not have fucking memset in this toolchain??
	uint8_t *raw = (uint8_t *)(&s_persistent_state);
	for (uint16_t i = 0; i < sizeof(s_persistent_state); i++)
	{
		raw[i] = 0;
	}
	s_persistent_state.lyle_hp = 5;
}

PersistentState *persistent_state_get(void)
{
	return &s_persistent_state;
}


