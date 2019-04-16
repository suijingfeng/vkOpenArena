#include "vk_pipelines.h"
#include "ref_import.h"
#include "vk_instance.h"

struct DebugPipelinesManager_t g_debugPipelines;


void vk_createDebugPipelines(void)
{
    ri.Printf(PRINT_ALL, " Create tris debug pipeline \n");
    {
        struct Vk_Pipeline_Def def;
        memset(&def, 0, sizeof(def));

        def.state_bits = GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE;
        vk_create_pipeline(&def, VK_FALSE, SHADOWS_RENDERING_DISABLED, &g_debugPipelines.tris);
    }
    
    ri.Printf(PRINT_ALL, " Create tris mirror debug pipeline \n");
    {
        struct Vk_Pipeline_Def def;
        memset(&def, 0, sizeof(def));

        def.state_bits = GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE;
        def.face_culling = CT_BACK_SIDED;
        vk_create_pipeline(&def, VK_FALSE, SHADOWS_RENDERING_DISABLED, &g_debugPipelines.tris_mirror);
    }

    ri.Printf(PRINT_ALL, " Create normals debug pipeline \n");
    {
        struct Vk_Pipeline_Def def;
        memset(&def, 0, sizeof(def));

        def.state_bits = GLS_DEPTHMASK_TRUE;
        vk_create_pipeline(&def, VK_TRUE, SHADOWS_RENDERING_DISABLED, &g_debugPipelines.normals);
    }


    ri.Printf(PRINT_ALL, " Create surface debug pipeline \n");
    {
        struct Vk_Pipeline_Def def;
        memset(&def, 0, sizeof(def));

        def.state_bits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
        vk_create_pipeline(&def, VK_FALSE, SHADOWS_RENDERING_DISABLED, &g_debugPipelines.surface_solid);
    }

    ri.Printf(PRINT_ALL, " Create surface debug outline pipeline \n");
    {
        struct Vk_Pipeline_Def def;
        memset(&def, 0, sizeof(def));

        def.state_bits = GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
        vk_create_pipeline(&def, VK_TRUE, SHADOWS_RENDERING_DISABLED, &g_debugPipelines.surface_outline);
    }
    
    ri.Printf(PRINT_ALL, " Create images debug pipeline \n");
    {
        struct Vk_Pipeline_Def def;
        memset(&def, 0, sizeof(def));

        def.state_bits = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
        vk_create_pipeline(&def, VK_FALSE, SHADOWS_RENDERING_DISABLED, &g_debugPipelines.images);
    }
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
