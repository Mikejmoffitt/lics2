#include "sfx.h"
#include "md/megadrive.h"
#include "system.h"
#include "common.h"

#include <stdlib.h>

static uint8_t cycle_count = 0;
static const uint8_t kcycle_max = 4;

static SfxChannelState channel_state[3];

static const SfxSample sfx_null[] =
{
	SFX_END()
};

// The sounds themselves.
static const SfxSample sfx_jump[] =
{
	SFX_SAMPLE(0x200, 0),
	SFX_SAMPLE(0x1D9, 0),
	SFX_SAMPLE(0x1B0, 0),
	SFX_SAMPLE(0x180, 0),
	SFX_SAMPLE(0x170, 0),
	SFX_SAMPLE(0x160, 1),
	SFX_SAMPLE(0x150, 2),
	SFX_SAMPLE(0x130, 3),
	SFX_SAMPLE(0x110, 4),
	SFX_SAMPLE(0x100, 8),
	SFX_SAMPLE(0x98, 10),
	SFX_END()
};

static const SfxSample sfx_walk1[] =
{
	SFX_SAMPLE(0x90, 0),
	SFX_SAMPLE(0x95, 2),
	SFX_SAMPLE(0x9A, 4),
	SFX_SAMPLE(0xA4, 8),
	SFX_SAMPLE(0xAE, 12),
	SFX_END()
};

static const SfxSample sfx_walk2[] =
{
	SFX_SAMPLE(0xB0, 0),
	SFX_SAMPLE(0xB5, 2),
	SFX_SAMPLE(0xBA, 4),
	SFX_SAMPLE(0xC4, 8),
	SFX_SAMPLE(0xCE, 12),
	SFX_END()
};

static const SfxSample sfx_hurt[] =
{
	SFX_SAMPLE(0x50, 0),
//	SFX_SAMPLE(0x57, 0),
	SFX_SAMPLE(0x5C, 0),
//	SFX_SAMPLE(0x62, 0),
	SFX_SAMPLE(0x69, 0),
//	SFX_SAMPLE(0x6F, 0),
	SFX_SAMPLE(0x74, 0),
	SFX_SAMPLE(0x7B, 0),
	SFX_SAMPLE(0x87, 0),
	SFX_SAMPLE(0x8F, 0),
	SFX_SAMPLE(0x99, 0),
	SFX_SAMPLE(0x105, 0),
	SFX_SAMPLE(0x114, 0),
	SFX_SAMPLE(0x121, 0),

	SFX_SAMPLE(0x40, 0),
	SFX_SAMPLE(0x43, 0),
	SFX_SAMPLE(0x46, 0),
	SFX_SAMPLE(0x4A, 0),
	SFX_SAMPLE(0x4D, 0),
	SFX_SAMPLE(0x53, 0),
	SFX_SAMPLE(0x56, 0),
//	SFX_SAMPLE(0x59, 0),
//	SFX_SAMPLE(0x5D, 0),
//	SFX_SAMPLE(0x62, 0),

	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),

	SFX_SAMPLE(0x40, 8),
	SFX_SAMPLE(0x43, 8),
	SFX_SAMPLE(0x46, 8),
	SFX_SAMPLE(0x4A, 8),
	SFX_SAMPLE(0x4D, 8),
	SFX_SAMPLE(0x53, 8),
	SFX_SAMPLE(0x56, 8),
//	SFX_SAMPLE(0x59, 8),
//	SFX_SAMPLE(0x5D, 8),
//	SFX_SAMPLE(0x62, 8),
	SFX_END()
};

static const SfxSample sfx_cube_lift[] =
{
	SFX_SAMPLE(0x01C0, 0),
	SFX_SAMPLE(0x0200, 0),
	SFX_SAMPLE(0x0280, 0),
	SFX_SAMPLE(0x0300, 0),
	SFX_SAMPLE(0x03E0, 0),
	SFX_SAMPLE(0x0250, 0),
	SFX_SAMPLE(0x01A0, 0),
	SFX_SAMPLE(0x0160, 0),
	SFX_SAMPLE(0x0120, 0),
	SFX_SAMPLE(0x0100, 0),
	SFX_SAMPLE(0x0090, 0),
	SFX_END()
};

static const SfxSample sfx_cube_spawn[] =
{
	SFX_SAMPLE(0x03FF, 0),
	SFX_SAMPLE(0x0390, 0),
	SFX_SAMPLE(0x0320, 0),
	SFX_SAMPLE(0x02C0, 0),
	SFX_SAMPLE(0x0290, 0),
	SFX_SAMPLE(0x0240, 0),

	SFX_SAMPLE(0x02C0, 0),
	SFX_SAMPLE(0x0290, 0),
	SFX_SAMPLE(0x0240, 0),
	SFX_SAMPLE(0x01F0, 0),
	SFX_SAMPLE(0x01A0, 0),
	SFX_SAMPLE(0x0160, 0),

	SFX_SAMPLE(0x02C0 - 0x10, 0),
	SFX_SAMPLE(0x0290 - 0x10, 0),
	SFX_SAMPLE(0x0240 - 0x10, 0),
	SFX_SAMPLE(0x01F0 - 0x10, 0),
	SFX_SAMPLE(0x01A0 - 0x10, 0),
	SFX_SAMPLE(0x0160 - 0x10, 0),

	SFX_SAMPLE(0x02C0 - 0x22, 0),
	SFX_SAMPLE(0x0290 - 0x22, 0),
	SFX_SAMPLE(0x0240 - 0x22, 0),
	SFX_SAMPLE(0x01F0 - 0x22, 0),
	SFX_SAMPLE(0x01A0 - 0x22, 0),
	SFX_SAMPLE(0x0160 - 0x22, 0),

	SFX_SAMPLE(0x02C0 - 0x44, 0),
	SFX_SAMPLE(0x0290 - 0x44, 0),
	SFX_SAMPLE(0x0240 - 0x44, 0),
	SFX_SAMPLE(0x01F0 - 0x44, 0),
	SFX_SAMPLE(0x01A0 - 0x44, 0),
	SFX_SAMPLE(0x0160 - 0x44, 0),

	SFX_SAMPLE(0x02C0 - 0x69, 0),
	SFX_SAMPLE(0x0290 - 0x69, 0),
	SFX_SAMPLE(0x0240 - 0x69, 0),
	SFX_SAMPLE(0x01F0 - 0x69, 0),
	SFX_SAMPLE(0x01A0 - 0x69, 0),
	SFX_SAMPLE(0x0160 - 0x69, 0),

	SFX_SAMPLE(0x02C0 - 0x99, 0),
	SFX_SAMPLE(0x0290 - 0x99, 0),
	SFX_SAMPLE(0x0240 - 0x99, 0),
	SFX_SAMPLE(0x01F0 - 0x99, 0),
	SFX_SAMPLE(0x01A0 - 0x99, 0),
	SFX_SAMPLE(0x0160 - 0x99, 0),

	SFX_SAMPLE(0x02C0 - 0xB9, 0),
	SFX_SAMPLE(0x0290 - 0xB9, 0),
	SFX_SAMPLE(0x0240 - 0xB9, 0),
	SFX_SAMPLE(0x01F0 - 0xB9, 0),
	SFX_SAMPLE(0x01A0 - 0xB9, 0),
	SFX_SAMPLE(0x0160 - 0xB9, 0),

	SFX_SAMPLE(0x02C0 - 0xE9, 0),
	SFX_SAMPLE(0x0290 - 0xE9, 0),
	SFX_SAMPLE(0x0240 - 0xE9, 0),
	SFX_SAMPLE(0x01F0 - 0xE9, 0),
	SFX_SAMPLE(0x01A0 - 0xE9, 0),
	SFX_SAMPLE(0x0160 - 0xE9, 0),

	SFX_SAMPLE(0x02C0 - 0x129, 0),
	SFX_SAMPLE(0x0290 - 0x129, 0),
	SFX_SAMPLE(0x0240 - 0x129, 0),
	SFX_SAMPLE(0x01F0 - 0x129, 0),
	SFX_SAMPLE(0x01A0 - 0x129, 0),
	SFX_SAMPLE(0x0160 - 0x129, 0),

	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),

	SFX_SAMPLE(0x02C0 - 0x129, 6),
	SFX_SAMPLE(0x0290 - 0x129, 6),
	SFX_SAMPLE(0x0240 - 0x129, 6),
	SFX_SAMPLE(0x01F0 - 0x129, 6),
	SFX_SAMPLE(0x01A0 - 0x129, 6),
	SFX_SAMPLE(0x0160 - 0x129, 6),

	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),
	SFX_SAMPLE(0, 0xF),

	SFX_SAMPLE(0x02C0 - 0x129, 9),
	SFX_SAMPLE(0x0290 - 0x129, 9),
	SFX_SAMPLE(0x0240 - 0x129, 9),
	SFX_SAMPLE(0x01F0 - 0x129, 9),
	SFX_SAMPLE(0x01A0 - 0x129, 9),
	SFX_SAMPLE(0x0160 - 0x129, 9),
	SFX_END()
};

static const SfxSample sfx_cube_toss[] =
{
	SFX_SAMPLE(0x290, 0),
	SFX_SAMPLE(0x240, 0),
	SFX_SAMPLE(0x1E0, 0),
	SFX_SAMPLE(0x1B4, 0),
	SFX_SAMPLE(0x174, 0),
	SFX_SAMPLE(0x154, 0),
	SFX_SAMPLE(0x124, 0),
	SFX_SAMPLE(0x94, 0),
	SFX_SAMPLE(0x64, 0),
	SFX_SAMPLE(0x53, 0),
	SFX_SAMPLE(0x47, 0),
	SFX_SAMPLE(0x45, 0),
	SFX_SAMPLE(0x3E, 0),
	SFX_SAMPLE(0x39, 0),
	SFX_SAMPLE(0x34, 0),

	SFX_SAMPLE(0x154, 6),
	SFX_SAMPLE(0x124, 6),
	SFX_SAMPLE(0x94, 6),
	SFX_SAMPLE(0x64, 6),
	SFX_SAMPLE(0x53, 6),
	SFX_SAMPLE(0x47, 6),
	SFX_SAMPLE(0x45, 6),
	SFX_SAMPLE(0x3E, 6),
	SFX_SAMPLE(0x39, 6),
	SFX_SAMPLE(0x34, 6),
	SFX_END()
};

static const SfxSample sfx_cube_bounce[] =
{
	SFX_SAMPLE(0x0200, 0),
	SFX_SAMPLE(0x0300, 0),
	SFX_SAMPLE(0x03E0, 0),
	SFX_SAMPLE(0x0100, 0),
	SFX_SAMPLE(0x0140, 0),
	SFX_SAMPLE(0x01E0, 0),
	SFX_END()
};

static const SfxSample sfx_cube_hit[] =
{
	SFX_SAMPLE(0x0040, 0),
	SFX_SAMPLE(0x0200, 0),
	SFX_SAMPLE(0x0020, 0),
	SFX_SAMPLE(0x0074, 0),
	SFX_SAMPLE(0x0090, 0),
	SFX_SAMPLE(0x00A0, 0),
	SFX_SAMPLE(0x00B0, 0),
	SFX_SAMPLE(0x00C0, 0),
	SFX_SAMPLE(0x00D0, 0),
	SFX_SAMPLE(0x00E0, 0),
	SFX_SAMPLE(0x00F0, 0),
	SFX_SAMPLE(0x0108, 0),
	SFX_SAMPLE(0x0120, 0),
	SFX_SAMPLE(0x0138, 0),
	SFX_SAMPLE(0x0150, 0),

	SFX_SAMPLE(0x0090, 2),
	SFX_SAMPLE(0x00A0, 2),
	SFX_SAMPLE(0x00B0, 2),
	SFX_SAMPLE(0x00C0, 2),
	SFX_SAMPLE(0x00D0, 2),
	SFX_SAMPLE(0x00E0, 2),
	SFX_SAMPLE(0x00F0, 2),
	SFX_SAMPLE(0x0108, 2),
	SFX_SAMPLE(0x0120, 2),
	SFX_SAMPLE(0x0138, 2),
	SFX_SAMPLE(0x0150, 2),

	SFX_SAMPLE(0x0090, 4),
	SFX_SAMPLE(0x00A0, 4),
	SFX_SAMPLE(0x00B0, 4),
	SFX_SAMPLE(0x00C0, 4),
	SFX_SAMPLE(0x00D0, 4),
	SFX_SAMPLE(0x00E0, 4),
	SFX_SAMPLE(0x00F0, 4),
	SFX_SAMPLE(0x0108, 4),
	SFX_SAMPLE(0x0120, 4),
	SFX_SAMPLE(0x0138, 4),
	SFX_SAMPLE(0x0150, 4),
	SFX_END()
};

static const SfxSample sfx_cube_fizzle[] =
{
	SFX_SAMPLE(0x0188, 0),
	SFX_SAMPLE(0x0160, 0),
	SFX_SAMPLE(0x0150, 0),
	SFX_SAMPLE(0x0140, 0),
	SFX_SAMPLE(0x0130, 0),
	SFX_SAMPLE(0x0120, 0),

	SFX_SAMPLE(0x01B7, 0),
	SFX_SAMPLE(0x0188, 1),
	SFX_SAMPLE(0x0170, 0),
	SFX_SAMPLE(0x0160, 1),
	SFX_SAMPLE(0x0150, 1),
	SFX_SAMPLE(0x0140, 1),

	SFX_SAMPLE(0x01E5, 2),
	SFX_SAMPLE(0x01B7, 1),
	SFX_SAMPLE(0x01A3, 2),
	SFX_SAMPLE(0x0188, 1),
	SFX_SAMPLE(0x0170, 2),
	SFX_SAMPLE(0x0160, 2),

	SFX_SAMPLE(0x01E5, 4),
	SFX_SAMPLE(0x01B7, 5),
	SFX_SAMPLE(0x01A3, 4),
	SFX_SAMPLE(0x0188, 5),
	SFX_SAMPLE(0x0170, 4),
	SFX_SAMPLE(0x0160, 5),

	SFX_SAMPLE(0x01E5, 6),
	SFX_SAMPLE(0x01B7, 7),
	SFX_SAMPLE(0x01A3, 6),
	SFX_SAMPLE(0x0188, 7),
	SFX_SAMPLE(0x0170, 6),
	SFX_SAMPLE(0x0160, 7),
	SFX_END()
};

static const SfxSample sfx_obj_burst[] =
{
	SFX_SAMPLE(0x0240, 0),
	SFX_SAMPLE(0x0270, 0),
	SFX_SAMPLE(0x02A0, 0),
	SFX_SAMPLE(0x02F0, 0),
	SFX_SAMPLE(0x0330, 0),
	SFX_SAMPLE(0x0380, 0),
	SFX_SAMPLE(0x03B0, 0),
	SFX_SAMPLE(0x03FF, 0),

	SFX_SAMPLE(0x00F0 + 0x00, 0),
	SFX_SAMPLE(0x00F0 + 0x08, 0),
	SFX_SAMPLE(0x00F0 + 0x10, 0),
	SFX_SAMPLE(0x00F0 + 0x18, 0),
	SFX_SAMPLE(0x00F0 + 0x20, 0),
	SFX_SAMPLE(0x00F0 + 0x28, 0),
	SFX_SAMPLE(0x00F0 + 0x30, 0),
	SFX_SAMPLE(0x00F0 + 0x40, 0),
	SFX_SAMPLE(0x00F0 + 0x50, 0),
	SFX_SAMPLE(0x00F0 + 0x50, 0),
	SFX_SAMPLE(0x00F0 + 0x60, 0),
	SFX_SAMPLE(0x00F0 + 0x70, 0),
	SFX_SAMPLE(0x00F0 + 0x80, 0),
	SFX_SAMPLE(0x00F0 + 0x90, 0),
	SFX_SAMPLE(0x00F0 + 0xA0, 0),
	SFX_SAMPLE(0x00F0 + 0xB0, 0),
	SFX_SAMPLE(0x00F0 + 0xC0, 0),

	SFX_SAMPLE(0x00F0 + 0x00, 6),
	SFX_SAMPLE(0x00F0 + 0x08, 6),
	SFX_SAMPLE(0x00F0 + 0x10, 6),
	SFX_SAMPLE(0x00F0 + 0x18, 6),
	SFX_SAMPLE(0x00F0 + 0x20, 6),
	SFX_SAMPLE(0x00F0 + 0x28, 6),
	SFX_SAMPLE(0x00F0 + 0x30, 6),
	SFX_SAMPLE(0x00F0 + 0x40, 6),
	SFX_SAMPLE(0x00F0 + 0x50, 6),
	SFX_SAMPLE(0x00F0 + 0x50, 6),
	SFX_SAMPLE(0x00F0 + 0x60, 6),
	SFX_SAMPLE(0x00F0 + 0x70, 6),
	SFX_SAMPLE(0x00F0 + 0x80, 6),
	SFX_SAMPLE(0x00F0 + 0x90, 6),
	SFX_SAMPLE(0x00F0 + 0xA0, 6),
	SFX_SAMPLE(0x00F0 + 0xB0, 6),
	SFX_SAMPLE(0x00F0 + 0xC0, 6),
	SFX_END()
};

// Sound ID LUT.
static const SfxSample *stream_by_id[] =
{
	[SFX_NULL] = sfx_null,
	[SFX_JUMP] = sfx_jump,
	[SFX_WALK1] = sfx_walk1,
	[SFX_WALK2] = sfx_walk2,
	[SFX_HURT] = sfx_hurt,
	[SFX_CUBE_LIFT] = sfx_cube_lift,
	[SFX_CUBE_SPAWN] = sfx_cube_spawn,
	[SFX_CUBE_TOSS] = sfx_cube_toss,
	[SFX_CUBE_BOUNCE] = sfx_cube_bounce,
	[SFX_CUBE_HIT] = sfx_cube_hit,
	[SFX_CUBE_FIZZLE] = sfx_cube_fizzle,
	[SFX_OBJ_BURST] = sfx_obj_burst,
};

// Sound routines.
void sfx_init(void)
{
	vdp_set_hint_line(system_is_ntsc() ? 220 / 5 : 220 / 6);
	vdp_set_hint_en(1);

	int8_t i = ARRAYSIZE(channel_state);
	while (i--)
	{
		SfxChannelState *s = &channel_state[i];
		s->id = SFX_NULL;
		s->stream = NULL;
		s->channel = i;
		s->priority = 0x7F;
		psg_vol(s->channel, 0xF);
	}
}

void sfx_tick(void)
{
	cycle_count++;
	if (cycle_count >= kcycle_max) cycle_count = 0;
	else return;

	SYS_BARRIER();
	sys_di();
	SYS_BARRIER();
	int8_t i = ARRAYSIZE(channel_state);
	while (i--)
	{
		SfxChannelState *s = &channel_state[i];
		if (!s->stream)
		{
			psg_vol(s->channel, 0xF);
			continue;
		}

		const SfxSample *sample = s->stream;
		if (!sample->valid)
		{
			s->stream = NULL;
			psg_vol(s->channel, 0xF);
			continue;
		}
		else
		{
			s->stream++;
		}

		psg_pitch(s->channel, sample->pitch);
		psg_vol(s->channel, sample->vol);
	}
	SYS_BARRIER();
	sys_ei();
	SYS_BARRIER();
}

void sfx_play_on_channel(SfxId id, int8_t priority, int8_t channel)
{
	sys_di();
	const SfxSample *stream = stream_by_id[id];
	if (stream == NULL) return;
	SfxChannelState *s = &channel_state[channel];
	if (priority > s->priority) return;
	s->id = id;
	s->stream = stream;
	s->channel = channel;
	s->priority = priority;
	sys_ei();
}

void sfx_play(SfxId id, int8_t priority)
{
	sys_di();
	const SfxSample *stream = stream_by_id[id];
	if (stream == NULL) return;
	for (int8_t i = 0; i < ARRAYSIZE(channel_state); i++)
	{
		SfxChannelState *s = &channel_state[i];
		if (s->stream) continue;
		s->id = id;
		s->stream = stream;
		s->channel = i;
		s->priority = priority;
		goto finish;
	}

	// If we've reached this part then we need to boot a stream that has a
	// lower priority than our new one
	int8_t lowest_priority = -1;
	int8_t lowest_index = -1;
	for (int8_t i = 0; i < ARRAYSIZE(channel_state); i++)
	{
		SfxChannelState *s = &channel_state[i];
		if (s->priority > lowest_priority)
		{
			lowest_priority = s->priority;
			lowest_index = i;
		}
	}

	// Couldn't find one to boot - so abort.
	if (lowest_priority == -1) goto finish;
	if (lowest_priority < priority) goto finish;
	
	SfxChannelState *s = &channel_state[lowest_index];
	s->id = id;
	s->stream = stream;
	s->channel = lowest_index;
	s->priority = priority;

finish:
	sys_ei();
}

void sfx_stop(SfxId id)
{
	sys_di();
	int8_t i = ARRAYSIZE(channel_state);
	while (i--)
	{
		SfxChannelState *s = &channel_state[i];
		if (s->stream && s->id == id)
		{
			s->stream = NULL;
			psg_vol(s->channel, 0xF);
			break;
		}
	}

	sys_ei();
}

void sfx_stop_all(void)
{
	sys_di();
	int8_t i = ARRAYSIZE(channel_state);
	while (i--)
	{
		channel_state[i].stream = NULL;
		psg_vol(i, 0xF);
	}

	sys_ei();
}

