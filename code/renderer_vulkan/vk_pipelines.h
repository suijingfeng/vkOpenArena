#ifndef VK_PIPELINES_H_
#define VK_PIPELINES_H_

#include "tr_shader.h"
#include "vk_shaders.h"


struct GlobalPipelinesManager_t
{
	//
	// Standard pipelines.
	//
	VkPipeline skybox_pipeline;

	// dim 0 is based on fogPass_t: 0 - corresponds to FP_EQUAL, 1 - corresponds to FP_LE.
	// dim 1 is directly a cullType_t enum value.
	// dim 2 is a polygon offset value (0 - off, 1 - on).
	VkPipeline fog_pipelines[2][3][2];

	// dim 0 is based on dlight additive flag: 0 - not additive, 1 - additive
	// dim 1 is directly a cullType_t enum value.
	// dim 2 is a polygon offset value (0 - off, 1 - on).
	VkPipeline dlight_pipelines[2][3][2];
};

struct DebugPipelinesManager_t {
	// debug visualization pipelines
	VkPipeline tris;
	VkPipeline tris_mirror;
	VkPipeline normals;
	VkPipeline surface_solid;
	VkPipeline surface_outline;
	VkPipeline images;
};


void vk_create_pipeline(
        uint32_t state_bits,
        enum Vk_Shader_Type shader_type,
        enum CullType_t face_culling,
        VkBool32 clipping_plane,
        VkBool32 mirror,
        VkBool32 polygon_offset,
        VkBool32 isLine, 
        VkPipeline* const pPipeLine);

void vk_createStandardPipelines(void);
void vk_createDebugPipelines(void);

// create pipelines for each stage
void vk_create_shader_stage_pipelines(shaderStage_t *pStage, shader_t* pShader);
void vk_destroyShaderStagePipeline(void);


void vk_createPipelineLayout(VkDescriptorSetLayout desc_layout, VkPipelineLayout * const pPLayout);
void vk_destroy_pipeline_layout(void);


void vk_InitShaderStagePipeline(void);

void vk_destroyGlobalStagePipeline(void);
void vk_destroyDebugPipelines(void);

void R_PipelineList_f(void);


extern struct GlobalPipelinesManager_t g_globalPipelines;
extern struct DebugPipelinesManager_t g_debugPipelines;


#endif
