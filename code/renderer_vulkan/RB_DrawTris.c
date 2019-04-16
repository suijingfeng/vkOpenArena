#include "RB_DrawTris.h"
#include "tr_globals.h"
#include "vk_shade_geometry.h"
#include "vk_pipelines.h"
//#include "tr_backend.h"
#include "R_ShaderCommands.h"



/*
================
Draws triangle outlines for debugging
================
*/
void RB_DrawTris (shaderCommands_t * pInput, int isMirror)
{
	updateCurDescriptor( tr.whiteImage->descriptor_set, 0);

    memset(pInput->svars.colors, 255, pInput->numVertexes * 4 );
    VkPipeline pipeline = isMirror ? g_debugPipelines.tris_mirror : g_debugPipelines.tris;
    vk_shade_geometry(pipeline, VK_FALSE, DEPTH_RANGE_ZERO, VK_TRUE);
}
