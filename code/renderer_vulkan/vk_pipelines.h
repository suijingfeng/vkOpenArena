#ifndef VK_PIPELINES_H_
#define VK_PIPELINES_H_

#include "tr_local.h"
#include "vk_shaders.h"

// used with cg_shadows == 2
enum Vk_Shadow_Phase {
    SHADOWS_RENDERING_DISABLED,
	SHADOWS_RENDERING_EDGES,
    SHADOWS_RENDERING_FULLSCREEN_QUAD
};


struct Vk_Pipeline_Def {
    VkPipeline pipeline;
    uint32_t state_bits; // GLS_XXX flags
	cullType_t face_culling;// cullType_t
	VkBool32 polygon_offset;
	VkBool32 clipping_plane;
	VkBool32 mirror;
	VkBool32 line_primitives;
    enum Vk_Shader_Type shader_type;
	enum Vk_Shadow_Phase shadow_phase;
};


struct GlobalPipelinesManager_t
{
	//
	// Standard pipelines.
	//
	VkPipeline skybox_pipeline;

	// dim 0: 0 - front side, 1 - back size
	// dim 1: 0 - normal view, 1 - mirror view
	VkPipeline shadow_volume_pipelines[2][2];
	VkPipeline shadow_finish_pipeline;

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


void vk_create_pipeline(const struct Vk_Pipeline_Def* def, VkPipeline* pPipeLine);

void vk_createStandardPipelines(void);
void vk_createDebugPipelines(void);

// create pipelines for each stage
void vk_create_shader_stage_pipelines(shaderStage_t *pStage, shader_t* pShader);
void vk_createPipelineLayout(void);

void vk_destroyShaderStagePipeline(void);
void vk_destroyGlobalStagePipeline(void);
void vk_destroyDebugPipelines(void);

void R_PipelineList_f(void);



extern struct GlobalPipelinesManager_t g_stdPipelines;
extern struct DebugPipelinesManager_t g_debugPipelines;


#endif
