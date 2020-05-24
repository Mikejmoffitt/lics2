#include "progress.h"
#include "md/megadrive.h"
#include "system.h"
#include "common.h"

// SRAM address +$0000 is used as scratch memory so we can test if SRAM is
// working properly. Then, each progress slot has a few magic numbers as
// a sanity test, which should also help with version mismatches.
// Valid save data starts at +$0002, with a number of progress slots laid
// out.


#define PROGRESS_MAGIC_0 0xBEEF
#define PROGRESS_MAGIC_1 0xDADA

#define PROGRESS_SRAM_POS 0x0002

static ProgressSlot progress_slots[1];

static uint16_t current_slot;

static uint16_t sram_is_working;

static void reset_slot(int8_t slot)
{
	return;
	uint8_t *d = (uint8_t *)(&progress_slots[slot]);
	for (uint16_t i = 0; i < sizeof(&progress_slots[slot]); i++) *d++ = 0;
	progress_slots[slot].magic_0 = PROGRESS_MAGIC_0;
	progress_slots[slot].magic_1 = PROGRESS_MAGIC_1;
}

int progress_init(void)
{
	sram_is_working = 0xBEEF;
	sram_write(0, &sram_is_working, sizeof(sram_is_working));
	sram_is_working = 0x5555;
	sram_read(0, &sram_is_working, sizeof(sram_is_working));

	sram_is_working = (sram_is_working == 0xBEEF);

	if (sram_is_working)
	{
		sram_read(PROGRESS_SRAM_POS, progress_slots, sizeof(progress_slots));
		for (uint16_t i = 0; i < ARRAYSIZE(progress_slots); i++)
		{
			if (progress_slots[i].magic_0 != PROGRESS_MAGIC_0 ||
			    progress_slots[i].magic_1 != PROGRESS_MAGIC_1)
			{
				reset_slot(i);
			}
		}
	}
	else
	{
		for (uint16_t i = 0; i < ARRAYSIZE(progress_slots); i++)
		{
			reset_slot(i);
		}
	}

	return 1;
}

uint8_t progress_is_sram_working(void)
{
	return sram_is_working;
}

void progress_select_slot(uint16_t slot)
{
	SYSTEM_ASSERT(slot <= ARRAYSIZE(progress_slots));
	current_slot = slot;
}

void progress_save(void)
{
	if (!sram_is_working) return;
	sram_write(PROGRESS_SRAM_POS, progress_slots, sizeof(progress_slots));
}

void progress_erase(void)
{
	SYSTEM_ASSERT(current_slot <= ARRAYSIZE(progress_slots));
	reset_slot(current_slot);
}

ProgressSlot *progress_get(void)
{
	SYSTEM_ASSERT(current_slot <= ARRAYSIZE(progress_slots));
	return &progress_slots[current_slot];
}
