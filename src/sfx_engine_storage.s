
	.section	.bss
	.include	"src/sfx_engine_types.inc"

	.global	SfxChans

SfxChans:
	ds.b	SFXCHAN_TOTAL_BYTES
