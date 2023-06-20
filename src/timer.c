#include "timer.h"
#include "progress.h"

#include <stdbool.h>

static bool s_active;

void timer_stop(void)
{
	s_active = false;
}

void timer_start(void)
{
	s_active = true;
}

void timer_poll(void)
{
	ProgressSlot *p = progress_get();
	if (s_active) p->elapsed_frames++;
}
