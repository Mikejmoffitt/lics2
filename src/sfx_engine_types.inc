/* Register defs */

.set	PSG_BASE, 0xC00011

.set	SFXOP_REST,             0x00  /* Rests for n.w ticks. */
.set	SFXOP_END,              0x01  /* Terminates playback of the sound stream. */
.set	SFXOP_CALL,             0x02  /* Calls a subroutine. */
.set	SFXOP_RET,              0x03  /* Returns from a subroutine. */
.set	SFXOP_LOOP_SET,         0x04  /* Sets loop pointer to next instruction, and loop counter to arg.w */
.set	SFXOP_LOOP_END,         0x05  /* Decrements loop counter and branches to loop pointer if counter doesn't underflow */
.set	SFXOP_ENV,              0x06  /* Assigns an envelope macro (ptr).l */
.set	SFXOP_PERIOD,           0x07  /* Sets pitch by direct value */
.set	SFXOP_SWEEP,            0x08  /* Sets pitch per poll / 16.w */
.set	SFXOP_NOISE_VOL,        0x09  /* Sets normal noise mode tap. */
.set	SFXOP_NOISE_TONE,       0x0A  /* Sets shorter, periodic noise mode tap. */
.set	SFXOP_PERIOD_ADD,       0x0B  /* Adds to period */
.set	SFXOP_JMP,              0x0C  /* Jump */


/* Channel structure */
.set	SFXCHAN_id,             0
.set	SFXCHAN_prio,           SFXCHAN_id+2
.set	SFXCHAN_head,           SFXCHAN_prio+2
.set	SFXCHAN_loop_head,      SFXCHAN_head+4
.set	SFXCHAN_loop_cnt,       SFXCHAN_loop_head+4
.set	SFXCHAN_rests,          SFXCHAN_loop_cnt+2
.set	SFXCHAN_regbase,        SFXCHAN_rests+2
.set	SFXCHAN_period,         SFXCHAN_regbase+2
.set	SFXCHAN_sweep,          SFXCHAN_period+2
.set	SFXCHAN_env_head,       SFXCHAN_sweep+2
.set	SFXCHAN_sp,             SFXCHAN_env_head+4
.set	SFXCHAN_stack,          SFXCHAN_sp+4
.set	SFXCHAN_pad,            SFXCHAN_stack+(4*SFXCHAN_STACK_DEPTH)

.set	SFXCHAN_STACK_DEPTH,    8
.set	SFXCHAN_SIZE,           SFXCHAN_pad+2

.set	SFXCHAN_CHANNEL_COUNT,  3
.set	SFXCHAN_TOTAL_BYTES,    SFXCHAN_SIZE+SFXCHAN_SIZE
