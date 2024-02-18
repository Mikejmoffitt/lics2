#ifndef OBJ_ENDLYLE_H
#define OBJ_ENDLYLE_H

#include "obj.h"

#include <md/megadrive.h>

typedef struct O_EndLyle
{
	Obj head;
	CSprParam cspr[2];
	bool initialized;
} O_EndLyle;

void o_load_endlyle(Obj *o, uint16_t data);
void o_unload_endlyle(void);

#endif  // OBJ_ENDLYLE_H
