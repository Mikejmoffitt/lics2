// UTF-8 string resources, for localized text data.
#ifndef STR_H
#define STR_H

typedef enum Locale
{
	LOCALE_EN,
	LOCALE_JA,
} Locale;

typedef enum StringId
{
	STR_NULL,
	STR_LYLE_TOO_WEAK,
	STR_GET_MAP,
	STR_GET_CUBE_LIFT,
	STR_GET_CUBE_JUMP,
	STR_GET_CUBE_KICK,
	STR_GET_ORANGE_CUBE,
	STR_GET_PHANTOM,
	STR_GET_PHANTOM_DOUBLE_DAMAGE,
	STR_GET_PHANTOM_HALF_TIME,
	STR_GET_PHANTOM_CHEAP,
	STR_GET_HP_ORB,
	STR_GET_CP_ORB,
	STR_BUTTON_CHECK,
	STR_GAME_WIP,
	STR_INVALID
} StringId;

void str_set_locale(Locale locale);

const char *str_get(StringId id);

#endif // STR_H
