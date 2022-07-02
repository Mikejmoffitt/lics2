#include "sfx.h"
#include "md/megadrive.h"
#include "system.h"
#include "common.h"
#include <stdlib.h>

#define SFX_P(_pitch_, _vol_) {1, ((int)(_pitch_)) & 0x3FF, (_vol_) & 0x0F}
#define SFX_END() {0}
#define SFX_LOOP() {1, 0xFFFF, 0}

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
	SFX_P(512 * 1.4, 0),
	SFX_P(512 * 1.1, 0),
	SFX_P(512 * 0.85, 0),
	SFX_P(512 * 0.80, 0),
	SFX_P(512 * 0.77, 0),
	SFX_P(512 * 0.74, 1),
	SFX_P(512 * 0.71, 1),
	SFX_P(512 * 0.68, 2),
	SFX_P(512 * 0.65, 2),
	SFX_P(512 * 0.62, 3),
	SFX_P(512 * 0.59, 4),
	SFX_P(512 * 0.56, 5),
	SFX_P(512 * 0.54, 6),
	SFX_P(512 * 0.52, 8),
	SFX_P(512 * 0.50, 10),
/*	SFX_P(0x200, 0),
	SFX_P(0x1D9, 0),
	SFX_P(0x1B0, 0),
	SFX_P(0x180, 0),
	SFX_P(0x170, 0),
	SFX_P(0x160, 1),
	SFX_P(0x150, 2),
	SFX_P(0x130, 3),
	SFX_P(0x110, 4),
	SFX_P(0x100, 8),
	SFX_P(0x98, 10),*/
	SFX_END()
};

static const SfxSample sfx_walk1[] =
{
	SFX_P(0x90, 2),
	SFX_P(0x9B, 4),
	SFX_P(0xA7, 7),
	SFX_P(0xB2, 11),
	SFX_P(0xBF, 13),
	SFX_END()
};

static const SfxSample sfx_walk2[] =
{
	SFX_P(0xB0, 2),
	SFX_P(0xBB, 4),
	SFX_P(0xC7, 7),
	SFX_P(0xD2, 11),
	SFX_P(0xDF, 13),
	SFX_END()
};

static const SfxSample sfx_hurt[] =
{
	SFX_P(0x50, 0),
//	SFX_P(0x57, 0),
	SFX_P(0x5C, 0),
//	SFX_P(0x62, 0),
	SFX_P(0x69, 0),
//	SFX_P(0x6F, 0),
	SFX_P(0x74, 0),
	SFX_P(0x7B, 0),
	SFX_P(0x87, 0),
	SFX_P(0x8F, 0),
	SFX_P(0x99, 0),
	SFX_P(0x105, 0),
	SFX_P(0x114, 0),
	SFX_P(0x121, 0),

	SFX_P(0x40, 0),
	SFX_P(0x43, 0),
	SFX_P(0x46, 0),
	SFX_P(0x4A, 0),
	SFX_P(0x4D, 0),
	SFX_P(0x53, 0),
	SFX_P(0x56, 0),
//	SFX_P(0x59, 0),
//	SFX_P(0x5D, 0),
//	SFX_P(0x62, 0),

	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),

	SFX_P(0x40, 8),
	SFX_P(0x43, 8),
	SFX_P(0x46, 8),
	SFX_P(0x4A, 8),
	SFX_P(0x4D, 8),
	SFX_P(0x53, 8),
	SFX_P(0x56, 8),
//	SFX_P(0x59, 8),
//	SFX_P(0x5D, 8),
//	SFX_P(0x62, 8),
	SFX_END()
};

static const SfxSample sfx_cube_lift[] =
{
	SFX_P(0x60 + 0x01C0, 0),
	SFX_P(0x60 + 0x0200, 0),
	SFX_P(0x60 + 0x0280, 0),
	SFX_P(0x60 + 0x0300, 0),
	SFX_P(0x03FF, 0),
	SFX_P(0x60 + 0x0250, 0),
	SFX_P(0x60 + 0x01A0, 0),
	SFX_P(0x60 + 0x0160, 0),
	SFX_P(0x60 + 0x0120, 0),
	SFX_P(0x60 + 0x0100, 0),
	SFX_P(0x60 + 0x0090, 0),
	SFX_P(0x60 + 0x01A0, 4),
	SFX_P(0x60 + 0x0160, 4),
	SFX_P(0x60 + 0x0120, 4),
	SFX_P(0x60 + 0x0100, 4),
	SFX_P(0x60 + 0x0090, 4),
	SFX_END()
};

static const SfxSample sfx_cube_spawn[] =
{
	SFX_P(0x03FF, 0),
	SFX_P(0x0390, 0),
	SFX_P(0x0320, 0),
	SFX_P(0x02C0, 0),
	SFX_P(0x0290, 0),
	SFX_P(0x0240, 0),

	SFX_P(0x02C0, 0),
	SFX_P(0x0290, 0),
	SFX_P(0x0240, 0),
	SFX_P(0x01F0, 0),
	SFX_P(0x01A0, 0),
	SFX_P(0x0160, 0),

	SFX_P(0x02C0 - 0x10, 0),
	SFX_P(0x0290 - 0x10, 0),
	SFX_P(0x0240 - 0x10, 0),
	SFX_P(0x01F0 - 0x10, 0),
	SFX_P(0x01A0 - 0x10, 0),
	SFX_P(0x0160 - 0x10, 0),

	SFX_P(0x02C0 - 0x22, 0),
	SFX_P(0x0290 - 0x22, 0),
	SFX_P(0x0240 - 0x22, 0),
	SFX_P(0x01F0 - 0x22, 0),
	SFX_P(0x01A0 - 0x22, 0),
	SFX_P(0x0160 - 0x22, 0),

	SFX_P(0x02C0 - 0x44, 0),
	SFX_P(0x0290 - 0x44, 0),
	SFX_P(0x0240 - 0x44, 0),
	SFX_P(0x01F0 - 0x44, 0),
	SFX_P(0x01A0 - 0x44, 0),
	SFX_P(0x0160 - 0x44, 0),

	SFX_P(0x02C0 - 0x69, 0),
	SFX_P(0x0290 - 0x69, 0),
	SFX_P(0x0240 - 0x69, 0),
	SFX_P(0x01F0 - 0x69, 0),
	SFX_P(0x01A0 - 0x69, 0),
	SFX_P(0x0160 - 0x69, 0),

	SFX_P(0x02C0 - 0x99, 0),
	SFX_P(0x0290 - 0x99, 0),
	SFX_P(0x0240 - 0x99, 0),
	SFX_P(0x01F0 - 0x99, 0),
	SFX_P(0x01A0 - 0x99, 0),
	SFX_P(0x0160 - 0x99, 0),

	SFX_P(0x02C0 - 0xB9, 0),
	SFX_P(0x0290 - 0xB9, 0),
	SFX_P(0x0240 - 0xB9, 0),
	SFX_P(0x01F0 - 0xB9, 0),
	SFX_P(0x01A0 - 0xB9, 0),
	SFX_P(0x0160 - 0xB9, 0),

	SFX_P(0x02C0 - 0xE9, 0),
	SFX_P(0x0290 - 0xE9, 0),
	SFX_P(0x0240 - 0xE9, 0),
	SFX_P(0x01F0 - 0xE9, 0),
	SFX_P(0x01A0 - 0xE9, 0),
	SFX_P(0x0160 - 0xE9, 0),

	SFX_P(0x02C0 - 0x129, 0),
	SFX_P(0x0290 - 0x129, 0),
	SFX_P(0x0240 - 0x129, 0),
	SFX_P(0x01F0 - 0x129, 0),
	SFX_P(0x01A0 - 0x129, 0),
	SFX_P(0x0160 - 0x129, 0),

	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),

	SFX_P(0x02C0 - 0x129, 6),
	SFX_P(0x0290 - 0x129, 6),
	SFX_P(0x0240 - 0x129, 6),
	SFX_P(0x01F0 - 0x129, 6),
	SFX_P(0x01A0 - 0x129, 6),
	SFX_P(0x0160 - 0x129, 6),

	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),
	SFX_P(0, 0xF),

	SFX_P(0x02C0 - 0x129, 9),
	SFX_P(0x0290 - 0x129, 9),
	SFX_P(0x0240 - 0x129, 9),
	SFX_P(0x01F0 - 0x129, 9),
	SFX_P(0x01A0 - 0x129, 9),
	SFX_P(0x0160 - 0x129, 9),
	SFX_END()
};

static const SfxSample sfx_cube_toss[] =
{
	SFX_P(0x0003 + 0x290, 0),
	SFX_P(0x0003 + 0x240, 0),
	SFX_P(0x0003 + 0x1E0, 0),
	SFX_P(0x0003 + 0x1B4, 0),
	SFX_P(0x0003 + 0x174, 0),
	SFX_P(0x0003 + 0x154, 0),
	SFX_P(0x0003 + 0x124, 0),
	SFX_P(0x0003 + 0x94, 0),
	SFX_P(0x0003 + 0x64, 0),
	SFX_P(0x0003 + 0x53, 0),
	SFX_P(0x0003 + 0x47, 0),
	SFX_P(0x0003 + 0x45, 0),
	SFX_P(0x0003 + 0x3E, 0),
	SFX_P(0x0003 + 0x39, 0),
	SFX_P(0x0003 + 0x34, 0),

	SFX_P(0x0003 + 0x154, 6),
	SFX_P(0x0003 + 0x124, 6),
	SFX_P(0x0003 + 0x94, 6),
	SFX_P(0x0003 + 0x64, 6),
	SFX_P(0x0003 + 0x53, 6),
	SFX_P(0x0003 + 0x47, 6),
	SFX_P(0x0003 + 0x45, 6),
	SFX_P(0x0003 + 0x3E, 6),
	SFX_P(0x0003 + 0x39, 6),
	SFX_P(0x0003 + 0x34, 6),
	SFX_END()
};

static const SfxSample sfx_cube_bounce[] =
{
	SFX_P(0x0200, 0),
	SFX_P(0x0300, 0),
	SFX_P(0x03E0, 0),
	SFX_P(0x0100, 0),
	SFX_P(0x0140, 0),
	SFX_P(0x01E0, 0),
	SFX_END()
};

static const SfxSample sfx_cube_hit[] =
{
	SFX_P(0x0040, 0),
	SFX_P(0x0200, 0),
	SFX_P(0x0020, 0),
	SFX_P(0x0074, 0),
	SFX_P(0x0090, 0),
	SFX_P(0x00A0, 0),
	SFX_P(0x00B0, 0),
	SFX_P(0x00C0, 0),
	SFX_P(0x00D0, 0),
	SFX_P(0x00E0, 0),
	SFX_P(0x00F0, 0),
	SFX_P(0x0108, 0),
	SFX_P(0x0120, 0),
	SFX_P(0x0138, 0),
	SFX_P(0x0150, 0),

	SFX_P(0x0090, 2),
	SFX_P(0x00A0, 2),
	SFX_P(0x00B0, 2),
	SFX_P(0x00C0, 2),
	SFX_P(0x00D0, 2),
	SFX_P(0x00E0, 2),
	SFX_P(0x00F0, 2),
	SFX_P(0x0108, 2),
	SFX_P(0x0120, 2),
	SFX_P(0x0138, 2),
	SFX_P(0x0150, 2),

	SFX_P(0x0090, 4),
	SFX_P(0x00A0, 4),
	SFX_P(0x00B0, 4),
	SFX_P(0x00C0, 4),
	SFX_P(0x00D0, 4),
	SFX_P(0x00E0, 4),
	SFX_P(0x00F0, 4),
	SFX_P(0x0108, 4),
	SFX_P(0x0120, 4),
	SFX_P(0x0138, 4),
	SFX_P(0x0150, 4),
	SFX_END()
};

static const SfxSample sfx_cube_fizzle[] =
{
	SFX_P(0x0188, 0),
	SFX_P(0x0160, 0),
	SFX_P(0x0150, 0),
	SFX_P(0x0140, 0),
	SFX_P(0x0130, 0),
	SFX_P(0x0120, 0),

	SFX_P(0x01B7, 0),
	SFX_P(0x0188, 1),
	SFX_P(0x0170, 0),
	SFX_P(0x0160, 1),
	SFX_P(0x0150, 1),
	SFX_P(0x0140, 1),

	SFX_P(0x01E5, 2),
	SFX_P(0x01B7, 1),
	SFX_P(0x01A3, 2),
	SFX_P(0x0188, 1),
	SFX_P(0x0170, 2),
	SFX_P(0x0160, 2),

	SFX_P(0x01E5, 4),
	SFX_P(0x01B7, 5),
	SFX_P(0x01A3, 4),
	SFX_P(0x0188, 5),
	SFX_P(0x0170, 4),
	SFX_P(0x0160, 5),

	SFX_P(0x01E5, 6),
	SFX_P(0x01B7, 7),
	SFX_P(0x01A3, 6),
	SFX_P(0x0188, 7),
	SFX_P(0x0170, 6),
	SFX_P(0x0160, 7),
	SFX_END()
};

static const SfxSample sfx_obj_burst[] =
{
	SFX_P(0X0240, 0),
	SFX_P(0X0270, 0),
	SFX_P(0X02A0, 0),
	SFX_P(0x02F0, 0),
	SFX_P(0x0330, 0),
	SFX_P(0x0380, 0),
	SFX_P(0x03B0, 0),
	SFX_P(0x03FF, 0),

	SFX_P(0x00F0 + 0x00, 0),
	SFX_P(0x00F0 + 0x08, 0),
	SFX_P(0x00F0 + 0x10, 0),
	SFX_P(0x00F0 + 0x18, 0),
	SFX_P(0x00F0 + 0x20, 0),
	SFX_P(0x00F0 + 0x28, 0),
	SFX_P(0x00F0 + 0x30, 0),
	SFX_P(0x00F0 + 0x40, 0),
	SFX_P(0x00F0 + 0x50, 0),
	SFX_P(0x00F0 + 0x50, 0),
	SFX_P(0x00F0 + 0x60, 0),
	SFX_P(0x00F0 + 0x70, 0),
	SFX_P(0x00F0 + 0x80, 0),
	SFX_P(0x00F0 + 0x90, 0),
	SFX_P(0x00F0 + 0xA0, 0),
	SFX_P(0x00F0 + 0xB0, 0),
	SFX_P(0x00F0 + 0xC0, 0),

	SFX_P(0x00F0 + 0x00, 6),
	SFX_P(0x00F0 + 0x08, 6),
	SFX_P(0x00F0 + 0x10, 6),
	SFX_P(0x00F0 + 0x18, 6),
	SFX_P(0x00F0 + 0x20, 6),
	SFX_P(0x00F0 + 0x28, 6),
	SFX_P(0x00F0 + 0x30, 6),
	SFX_P(0x00F0 + 0x40, 6),
	SFX_P(0x00F0 + 0x50, 6),
	SFX_P(0x00F0 + 0x50, 6),
	SFX_P(0x00F0 + 0x60, 6),
	SFX_P(0x00F0 + 0x70, 6),
	SFX_P(0x00F0 + 0x80, 6),
	SFX_P(0x00F0 + 0x90, 6),
	SFX_P(0x00F0 + 0xA0, 6),
	SFX_P(0x00F0 + 0xB0, 6),
	SFX_P(0x00F0 + 0xC0, 6),
	SFX_END()
};

static const SfxSample sfx_obj_burst_hi[] =
{
	SFX_P(0.5*0X0240, 2),
	SFX_P(0.5*0X0270, 2),
	SFX_P(0.5*0X02A0, 2),
	SFX_P(0.5*0x02F0, 2),
	SFX_P(0.5*0x0330, 2),
	SFX_P(0.5*0x0380, 2),
	SFX_P(0.5*0x03B0, 2),
	SFX_P(0.5*0x03FF, 2),

	SFX_P(0.5*(0x00F0 + 0x02), 2),
	SFX_P(0.5*(0x00F0 + 0x08), 2),
	SFX_P(0.5*(0x00F0 + 0x12), 2),
	SFX_P(0.5*(0x00F0 + 0x18), 2),
	SFX_P(0.5*(0x00F0 + 0x22), 2),
	SFX_P(0.5*(0x00F0 + 0x28), 2),
	SFX_P(0.5*(0x00F0 + 0x32), 2),
	SFX_P(0.5*(0x00F0 + 0x42), 2),
	SFX_P(0.5*(0x00F0 + 0x52), 2),
	SFX_P(0.5*(0x00F0 + 0x52), 2),
	SFX_P(0.5*(0x00F0 + 0x62), 2),
	SFX_P(0.5*(0x00F0 + 0x72), 2),
	SFX_P(0.5*(0x00F0 + 0x82), 2),
	SFX_P(0.5*(0x00F0 + 0x92), 2),
	SFX_P(0.5*(0x00F0 + 0xA2), 2),
	SFX_P(0.5*(0x00F0 + 0xB2), 2),
	SFX_P(0.5*(0x00F0 + 0xC2), 2),

	SFX_P(0.5*(0x00F0 + 0x02), 8),
	SFX_P(0.5*(0x00F0 + 0x08), 8),
	SFX_P(0.5*(0x00F0 + 0x12), 8),
	SFX_P(0.5*(0x00F0 + 0x18), 8),
	SFX_P(0.5*(0x00F0 + 0x22), 8),
	SFX_P(0.5*(0x00F0 + 0x28), 8),
	SFX_P(0.5*(0x00F0 + 0x32), 8),
	SFX_P(0.5*(0x00F0 + 0x42), 8),
	SFX_P(0.5*(0x00F0 + 0x52), 8),
	SFX_P(0.5*(0x00F0 + 0x52), 8),
	SFX_P(0.5*(0x00F0 + 0x62), 8),
	SFX_P(0.5*(0x00F0 + 0x72), 8),
	SFX_P(0.5*(0x00F0 + 0x82), 8),
	SFX_P(0.5*(0x00F0 + 0x92), 8),
	SFX_P(0.5*(0x00F0 + 0xA2), 8),
	SFX_P(0.5*(0x00F0 + 0xB2), 8),
	SFX_P(0.5*(0x00F0 + 0xC2), 8),
	SFX_END()
};

#define SFX_TELE_SWEEP(base, vol) \
	SFX_P(base + 0x10, vol),\
	SFX_P(base - 0x00, vol),\
	SFX_P(base - 0x10, vol),\
	SFX_P(base - 0x20, vol),\
	SFX_P(base - 0x30, vol),\
	SFX_P(base - 0x40, vol),\
	SFX_P(base - 0x54, vol),\
	SFX_P(base - 0x66, vol),\
	SFX_P(base - 0x7B, vol),\
	SFX_P(base - 0x81, vol),\
	SFX_P(base - 0x8C, vol),\
	SFX_P(base - 0x9B, vol),\
	SFX_P(base - 0xA3, vol),\
	SFX_P(base - 0xAD, vol),\
	SFX_P(base - 0xB7, vol)\

static const SfxSample sfx_teleport[] =
{
	SFX_TELE_SWEEP(0x0160, 5),
	SFX_TELE_SWEEP(0x0160, 3),
	SFX_TELE_SWEEP(0x0160, 1),
	SFX_TELE_SWEEP(0x0160, 0),
	SFX_TELE_SWEEP(0x0160, 1),
	SFX_TELE_SWEEP(0x0160, 3),
	SFX_TELE_SWEEP(0x0160, 5),
	SFX_END()
};

static const SfxSample sfx_teleport_2[] =
{
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(0, 15),

	SFX_TELE_SWEEP(0x0160, 5),
	SFX_TELE_SWEEP(0x0160, 3),
	SFX_TELE_SWEEP(0x0160, 1),
	SFX_TELE_SWEEP(0x0160, 0),
	SFX_TELE_SWEEP(0x0160, 1),
	SFX_TELE_SWEEP(0x0160, 3),
	SFX_TELE_SWEEP(0x0160, 5),

	SFX_END()
};

#undef SFX_TELE_SWEEP

static const SfxSample sfx_boingo_jump[] =
{
	SFX_P(0x380, 0),
	SFX_P(0x380, 0),
	SFX_P(0x1C0, 0),
	SFX_P(0x220, 1),
	SFX_P(0x110 - 0x10, 2),
	SFX_P(0x0C0 - 0x10, 3),
	SFX_P(0x110 - 0x1D, 4),
	SFX_P(0x0C0 - 0x1D, 5),
	SFX_P(0x110 - 0x24, 6),
	SFX_P(0x0C0 - 0x24, 7),
	SFX_END()
};

static const SfxSample sfx_powerup_get[] =
{
	SFX_P(-300 + 529, 0),
	SFX_P(-300 + 589, 0),
	SFX_P(-300 + 630, 0),
	SFX_P(-200 + 700, 0),
	SFX_P(-200 + 810, 0),
	SFX_P(-100 + 940, 0),
	SFX_P(-100 + 1023, 0),
	SFX_P(-300 + 600, 0),
	SFX_P(-310 + 580, 0),
	SFX_P(-320 + 560, 0),
	SFX_P(-320 + 540, 0),
	SFX_P(-330 + 520, 0),
	SFX_P(-330 + 500, 0),
	SFX_P(-340 + 480, 0),
	SFX_P(-340 + 460, 0),
	SFX_P(-350 + 440, 0),
	SFX_P(-350 + 420, 0),
	SFX_P(-360 + 400, 0),

	SFX_P(-330 + 520, 3),
	SFX_P(-330 + 500, 3),
	SFX_P(-340 + 480, 3),
	SFX_P(-340 + 460, 3),
	SFX_P(-350 + 440, 3),
	SFX_P(-350 + 420, 3),
	SFX_P(-360 + 400, 3),

	SFX_P(-330 + 520, 6),
	SFX_P(-330 + 500, 6),
	SFX_P(-340 + 480, 6),
	SFX_P(-340 + 460, 6),
	SFX_P(-350 + 440, 6),
	SFX_P(-350 + 420, 6),
	SFX_P(-360 + 400, 6),
	SFX_END()
};

static const SfxSample sfx_magibear_shot[] =
{
	SFX_P(0x0242, 0),
	SFX_P(0x0273, 0),
	SFX_P(0x02F5, 0),
	SFX_P(0x0336, 0),
	SFX_P(0x03B8, 0),
	SFX_P(0x03F9, 0),
	SFX_P(0x70, 0),
	SFX_P(0xA0, 0),
	SFX_P(0x72, 0),
	SFX_P(0xA3, 0),
	SFX_P(0x74, 0),
	SFX_P(0xA6, 0),
	SFX_P(0x40, 0),
	SFX_P(0x60, 0),
	SFX_P(0x42, 1),
	SFX_P(0x5D, 2),
	SFX_P(0x44, 3),
	SFX_P(0x5A, 4),
	SFX_P(0x46, 5),
	SFX_P(0x57, 6),
	SFX_P(0x48, 7),
	SFX_P(0x54, 8),
	SFX_END()
};

static const SfxSample sfx_gaxter_shot[] =
{
	SFX_P(0x3FE, 0),
	SFX_P(0x02, 0),
	SFX_P(0x00F0 + 0x00, 0),
	SFX_P(0x00F0 + 0x10, 0),
	SFX_P(0x00F0 + 0x20, 0),
	SFX_P(0x00F0 + 0x30, 0),
	SFX_P(0x00F0 + 0x50, 8),
	SFX_P(0x00F0 + 0x60, 10),
	SFX_P(0x00F0 + 0x80, 12),
	SFX_END()
};

static const SfxSample sfx_gaxter_shot_2[] =
{
	SFX_P(0x3FD, 0),
	SFX_P(0x01, 0),
	SFX_P(0x0140 + 0x00, 0),
	SFX_P(0x0140 + 0x10, 0),
	SFX_P(0x0140 + 0x20, 0),
	SFX_P(0x0140 + 0x30, 0),
	SFX_P(0x0140 + 0x50, 8),
	SFX_P(0x0140 + 0x60, 10),
	SFX_P(0x0140 + 0x80, 12),
	SFX_END()
};

static const SfxSample sfx_explode[] =
{
	SFX_P(0x020, 0),
	SFX_P(0X0240, 0),
	SFX_P(0X0270, 0),
	SFX_P(0X02A0, 0),
	SFX_P(0x02F0, 0),
	SFX_P(0x0330, 0),
	SFX_P(0x0380, 0),

	SFX_P(0x010, 0),
	SFX_P(0x00d0 + 0x00, 0),
	SFX_P(0x0080 + 0x08, 0),
	SFX_P(0x00d0 + 0x10, 0),
	SFX_P(0x0080 + 0x18, 0),
	SFX_P(0x00d0 + 0x20, 0),
	SFX_P(0x0080 + 0x28, 0),
	SFX_P(0x00d0 + 0x30, 0),
	SFX_P(0x0080 + 0x40, 0),
	SFX_P(0x00d0 + 0x50, 0),
	SFX_P(0x0080 + 0x50, 0),
	SFX_P(0x00d0 + 0x60, 0),
	SFX_P(0x0080 + 0x70, 0),
	SFX_P(0x00d0 + 0x80, 0),
	SFX_P(0x0080 + 0x90, 1),
	SFX_P(0x00d0 + 0xA0, 2),
	SFX_P(0x0080 + 0xB0, 4),
	SFX_P(0x00d0 + 0xC0, 6),
	SFX_P(0x0080 + 0xD0, 9),
	SFX_P(0x00d0 + 0xE0, 0xB),
	SFX_P(0x0080 + 0xF0, 0xE),
	SFX_END()
};

static const SfxSample sfx_elevator[] =
{
	SFX_P(0x3FF, 0),
	SFX_P(0x3FF, 1),
	SFX_P(0x3FF, 2),
	SFX_P(0x3FF, 3),
	SFX_P(0x3FF, 2),
	SFX_P(0x3FF, 1),
	SFX_LOOP(),
};

static const SfxSample sfx_elevator_2[] =
{
	SFX_P(0x3E3, 0),
	SFX_P(0x3E3, 1),
	SFX_P(0x3E3, 2),
	SFX_P(0x3E3, 3),
	SFX_P(0x3E3, 4),
	SFX_P(0x3E3, 3),
	SFX_P(0x3E3, 2),
	SFX_P(0x3E3, 1),
	SFX_LOOP(),
};

#define PAUSENOTE(f, p) \
SFX_P(f, 1 + p), \
SFX_P(f, 0 + p), \
SFX_P(f, 1 + p), \
SFX_P(f, 2 + p), \
SFX_P(f, 4 + p), \
SFX_P(f, 6 + p)

static const SfxSample sfx_pause_1[] =
{
	PAUSENOTE(PSG_BASE_B / 2, 0),
	PAUSENOTE(PSG_BASE_Eb / 4, 0),
	PAUSENOTE(PSG_BASE_Gb / 4, 0),
	PAUSENOTE(PSG_BASE_B / 4, 0),
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(PSG_BASE_B / 4, 5), \
	SFX_P(PSG_BASE_B / 4, 3), \
	PAUSENOTE(PSG_BASE_B / 4, 1),
	SFX_END()
};

#undef PAUSENOTE

#define PAUSENOTE(f) \
SFX_P(f, 0), \
SFX_P(f, 1), \
SFX_P(f, 2), \
SFX_P(f, 3), \
SFX_P(f, 4), \
SFX_P(f, 15)

static const SfxSample sfx_pause_2[] =
{
	PAUSENOTE(PSG_BASE_Eb),
	PAUSENOTE(PSG_BASE_Gb),
	PAUSENOTE(PSG_BASE_B),
	PAUSENOTE(PSG_BASE_Eb / 2),
	SFX_END()
};

#undef PAUSENOTE

static const SfxSample sfx_moo_1[] =
{
	SFX_P(900, 5),
	SFX_P(950, 4),
	SFX_P(1000, 3),
	SFX_P(1023, 2),
	SFX_P(925, 2),
	SFX_P(842, 2),
	SFX_P(768, 2),
	SFX_P(722, 2),
	SFX_P(700, 2),
	SFX_P(689, 2),
	SFX_P(682, 2),
	SFX_P(678, 2),
	SFX_P(678, 2),
	SFX_P(677, 2),
	SFX_P(677, 2),
	SFX_P(676, 1),
	SFX_P(676, 1),
	SFX_P(676, 1),
	SFX_P(634, 1),
	SFX_P(588, 1),
	SFX_P(566, 1),
	SFX_P(555, 1),
	SFX_P(540, 1),
	SFX_P(520, 0),
	SFX_P(515, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(509, 0),
	SFX_P(515, 0),
	SFX_P(520, 0),
	SFX_P(540, 1),
	SFX_P(555, 1),
	SFX_P(566, 1),
	SFX_P(588, 1),
	SFX_P(634, 1),
	SFX_P(677, 2),
	SFX_P(678, 2),
	SFX_P(679, 2),
	SFX_P(712, 2),
	SFX_P(758, 2),
	SFX_P(818, 2),
	SFX_P(923, 2),
	SFX_P(1022, 2),
	SFX_END()
};

static const SfxSample sfx_moo_2[] =
{
#define MOO_DETUNE -30
	SFX_P(MOO_DETUNE + 900, 5),
	SFX_P(MOO_DETUNE + 950, 4),
	SFX_P(MOO_DETUNE + 1000, 3),
	SFX_P(MOO_DETUNE + 1023, 2),
	SFX_P(MOO_DETUNE + 925, 2),
	SFX_P(MOO_DETUNE + 842, 2),
	SFX_P(MOO_DETUNE + 768, 2),
	SFX_P(MOO_DETUNE + 722, 2),
	SFX_P(MOO_DETUNE + 700, 2),
	SFX_P(MOO_DETUNE + 689, 2),
	SFX_P(MOO_DETUNE + 682, 2),
	SFX_P(MOO_DETUNE + 678, 2),
	SFX_P(MOO_DETUNE + 678, 2),
	SFX_P(MOO_DETUNE + 677, 2),
	SFX_P(MOO_DETUNE + 677, 2),
	SFX_P(MOO_DETUNE + 676, 1),
	SFX_P(MOO_DETUNE + 676, 1),
	SFX_P(MOO_DETUNE + 676, 1),
	SFX_P(MOO_DETUNE + 634, 1),
	SFX_P(MOO_DETUNE + 588, 1),
	SFX_P(MOO_DETUNE + 566, 1),
	SFX_P(MOO_DETUNE + 555, 1),
	SFX_P(MOO_DETUNE + 540, 1),
	SFX_P(MOO_DETUNE + 520, 0),
	SFX_P(MOO_DETUNE + 515, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 509, 0),
	SFX_P(MOO_DETUNE + 515, 0),
	SFX_P(MOO_DETUNE + 520, 0),
	SFX_P(MOO_DETUNE + 540, 1),
	SFX_P(MOO_DETUNE + 555, 1),
	SFX_P(MOO_DETUNE + 566, 1),
	SFX_P(MOO_DETUNE + 588, 1),
	SFX_P(MOO_DETUNE + 634, 1),
	SFX_P(MOO_DETUNE + 677, 2),
	SFX_P(MOO_DETUNE + 678, 2),
	SFX_P(MOO_DETUNE + 679, 2),
	SFX_P(MOO_DETUNE + 712, 2),
	SFX_P(MOO_DETUNE + 758, 2),
	SFX_P(MOO_DETUNE + 818, 2),
	SFX_P(MOO_DETUNE + 923, 2),
	SFX_P(MOO_DETUNE + 1022, 2),
#undef MOO_DETUNE
	SFX_END()
};

#define GIVER_GEN(base, mag) \
	SFX_P(base, 0), \
	SFX_P(base - (mag * 1), 7), \
	SFX_P(base - (mag * 2), 0), \
	SFX_P(base - (mag * 3), 6), \
	SFX_P(base - (mag * 4), 0), \
	SFX_P(base - (mag * 5), 6), \
	SFX_P(base - (mag * 6), 0), \
	SFX_P(base - (mag * 7), 6), \
	SFX_P(base - (mag * 8), 0), \
	SFX_P(base - (mag * 9), 5), \
	SFX_P(base - (mag * 10), 0), \
	SFX_P(base - (mag * 11), 5), \
	SFX_P(base - (mag * 12), 0), \
	SFX_P(base - (mag * 13), 4), \
	SFX_P(base - (mag * 14), 0), \
	SFX_P(base - (mag * 15), 4), \
	SFX_P(base - (mag * 16), 0), \
	SFX_P(base - (mag * 17), 3), \
	SFX_P(base - (mag * 18), 0), \
	SFX_P(base - (mag * 19), 3), \
	SFX_P(base - (mag * 20), 0), \
	SFX_P(base - (mag * 21), 2), \
	SFX_P(base - (mag * 22), 0), \
	SFX_P(base - (mag * 23), 2), \
	SFX_P(base - (mag * 24), 0), \
	SFX_P(base - (mag * 25), 1), \
	SFX_P(base - (mag * 26), 0), \
	SFX_P(base - (mag * 27), 1), \
	SFX_P(base - (mag * 28), 0), \
	SFX_P(base - (mag * 29), 0), \
	SFX_P(base - (mag * 30), 0), \
	SFX_P(base - (mag * 31), 0), \
	SFX_P(base - (mag * 32), 0), \
	SFX_P(base - (mag * 33), 0), \
	SFX_P(base - (mag * 34), 0), \
	SFX_P(base - (mag * 21), 9), \
	SFX_P(base - (mag * 22), 7), \
	SFX_P(base - (mag * 23), 9), \
	SFX_P(base - (mag * 24), 7), \
	SFX_P(base - (mag * 25), 8), \
	SFX_P(base - (mag * 26), 7), \
	SFX_P(base - (mag * 27), 8), \
	SFX_P(base - (mag * 28), 7), \
	SFX_P(base - (mag * 29), 7), \
	SFX_P(base - (mag * 30), 7), \
	SFX_P(base - (mag * 31), 7), \
	SFX_P(base - (mag * 32), 7), \
	SFX_P(base - (mag * 33), 7), \
	SFX_P(base - (mag * 34), 7),

#define SWEEPMAG 20
static const SfxSample sfx_giver_1[] =
{
	GIVER_GEN(1023, 20)
	SFX_END()
};

static const SfxSample sfx_giver_2[] =
{
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(0, 15),
	SFX_P(0, 15),
	GIVER_GEN(780, 18)
	SFX_END()
};

static const SfxSample sfx_giver_3[] =
{
	SFX_P(0, 15),
	SFX_P(0, 15),
	GIVER_GEN(500, 10)
	SFX_END()
};

#undef SWEEPMAG

#define SELECTNOTE(f, e) \
SFX_P(f, e+0), \
SFX_P(f, e+1)

static const SfxSample sfx_select_1[] =
{
	SELECTNOTE(PSG_BASE_C, 0),
	SELECTNOTE(PSG_BASE_C, 2),
	SELECTNOTE(PSG_BASE_E, 0),
	SELECTNOTE(PSG_BASE_E, 2),
	SELECTNOTE(PSG_BASE_G, 0),
	SELECTNOTE(PSG_BASE_G, 2),
	SELECTNOTE(PSG_BASE_B, 0),
	SELECTNOTE(PSG_BASE_B, 2),
	SELECTNOTE(PSG_BASE_D / 2, 0),
	SELECTNOTE(PSG_BASE_D / 2, 2),
	SELECTNOTE(PSG_BASE_G, 3),
	SELECTNOTE(PSG_BASE_B, 5),
	SELECTNOTE(PSG_BASE_B, 3),
	SELECTNOTE(PSG_BASE_D / 2, 5),
	SELECTNOTE(PSG_BASE_D / 2, 3),
	SELECTNOTE(PSG_BASE_G, 4),
	SELECTNOTE(PSG_BASE_B, 6),
	SELECTNOTE(PSG_BASE_B, 4),
	SELECTNOTE(PSG_BASE_D / 2, 6),
	SELECTNOTE(PSG_BASE_D / 2, 4),
	SFX_END()
};
#undef SELECTNOTE

#define SELECTNOTE(f, e) \
SFX_P(f / 2, e+0), \
SFX_P(f / 2, e+1)
static const SfxSample sfx_select_2[] =
{
	SELECTNOTE(PSG_BASE_C, 0),
	SELECTNOTE(PSG_BASE_C, 2),
	SELECTNOTE(PSG_BASE_E, 0),
	SELECTNOTE(PSG_BASE_E, 2),
	SELECTNOTE(PSG_BASE_G, 0),
	SELECTNOTE(PSG_BASE_G, 2),
	SELECTNOTE(PSG_BASE_B, 0),
	SELECTNOTE(PSG_BASE_B, 2),
	SELECTNOTE(PSG_BASE_D / 2, 0),
	SELECTNOTE(PSG_BASE_D / 2, 2),
	SELECTNOTE(PSG_BASE_G, 3),
	SELECTNOTE(PSG_BASE_B, 5),
	SELECTNOTE(PSG_BASE_B, 3),
	SELECTNOTE(PSG_BASE_D / 2, 5),
	SELECTNOTE(PSG_BASE_D / 2, 3),
	SELECTNOTE(PSG_BASE_G, 4),
	SELECTNOTE(PSG_BASE_B, 6),
	SELECTNOTE(PSG_BASE_B, 4),
	SELECTNOTE(PSG_BASE_D / 2, 6),
	SELECTNOTE(PSG_BASE_D / 2, 4),
	SFX_END()
};

#undef SELECTNOTE

static const SfxSample sfx_beep[] =
{
	SFX_P(PSG_BASE_D / 2, 0),
	SFX_P(PSG_BASE_D / 2, 0),
	SFX_P(PSG_BASE_D / 2, 0),
	SFX_P(PSG_BASE_D / 4, 0),
	SFX_P(PSG_BASE_D / 4, 0),
	SFX_P(PSG_BASE_D / 4, 0),
	SFX_P(PSG_BASE_D / 2, 0),
	SFX_P(PSG_BASE_D / 2, 0),
	SFX_P(PSG_BASE_D / 2, 0),
	SFX_P(PSG_BASE_D / 4, 0),
	SFX_P(PSG_BASE_D / 4, 0),
	SFX_P(PSG_BASE_D / 4, 0),
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
	[SFX_OBJ_BURST_HI] = sfx_obj_burst_hi,
	[SFX_TELEPORT] = sfx_teleport,
	[SFX_TELEPORT_2] = sfx_teleport_2,
	[SFX_BOINGO_JUMP] = sfx_boingo_jump,
	[SFX_POWERUP_GET] = sfx_powerup_get,
	[SFX_MAGIBEAR_SHOT] = sfx_magibear_shot,
	[SFX_GAXTER_SHOT] = sfx_gaxter_shot,
	[SFX_GAXTER_SHOT_2] = sfx_gaxter_shot_2,
	[SFX_EXPLODE] = sfx_explode,
	[SFX_ELEVATOR] = sfx_elevator,
	[SFX_ELEVATOR_2] = sfx_elevator_2,
	[SFX_PAUSE_1] = sfx_pause_1,
	[SFX_PAUSE_2] = sfx_pause_2,
	[SFX_MOO_1] = sfx_moo_1,
	[SFX_MOO_2] = sfx_moo_2,
	[SFX_GIVER_1] = sfx_giver_1,
	[SFX_GIVER_2] = sfx_giver_2,
	[SFX_GIVER_3] = sfx_giver_3,
	[SFX_BEEP] = sfx_beep,
	[SFX_SELECT_1] = sfx_select_1,
	[SFX_SELECT_2] = sfx_select_2,
};

// Sound routines.
static void sfx_tick(void)
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
		else if (sample->pitch == 0xFFFF)
		{
			s->stream = s->stream_base;
			sample = s->stream;
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

int sfx_init(void)
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

	md_irq_register(MD_IRQ_HBLANK, sfx_tick);

	return 1;
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
	s->stream_base = stream;
	s->channel = channel;
	s->priority = priority;
	sys_ei();
}

void sfx_play(SfxId id, int8_t priority)
{
	sys_di();
	const SfxSample *stream = stream_by_id[id];
	if (stream == NULL) return;
	for (uint8_t i = 0; i < ARRAYSIZE(channel_state); i++)
	{
		SfxChannelState *s = &channel_state[i];
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
	for (uint8_t i = 0; i < ARRAYSIZE(channel_state); i++)
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
	s->stream_base = stream;
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
#undef SFX_P
#undef SFX_END
