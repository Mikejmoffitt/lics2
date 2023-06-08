
	.section	.bss
	.include	"sfx_engine_types.inc"

	.global	SfxChans

SfxChans:
	ds.b	SFXCHAN_TOTAL_BYTES
