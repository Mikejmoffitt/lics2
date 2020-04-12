#include "music.h"
#include "echo/echo.h"
#include <stdint.h>
#include <stdlib.h>

#include "res.h"

static uint8_t current_track;

static const void *instrument_list[] =
{
	res_mus_eif_bass1_eif,
	res_mus_eif_sqchords1_eif,
	res_mus_eif_sawlead_eif,
	res_mus_eef_flat_eef,
	res_mus_ewf_bgm1_bass1_ewf,
	res_mus_ewf_bgm1_snare1_ewf,
	res_mus_eef_hat_eef,
	res_mus_eef_qdecay_eef,
	res_mus_ewf_bgm1_hat1_ewf,
	res_mus_ewf_snare2_ewf,
	res_mus_eif_bass2_eif,
	res_mus_eif_saw2_eif,
	res_mus_eif_saw2at_eif,
	res_mus_eif_buzz_eif,
	res_mus_eif_buzztar_eif,
	res_mus_eif_buzztarc1_eif,
	res_mus_eif_drone_eif,
	res_mus_eif_unsettling_eif,
	res_mus_ewf_snare3_ewf,
	res_mus_eef_arp1_eef,
	res_mus_eef_arp2_eef,
	res_mus_eif_sawdecay_eif,
	res_mus_eif_modhorn_eif,
	res_mus_eif_sine_eif,
	res_mus_eif_sqpluck_eif,
	res_mus_eif_evilbass_eif,
	res_mus_ewf_bassdrum2_ewf,
	res_mus_ewf_snare4_ewf,
	res_mus_ewf_hat2_ewf,
	NULL
};

static const void *bgm_list[] =
{
	NULL,
	res_mus_esf_bgm1_esf,
	res_mus_esf_bgm2_esf,
	res_mus_esf_bgm3_esf,
	res_mus_esf_bgm4_esf,
	res_mus_esf_bgm5_esf,
	res_mus_esf_bgm6_esf,
	NULL,
	res_mus_esf_bgm8_esf,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	res_mus_esf_bgm14_esf,
	res_mus_esf_bgm15_esf,
};

int music_init(void)
{
	echo_init(instrument_list);
	return 1;
}

void music_play(uint8_t track)
{
	if (track == current_track) return;
	current_track = track;
	if (track == 0)
	{
		echo_stop_bgm();
		return;
	}
	echo_play_bgm(bgm_list[track]);

	// TODO: CD audio
}

void music_stop(void)
{
	music_play(0);
}
