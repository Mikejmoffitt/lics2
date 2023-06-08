#include "sfx.h"
#include "md/megadrive.h"
#include "system.h"
#include "sfx_engine.h"

#include <stdlib.h>

void sfx_init(void)
{
	sfx_engine_init();

	md_irq_register_unsafe(MD_IRQ_HBLANK, sfx_engine_irq_handler);
	md_vdp_set_hint_line(system_is_ntsc() ? 220 / 5 : 220 / 6);
	md_vdp_set_hint_en(1);

}

void sfx_play(SfxId id, uint16_t priority)
{
	md_sys_di();
	sfx_engine_play(id, priority);
	md_sys_ei();
}

void sfx_stop(SfxId id)
{
	md_sys_di();
	sfx_engine_stop(id);
	md_sys_ei();
}

void sfx_stop_all(void)
{
	md_sys_di();
	sfx_engine_init();
	md_sys_ei();
}
#undef SFX_P
#undef SFX_END
