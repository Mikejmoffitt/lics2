/* md-toolchain interrupt handlers
Michael Moffitt 2018-2022
*/

/* This is a flag used for vblank waiting. */
	.extern	g_vblank_wait
/* This runs the sound effects. */
	.extern sfx_tick
/* IRQ vectors unused */
_v_irq1:
_v_irq3:
_v_irq5:
_v_irq7:
	rte

/* /TH pin change, from controller port */
_v_irq2:
	rte

/* H-blank Interrupt */
_v_irq4:
	movem.l	%d0-%d1/%a0-%a1, -(%sp)
	jsr	sfx_tick
	movem.l	(%sp)+, %d0-%d1/%a0-%a1
	rte

/* V-blank Interrupt */
_v_irq6:
	/*
	This flag is cleared in vblank so that software may set it high,
	and then block on it transitioning low in order to synchronize.
	*/
	clr.w	g_vblank_wait
	rte
