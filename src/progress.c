#include "progress.h"
#include "md/megadrive.h"
#include "system.h"

#include "obj/lyle.h"

// SRAM address $0000 is used as scratch memory so we can test if SRAM is
// working properly. Then, each progress slot has a few magic numbers as
// a sanity test, which should also help with version mismatches.
// Valid save data starts at $0002, with a number of progress slots laid
// out.
#define PROGRESS_SRAM_POS 0x0002

#define PROGRESS_MAGIC_0 0x00068000
#define PROGRESS_MAGIC_1 0x8008135

static ProgressSlot s_progress_slots[1];

static uint16_t s_current_slot;

static uint16_t s_sram_is_working;

static void reset_slot(int8_t slot)
{
	SYSTEM_ASSERT(sizeof(s_progress_slots[0]) % sizeof(uint16_t) == 0);

	volatile uint16_t *raw_mem_uint32 = (volatile uint16_t *)&s_progress_slots[slot];
	for (uint16_t j = 0; j < sizeof(s_progress_slots[0]) / sizeof(uint16_t); j++)
	{
		raw_mem_uint32[j] = 0;
	}

	s_progress_slots[slot].magic_0 = PROGRESS_MAGIC_0;
	s_progress_slots[slot].magic_1 = PROGRESS_MAGIC_1;
	s_progress_slots[slot].hp_capacity = LYLE_START_HP;
}

int progress_init(void)
{
#ifdef MDK_TARGET_C2
	s_sram_is_working = 0;
	for (uint16_t i = 0; i < ARRAYSIZE(s_progress_slots); i++)
	{
		reset_slot(i);
	}
	return 1;
#endif
	// Test that SRAM is working correctly. The first byte is reserved for
	// this check.
	s_sram_is_working = 0xBEEF;
	md_sram_write(0, &s_sram_is_working, sizeof(s_sram_is_working));
	s_sram_is_working = 0x5555;
	md_sram_read(0, &s_sram_is_working, sizeof(s_sram_is_working));

	s_sram_is_working = (s_sram_is_working == 0xBEEF);

	if (s_sram_is_working)
	{
		// Load and validate all save slots. Slots that do not contain the
		// right magic numbers will be cleared.
		md_sram_read(PROGRESS_SRAM_POS, s_progress_slots, sizeof(s_progress_slots));
		for (uint16_t i = 0; i < ARRAYSIZE(s_progress_slots); i++)
		{
			if (s_progress_slots[i].magic_0 != PROGRESS_MAGIC_0 ||
			    s_progress_slots[i].magic_1 != PROGRESS_MAGIC_1)
			{
				reset_slot(i);
			}
		}
	}
	else
	{
		// Clear all slots.
		for (uint16_t i = 0; i < ARRAYSIZE(s_progress_slots); i++)
		{
			reset_slot(i);
		}
	}

progress_select_slot(0);

	return 1;
}

uint8_t progress_is_sram_working(void)
{
	return s_sram_is_working;
}

void progress_select_slot(uint16_t slot)
{
	SYSTEM_ASSERT(slot <= ARRAYSIZE(s_progress_slots));
	s_current_slot = slot;
}

void progress_save(void)
{
	if (!s_sram_is_working) return;
	md_sram_write(PROGRESS_SRAM_POS, s_progress_slots, sizeof(s_progress_slots));
}

void progress_erase(void)
{
	SYSTEM_ASSERT(s_current_slot <= ARRAYSIZE(s_progress_slots));
	reset_slot(s_current_slot);
}

ProgressSlot *progress_get(void)
{
	SYSTEM_ASSERT(s_current_slot <= ARRAYSIZE(s_progress_slots));
	return &s_progress_slots[s_current_slot];
}
