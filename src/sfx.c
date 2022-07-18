#include "sfx.h"
#include "md/megadrive.h"
#include "system.h"

#include <stdlib.h>

#include "sfx_defs.c.inc"

#define SFX_TICKS_PER_ITERATION 4

static SfxChannelState s_channels[3];

// Incremented by H-IRQ callback. When it surpasses SFX_TICKS_PER_ITERATION,
// sfx_tick() can be called by sfx_poll().
static uint16_t s_ticks;

// Sound routines.
static void sfx_tick(void)
{
	int8_t i = ARRAYSIZE(s_channels);
	while (i--)
	{
		SfxChannelState *s = &s_channels[i];
		if (!s->stream)
		{
			md_psg_vol(s->channel, 0xF);
			continue;
		}

		const SfxSample *sample = s->stream;
		if (!sample->valid)
		{
			s->stream = NULL;
			md_psg_vol(s->channel, 0xF);
			continue;
		}
		else if (sample->pitch == 0xFFFF)
		{
			s->stream = s->stream_base;
			sample = s->stream;
		}
		else
		{
			s->stream++;
		}

		md_psg_pitch(s->channel, sample->pitch);
		md_psg_vol(s->channel, sample->vol);
	}
}

// Returns non-zero if enough ticks could be consumed to run the sound engine.
// Disables interrupts during to ensure timer callback does not cause trouble.
static uint16_t maybe_consume_ticks(void)
{
	if (s_ticks < SFX_TICKS_PER_ITERATION) return 0;
	md_sys_di();
	uint16_t ret = 0;
	if (s_ticks >= SFX_TICKS_PER_ITERATION)
	{
		s_ticks -= SFX_TICKS_PER_ITERATION;
		ret = 1;
	}
	md_sys_ei();
	return ret;
}

void sfx_poll(void)
{
	while (maybe_consume_ticks())
	{
		sfx_tick();
	}
}

void h_irq_callback(void)
{
	s_ticks++;
	sfx_poll();
}

void sfx_init(void)
{
	md_vdp_set_hint_line(system_is_ntsc() ? 220 / 5 : 220 / 6);
	md_vdp_set_hint_en(1);

	int8_t i = ARRAYSIZE(s_channels);
	while (i--)
	{
		SfxChannelState *s = &s_channels[i];
		s->id = SFX_NULL;
		s->stream = NULL;
		s->channel = i;
		s->priority = 0x7F;
		md_psg_vol(s->channel, 0xF);
	}

	md_irq_register(MD_IRQ_HBLANK, h_irq_callback);
}

void sfx_play_on_channel(SfxId id, int8_t priority, int8_t channel)
{
	md_sys_di();
	const SfxSample *stream = stream_by_id[id];
	if (stream == NULL) return;
	SfxChannelState *s = &s_channels[channel];
	if (priority > s->priority) return;
	s->id = id;
	s->stream = stream;
	s->stream_base = stream;
	s->channel = channel;
	s->priority = priority;
	md_sys_ei();
}

void sfx_play(SfxId id, int8_t priority)
{
	md_sys_di();
	const SfxSample *stream = stream_by_id[id];
	if (stream == NULL) return;
	for (uint8_t i = 0; i < ARRAYSIZE(s_channels); i++)
	{
		SfxChannelState *s = &s_channels[i];
		if (s->stream) continue;
		s->id = id;
		s->stream = stream;
		s->stream_base = stream;
		s->channel = i;
		s->priority = priority;
		goto finish;
	}

	// If we've reached this part then we need to boot a stream that has a
	// lower priority than our new one
	int8_t lowest_priority = -1;
	int8_t lowest_index = -1;
	for (uint8_t i = 0; i < ARRAYSIZE(s_channels); i++)
	{
		SfxChannelState *s = &s_channels[i];
		if (s->priority > lowest_priority)
		{
			lowest_priority = s->priority;
			lowest_index = i;
		}
	}

	// Couldn't find one to boot - so abort.
	if (lowest_priority == -1) goto finish;
	if (lowest_priority < priority) goto finish;
	
	SfxChannelState *s = &s_channels[lowest_index];
	s->id = id;
	s->stream = stream;
	s->stream_base = stream;
	s->channel = lowest_index;
	s->priority = priority;

finish:
	md_sys_ei();
}

void sfx_stop(SfxId id)
{
	md_sys_di();
	int8_t i = ARRAYSIZE(s_channels);
	while (i--)
	{
		SfxChannelState *s = &s_channels[i];
		if (s->stream && s->id == id)
		{
			s->stream = NULL;
			md_psg_vol(s->channel, 0xF);
			break;
		}
	}

	md_sys_ei();
}

void sfx_stop_all(void)
{
	md_sys_di();
	int8_t i = ARRAYSIZE(s_channels);
	while (i--)
	{
		s_channels[i].stream = NULL;
		md_psg_vol(i, 0xF);
	}

	md_sys_ei();
}
#undef SFX_P
#undef SFX_END
