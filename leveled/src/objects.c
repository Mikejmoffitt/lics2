#include "objects.h"

static const ObjInfo s_obj_info_defs[OBJ_INVALID] =
{
	[OBJ_ENTRANCE]      = {"Room Ptr", 16, 32},
	[OBJ_CUBE]          = {"Cube    ", 16, 16},
	[OBJ_METAGRUB]      = {"Metagrub", 24, 8},

	[OBJ_FLIP]          = {"Flip    ", 24, 16},
	[OBJ_BOINGO]        = {"Boingo  ", 24, 16},
	[OBJ_ITEM]          = {"Item    ", 16, 16},
	[OBJ_GAXTER1]       = {"Gaxter 1", 16, 16},

	[OBJ_GAXTER2]       = {"Gaxter 2", 16, 16},
	[OBJ_BUGGO1]        = {"Buggo 1 ", 16, 16},
	[OBJ_BUGGO2]        = {"Buggo 2 ", 16, 16},
	[OBJ_DANCYFLOWER]   = {"Dncyflwr", 24, 48},

	[OBJ_JRAFF]         = {"Jraff   ", 24, 64},
	[OBJ_PILLA]         = {"Pilla   ", 16, 16},
	[OBJ_HEDGEDOG]      = {"Hedgedog", 24, 16},
	[OBJ_SHOOT]         = {"Shoot   ", 24, 16},

	[OBJ_LASER]         = {"Laser   ", 16, 16},
	[OBJ_KILLZAM]       = {"Killzam ", 16, 24},
	[OBJ_FLARGY]        = {"Flargy  ", 16, 32},
	[OBJ_PLANT]         = {"Plant   ", 32, 48},

	[OBJ_TOSSMUFFIN]    = {"Tossmuff", 16, 24},
	[OBJ_TELEPORTER]    = {"Teleprtr", 32, 32},
	[OBJ_MAGIBEAR]      = {"Magibear", 40, 32},
	[OBJ_LAVA]          = {"Lava Gen", 16, 16},
	
	[OBJ_COW]           = {"Cow     ", 40, 24},
	[OBJ_CONTAINER]     = {"Containr", 16, 16},
	[OBJ_HOOP]          = {"Hoop    ", 16, 16},
	[OBJ_FALSEBLOCK]    = {"Falseblk", 16, 16},

	[OBJ_CP_PAD]        = {"CP Pad  ", 32, 8},
	[OBJ_CP_METER]      = {"CP Meter", 16, 8},
	[OBJ_DOG]           = {"Blue Dog", 48, 32},
	[OBJ_ELEVATOR]      = {"Elevator", 32, 64},

	[OBJ_ELEVATOR_STOP] = {"Ele.Stop", 32, 8},
	[OBJ_FISSINS1]      = {"Fissins1", 16, 16},
	[OBJ_BOSS1]         = {"Boss 1  ", 48, 32},
	[OBJ_BOSS2]         = {"Boss 2  ", 64, 64},

	[OBJ_BOSSF1]        = {"Boss F1 ", 64, 64},
	[OBJ_BOSSF2]        = {"Boss F2 ", 64, 64},
	[OBJ_EGG]           = {"Egg     ", 32, 32},
	[OBJ_FISSINS2]      = {"Fissins2", 16, 16},

	[OBJ_BOUNDS]        = {"Bounds  ", 16, 96},
	[OBJ_SMALLEGG]      = {"SmallEgg", 16, 16},
	[OBJ_BASKETBALL]    = {"BsktBall", 16, 16},
	[OBJ_LAVAANIM]      = {"LavaAnim", 8, 8},

	[OBJ_SPOOKO]        = {"Spooko  ", 16, 16},
	[OBJ_WIP]           = {"WIP     ", 48, 32},
	[OBJ_BGSCROLLY]     = {"BgScroll", 8, 8},
	[OBJ_FAKECUBE]      = {"FakeCube", 16, 16},

	[OBJ_COLUMNS]       = {"Columns", 16, 16},
	[OBJ_GRASSES]       = {"Grasses", 16, 16},
	[OBJ_PURPLETREE]    = {"PrplTree", 16, 16},
	[OBJ_WNDWBACK]      = {"WndwBack", 16, 16},
	[OBJ_SCRLOCK]       = {"ScrLock ", 16, 16},
	[OBJ_TITLE]         = {"TitleScr", 112, 72},
	[OBJ_BOGOLOGO]      = {"BogoLogo", 128, 48},
};

const ObjInfo *object_get_info(ObjType i)
{
	return &s_obj_info_defs[i];
}
