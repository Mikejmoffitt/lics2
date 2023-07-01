#include "game.h"
#include "md/megadrive.h"
// ------
void main(void)
{
	megadrive_init();
	game_main();
}
