#ifndef VK_SHADE_GEOMETRY
#define VK_SHADE_GEOMETRY

#include "vk_instance.h"


enum Vk_Depth_Range {
	DEPTH_RANGE_NORMAL, // [0..1]
	DEPTH_RANGE_ZERO, // [0..0]
	DEPTH_RANGE_ONE, // [1..1]
	DEPTH_RANGE_WEAPON // [0..0.3]
};

struct shaderCommands_s;

const float * getptr_modelview_matrix(void);

void set_modelview_matrix(const float mv[16]);
void R_Set2dProjectMatrix(float width, float height);


void vk_shade(VkPipeline pipeline, struct shaderCommands_s * const pTess,
        VkDescriptorSet* const pDesSet, VkBool32 multitexture, VkBool32 indexed);

void vk_UploadXYZI(const float (* const pXYZ)[4], uint32_t nVertex, 
        const uint32_t* const pIdx, uint32_t nIndex);

void updateMVP(VkBool32 isPortal, VkBool32 is2D, const float mvMat4x4[16]);
void vk_resetGeometryBuffer(void);

void vk_createVertexBuffer(void);
void vk_createIndexBuffer(void);

VkBuffer vk_getIndexBuffer(void);
VkBuffer vk_getVertexBuffer(void);

void vk_destroy_shading_data(void);

void vk_rcdUpdateViewport(VkBool32 is2D, enum Vk_Depth_Range depRg);

void vk_clearDepthStencilAttachments(void);

void RB_StageIteratorGeneric( struct shaderCommands_s * const pTess, VkBool32 isPortal, VkBool32 is2d );
#endif
