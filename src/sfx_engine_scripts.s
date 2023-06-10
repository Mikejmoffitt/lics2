	
	.include	"sfx_engine_types.inc"
	.section	.text
	.global	sfx_engine_sound_list
	.global	sfx_engine_envelope_default

.sfx_engine_sound_list:
#
# Envelopes
#
sfx_engine_envelope_default:
env_default:
	dc.b	0
	dc.b	-1
env_att0:
	dc.b	0
	dc.b	-1
env_att1:
	dc.b	1
	dc.b	-1
env_att2:
	dc.b	2
	dc.b	-1
env_att3:
	dc.b	3
	dc.b	-1
env_att4:
	dc.b	4
	dc.b	-1
env_att5:
	dc.b	5
	dc.b	-1
env_att6:
	dc.b	6
	dc.b	-1
env_att7:
	dc.b	7
	dc.b	-1
env_att8:
	dc.b	8
	dc.b	-1
env_att9:
	dc.b	9
	dc.b	-1
env_att10:
	dc.b	10
	dc.b	-1
env_att11:
	dc.b	11
	dc.b	-1
env_att12:
	dc.b	12
	dc.b	-1
env_att13:
	dc.b	13
	dc.b	-1
env_att14:
	dc.b	14
	dc.b	-1
env_att15:
	dc.b	15
	dc.b	-1
env_decay:
	.rept	18
	dc.b	0
	.endr
	.rept	8
	dc.b	1
	.endr
	.rept	7
	dc.b	2
	.endr
	.rept	6
	dc.b	3
	.endr
	.rept	5
	dc.b	4
	.endr
	dc.b	-1
env_slowdecay:
	.rept	45
	dc.b	0
	.endr
	.rept	18
	dc.b	1
	.endr
	.rept	16
	dc.b	2
	.endr
	.rept	14
	dc.b	3
	.endr
	dc.b	-1
env_walk_decay:
	dc.b	1
	dc.b	1
	dc.b	1
	dc.b	1
	dc.b	1
	dc.b	1
	dc.b	2
	dc.b	3
	dc.b	4
	dc.b	5
	dc.b	6
	dc.b	7
	dc.b	8
	dc.b	9
	dc.b	10
	dc.b	11
	dc.b	12
	dc.b	13
	dc.b	14
	dc.b	15
	dc.b	-1
env_hurt:
	.rept	25
	dc.b	0
	.endr
	.rept	35
	dc.b	1
	.endr
	dc.b	2
	dc.b	-1
env_hurt_reduced:
	.rept	25
	dc.b	6
	.endr
	.rept	35
	dc.b	7
	.endr
	dc.b	8
	dc.b	-1
env_scratchy:
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	0
	dc.b	15
	dc.b	-1

env_pausenote:
	dc.b	1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 10, 15
	dc.b	-1
env_pausenote_quiet:
	dc.b	5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 7, 7, 8, 8, 9, 9, 10,10,11,11,12,12,13,14,15
	dc.b	-1

.align	2

#
# Sound effects
#
sfx_null:
	dc.w	SFXOP_END

sfx_jump:
	dc.w	SFXOP_ENV
	dc.l	env_decay
	dc.w	SFXOP_PERIOD, 0x27FF
	dc.w	SFXOP_SWEEP, -140
	dc.w	SFXOP_REST, 50
	dc.w	SFXOP_END

sfx_walk1:
	dc.w	SFXOP_PERIOD, 0x0870

sfx_walk_sub:
	dc.w	SFXOP_ENV
	dc.l	env_walk_decay
	dc.w	SFXOP_SWEEP, 90
	dc.w	SFXOP_REST, 80
	dc.w	SFXOP_END


sfx_walk2:
	dc.w	SFXOP_PERIOD, 0x0A00
	dc.w	SFXOP_JMP
	dc.l	sfx_walk_sub

sfx_hurt_sub:
	dc.w	SFXOP_SWEEP, 47
	dc.w	SFXOP_REST, 37
	dc.w	SFXOP_RET


sfx_hurt:
	dc.w	SFXOP_ENV
	dc.l	env_hurt
	dc.w	SFXOP_PERIOD, 0x0520
	dc.w	SFXOP_CALL
	dc.l	sfx_hurt_sub

	dc.w	SFXOP_ENV
	dc.l	env_hurt
	dc.w	SFXOP_PERIOD, 0x03B0
	dc.w	SFXOP_PERIOD_SAVE
	dc.w	SFXOP_CALL
	dc.l	sfx_hurt_sub

	dc.w	SFXOP_ENV
	dc.l	env_hurt_reduced
	dc.w	SFXOP_PERIOD_LOAD
	dc.w	SFXOP_CALL
	dc.l	sfx_hurt_sub

	dc.w	SFXOP_END


sfx_cube_lift:
	dc.w	SFXOP_ENV
	dc.l	env_att0
	dc.w	SFXOP_PERIOD, 0x2200
	dc.w	SFXOP_SWEEP, 464
	dc.w	SFXOP_REST, 16

	dc.w	SFXOP_ENV
	dc.l	env_att15
	dc.w	SFXOP_REST, 4

	dc.w	SFXOP_ENV
	dc.l	env_att0
	dc.w	SFXOP_PERIOD, 0x2800
	dc.w	SFXOP_PERIOD_SAVE
	dc.w	SFXOP_SWEEP, -298
	dc.w	SFXOP_REST, 24

	dc.w	SFXOP_ENV
	dc.l	env_att6
	dc.w	SFXOP_PERIOD_LOAD
	dc.w	SFXOP_SWEEP, -298
	dc.w	SFXOP_REST, 24

	dc.w	SFXOP_END


sfx_cube_spawn_sub:
	dc.w	SFXOP_PERIOD_SAVE
	dc.w	SFXOP_SWEEP, -280
	dc.w	SFXOP_REST, 19
	dc.w	SFXOP_PERIOD_LOAD
	dc.w	SFXOP_RET

sfx_cube_spawn:
	dc.w	SFXOP_ENV
	dc.l	env_att0
	dc.w	SFXOP_PERIOD, 0x3FFF
	dc.w	SFXOP_LOOP_SET, 9
	dc.w	SFXOP_CALL
	dc.l	sfx_cube_spawn_sub
	dc.w	SFXOP_PERIOD_ADD, -1000
	dc.w	SFXOP_LOOP_END
	dc.w	SFXOP_ENV
	dc.l	env_att6
	dc.w	SFXOP_PERIOD, 0x2880
	dc.w	SFXOP_LOOP_SET, 3
	dc.w	SFXOP_CALL
	dc.l	sfx_cube_spawn_sub
	dc.w	SFXOP_PERIOD_ADD, -1000
	dc.w	SFXOP_LOOP_END
	dc.w	SFXOP_END

sfx_cube_toss:
	dc.w	SFXOP_PERIOD, 0x1D00
	dc.w	SFXOP_ENV
	dc.l	env_att0
	dc.w	SFXOP_SWEEP, -140
	dc.w	SFXOP_REST, 40
	dc.w	SFXOP_SWEEP, -50
	dc.w	SFXOP_PERIOD_SAVE
	dc.w	SFXOP_REST, 20

	dc.w	SFXOP_ENV
	dc.l	env_att6
	dc.w	SFXOP_PERIOD_LOAD
	dc.w	SFXOP_REST, 20
	dc.w	SFXOP_END

sfx_cube_bounce:
	dc.w	SFXOP_PERIOD, 0x2000
	dc.w	SFXOP_SWEEP, 400
	dc.w	SFXOP_ENV
	dc.l	env_att0
	dc.w	SFXOP_REST, 20
	dc.w	SFXOP_PERIOD, 0x0D00
	dc.w	SFXOP_SWEEP, -200
	dc.w	SFXOP_REST, 4
	
	dc.w	SFXOP_END

sfx_cube_hit_sub:
	dc.w	SFXOP_PERIOD_LOAD
	dc.w	SFXOP_SWEEP, 140
	dc.w	SFXOP_REST, 5
#	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_REST, 40
	dc.w	SFXOP_RET

sfx_cube_hit:
	dc.w	SFXOP_PERIOD, 0x400
	dc.w	SFXOP_PERIOD_SAVE
	dc.w	SFXOP_ENV
	dc.l	env_att0

#	dc.w	SFXOP_NOISE_TONE, 0x04
#	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_CALL
	dc.l	sfx_cube_hit_sub

	dc.w	SFXOP_ENV
	dc.l	env_att2
	dc.w	SFXOP_CALL
	dc.l	sfx_cube_hit_sub

	dc.w	SFXOP_ENV
	dc.l	env_att4
	dc.w	SFXOP_CALL
	dc.l	sfx_cube_hit_sub

	dc.w	SFXOP_END

sfx_cube_fizzle:
	dc.w	SFXOP_PERIOD, 0x2400
	dc.w	SFXOP_ENV
	dc.l	env_slowdecay

	dc.w	SFXOP_SWEEP, -120
	dc.w	SFXOP_REST, 20

	dc.w	SFXOP_LOOP_SET, 3
	dc.w	SFXOP_PERIOD_SAVE
	dc.w	SFXOP_REST, 25
	dc.w	SFXOP_PERIOD_LOAD

	dc.w	SFXOP_LOOP_END
	dc.w	SFXOP_END

sfx_obj_burst:
	dc.w	SFXOP_PERIOD, 0x2400
	dc.w	SFXOP_ENV
	dc.l	env_att0

	dc.w	SFXOP_SWEEP, 222
	dc.w	SFXOP_REST, 32

	dc.w	SFXOP_PERIOD, 0x0F00
	dc.w	SFXOP_SWEEP, 50
	dc.w	SFXOP_REST, 56

	dc.w	SFXOP_ENV
	dc.l	env_att6

	dc.w	SFXOP_PERIOD, 0x0F00
	dc.w	SFXOP_SWEEP, 50
	dc.w	SFXOP_REST, 56
	dc.w	SFXOP_END

sfx_obj_burst_hi:
	dc.w	SFXOP_PERIOD, 0x2400/2
	dc.w	SFXOP_ENV
	dc.l	env_att0

	dc.w	SFXOP_SWEEP, 111
	dc.w	SFXOP_REST, 32

	dc.w	SFXOP_PERIOD, 0x0F00/2
	dc.w	SFXOP_SWEEP, 25
	dc.w	SFXOP_REST, 56

	dc.w	SFXOP_ENV
	dc.l	env_att6

	dc.w	SFXOP_PERIOD, 0x0F00/2
	dc.w	SFXOP_SWEEP, 25
	dc.w	SFXOP_REST, 56
	dc.w	SFXOP_END

sfx_teleport_sub:
	dc.w	SFXOP_PERIOD, 0x1600
	dc.w	SFXOP_SWEEP, -2184
	dc.w	SFXOP_REST, 30
	dc.w	SFXOP_SWEEP, 2184
	dc.w	SFXOP_REST, 30
	dc.w	SFXOP_RET

sfx_teleport:
	dc.w	SFXOP_ENV

	dc.l	env_att5
	dc.w	SFXOP_CALL
	dc.l	sfx_teleport_sub
	dc.w	SFXOP_ENV
	dc.l	env_att3
	dc.w	SFXOP_CALL
	dc.l	sfx_teleport_sub
	dc.w	SFXOP_ENV
	dc.l	env_att1
	dc.w	SFXOP_CALL
	dc.l	sfx_teleport_sub
	dc.w	SFXOP_ENV
	dc.l	env_att0
	dc.w	SFXOP_CALL
	dc.l	sfx_teleport_sub
	dc.w	SFXOP_ENV
	dc.l	env_att1
	dc.w	SFXOP_CALL
	dc.l	sfx_teleport_sub
	dc.w	SFXOP_ENV
	dc.l	env_att3
	dc.w	SFXOP_CALL
	dc.l	sfx_teleport_sub
	dc.w	SFXOP_ENV
	dc.l	env_att5
	dc.w	SFXOP_CALL
	dc.l	sfx_teleport_sub
	dc.w	SFXOP_END

sfx_boingo_jump:
	dc.w	SFXOP_ENV
	dc.l	env_att0
	dc.w	SFXOP_PERIOD, 0x3000
	dc.w	SFXOP_SWEEP, 200
	dc.w	SFXOP_REST, 14
	dc.w	SFXOP_ENV
	dc.l	env_att15
	dc.w	SFXOP_REST, 9

	dc.w	SFXOP_PERIOD, 0x08D0
	dc.w	SFXOP_LOOP_SET, 3
	dc.w	SFXOP_ENV
	dc.l	env_scratchy
	dc.w	SFXOP_SWEEP, -100
	dc.w	SFXOP_REST, 2
	dc.w	SFXOP_SWEEP, 40
	dc.w	SFXOP_REST, 2
	dc.w	SFXOP_LOOP_END

	dc.w	SFXOP_END

sfx_powerup_get:
	dc.w	SFXOP_ENV
	dc.l	env_att0
	dc.w	SFXOP_PERIOD, 0x0700
	dc.w	SFXOP_SWEEP, 100
	dc.w	SFXOP_REST, 40

	dc.w	SFXOP_PERIOD_SAVE
	dc.w	SFXOP_SWEEP, -140
	dc.w	SFXOP_REST, 35

	dc.w	SFXOP_ENV
	dc.l	env_att3
	dc.w	SFXOP_PERIOD_LOAD
	dc.w	SFXOP_REST, 35

	dc.w	SFXOP_ENV
	dc.l	env_att5
	dc.w	SFXOP_PERIOD_LOAD
	dc.w	SFXOP_REST, 35

	dc.w	SFXOP_END

sfx_magibear_shot:
	dc.w	SFXOP_ENV
	dc.l	env_decay
	dc.w	SFXOP_PERIOD, 0x1000
	dc.w	SFXOP_SWEEP, -1400
	dc.w	SFXOP_REST, 26
	dc.w	SFXOP_SWEEP, 80
	dc.w	SFXOP_REST, 26

	dc.w	SFXOP_END

sfx_gaxter_shot:
	dc.w	SFXOP_ENV
	dc.l	env_att15
	dc.w	SFXOP_NOISE_TONE, 0x03
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_PERIOD, 0x0010
	dc.w	SFXOP_SWEEP, 0x0B
	dc.w	SFXOP_REST, 50
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END

sfx_killzam_warp:
	dc.w	SFXOP_ENV
	dc.l	env_att15
	dc.w	SFXOP_NOISE_TONE, 0x03
	dc.w	SFXOP_NOISE_VOL, 2
	dc.w	SFXOP_PERIOD, 0x0400
	dc.w	SFXOP_SWEEP, -0x10
	dc.w	SFXOP_REST, 0x1B
	dc.w	SFXOP_PERIOD_SAVE

	dc.w	SFXOP_NOISE_VOL, 1
	dc.w	SFXOP_REST, 0x1B
	dc.w	SFXOP_PERIOD_LOAD

	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_REST, 0x1B
	dc.w	SFXOP_PERIOD_LOAD

	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_REST, 0x1B
	dc.w	SFXOP_PERIOD_LOAD

	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_REST, 0x1B
	dc.w	SFXOP_PERIOD_LOAD

	dc.w	SFXOP_NOISE_VOL, 1
	dc.w	SFXOP_REST, 0x1B
	dc.w	SFXOP_PERIOD_LOAD

	dc.w	SFXOP_NOISE_VOL, 2
	dc.w	SFXOP_REST, 0x1B
	dc.w	SFXOP_PERIOD_LOAD

	dc.w	SFXOP_NOISE_VOL, 3
	dc.w	SFXOP_REST, 0x1B
	dc.w	SFXOP_PERIOD_LOAD

	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END

sfx_explode:
	dc.w	SFXOP_NOISE_TONE, 0x07
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_ENV
	dc.l	env_att13
	dc.w	SFXOP_PERIOD, 0x300
	dc.w	SFXOP_SWEEP, 60
	dc.w	SFXOP_REST, 17
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_REST, 5

	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_PERIOD, 0x080
	dc.w	SFXOP_REST, 50

	dc.w	SFXOP_PERIOD_SAVE
	dc.w	SFXOP_REST, 50
	dc.w	SFXOP_NOISE_VOL, 4
	dc.w	SFXOP_PERIOD_LOAD
	dc.w	SFXOP_REST, 50

	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END

sfx_elevator:
	dc.w	SFXOP_PERIOD, 0x3D00
	dc.w	SFXOP_ENV
	dc.l	env_att2
sfx_elevator_forever:
	dc.w	SFXOP_SWEEP, 4
	dc.w	SFXOP_REST, 20
	dc.w	SFXOP_SWEEP, -4
	dc.w	SFXOP_REST, 20
	dc.w	SFXOP_JMP
	dc.l	sfx_elevator_forever

sfx_elevator_2:
	dc.w	SFXOP_PERIOD, 0x3C40
	dc.w	SFXOP_ENV
	dc.l	env_att2
	dc.w	SFXOP_JMP
	dc.l	sfx_elevator_forever

.set	PAUSE_NOTE_LEN, 23

sfx_pause:
	dc.w	SFXOP_ENV
	dc.l	env_pausenote
	dc.w	SFXOP_PERIOD, 0x0E20/2
	dc.w	SFXOP_REST, PAUSE_NOTE_LEN

	dc.w	SFXOP_ENV
	dc.l	env_pausenote
	dc.w	SFXOP_PERIOD, 0x1680/4
	dc.w	SFXOP_REST, PAUSE_NOTE_LEN

	dc.w	SFXOP_ENV
	dc.l	env_pausenote
	dc.w	SFXOP_PERIOD, 0x12E0/4
	dc.w	SFXOP_REST, PAUSE_NOTE_LEN

	dc.w	SFXOP_ENV
	dc.l	env_pausenote
	dc.w	SFXOP_PERIOD, 0x0E20/4
	dc.w	SFXOP_REST, PAUSE_NOTE_LEN*2

	dc.w	SFXOP_ENV
	dc.l	env_pausenote_quiet
	dc.w	SFXOP_REST, PAUSE_NOTE_LEN
	dc.w	SFXOP_END

sfx_pause2:
	dc.w	SFXOP_ENV
	dc.l	env_pausenote
	dc.w	SFXOP_PERIOD, 0x1680
	dc.w	SFXOP_REST, PAUSE_NOTE_LEN

	dc.w	SFXOP_ENV
	dc.l	env_pausenote
	dc.w	SFXOP_PERIOD, 0x12E0
	dc.w	SFXOP_REST, PAUSE_NOTE_LEN

	dc.w	SFXOP_ENV
	dc.l	env_pausenote
	dc.w	SFXOP_PERIOD, 0x0E20
	dc.w	SFXOP_REST, PAUSE_NOTE_LEN

	dc.w	SFXOP_ENV
	dc.l	env_pausenote
	dc.w	SFXOP_PERIOD, 0x1680/2
	dc.w	SFXOP_REST, PAUSE_NOTE_LEN*2

	dc.w	SFXOP_ENV
	dc.l	env_pausenote_quiet
	dc.w	SFXOP_REST, PAUSE_NOTE_LEN
	dc.w	SFXOP_END

sfx_giver:
	dc.w	SFXOP_ENV
	dc.l	env_att14
	dc.w	SFXOP_NOISE_TONE, 0x03
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_PERIOD, 0x0600
	dc.w	SFXOP_SWEEP, -6
	dc.w	SFXOP_REST, 0x100
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END

sfx_beep:
	dc.w	SFXOP_ENV
	dc.l	env_att15
	dc.w	SFXOP_NOISE_TONE, 0x03
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_PERIOD, 0x200
	dc.w	SFXOP_REST, 3*4
	dc.w	SFXOP_PERIOD, 0x200/2
	dc.w	SFXOP_REST, 3*4
	dc.w	SFXOP_PERIOD, 0x200
	dc.w	SFXOP_REST, 3*4
	dc.w	SFXOP_PERIOD, 0x200/2
	dc.w	SFXOP_REST, 3*4
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END


sfx_beep_pulse:
	dc.w	SFXOP_ENV
	dc.l	env_att0
	dc.w	SFXOP_PERIOD, 0x17D0/2
	dc.w	SFXOP_REST, 3*4
	dc.w	SFXOP_PERIOD, 0x17D0/4
	dc.w	SFXOP_REST, 3*4
	dc.w	SFXOP_PERIOD, 0x17D0/2
	dc.w	SFXOP_REST, 3*4
	dc.w	SFXOP_PERIOD, 0x17D0/4
	dc.w	SFXOP_REST, 3*4
	dc.w	SFXOP_END

.set	SELSOUND_NOTE_LEN, 20

sfx_select:
	dc.w	SFXOP_ENV
	dc.l	env_att15
	dc.w	SFXOP_NOISE_TONE, 0x03
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_PERIOD, 0x01C0*2
	dc.w	SFXOP_REST, SELSOUND_NOTE_LEN
	dc.w	SFXOP_PERIOD, 0x0180*2
	dc.w	SFXOP_REST, SELSOUND_NOTE_LEN
	dc.w	SFXOP_PERIOD, 0x0130*2
	dc.w	SFXOP_REST, SELSOUND_NOTE_LEN
	dc.w	SFXOP_PERIOD, 0x0100*2
	dc.w	SFXOP_REST, SELSOUND_NOTE_LEN*2
	dc.w	SFXOP_NOISE_VOL, 4
	dc.w	SFXOP_PERIOD, 0x0180*2
	dc.w	SFXOP_REST, SELSOUND_NOTE_LEN
	dc.w	SFXOP_PERIOD, 0x0130*2
	dc.w	SFXOP_REST, SELSOUND_NOTE_LEN
	dc.w	SFXOP_PERIOD, 0x0100*2
	dc.w	SFXOP_REST, SELSOUND_NOTE_LEN*2
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END

sfx_slam:
	dc.w	SFXOP_NOISE_TONE, 0x07
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_ENV
	dc.l	env_att13
	dc.w	SFXOP_PERIOD, 0x100
	dc.w	SFXOP_SWEEP, 60
	dc.w	SFXOP_REST, 20
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_REST, 10
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_PERIOD, 0x100
	dc.w	SFXOP_SWEEP, 100
	dc.w	SFXOP_REST, 120
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END

sfx_meow:
	dc.w	SFXOP_NOISE_TONE, 0x03
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_ENV
	dc.l	env_att15

	dc.w	SFXOP_PERIOD, 0x00E0
	dc.w	SFXOP_SWEEP, -2
	dc.w	SFXOP_REST, 30

	dc.w	SFXOP_SWEEP, -1
	dc.w	SFXOP_REST, 34
	dc.w	SFXOP_SWEEP, 0
	dc.w	SFXOP_REST, 20
	dc.w	SFXOP_SWEEP, 1
	dc.w	SFXOP_REST, 40

	dc.w	SFXOP_SWEEP, 3

	dc.w	SFXOP_REST, 52
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END

sfx_snore:
	dc.w	SFXOP_NOISE_TONE, 0x03
	dc.w	SFXOP_NOISE_VOL, 2
	dc.w	SFXOP_ENV
	dc.l	env_att15

	dc.w	SFXOP_PERIOD, 0x0340
	dc.w	SFXOP_LOOP_SET, 20
	dc.w	SFXOP_NOISE_VOL, 2
	dc.w	SFXOP_REST, 1
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_REST, 1
	dc.w	SFXOP_LOOP_END

	dc.w	SFXOP_PERIOD, 0x02E0
	dc.w	SFXOP_LOOP_SET, 15
	dc.w	SFXOP_NOISE_VOL, 3
	dc.w	SFXOP_REST, 1
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_REST, 1
	dc.w	SFXOP_LOOP_END

	dc.w	SFXOP_PERIOD, 0x02D0
	dc.w	SFXOP_LOOP_SET, 10
	dc.w	SFXOP_NOISE_VOL, 4
	dc.w	SFXOP_REST, 1
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_REST, 1
	dc.w	SFXOP_LOOP_END

	dc.w	SFXOP_PERIOD, 0x02C0
	dc.w	SFXOP_LOOP_SET, 9
	dc.w	SFXOP_NOISE_VOL, 5
	dc.w	SFXOP_REST, 1
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_REST, 1
	dc.w	SFXOP_LOOP_END


	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END

sfx_moo:
	dc.w	SFXOP_NOISE_TONE, 0x03
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_ENV
	dc.l	env_att15

	dc.w	SFXOP_PERIOD, 0x0700
	dc.w	SFXOP_SWEEP, -8
	dc.w	SFXOP_REST, 88
	dc.w	SFXOP_SWEEP, 0
	dc.w	SFXOP_REST, 110
	dc.w	SFXOP_SWEEP, 9
	dc.w	SFXOP_REST, 100

	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END

sfx_boss_step:
	dc.w	SFXOP_ENV
	dc.l	env_att15
	dc.w	SFXOP_NOISE_TONE, 0x05
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_REST, 8
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_REST, 16
	dc.w	SFXOP_NOISE_TONE, 0x04
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_REST, 8
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END

sfx_sand:
	dc.w	SFXOP_ENV
	dc.l	env_att15
	dc.w	SFXOP_NOISE_TONE, 0x07
	dc.w	SFXOP_PERIOD, 0x0200
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_REST, 8
	dc.w	SFXOP_PERIOD, 0x0010
	dc.w	SFXOP_NOISE_VOL, 0
	dc.w	SFXOP_REST, 8
	dc.w	SFXOP_NOISE_VOL, 2
	dc.w	SFXOP_REST, 8
	dc.w	SFXOP_NOISE_VOL, 4
	dc.w	SFXOP_REST, 8
	dc.w	SFXOP_NOISE_VOL, 6
	dc.w	SFXOP_REST, 8
	dc.w	SFXOP_NOISE_VOL, 8
	dc.w	SFXOP_REST, 8
	dc.w	SFXOP_NOISE_VOL, 10
	dc.w	SFXOP_REST, 8
	dc.w	SFXOP_NOISE_VOL, 15
	dc.w	SFXOP_END

	sfx_engine_sound_list:
# SFX_NULL
	dc.l	sfx_null
# SFX_JUMP
	dc.l	sfx_jump
# SFX_WALK1
	dc.l	sfx_walk1
# SFX_WALK2
	dc.l	sfx_walk2
# SFX_HURT
	dc.l	sfx_hurt
# SFX_CUBE_LIFT
	dc.l	sfx_cube_lift
# SFX_CUBE_SPAWN
	dc.l	sfx_cube_spawn
# SFX_CUBE_TOSS
	dc.l	sfx_cube_toss
# SFX_CUBE_BOUNCE
	dc.l	sfx_cube_bounce
# SFX_CUBE_HIT
	dc.l	sfx_cube_hit
# SFX_CUBE_FIZZLE
	dc.l	sfx_cube_fizzle
# SFX_OBJ_BURST
	dc.l	sfx_obj_burst
# SFX_OBJ_BURST_HI
	dc.l	sfx_obj_burst_hi
# SFX_TELEPORT
	dc.l	sfx_teleport
# SFX_KILLZAM_WARP
	dc.l	sfx_killzam_warp + SFX_FLAG_CH3
# SFX_BOINGO_JUMP
	dc.l	sfx_boingo_jump
# SFX_POWERUP_GET
	dc.l	sfx_powerup_get
# SFX_MAGIBEAR_SHOT
	dc.l	sfx_magibear_shot
# SFX_GAXTER_SHOT
	dc.l	sfx_gaxter_shot + SFX_FLAG_CH3
# SFX_MEOW
	dc.l	sfx_meow + SFX_FLAG_CH3
# SFX_EXPLODE
	dc.l	sfx_explode + SFX_FLAG_CH3
# SFX_ELEVATOR
	dc.l	sfx_elevator
# SFX_ELEVATOR_2
	dc.l	sfx_elevator_2
# SFX_PAUSE_1
	dc.l	sfx_pause
# SFX_PAUSE_2
	dc.l	sfx_pause2
# SFX_MOO
	dc.l	sfx_moo + SFX_FLAG_CH3
# SFX_BOSS_STEP
	dc.l	sfx_boss_step
# SFX_GIVER
	dc.l	sfx_giver + SFX_FLAG_CH3
# SFX_SNORE
	dc.l	sfx_snore + SFX_FLAG_CH3
# SFX_SAND
	dc.l	sfx_sand + SFX_FLAG_CH3
# SFX_BEEP
	dc.l	sfx_beep + SFX_FLAG_CH3
# SFX_SELECT
	dc.l	sfx_select + SFX_FLAG_CH3
# free
	dc.l	sfx_null
# SFX_SLAM
	dc.l	sfx_slam + SFX_FLAG_CH3
