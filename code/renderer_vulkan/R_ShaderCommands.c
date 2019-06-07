#include "ref_import.h"

#include "R_ShaderCommands.h"


shaderCommands_t tess;


void R_ClearShaderCommand(void)
{
	memset( &tess, 0, sizeof( tess ) );

    if ( (intptr_t)tess.xyz & 15 ) {
		ri.Printf( PRINT_ALL, "WARNING: tess.xyz not 16 byte aligned\n" );
	}
}

float R_GetTessShaderTime(void)
{
    return tess.shaderTime;
}
