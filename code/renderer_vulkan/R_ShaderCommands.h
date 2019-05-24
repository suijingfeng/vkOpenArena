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
	vec2_t		texcoords[NUM_TEXTURE_BUNDLES][SHADER_MAX_VERTEXES];
} stageVars_t;

typedef struct shaderCommands_s 
{
	unsigned int indexes[SHADER_MAX_INDEXES];
	float		xyz[SHADER_MAX_VERTEXES][4];
	vec4_t		normal[SHADER_MAX_VERTEXES];
	vec2_t		texCoords[SHADER_MAX_VERTEXES][2];
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
	shaderStage_t	**xstages;
} shaderCommands_t;

extern shaderCommands_t	tess;

void R_ClearShaderCommand(void);
void RB_StageIteratorSky( shaderCommands_t * const pTess );

#endif
