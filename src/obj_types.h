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
	OBJ_BOSS2         = 35,  // The second boss.
	OBJ_VYLE1         = 36,  // First Vyle boss, prior to transformation.
	OBJ_VYLE2         = 37,  // Vyle for the cutscene and second boss phase.
	OBJ_EGG           = 38,  // Large teal egg that falls when struck.
	OBJ_FISSINS2      = 39,  // Jumps horizontally in and out of sand.
	OBJ_BOUNDS        = 40,  // Enemy path marker.
	OBJ_SMALL_EGG     = 41,  // Drops from pipe, can be bounced to dog.
	OBJ_BASKETBALL    = 42,  // Orange bouncing ball. Swishes hoops.
	OBJ_LAVAANIM      = 43,  // Causes a tile DMA to occur to animate lava.
	OBJ_SPOOKO        = 44,  // Just a little skull sprite.
	OBJ_WIP           = 45,  // delete
	OBJ_BGSCROLL      = 46,  // Forces the camera Y scroll position.
	OBJ_FAKECUBE      = 47,  // Used above the boss arena.
	OBJ_RADIO         = 48,  // Releases an item (HP orb 4 in the original).
	OBJ_CHIMNEY       = 49,  // Releases an item when struck.
	OBJ_CORK          = 50,  // Releases an item when struck.
	OBJ_BROKEN_EGG    = 51,  // The results of OBJ_EGG falling.
	OBJ_CHICK         = 52,  // Hatched out of OBJ_BROKEN_EGG.
	OBJ_LAVAKILL      = 53,  // Kills Lyle if he goes below and to the right.
	OBJ_ROCKMAN_DOOR  = 54,  // Door that seals off a boss arena.
	OBJ_PSYCHOWAVE    = 55,  // Kitty powered psychowave.
	OBJ_KEDDUMS       = 56,  // Lyle's cat in the psychowave.
	OBJ_TVSCREEN      = 57,  // Used in the teleporter room.
	OBJ_SECRETTV      = 58,  // Used in the hidden blue room.
	OBJ_POINTYSIGN    = 59,  // Sign cautioning about "LARGE POINTY THING".
	OBJ_CLOAK         = 60,  // Vyle's discarded cloak.
	OBJ_BIGEXPLOSION  = 61,  // Big explosion.

	OBJ_ENDLYLE      = 116,  // Ending lyle portrait.
	OBJ_STAFFROLL    = 117,  // Staff roll script.
	OBJ_GAMEOVER     = 118,  // Game over cube / sequence.
	OBJ_TECHNOBG     = 119,  // Tile DMA for technozone foreground.
	OBJ_BGTILE       = 120,  // Displays a background tile as a sprite.
	OBJ_COLUMNS      = 121,  // Tile DMA for columns foreground.
	OBJ_GRASSES      = 122,  // Tile DMA for grasses foreground.
	OBJ_PURPLETREE   = 123,  // Sprite pretending to be a purple tree top.
	OBJ_WNDWBACK     = 124,  // Blocks out the house's windows during the intro.
	OBJ_SCRLOCK      = 125,  // Locks scrolling vertically.
	OBJ_TITLE        = 126,  // Title screen object.
	OBJ_BOGOLOGO     = 127,  // Bogologo object.

	// Objects 128 and above are special objects added by the engine, rather
	// than being found in map data.i
	OBJ_EXPLODER = 135,

	OBJ_TEMPLATE = 255,
	OBJ_INVALID = 256
} ObjType;

#endif  // OBJ_TYPES_H
