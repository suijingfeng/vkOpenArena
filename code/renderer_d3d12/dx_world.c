
#include "dx_world.h"

Dx_World dx_world;


float * R_GetModelViewMatPtr(void)
{
	return dx_world.modelview_transform;
}