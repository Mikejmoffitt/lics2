#include "music.h"
#include "echo/echo.h"
#include <stdint.h>
#include <stdlib.h>
#include "md/megadrive.h"

#include "res.h"

#ifndef MDK_TARGET_C2

static uint8_t s_current_track;
static uint8_t s_pending_track;

static const void * const instrument_list[] =
{
	[0x00] = res_mus_eif_00_bass1_eif,
	[0x01] = res_mus_eif_01_chords1_eif,
	[0x02] = res_mus_eif_02_saw1_eif,
	[0x03] = res_mus_eef_flat_eef,
	[0x04] = res_mus_ewf_04_kick1_ewf,
	[0x05] = res_mus_ewf_05_snare1_ewf,
	[0x06] = res_mus_eef_hat_eef,
	[0x07] = res_mus_eif_07_sq_arps_eif,
	[0x08] = res_mus_ewf_08_hat1_ewf,
	[0x09] = res_mus_ewf_09_snare2_ewf,
	[0x0A] = res_mus_eif_0a_bass2_eif,
	[0x0B] = res_mus_eif_0b_synth1_eif,
	[0x0C] = res_mus_eif_0c_saw_att_eif,
	[0x0D] = res_mus_eif_0d_buzz1_eif,
	[0x0E] = res_mus_eif_0e_synth2_eif,
	[0x0F] = res_mus_eif_0f_synth3_eif,
	[0x10] = res_mus_eif_10_drone1_eif,
	[0x11] = res_mus_eif_11_drone2_eif,
	[0x12] = res_mus_ewf_12_snare2_att_ewf,
	[0x13] = res_mus_eef_arp1_eef,
	[0x14] = res_mus_eef_arp2_eef,
	[0x15] = res_mus_eif_15_saw_quick_decay_eif,
	[0x16] = res_mus_eif_16_synth4_eif,
	[0x17] = res_mus_eif_17_sine_decay_eif,
	[0x18] = res_mus_eif_18_square_pluck1_eif,
	[0x19] = res_mus_eif_19_bass3_eif,
	[0x1A] = res_mus_ewf_1a_kick2_ewf,
	[0x1B] = res_mus_ewf_1b_snare3_ewf,
	[0x1C] = res_mus_ewf_1c_hat2_ewf,
	[0x1D] = res_mus_eif_1d_bass4_eif,
	[0x1E] = res_mus_eif_1e_square_pluck2_eif,
	[0x1F] = res_mus_eif_1f_saw_pluck1_eif,
	[0x20] = res_mus_eif_20_bass5_eif,
	[0x21] = res_mus_eif_21_square_pluck3_eif,
	[0x22] = res_mus_eif_22_saw2_eif,
	[0x23] = res_mus_eif_23_synth5_eif,
	[0x24] = res_mus_eif_24_synth6_eif,
	[0x25] = res_mus_eif_25_synth7_eif,
	[0x26] = res_mus_eif_26_square_pluck4_eif,
	[0x27] = res_mus_eif_27_synth8_eif,
	[0x28] = res_mus_eif_28_square_pluck5_eif,
	[0x29] = res_mus_ewf_29_kick3_ewf,
	[0x2A] = res_mus_ewf_2a_kick4_ewf,
	[0x2B] = res_mus_ewf_2b_kick5_ewf,
	[0x2C] = res_mus_ewf_2c_kick6_ewf,
	[0x2D] = res_mus_ewf_2d_snare4_ewf,
	[0x2E] = res_mus_eif_2e_odd1_eif,
	[0x2F] = res_mus_eif_2f_odd1_att_eif,
	[0x30] = res_mus_eif_30_bass6_eif,
	[0x31] = res_mus_eif_31_square1_eif,
	[0x32] = res_mus_eif_0a_bass2_eif,  // TODO: Not great to do this
	[0x33] = res_mus_ewf_33_kick7_ewf,
	[0x34] = res_mus_ewf_34_snare5_ewf,
	[0x35] = res_mus_eif_35_slow_horn1_eif,
	[0x36] = res_mus_eif_36_synth9_eif,
	[0x37] = res_mus_eif_37_square2_eif,
	[0x38] = res_mus_eif_38_organ1_eif,
	[0x39] = res_mus_eif_39_saw_pluck2_eif,
	[0x3A] = res_mus_eif_3a_bass7_eif,
	[0x3B] = res_mus_eif_3b_saw3_eif,
	[0x3C] = res_mus_ewf_3c_kick8_ewf,
	[0x3D] = res_mus_ewf_3d_snare6_ewf,
	[0x3E] = res_mus_eif_3e_organ1_att_eif,
	[0x3F] = res_mus_ewf_3f_kick9_ewf,
	[0x40] = res_mus_ewf_40_hat3_ewf,
	[0x41] = res_mus_ewf_41_hat4_ewf,
	[0x42] = res_mus_ewf_42_snare7_ewf,
	[0x43] = res_mus_ewf_43_bgm11_fill_ewf,
	[0x44] = res_mus_eif_44_saw4_eif,
	[0x45] = res_mus_eif_45_saw4_att_eif,
	[0x46] = res_mus_eif_46_bass8_buzz_eif,
	[0x47] = res_mus_eif_47_organ2_eif,
	[0x48] = res_mus_eif_48_bass9_buzz_eif,
	[0x49] = res_mus_eif_49_saw5_eif,
	[0x4A] = res_mus_eif_4a_pulse25_eif,
	[0x4B] = res_mus_ewf_4b_kick10_ewf,
	[0x4C] = res_mus_ewf_4c_snare8_ewf,
	[0x4D] = res_mus_ewf_4d_hat5_ewf,
	[0x4E] = res_mus_eif_4e_synth10_buzz_eif,
	[0x4F] = res_mus_eif_4f_title_arps_eif,
	[0x50] = res_mus_eif_50_title_chord_eif,
	[0x51] = res_mus_eif_51_title_saw_att_eif,
	[0x52] = res_mus_eif_52_sine_decay_2_eif,
	[0x53] = res_mus_eif_53_synth5_with_decay_eif,
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
	{res_mus_esf_bgm2_esf, 0xCD},
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
	{res_mus_esf_bgm13_esf, 0xC9},
	{res_mus_esf_bgm14_esf, 0xC9},
	{res_mus_esf_bgm15_esf, 0xC7},
	{res_mus_esf_bgm16_esf, 0xC7},
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
	if (s_current_track == s_pending_track) return;
	
	s_current_track = s_pending_track;
	if (s_current_track == 0)
	{
		echo_stop_bgm();
		echo_play_sfx(psg_lock_esf);
		return;
	}

	// Use an ESF direct event to set the OPM's timer B for the tempo.
	uint8_t direct_cmd_buffer[4];
	direct_cmd_buffer[0] = 0xF8;  // FM register bank 0
	direct_cmd_buffer[1] = 0x26;  // Timer B register
	direct_cmd_buffer[2] = bgm_list[s_current_track].tempo;
	direct_cmd_buffer[3] = 0xFF;  // Terminator
	echo_play_direct(direct_cmd_buffer);

	echo_play_bgm(bgm_list[s_current_track].data);
	echo_play_sfx(psg_lock_esf);
}

void music_play(uint8_t track)
{
	if (track == s_current_track) return;
	echo_stop_bgm();
	echo_stop_sfx();
	s_pending_track = track;
}

void music_stop(void)
{
	music_play(0);
}

#else

// System C2 dummy functions, for now.
int music_init(void)
{
	return 1;
}
void music_handle_pending(void)
{

}

void music_play(uint8_t track)
{
	(void)track;
}

void music_stop(void)
{

}

#endif  // MDK_TARGET_C2
