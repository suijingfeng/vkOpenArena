#include "vk_pipelines.h"
#include "ref_import.h"
#include "vk_instance.h"

struct DebugPipelinesManager_t g_debugPipelines;


void vk_createDebugPipelines(void)
{
    ri.Printf(PRINT_ALL, " Create tris debug pipeline \n");
    vk_create_pipeline( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE, 
            ST_SINGLE_TEXTURE, CT_FRONT_SIDED, SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, VK_FALSE, VK_FALSE, VK_FALSE,
            &g_debugPipelines.tris );

    // why is Mirror not true ???
    ri.Printf(PRINT_ALL, " Create tris mirror debug pipeline \n");
    vk_create_pipeline( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE, 
            ST_SINGLE_TEXTURE, CT_BACK_SIDED, SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, VK_FALSE, VK_FALSE, VK_FALSE,
            &g_debugPipelines.tris_mirror );


    ri.Printf(PRINT_ALL, " Create normals debug pipeline \n");
    vk_create_pipeline( GLS_DEPTHMASK_TRUE, 
            ST_SINGLE_TEXTURE, CT_FRONT_SIDED, SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, VK_FALSE, VK_FALSE, VK_TRUE,
            &g_debugPipelines.normals );


    ri.Printf(PRINT_ALL, " Create surface debug solid pipeline \n");
    vk_create_pipeline( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE, 
            ST_SINGLE_TEXTURE, CT_FRONT_SIDED, SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, VK_FALSE, VK_FALSE, VK_FALSE,
            &g_debugPipelines.surface_solid );


    ri.Printf(PRINT_ALL, " Create surface debug outline pipeline \n");    
    vk_create_pipeline(
        GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE, 
            ST_SINGLE_TEXTURE, CT_FRONT_SIDED, SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, VK_FALSE, VK_FALSE, VK_TRUE,
            &g_debugPipelines.surface_outline );



    ri.Printf(PRINT_ALL, " Create images debug pipeline \n");
    vk_create_pipeline( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA, 
            ST_SINGLE_TEXTURE, CT_FRONT_SIDED, SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, VK_FALSE, VK_FALSE, VK_FALSE,
            &g_debugPipelines.tris );

}


void vk_destroyDebugPipelines(void)
{
    ri.Printf(PRINT_ALL, " Destroy debug stage pipeline. \n");

	qvkDestroyPipeline(vk.device, g_debugPipelines.tris, NULL);
	qvkDestroyPipeline(vk.device, g_debugPipelines.tris_mirror, NULL);
	qvkDestroyPipeline(vk.device, g_debugPipelines.normals, NULL);
	qvkDestroyPipeline(vk.device, g_debugPipelines.surface_solid, NULL);
	qvkDestroyPipeline(vk.device, g_debugPipelines.surface_outline, NULL);
	qvkDestroyPipeline(vk.device, g_debugPipelines.images, NULL);

    memset(&g_debugPipelines, 0, sizeof(g_debugPipelines));
}
