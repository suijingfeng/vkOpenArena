#include "tr_local.h"
#include "vk_instance.h"
#include "vk_pipelines.h"
#include "tr_shader.h"


struct GlobalPipelinesManager_t g_globalPipelines;


void vk_createStandardPipelines(void)
{      
    ri.Printf(PRINT_ALL, " Create skybox pipeline \n");
    {

        struct Vk_Pipeline_Def def;
        memset(&def, 0, sizeof(def));

        def.shader_type = ST_SINGLE_TEXTURE;
        def.state_bits = 0;
        def.face_culling = CT_FRONT_SIDED;
        def.polygon_offset = VK_FALSE;
        def.clipping_plane = VK_FALSE;
        def.mirror = VK_FALSE;
        
        vk_create_pipeline(&def, VK_FALSE, SHADOWS_RENDERING_DISABLED, &g_globalPipelines.skybox_pipeline);
    }

    ri.Printf(PRINT_ALL, " Create Q3 stencil shadows pipeline \n");
    {
        {
            struct Vk_Pipeline_Def def;
            memset(&def, 0, sizeof(def));


            def.polygon_offset = VK_FALSE;
            def.state_bits = 0;
            def.shader_type = ST_SINGLE_TEXTURE;
            def.clipping_plane = VK_FALSE;

            cullType_t cull_types[2] = {CT_FRONT_SIDED, CT_BACK_SIDED};
            VkBool32 mirror_flags[2] = {VK_TRUE, VK_FALSE};

            int i = 0; 
            int j = 0;

            for (i = 0; i < 2; i++)
            {
                def.face_culling = cull_types[i];
                for (j = 0; j < 2; j++)
                {
                    def.mirror = mirror_flags[j];
                    
                    vk_create_pipeline(&def, VK_FALSE ,SHADOWS_RENDERING_EDGES, 
                            &g_globalPipelines.shadow_volume_pipelines[i][j]);
                }
            }
        }

        {
            struct Vk_Pipeline_Def def;
            memset(&def, 0, sizeof(def));

            def.face_culling = CT_FRONT_SIDED;
            def.polygon_offset = VK_FALSE;
            def.state_bits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
            def.shader_type = ST_SINGLE_TEXTURE;
            def.clipping_plane = VK_FALSE;
            def.mirror = VK_FALSE;
            
            vk_create_pipeline(&def, VK_FALSE, SHADOWS_RENDERING_FULLSCREEN_QUAD, 
                    &g_globalPipelines.shadow_finish_pipeline);
        }
    }


    ri.Printf(PRINT_ALL, " Create fog and dlights pipeline \n");
    {
        struct Vk_Pipeline_Def def;
        memset(&def, 0, sizeof(def));


        def.shader_type = ST_SINGLE_TEXTURE;
        def.clipping_plane = VK_FALSE;
        def.mirror = VK_FALSE;

        unsigned int fog_state_bits[2] = {
            GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL,
            GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA
        };
        unsigned int dlight_state_bits[2] = {
            GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL,
            GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL
        };
        
        VkBool32 polygon_offset[2] = {VK_FALSE, VK_TRUE};

        int i = 0, j = 0, k = 0;

        for (i = 0; i < 2; i++)
        {
            unsigned fog_state = fog_state_bits[i];
            unsigned dlight_state = dlight_state_bits[i];

            for (j = 0; j < 3; j++)
            {
                def.face_culling = j; // cullType_t value

                for ( k = 0; k < 2; k++)
                {
                    def.polygon_offset = polygon_offset[k];

                    def.state_bits = fog_state;
                    vk_create_pipeline(&def, VK_FALSE, SHADOWS_RENDERING_DISABLED, 
                            &g_globalPipelines.fog_pipelines[i][j][k]);

                    def.state_bits = dlight_state;
                    vk_create_pipeline(&def, VK_FALSE, SHADOWS_RENDERING_DISABLED, 
                            &g_globalPipelines.dlight_pipelines[i][j][k]);
                }
            }
        }
    }
}


void vk_destroyGlobalStagePipeline(void)
{
    ri.Printf(PRINT_ALL, " Destroy global stage pipeline. \n");
    
    int i, j, k;

	qvkDestroyDescriptorSetLayout(vk.device, vk.set_layout, NULL); 
    qvkDestroyPipelineLayout(vk.device, vk.pipeline_layout, NULL);
    // You don't need to explicitly clean up descriptor sets,
    // because they will be automaticall freed when the descripter pool
    // is destroyed.
   	qvkDestroyDescriptorPool(vk.device, vk.descriptor_pool, NULL);    
    // 
    qvkDestroyPipeline(vk.device, g_globalPipelines.skybox_pipeline, NULL);
	for (i = 0; i < 2; i++)
		for (j = 0; j < 2; j++)
        {
			qvkDestroyPipeline(vk.device, g_globalPipelines.shadow_volume_pipelines[i][j], NULL);
		}
	
    qvkDestroyPipeline(vk.device, g_globalPipelines.shadow_finish_pipeline, NULL);
	
    
    for (i = 0; i < 2; i++) {
		for (j = 0; j < 3; j++)
			for (k = 0; k < 2; k++)
            {
				qvkDestroyPipeline(vk.device, g_globalPipelines.fog_pipelines[i][j][k], NULL);
				qvkDestroyPipeline(vk.device, g_globalPipelines.dlight_pipelines[i][j][k], NULL);
			}
    }

    memset(&g_globalPipelines, 0, sizeof(g_globalPipelines));
}
