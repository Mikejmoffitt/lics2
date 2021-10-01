#ifndef OBJ_TYPES_H
#define OBJ_TYPES_H

// Make sure to hook these up to their related init functions in obj.c.
typedef enum ObjType
{
	OBJ_NULL          =  0,  // Nothing.
	OBJ_ENTRANCE      =  1,  // "Door" pointer.
	OBJ_CUBE          =  2,  // A cube placed on the map.
	OBJ_METAGRUB      =  3,  // Small grey worm thing.
	OBJ_FLIP          =  4,  // Teal bird that flies back and forth.
	OBJ_BOINGO        =  5,  // Angry little jumping blue guy.
	OBJ_ITEM          =  6,  // One of several ability-expanding items.
	OBJ_GAXTER1       =  7,  // Grey fly that seeks the player.
	OBJ_GAXTER2       =  8,  // Hovering pink fly that shoots angled bullets.
	OBJ_BUGGO1        =  9,  // Beetle that hugs the ceiling, firing spikes.
	OBJ_BUGGO2        = 10,  // Grey beetle on the floor that emits sparks.
	OBJ_DANCYFLOWER   = 11,  // Large dancing flower that blocks the way.
	OBJ_JRAFF         = 12,  // Tall grey giraffe-like enemy.
	OBJ_PILLA         = 13,  // Segmented enemy that runs quickly.
	OBJ_HEDGEDOG      = 14,  // Blue dog that jumps and fires a spread shot.
	OBJ_SHOOT         = 15,  // Red version of "flip" that dives at the player.
	OBJ_LASER         = 16,  // A laser column that phases in and out.
	OBJ_KILLZAM       = 17,  // Magic enemy that appears and fires at player.
	OBJ_FLARGY        = 18,  // Ugly guy who punches back cubes thrown at him.
	OBJ_PLANT         = 19,  // Large evil plant that spits projectiles.
	OBJ_TOSSMUFFIN    = 20,  // Picks up and throws cubes in his path.
	OBJ_TELEPORTER    = 21,  // Triggers a room exit when stood on.
	OBJ_MAGIBEAR      = 22,  // Belches sinusoidal attack patterns.
	OBJ_LAVA          = 23,  // A column of lava spewing from a pipe.
	OBJ_COW           = 24,  // A cow that jumps up when hit with a cube.
	OBJ_BALL          = 25,  // Orange ball containing an orb.
	OBJ_HOOP          = 26,  // A hoop, through which a ball may drop.
	OBJ_FALSEBLOCK    = 27,  // Fizzles out when all hoops are swished.
	OBJ_CP_GIVER      = 28,  // Long arm that absorbs cube power orbs.
	OBJ_CP_METER      = 29,  // Meter that shows Lyle's cube power count.
	OBJ_DOG           = 30,  // Blue dog that consumes small eggs.
	OBJ_ELEVATOR      = 31,  // Like it sounds. Goes up and down.
	OBJ_ELEVATOR_STOP = 32,  // Marks where an elevator should stop.
	OBJ_FISSINS1      = 33,  // Jumps up out of lava and falls back down.
	OBJ_BOSS1         = 34,  // The first boss.
	OBJ_BOSS2         = 35,  // TODO
	OBJ_BOSSF1        = 36,  // delete
	OBJ_BOSSF2        = 37,  // delete
	OBJ_EGG           = 38,  // Large teal egg that falls when struck.
	OBJ_FISSINS2      = 39,  // Jumps horizontally in and out of sand.
	OBJ_BOUNDS        = 40,  // Enemy path marker.
	OBJ_SMALL_EGG     = 41,  // Drops from pipe, can be bounced to dog.
	OBJ_BASKETBALL    = 42,  // Orange bouncing ball. Swishes hoops.
	OBJ_LAVAANIM      = 43,  // Causes a tile DMA to occur to animate lava.
	OBJ_SPOOKO        = 44,  // TODO Just a little skull sprite.
	OBJ_WIP           = 45,  // TODO Shows a "Work in Progress!" sign.
	OBJ_BGSCROLLY     = 46,  // Forces the camera Y scroll position.
	OBJ_FAKECUBE      = 47,  // Used above the boss arena.
	OBJ_RADIO         = 48,  // Releases an item (HP orb 4 in the original).
	OBJ_CHIMNEY       = 49,  // Releases an item when struck.
	OBJ_CORK          = 50,  // Releases an item when struck (HP orb 8 in the original).
	OBJ_BROKEN_EGG    = 51,  // The results of OBJ_EGG falling.
	OBJ_CHICK         = 52,  // Hatched out of OBJ_BROKEN_EGG.

	OBJ_TECHNOBG     = 119,  // Causes a tile DMA for the technozone FG.
	OBJ_BGTILE       = 120,  // Displays a background tile as a sprite.
	OBJ_COLUMNS      = 121,  // Tile DMA for columns foreground.
	OBJ_GRASSES      = 122,  // Tile DMA for grasses foreground.
	OBJ_PURPLETREE   = 123,  // Sprite pretending to be a purple tree top.
	OBJ_WNDWBACK     = 124,  // Blocks out window in the start of the game.
	OBJ_SCRLOCK      = 125,  // Locks scrolling vertically.
	OBJ_TITLE        = 126,  // Title screen object.
	OBJ_BOGOLOGO     = 127,  // Bogologo object.

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
	OBJ_PAUSE = 137,

	OBJ_TEMPLATE = 255,
	OBJ_INVALID = 256
} ObjType;

#endif  // OBJ_TYPES_H
