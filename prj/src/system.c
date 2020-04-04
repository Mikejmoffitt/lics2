#include "system.h"

#include "md/megadrive.h"

static int16_t is_ntsc_cache;

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
