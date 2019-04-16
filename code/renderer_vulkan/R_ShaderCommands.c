
#include "R_ShaderCommands.h"


shaderCommands_t tess;


void R_ClearShaderCommand(void)
{
	memset( &tess, 0, sizeof( tess ) );
}
