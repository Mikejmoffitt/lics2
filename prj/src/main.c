#include "task.h"
#include "common.h"
// ------
#include "md/megadrive.h"
#include "util/text.h"

static Task tasks[40];
extern const unsigned char res_font_gfx_bin[];
extern const unsigned char res_font_pal_bin[];

static void init_func(Task *task)
{
	// Print a simple message in the center of plane A
	megadrive_init();
	text_init(res_font_gfx_bin, 3072, 0x400, res_font_pal_bin, 0);
	text_puts(VDP_PLANE_A, 14, 11, "Hello World");
	megadrive_finish();
	task->status = 0;
}

static void pre_func(Task *task)
{
	static int y = 1;
	text_puts(VDP_PLANE_A, 14, y, "Run");
	y++;
	if (y >= 32) y = 0;
}

static void post_func(Task *task)
{
	megadrive_finish();
}

void main(void)
{
	// Print a simple message in the center of plane A
	task_init(tasks, ARRAYSIZE(tasks));

	task_add(tasks, init_func, 0);
	task_add(tasks, pre_func, TASK_STATUS_STICKY);
	task_set(tasks, ARRAYSIZE(tasks) - 2, post_func, TASK_STATUS_STICKY);

	while (1)
	{
		task_iterate(tasks);
	}
}
