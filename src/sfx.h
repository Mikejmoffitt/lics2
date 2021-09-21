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
	SFX_TELEPORT_2,
	SFX_BOINGO_JUMP,
	SFX_POWERUP_GET,
	SFX_MAGIBEAR_SHOT,
	SFX_GAXTER_SHOT,
	SFX_GAXTER_SHOT_2,
	SFX_EXPLODE,
	SFX_ELEVATOR,
	SFX_ELEVATOR_2,
	SFX_PAUSE_1,
	SFX_PAUSE_2,
	SFX_MOO_1,
	SFX_MOO_2,
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
	const SfxSample *stream_base;
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
