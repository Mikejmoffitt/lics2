#ifndef OBJECTS_H
#define OBJECTS_H

#include "../../prj/src/obj_types.h"

typedef struct ObjInfo
{
	char name[10];
	int width;
	int height;
} ObjInfo;

const ObjInfo *object_get_info(ObjType i);

#endif
