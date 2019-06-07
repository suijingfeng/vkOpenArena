#ifndef R_SHADER_COMMAND_H_
#define R_SHADER_COMMAND_H_

#include "R_ShaderStage.h"
#include "tr_shader.h"
#include "R_ShaderCommonDef.h"


/*
====================================================================

TESSELATOR/SHADER DECLARATIONS

====================================================================
*/

typedef struct stageVars
{
	uint8_t     colors[SHADER_MAX_VERTEXES][4];
	float		texcoords[NUM_TEXTURE_BUNDLES][SHADER_MAX_VERTEXES][2];
} stageVars_t;

typedef struct shaderCommands_s 
{
	unsigned int indexes[SHADER_MAX_INDEXES];
	float		xyz[SHADER_MAX_VERTEXES][4];
	float		normal[SHADER_MAX_VERTEXES][4];
	float		texCoords[SHADER_MAX_VERTEXES][2][2];
	uint8_t     vertexColors[SHADER_MAX_VERTEXES][4];
	int			vertexDlightBits[SHADER_MAX_VERTEXES];

	stageVars_t	svars;

	uint8_t     constantColor255[SHADER_MAX_VERTEXES][4];

	struct shader_s * shader;
    float   shaderTime;
	int			fogNum;

	int			dlightBits;	// or together of all vertexDlightBits

	int			numIndexes;
	int			numVertexes;

	// info extracted from current shader
	int			numPasses;
	struct shaderStage_s **xstages;
} shaderCommands_t;



void R_ClearShaderCommand(void);
float R_GetTessShaderTime(void);


#endif
