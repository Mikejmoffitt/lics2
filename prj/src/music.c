#include "music.h"
#include "echo/echo.h"
#include <stdint.h>
#include <stdlib.h>
#include "md/megadrive.h"
#include "mps/mps_play.h"

#include "res.h"

static uint8_t current_track;
static uint8_t pending_track;
static uint8_t frames_to_delay_pending_track;
#define TRACK_DELAY 5

static const void *instrument_list[] =
{
	[0x00] = res_mus_eif_bgm1_bass1_eif,
	[0x01] = res_mus_eif_bgm1_chords_eif,
	[0x02] = res_mus_eif_sawlead_eif,
	[0x03] = res_mus_eef_flat_eef,
	[0x04] = res_mus_ewf_bgm2_kick1_ewf,
	[0x05] = res_mus_ewf_bgm1_snare1_ewf,
	[0x06] = res_mus_eef_hat_eef,
	[0x07] = res_mus_eif_bgm1_sq_arps_eif,
	[0x08] = res_mus_ewf_bgm2_hat1_ewf,
	[0x09] = res_mus_ewf_bgm2_snare1_ewf,
	[0x0A] = res_mus_eif_bgm8_bass_eif,
	[0x0B] = res_mus_eif_bgm3_chords_eif,
	[0x0C] = res_mus_eif_saw2at_eif,
	[0x0D] = res_mus_eif_bgm2_buzz_eif,
	[0x0E] = res_mus_eif_bgm3_backing_eif,
	[0x0F] = res_mus_eif_bgm3_arps_eif,
	[0x10] = res_mus_eif_drone_eif,
	[0x11] = res_mus_eif_unsettling_eif,
	[0x12] = res_mus_ewf_bgm3_snare2_ewf,  // Quiet version of bgm2_snare1
	[0x13] = res_mus_eef_arp1_eef,
	[0x14] = res_mus_eef_arp2_eef,
	[0x15] = res_mus_eif_bgm8_chords_eif,
	[0x16] = res_mus_eif_modhorn_eif,
	[0x17] = res_mus_eif_sine_eif,
	[0x18] = res_mus_eif_sqpluck_eif,
	[0x19] = res_mus_eif_bgm6_bass_eif,
	[0x1A] = res_mus_ewf_bgm8_kick_ewf,
	[0x1B] = res_mus_ewf_bgm8_snare_ewf,
	[0x1C] = res_mus_ewf_bgm8_hat_ewf,
	[0x1D] = res_mus_eif_bgm3_bass_eif,
	[0x1E] = res_mus_eif_bgm4_lead1_eif,
	[0x1F] = res_mus_eif_bgm4_lead2_eif,
	[0x20] = res_mus_eif_bgm4_bass_eif,
	[0x21] = res_mus_eif_bgm4_sq_arps_att_eif,
	[0x22] = res_mus_eif_bgm2_bass_eif,
	[0x23] = res_mus_eif_bgm2_arps_eif,
	[0x24] = res_mus_eif_bgm2_lead1_eif,
	[0x25] = res_mus_eif_bgm5_lead1_eif,
	[0x26] = res_mus_eif_bgm5_lead2_eif,
	[0x27] = res_mus_eif_bgm5_bass_eif,
	[0x28] = res_mus_eif_bgm6_lead_eif,
	[0x29] = res_mus_ewf_bgm7_kick1_ewf,
	[0x2A] = res_mus_ewf_bgm7_kick2_ewf,
	[0x2B] = res_mus_ewf_bgm7_kick3_ewf,
	[0x2C] = res_mus_ewf_bgm7_kick4_ewf,
	[0x2D] = res_mus_ewf_bgm7_snare1_ewf,
	[0x2E] = res_mus_eif_bgm7_lead_eif,
	[0x2F] = res_mus_eif_bgm7_lead_att_eif,
	[0x30] = res_mus_eif_bgm7_bass_eif,
	[0x31] = res_mus_eif_bgm8_lead_eif,
	[0x32] = res_mus_eif_bgm8_bass_eif,
	[0x33] = res_mus_ewf_bgm9_kick1_ewf,
	[0x34] = res_mus_ewf_bgm9_snare1_ewf,
	[0x35] = res_mus_eif_bgm9_drone1_eif,
	[0x36] = res_mus_eif_bgm9_lead1_eif,
	[0x37] = res_mus_eif_bgm9_backing1_eif,
	[0x38] = res_mus_eif_bgm10_lead1_eif,
	[0x39] = res_mus_eif_bgm10_arps1_eif,
	[0x3A] = res_mus_eif_bgm10_bass_eif,
	[0x3B] = res_mus_eif_bgm10_arps2_eif,
	[0x3C] = res_mus_ewf_bgm10_kick_ewf,
	[0x3D] = res_mus_ewf_bgm10_snare_ewf,
	[0x3E] = res_mus_eif_bgm10_lead1_att_eif,
	[0x3F] = res_mus_ewf_bgm11_kick_ewf,
	[0x40] = res_mus_ewf_bgm11_hat1_ewf,
	[0x41] = res_mus_ewf_bgm11_hat2_ewf,
	[0x42] = res_mus_ewf_bgm11_snare_ewf,
	[0x43] = res_mus_ewf_bgm11_slide_ewf,
	[0x44] = res_mus_eif_bgm11_arps1_eif,
	[0x45] = res_mus_eif_bgm11_arps2_eif,
	[0x46] = res_mus_eif_bgm11_bass_eif,
	[0x47] = res_mus_eif_bgm11_lead1_eif,
	NULL
};

typedef struct BgmEntry
{
	const void *data;
	uint8_t tempo;
} BgmEntry;

static const BgmEntry bgm_list[] =
{
	{NULL, 0x00},
	{res_mus_esf_bgm1_esf, 0xBE},
	{res_mus_esf_bgm2_esf, 0xC8},
	{res_mus_esf_bgm3_esf, 0xC7},
	{res_mus_esf_bgm4_esf, 0xC7},
	{res_mus_esf_bgm5_esf, 0xBD},
	{res_mus_esf_bgm6_esf, 0xBE},
	{res_mus_esf_bgm7_esf, 0xBB},
	{res_mus_esf_bgm8_esf, 0xBD},
	{res_mus_esf_bgm9_esf, 0xBD},
	{res_mus_esf_bgm10_esf, 0xBD},
	{res_mus_esf_bgm11_esf, 0xBD},
	{NULL, 0xC9},
	{NULL, 0xC9},
	{res_mus_esf_bgm14_esf, 0xC9},
	{res_mus_esf_bgm15_esf, 0xC7},
};

int music_init(void)
{
	echo_init(instrument_list);
	return 1;
}

static const uint8_t psg_lock_esf[] =
{
	0xE8, 0xE9, 0xEA, 0xFF
};

void music_handle_pending(void)
{
	if (frames_to_delay_pending_track > 0)
	{
		frames_to_delay_pending_track--;
		return;
	}
	if (current_track == pending_track) return;
	
	current_track = pending_track;
	if (current_track == 0)
	{
		echo_stop_bgm();
		echo_play_sfx(psg_lock_esf);
		return;
	}

	echo_play_bgm(bgm_list[current_track].data);
	echo_play_sfx(psg_lock_esf);

	SYS_BARRIER();
	sys_z80_bus_req();
	opn_write(0, 0x26, bgm_list[current_track].tempo);
	sys_z80_bus_release();

}

void music_play(uint8_t track)
{
	if (track == current_track) return;
	frames_to_delay_pending_track = TRACK_DELAY;
	echo_stop_bgm();
	echo_stop_sfx();
	pending_track = track;
}

void music_stop(void)
{
	music_play(0);
}
