#include "VKimpl.h"
#include "vk_instance.h"
#include "vk_frame.h"
#include "vk_cmd.h"
#include "vk_depth_attachment.h"
#include "vk_pipelines.h"
#include "vk_shade_geometry.h"
#include "vk_shaders.h"
#include "vk_descriptor_sets.h"
#include "glConfig.h"
#include "ref_import.h" 


// vk_init have nothing to do with tr_init
// vk_instance should be small
// 
// After calling this function we get fully functional vulkan subsystem.
void vk_initialize(void)
{
    // This function is responsible for initializing a valid Vulkan subsystem.
    vk_createWindowImpl();

    vk_getProcAddress(); 
 
	// Swapchain. vk.physical_device required to be init. 
	vk_createSwapChain(vk.device, vk.surface, vk.surface_format);

	// Sync primitives.
    vk_create_sync_primitives();

	// we have to create a command pool before we can create command buffers
    // command pools manage the memory that is used to store the buffers and
    // command buffers are allocated from them.
    vk_create_command_pool(&vk.command_pool);
    
    vk_create_command_buffer(vk.command_pool, &vk.command_buffer);


    int width;
    int height;

    R_GetWinResolution(&width, &height);

    // Depth attachment image.
    vk_createDepthAttachment(width, height);

    vk_createFrameBuffers(width, height);

	// Pipeline layout.
	// You can use uniform values in shaders, which are globals similar to
    // dynamic state variables that can be changes at the drawing time to
    // alter the behavior of your shaders without having to recreate them.
    // They are commonly used to create texture samplers in the fragment 
    // shader. The uniform values need to be specified during pipeline
    // creation by creating a VkPipelineLayout object.
    
    // MAX_DRAWIMAGES = 2048
    vk_createDescriptorPool(2048);
    // The set of sets that are accessible to a pipeline are grouped into 
    // pipeline layout. Pipelines are created with respect to this pipeline
    // layout. Those descriptor sets can be bound into command buffers 
    // along with compatible pipelines to allow those pipeline to access
    // the resources in them.
    vk_createDescriptorSetLayout();
    // These descriptor sets layouts are aggregated into a single pipeline layout.
    vk_createPipelineLayout();

	//
	vk_createVertexBuffer();
    vk_createIndexBuffer();
	//
	// Shader modules.
	//
	vk_loadShaderModules();

	//
	// Standard pipelines.
	//
    vk_createStandardPipelines();
    //
    // debug pipelines
    // 
    vk_createDebugPipelines();

    vk.isInitialized = VK_TRUE;
}


VkBool32 isVKinitialied(void)
{
    return vk.isInitialized;
}

// Shutdown vulkan subsystem by releasing resources acquired by Vk_Instance.
void vk_shutdown(void)
{
    ri.Printf( PRINT_ALL, "vk_shutdown()\n" );

    vk_destroyDepthAttachment();

    vk_destroyFrameBuffers();

    vk_destroy_shading_data();

    vk_destroy_sync_primitives();
    
    vk_destroyShaderModules();

//  Those pipelines can be used across different maps ?
//  so we only destroy it when the client quit.
    vk_destroyGlobalStagePipeline();
    vk_destroyDebugPipelines();

    vk_destroy_pipeline_layout();

    vk_destroy_descriptor_pool();

    vk_destroy_commands();
	
    vk_clearProcAddress();

    ri.Printf( PRINT_ALL, " clear vk struct: vk \n" );
	memset(&vk, 0, sizeof(vk));
}

