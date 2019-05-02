#include "vk_instance.h"
#include "vk_pipelines.h"
#include "tr_shader.h"


struct GlobalPipelinesManager_t g_globalPipelines;


void vk_createStandardPipelines(void)
{
    ////
    ri.Printf(PRINT_ALL, " Create skybox pipeline. \n");
   
    vk_create_pipeline( 0, 
            ST_SINGLE_TEXTURE, CT_FRONT_SIDED, SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, VK_FALSE, VK_FALSE, VK_FALSE,
            &g_globalPipelines.skybox_pipeline );

    
    ////
    ri.Printf(PRINT_ALL, " Create Q3 stencil shadows volume pipelines. \n");
    
    vk_create_pipeline( 0, 
            ST_SINGLE_TEXTURE, CT_FRONT_SIDED, SHADOWS_RENDERING_EDGES, 
            VK_FALSE, VK_FALSE, VK_FALSE, VK_FALSE,
            &g_globalPipelines.shadow_volume_pipelines[0][0] );

    vk_create_pipeline( 0, 
            ST_SINGLE_TEXTURE, CT_FRONT_SIDED, SHADOWS_RENDERING_EDGES, 
            VK_FALSE, VK_TRUE, VK_FALSE, VK_FALSE,
            &g_globalPipelines.shadow_volume_pipelines[0][1] );

    vk_create_pipeline( 0, 
            ST_SINGLE_TEXTURE, CT_BACK_SIDED, SHADOWS_RENDERING_EDGES, 
            VK_FALSE, VK_FALSE, VK_FALSE, VK_FALSE,
            &g_globalPipelines.shadow_volume_pipelines[1][0] );

    vk_create_pipeline( 0, 
            ST_SINGLE_TEXTURE, CT_BACK_SIDED, SHADOWS_RENDERING_EDGES, 
            VK_FALSE, VK_TRUE, VK_FALSE, VK_FALSE,
            &g_globalPipelines.shadow_volume_pipelines[1][1] );


    ////
    ri.Printf(PRINT_ALL, " Create Q3 stencil shadows finish pipeline. \n");

    vk_create_pipeline( 
            GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO, 
            ST_SINGLE_TEXTURE, 
            CT_FRONT_SIDED,
            SHADOWS_RENDERING_FULLSCREEN_QUAD, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE,
            VK_FALSE,
            &g_globalPipelines.shadow_finish_pipeline );


    ////
    ri.Printf(PRINT_ALL, " Create fog pipelines. \n");

    unsigned int fog_state_bits[2] = {
        GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL,
        GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA
    };

///////////////////  i = 0
// j = 0
    vk_create_pipeline( fog_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_FRONT_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[0][0][0] );

    vk_create_pipeline( fog_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_FRONT_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[0][0][1] );

    // j = 1
    vk_create_pipeline( fog_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_BACK_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[0][1][0] );

    vk_create_pipeline( fog_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_BACK_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[0][1][1] );

    // j = 2
    vk_create_pipeline( fog_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_TWO_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[0][2][0] );

    vk_create_pipeline( fog_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_TWO_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[0][2][1] );

///////////////////  i = 1

    // j = 0
    vk_create_pipeline( fog_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_FRONT_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[1][0][0] );

    vk_create_pipeline( fog_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_FRONT_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[1][0][1] );

    // j = 1
    vk_create_pipeline( fog_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_BACK_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[1][1][0] );

    vk_create_pipeline( fog_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_BACK_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[1][1][1] );

    // j = 2
    vk_create_pipeline( fog_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_TWO_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[1][2][0] );

    vk_create_pipeline( fog_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_TWO_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.fog_pipelines[1][2][1] );




    ri.Printf(PRINT_ALL, " Create dlights pipelines. \n");
    unsigned int dlight_state_bits[2] = {
        GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL,
        GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL
    };

    // j = 0
    vk_create_pipeline( dlight_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_FRONT_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[0][0][0] );

    vk_create_pipeline( dlight_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_FRONT_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[0][0][1] );

    // j = 1
    vk_create_pipeline( dlight_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_BACK_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[0][1][0] );

    vk_create_pipeline( dlight_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_BACK_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[0][1][1] );

    // j = 2
    vk_create_pipeline( dlight_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_TWO_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[0][2][0] );

    vk_create_pipeline( dlight_state_bits[0], 
            ST_SINGLE_TEXTURE, 
            CT_TWO_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[0][2][1] );

///////////////////  i = 1
    // j = 0
    vk_create_pipeline( dlight_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_FRONT_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[1][0][0] );

    vk_create_pipeline( dlight_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_FRONT_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[1][0][1] );

    // j = 1
    vk_create_pipeline( dlight_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_BACK_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[1][1][0] );

    vk_create_pipeline( dlight_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_BACK_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[1][1][1] );

    // j = 2
    vk_create_pipeline( dlight_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_TWO_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_FALSE, // polygon_offset[0]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[1][2][0] );

    vk_create_pipeline( dlight_state_bits[1], 
            ST_SINGLE_TEXTURE, 
            CT_TWO_SIDED,
            SHADOWS_RENDERING_DISABLED, 
            VK_FALSE, 
            VK_FALSE, 
            VK_TRUE, // polygon_offset[1]
            VK_FALSE,
            &g_globalPipelines.dlight_pipelines[1][2][1] );
}


void vk_destroyGlobalStagePipeline(void)
{
    ri.Printf(PRINT_ALL, " Destroy global stage pipeline. \n");

    int i, j, k;

    qvkDestroyPipeline(vk.device, g_globalPipelines.skybox_pipeline, NULL);
    ri.Printf(PRINT_ALL, " Skybox pipeline destroyed. \n");

	for (i = 0; i < 2; i++)
    {
		for (j = 0; j < 2; j++)
        {
			qvkDestroyPipeline(vk.device, g_globalPipelines.shadow_volume_pipelines[i][j], NULL);
		}
    }
    ri.Printf(PRINT_ALL, " Stencil shadows volume pipelines destroyed. \n");

    qvkDestroyPipeline(vk.device, g_globalPipelines.shadow_finish_pipeline, NULL);
    ri.Printf(PRINT_ALL, " Stencil shadows finish pipelines destroyed. \n");  
	
    for (i = 0; i < 2; i++) {
		for (j = 0; j < 3; j++)
			for (k = 0; k < 2; k++)
            {
				qvkDestroyPipeline(vk.device, g_globalPipelines.fog_pipelines[i][j][k], NULL);
			}
    }

    ri.Printf(PRINT_ALL, " Fog pipeline destroyed. \n");

    for (i = 0; i < 2; i++) {
		for (j = 0; j < 3; j++)
			for (k = 0; k < 2; k++)
            {
				qvkDestroyPipeline(vk.device, g_globalPipelines.dlight_pipelines[i][j][k], NULL);
			}
    }

    ri.Printf(PRINT_ALL, " Dlights pipeline destroyed. \n");



    

    memset(&g_globalPipelines, 0, sizeof(g_globalPipelines));
}
