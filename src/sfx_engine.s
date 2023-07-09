
	.include	"src/sfx_engine_types.inc"
	.extern	SfxChans

	.section	.text

	.extern	sfx_engine_sound_list


#
# void sfx_engine_play(uint16_t id, uint16_t prio)
#
	.global	sfx_engine_play
.set	SPID, 4

# SP - pushed args
.set	ARG_REQ_ID, SPID+2
.set	ARG_REQ_PRIO, SPID+(4*1)+2

sfx_engine_play:
	move.w	ARG_REQ_ID(sp), d0
	beq	req_abort
	add.w	d0, d0
	add.w	d0, d0
	lea	sfx_engine_sound_list, a1
	adda.w	d0, a1
	tst.b	(a1)
	bpl	req_no_force_channel_3
# Sound effect specifically wants channel 3 (for noise purposes)
	lea	SfxChans+SFXCHAN_SIZE*2, a0
	bra	req_search_found

req_no_force_channel_3:
# Try to find an open channel.
	lea	SfxChans, a0
	move.w	#SFXCHAN_CHANNEL_COUNT-1, d0
	move.w	ARG_REQ_PRIO(sp), d1
req_search_top:
	tst.w	SFXCHAN_id(a0)
	beq	req_search_found
	lea	SFXCHAN_SIZE(a0), a0
	dbf	d0, req_search_top
# If no open channel, find one to replace by priority.
	lea	SfxChans, a0
	move.w	#SFXCHAN_CHANNEL_COUNT-1, d0
replace_search_top:
	cmp.w	SFXCHAN_prio(a0), d1
	bls	req_search_found
	dbf	d0, replace_search_top
	rts
# Register the sound in the channel at a0.
req_search_found:
	/* Set ID and head */
	move.w	ARG_REQ_ID(sp), SFXCHAN_id(a0)
	move.l	(a1), SFXCHAN_head(a0)

	lea	SFXCHAN_stack(a0), a1
	adda.w	#SFXCHAN_STACK_DEPTH*4, a1
	move.l	a1, SFXCHAN_sp(a0)

	moveq	#0, d0
	move.w	d0, SFXCHAN_rests(a0)
	move.w	d0, SFXCHAN_sweep(a0)
	move.l	#sfx_engine_envelope_default, SFXCHAN_env_head(a0)
	move.w	d1, SFXCHAN_prio(a0)
req_abort:
	rts


	.global	sfx_engine_init
sfx_engine_init:
	/* Silence all channels */
	move.b	#0x9F, PSG_BASE
	move.b	#0xBF, PSG_BASE
	move.b	#0xDF, PSG_BASE
	move.b	#0xFF, PSG_BASE
	/* Clear out channel structs */
	lea	SfxChans, a0
	moveq	#0, d0
	move.w	#(SFXCHAN_TOTAL_BYTES / 2)-1, d1
clear_top:
	move.w	d0, (a0)+
	dbf	d1, clear_top
	/* Assign register base values across channels */
	lea	SfxChans, a0
	move.w	#0x80, d0  /* register base */
	move.w	#SFXCHAN_CHANNEL_COUNT-1, d1
set_regbase_top:
	move.w	d0, SFXCHAN_regbase(a0)
	add.w	#0x20, d0
	lea	SFXCHAN_SIZE(a0), a0
	dbf	d1, set_regbase_top
	rts

	add.w	d0, d0

#
# void sfx_engine_stop(uint16_t id)
#
	.global	sfx_engine_stop
.set	SPID, 4

# SP - pushed args
.set	ARG_REQ_ID, SPID+2

sfx_engine_stop:
	lea	SfxChans, a0
	move.w	#SFXCHAN_CHANNEL_COUNT-1, d0
	move.w	ARG_REQ_ID(sp), d1
stop_search_top:
	cmp.w	SFXCHAN_id(a0), d1
	beq	channel_erase
	lea	SFXCHAN_SIZE(a0), a0
	dbf	d0, stop_search_top
	rts

#
#
#


# a0 = channel to clear
channel_erase:
	moveq	#0, d0
	move.l	d0, SFXCHAN_head(a0)
	move.w	d0, SFXCHAN_id(a0)
	move.w	SFXCHAN_regbase(a0), d0
	ori.w	#0x001F, d0  /* vol and silence */
	move.b	d0, PSG_BASE
	rts


#
# Channel poll
#


# a0 = channel to execute
sfx_chan_poll:
	move.w	SFXCHAN_rests(a0), d0
	beq	no_rest
	subq.w	#1, d0
	move.w	d0, SFXCHAN_rests(a0)

/* Tone processing */
	move.w	SFXCHAN_period(a0), d0
	add.w	SFXCHAN_sweep(a0), d0
	move.w	d0, SFXCHAN_period(a0)
	lsr.w	#4, d0
	move.w	d0, d1
	swap	d1

	/* Get low nybble of period mixed in with the latch register */
	move.w	SFXCHAN_regbase(a0), d1
	andi.w	#0x000F, d0  /* data */
	or.b	d0, d1
	move.b	d1, PSG_BASE

	/* Get upper six bits in there. */
	move.w	SFXCHAN_regbase(a0), d0
	andi.b	#0x7F, d0  /* data byte */
	swap	d1
	lsr.w	#4, d1
	andi.b	#0x3F, d1
	or.b	d1, d0
	move.b	d1, PSG_BASE

/* Volume processing */
	move.l	SFXCHAN_env_head(a0), a1
	move.b	(a1)+, d0
	tst.b	(a1)
	bpl	1f
	subq.l	#1, a1
1:
	move.l	a1, SFXCHAN_env_head(a0)
	move.w	SFXCHAN_regbase(a0), d1
	ori.b	#0x10, d1  /* Mark for volume latch */
	andi.b	#0x0F, d0
	or.b	d0, d1
	move.b	d1, PSG_BASE

	rts

no_rest:
	move.l	SFXCHAN_head(a0), d1
	beq	channel_erase
	movea.l	d1, a1
	move.w	(a1)+, d0
	add.w	d0, d0
	add.w	d0, d0
	jmp	opcode_jptbl(pc, d0.w)

opcode_jptbl:
	bra.w	op00
	bra.w	op01
	bra.w	op02
	bra.w	op03
	bra.w	op04
	bra.w	op05
	bra.w	op06
	bra.w	op07
	bra.w	op08
	bra.w	op09
	bra.w	op0a
	bra.w	op0b
	bra.w	op0c
	bra.w	op0d
	bra.w	op0e
op00:  /* rest */
	move.w	(a1)+, SFXCHAN_rests(a0)
	bra	poll_cycle_complete
op01:  /* end */
	move.w	SFXCHAN_regbase(a0), d1
	ori.b	#0x1F, d1  /* Mark for volume latch, silence F*/
	move.b	d1, PSG_BASE
	bra	channel_erase
op02:  /* call */
	move.l	(a1)+, d0
	/* Push return address */
	move.l	a1, d1
	move.l	SFXCHAN_sp(a0), a1
	move.l	d1, -(a1)
	move.l	a1, SFXCHAN_sp(a0)
	/* Go to new address */
	move.l	d0, a1
	bra	poll_cycle_complete
op03:  /* ret */
	move.l	SFXCHAN_sp(a0), a1
	move.l	(a1)+, d1
	move.l	a1, SFXCHAN_sp(a0)
	move.l	d1, a1
	bra	poll_cycle_complete
op04:  /* loop set */
	move.w	(a1)+, SFXCHAN_loop_cnt(a0)
	move.l	a1, SFXCHAN_loop_head(a0)
	bra	poll_cycle_complete
op05:  /* loop end */
	sub.w	#1, SFXCHAN_loop_cnt(a0)
	bmi	poll_cycle_complete
	move.l	SFXCHAN_loop_head(a0), a1
	bra	poll_cycle_complete
op06:  /* env */
	move.l	(a1)+, SFXCHAN_env_head(a0)
	bra	poll_cycle_complete
op07:  /* period */
	move.w	(a1)+, SFXCHAN_period(a0)
	bra	poll_cycle_complete
op08:  /* sweep */
	move.w	(a1)+, SFXCHAN_sweep(a0)
	bra	poll_cycle_complete
op09:  /* noise vol */
	move.w	(a1)+, d0
	andi.w	#0x000F, d0
	ori.w	#0x00F0, d0  /* Vol byte for noise channel */
	move.b	d0, PSG_BASE
	bra	poll_cycle_complete
op0a:  /* noise tone */
	move.w	(a1)+, d0
	andi.w	#0x0007, d0
	ori.w	#0x00E0, d0  /* Tone byte for noise channel */
	move.b	d0, PSG_BASE
	bra	poll_cycle_complete
op0b:  /* period add */
	move.w	(a1)+, d0
	add.w	d0, SFXCHAN_period(a0)
	bra	poll_cycle_complete
op0c:  /* jump */
	move.l	(a1)+, a1
	bra	poll_cycle_complete
op0d:	/* period save */
	move.w	SFXCHAN_period(a0), SFXCHAN_period_save(a0)
	bra	poll_cycle_complete
op0e:	/* period load */
	move.w	SFXCHAN_period_save(a0), SFXCHAN_period(a0)
	bra	poll_cycle_complete

poll_cycle_complete:
	move.l	a1, SFXCHAN_head(a0)

/* Go back to the top. */
	bra	sfx_chan_poll

	.global	sfx_engine_irq_handler
sfx_engine_irq_handler:
	lea	SfxChans, a0
	bsr	sfx_chan_poll
	lea	SfxChans+SFXCHAN_SIZE, a0
	bsr	sfx_chan_poll
	lea	SfxChans+2*SFXCHAN_SIZE, a0
	bra	sfx_chan_poll  /* bsr + rts */
