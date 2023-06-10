#ifndef SFX_H
#define SFX_H

#include <stdint.h>

// PSG sound effect streams.

typedef enum SfxId
{
	SFX_NULL,
	SFX_JUMP,
	SFX_WALK1,
	SFX_WALK2,
	SFX_HURT,
	SFX_CUBE_LIFT,
	SFX_CUBE_SPAWN,
	SFX_CUBE_TOSS,
	SFX_CUBE_BOUNCE,
	SFX_CUBE_HIT,
	SFX_CUBE_FIZZLE,
	SFX_OBJ_BURST,
	SFX_OBJ_BURST_HI,
	SFX_TELEPORT,
	SFX_KILLZAM_WARP,
	SFX_BOINGO_JUMP,
	SFX_POWERUP_GET,
	SFX_MAGIBEAR_SHOT,
	SFX_GAXTER_SHOT,
	SFX_MEOW,
	SFX_EXPLODE,
	SFX_ELEVATOR,
	SFX_ELEVATOR_2,
	SFX_PAUSE_1,
	SFX_PAUSE_2,
	SFX_MOO,
	SFX_BOSS_STEP,
	SFX_GIVER,
	SFX_SNORE,
	SFX_FREE3,
	SFX_BEEP,
	SFX_SELECT,
	SFX_FREE4,
	SFX_SLAM,
} SfxId;

// Initializes playback state, sets up horizontal interrupts for timing, and
// registers a timer interrupt callback (or, rather, an H-blank IRQ).
void sfx_init(void);

void sfx_play(SfxId id, uint16_t priority);
void sfx_stop(SfxId id);
void sfx_stop_all(void);

#endif  // SFX_H
