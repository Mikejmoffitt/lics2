#ifndef SFX_H
#define SFX_H

#include <stdint.h>

// PSG sound effect streams.

#define SFX_SAMPLE(_pitch_, _vol_) {1, (_pitch_) & 0x3FF, (_vol_) & 0x0F}
#define SFX_END() {0}

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
	SFX_TELEPORT,
	SFX_TELEPORT_2,
	SFX_BOINGO_JUMP,
	SFX_POWERUP_GET,

	SFX_EXPLODE,
} SfxId;

typedef struct SfxSample
{
	int8_t valid;
	uint16_t pitch;
	int8_t vol;
} SfxSample;

typedef struct SfxChannelState
{
	SfxId id;
	const SfxSample *stream;  // NULL if not playing a sample.
	int8_t channel;  // 0 - 2.
	int8_t priority;  // 0 = highest priority.
} SfxChannelState;

// This calls the main routine for the sound engine. It should be called at
// 300Hz. It spreads its work across six calls to reduce load. This is also
// what allows it to work across both NTSC and PAL without adjustment.
void sfx_tick(void);

int sfx_init(void);

void sfx_play(SfxId id, int8_t priority);
void sfx_play_on_channel(SfxId id, int8_t priority, int8_t channel);
void sfx_stop(SfxId id);
void sfx_stop_all(void);

#endif  // SFX_H
