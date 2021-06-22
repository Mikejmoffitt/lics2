#ifndef OBJ_TEMPLATE_H
#define OBJ_TEMPLATE_H

#include "obj.h"

typedef struct O_Template
{
	Obj head;
} O_Template;

void o_load_template(Obj *o, uint16_t data);
void o_unload_template(void);

#endif  // OBJ_TEMPLATE_H
