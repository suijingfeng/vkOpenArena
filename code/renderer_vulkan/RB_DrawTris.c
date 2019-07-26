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

    memset(pInput->svars.colors, 255, pInput->numVertexes * 4 );
    //vk_rcdUpdateViewport(is2D, DEPTH_RANGE_ZERO);
    vk_shade( isMirror ? g_debugPipelines.tris_mirror : g_debugPipelines.tris, 
            pInput, &tr.whiteImage->descriptor_set, VK_FALSE, VK_TRUE);
}
