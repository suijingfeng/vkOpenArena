#include "RB_DrawTris.h"
#include "tr_globals.h"
#include "vk_shade_geometry.h"
#include "vk_pipelines.h"


/*
================
Draws triangle outlines for debugging
================
*/
void RB_DrawTris (struct shaderCommands_s * const pInput, VkBool32 isMirror, VkBool32 is2D)
{
	// updateCurDescriptor( tr.whiteImage->descriptor_set, 0);

    memset(pInput->svars.colors, 255, pInput->numVertexes * 4 );
    //vk_rcdUpdateViewport(is2D, DEPTH_RANGE_ZERO);
    VkPipeline pipeline = isMirror ? g_debugPipelines.tris_mirror : g_debugPipelines.tris;
    vk_shade_geometry(pipeline, pInput, VK_FALSE, VK_TRUE);
}
