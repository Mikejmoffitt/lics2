#include "str.h"
#include <stdlib.h>

static Locale s_locale = LOCALE_JA;

static const char *strings_en[STR_INVALID] =
{
	// Dialogue for the pause screen.
//   XXXXXXXXXXXXXXXXXXXX x 13
	[STR_LYLE_TOO_WEAK] =
	"@LYLE IS TOO MUCH OF\n"
	"A WEAKLING TO PICK\n"
	"THESE UP JUST YET\n"
	"\n"
	"@TIME TO DO SOME\n"
	"SEARCHING",
	[STR_GET_MAP] =
	"@MAP NOW VIEWABLE ON\n"
	"PAUSE SCREEN\n"
	"\n"
	"@PRESS START DURING\n"
	"GAME TO PAUSE",
	[STR_GET_CUBE_LIFT] =
	"@STAND ON TOP OF\n"
	"CUBE & PRESS B\n"
	"TO LIFT\n"
	"\n"
	"@PRESS B TO THROW\n"
	"@UP & B FOR UPWARD\n"
	"THROW\n"
	"@DOWN & B FOR SHORT\n"
	"THROW\n"
	"@LEFT OR RIGHT & B\n"
	"FOR LONG THROW\n"
	"@CAREFUL NOT TO GET\n"
	"HIT BY THROWN CUBES",
	[STR_GET_CUBE_JUMP] =
	"@PRESS C WHILE\n"
	"JUMPING & HOLDING\n"
	"CUBE TO THROW CUBE\n"
	"DOWNWARDS & JUMP\n"
	"HIGHER",
	[STR_GET_CUBE_KICK] =
	"@STAND NEXT TO CUBE\n"
	"& PRESS B TO KICK\n",
	[STR_GET_ORANGE_CUBE] =
	"@LIFT THE LARGE\n"
	"ORANGE CUBES",
	[STR_GET_PHANTOM] =
	"@HOLD B TO CREATE\n"
	"PHANTOM CUBE\n"
	"\n"
	"@CONSUMES\n"
	"CUBE POINTS <CP>",
	[STR_GET_PHANTOM_DOUBLE_DAMAGE] =
	"@PHANTOM CUBE DAMAGE\n"
	"2X",
	[STR_GET_PHANTOM_HALF_TIME] =
	"@PHANTOM CUBE\n"
	"CREATION TIME HALVED",
	[STR_GET_PHANTOM_CHEAP] =
	"@CP CONSUMPTION\n"
	"HALVED",
	[STR_GET_HP_ORB] =
	"@MAXIMUM HP\n"
	"INCREASED 1 UNIT\n"
	"\n"
	"@HP RESTORED",
};

static const char *strings_ja[STR_INVALID] =
{
	// Dialogue for the pause screen.
//   ーーーーーーーーーーーーーーーーーーーー x 13
	[STR_LYLE_TOO_WEAK] =
	"@ライル　は　まだつよくない　ので\n"
	"キューブ　を　もちあげられません。\n"
	"\n"
	"@もっと　さぐったほう　がいいです。\n",
	[STR_GET_MAP] =
	"@ポーズがめんに　ちずがみえる　ように\n"
	"なりました。\n"
	"\n"
	"@スタートボタンを　おすとポーズします。\n",
	[STR_GET_CUBE_LIFT] =
	"@キューブのうえ　に　たちながら、\n"
	"B　ボタンを　おすと\n"
	"キューブを　もちあげます。\n"
	"\n"
	"@UP & B　ボタンを　おすと\n"
	"うえに　なげます。\n"
	"@DOWN & B　ボタンを　おすと\n"
	"ちかくに　なげます。\n"
	"@まえ　ほうこう & B　ボタン　をおす\n"
	"つよく　なげます。\n"
	"\n"
	"@とんでる　キューブが　あぶないですから\n"
	"きをつけて。",
	[STR_GET_CUBE_JUMP] =
	"@キューブ　を　もちながら　ジャンプして\n"
	"から　Cボタンを　おすと、\n"
	"キューブを　したに　なげます。\n",
	[STR_GET_CUBE_KICK] =
	"@キューブの　となりに　たちながら、\n"
	"Bボタンを　おすと　けとばします。",
	[STR_GET_ORANGE_CUBE] =
	"@オレンジのキューブがもちあげられます。\n",
	[STR_GET_PHANTOM] =
	"@Bボタンを　ながおし　したら、\n"
	"「ファントムキューブ」がつくられます。\n"
	"\n"
	"@「キューブ　ポイント」をつかいます。\n",
	[STR_GET_PHANTOM_DOUBLE_DAMAGE] =
	"@「ファントムキューブ」のダメージが\n"
	"あげました。",
	[STR_GET_PHANTOM_HALF_TIME] =
	"@「ファントムキューブ」つくるのは\n"
	"はやくなりました。",
	[STR_GET_PHANTOM_CHEAP] =
	"@「ファントムキューブ」の　ひようは\n"
	"はんぶん　になりました。",
	[STR_GET_HP_ORB] =
	"@HPのようりょうが　1ポイントに\n"
	"そうが　しました。"
	"\n"
	"@HPは　ほじょうしました。",
};

void str_set_locale(Locale locale)
{
	s_locale = locale;
}

const char *str_get(StringId id)
{
	const char *fetch = NULL;
	switch (s_locale)
	{
		default:
			return NULL;
		case LOCALE_EN:
			fetch = strings_en[id];
			break;
		case LOCALE_JA:
			fetch = strings_ja[id];
			break;
	}

	// If a locale does not define a certain string, default to English.
	if (fetch == NULL) fetch = strings_en[id];

	return fetch;
}
