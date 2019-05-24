#include "RB_DrawTris.h"
#include "tr_globals.h"
#include "vk_shade_geometry.h"
#include "vk_pipelines.h"


/*
================
Draws triangle outlines for debugging
================
*/
void RB_DrawTris (shaderCommands_t * const pInput, VkBool32 isMirror, VkBool32 is2D)
{
	// updateCurDescriptor( tr.whiteImage->descriptor_set, 0);

    memset(pInput->svars.colors, 255, pInput->numVertexes * 4 );
    VkPipeline pipeline = isMirror ? g_debugPipelines.tris_mirror : g_debugPipelines.tris;
    vk_shade_geometry(pipeline, VK_FALSE, is2D, DEPTH_RANGE_ZERO, VK_TRUE);
}
