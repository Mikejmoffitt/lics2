#ifndef OBJ_TYPES_H
#define OBJ_TYPES_H

// Make sure to hook these up to their related init functions in obj.c.
typedef enum ObjType
{
	OBJ_NULL          =  0,
	OBJ_ENTRANCE      =  1,
	OBJ_CUBE          =  2,
	OBJ_METAGRUB      =  3,
	OBJ_FLIP          =  4,
	OBJ_BOINGO        =  5,
	OBJ_ITEM          =  6,
	OBJ_GAXTER1       =  7,
	OBJ_GAXTER2       =  8,
	OBJ_BUGGO1        =  9,
	OBJ_BUGGO2        = 10,
	OBJ_DANCYFLOWER   = 11,
	OBJ_JRAFF         = 12,
	OBJ_PILLA         = 13,
	OBJ_HEDGEDOG      = 14,
	OBJ_SHOOT         = 15,
	OBJ_LASER         = 16,
	OBJ_KILLZAM       = 17,
	OBJ_FLARGY        = 18,
	OBJ_PLANT         = 19,
	OBJ_TOSSMUFFIN    = 20,
	OBJ_TELEPORTER    = 21,
	OBJ_MAGIBEAR      = 22,
	OBJ_LAVA          = 23,
	OBJ_COW           = 24,
	OBJ_CONTAINER     = 25,
	OBJ_HOOP          = 26,
	OBJ_FALSEBLOCK    = 27,
	OBJ_CP_PAD        = 28,
	OBJ_CP_METER      = 29,
	OBJ_DOG           = 30,
	OBJ_ELEVATOR      = 31,
	OBJ_ELEVATOR_STOP = 32,
	OBJ_FISSINS1      = 33,
	OBJ_BOSS1         = 34,
	OBJ_BOSS2         = 35,
	OBJ_BOSSF1        = 36,
	OBJ_BOSSF2        = 37,
	OBJ_EGG           = 38,
	OBJ_FISSINS2      = 39,
	OBJ_BOUNDS        = 40,
	OBJ_SMALLEGG      = 41,
	OBJ_BASKETBALL    = 42,
	OBJ_LAVAANIM      = 43,
	OBJ_SPOOKO        = 44,
	OBJ_WIP           = 45,
	OBJ_BGSCROLLY     = 46,
	OBJ_FAKECUBE      = 47,

	OBJ_GRASSES       = 122,
	OBJ_PURPLETREE    = 123,
	OBJ_WNDWBACK      = 124,
	OBJ_SCRLOCK       = 125,
	OBJ_TITLE         = 126,
	OBJ_BOGOLOGO      = 127,

	// Objects 128 and above are special objects added by the engine, rather
	// than being found in map data. They are singletons, and are not
	// instantiated from map data.
	OBJ_LYLE = 128,
	OBJ_CUBE_MANAGER = 129,
	OBJ_MAP = 130,
	OBJ_BG = 131,
	OBJ_HUD = 132,
	OBJ_PARTICLE_MANAGER = 133,
	OBJ_PROJECTILE_MANAGER = 134,
	OBJ_EXPLODER = 135,
	OBJ_POWERUP_MANAGER = 136,

	OBJ_TEMPLATE = 255,
	OBJ_INVALID = 256
} ObjType;

#endif  // OBJ_TYPES_H
