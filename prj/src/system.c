#include "system.h"

#include "md/megadrive.h"

static int16_t is_ntsc_cache;
static int16_t is_debug_enabled;

static volatile char activity[64];

int system_init(void)
{
	megadrive_init();
	is_ntsc_cache = (vdp_get_status() & VDP_STATUS_PAL) ? 0 : 1;
	return 1;
}

uint16_t system_is_ntsc(void)
{
	return is_ntsc_cache;
}

uint16_t system_rand(void)
{
	// TODO: Implement a better random, maybe an LSFR
	return vdp_get_hv_count();
}

int16_t system_is_debug_enabled(void)
{
	return is_debug_enabled;
}

void system_set_debug_enabled(int16_t en)
{
	is_debug_enabled = en;
}
