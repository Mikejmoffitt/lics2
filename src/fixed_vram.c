#include "fixed_vram.h"
#include "gfx.h"

void fixed_vram_load(void)
{
	gfx_load(gfx_get(GFX_SYS_HUD), HUD_VRAM_POSITION);
	gfx_load(gfx_get(GFX_SYS_CUBES), CUBE_VRAM_POSITION);
	gfx_load(gfx_get(GFX_SYS_POWERUP), POWERUP_VRAM_POSITION);
	gfx_load(gfx_get(GFX_SYS_PARTICLE), PARTICLE_VRAM_POSITION);
	gfx_load(gfx_get(GFX_SYS_PROJECTILE), PROJECTILE_VRAM_POSITION);
	gfx_load(gfx_get(GFX_SYS_PAUSE), PAUSE_VRAM_POSITION);
}
