#include "vk_shade_geometry.h"
#include "vk_instance.h"
#include "tr_globals.h"
#include "tr_cvar.h"
#include "vk_image.h"
#include "vk_pipelines.h"
#include "matrix_multiplication.h"
#include "tr_backend.h"
#include "glConfig.h"
#include "R_PortalPlane.h"
#include "tr_light.h"
#include "tr_shader.h"
#include "R_ShaderCommands.h"
#include "tr_shade.h"
#include "ref_import.h" 

void R_GetFogArray(fog_t **ppFogs, uint32_t* pNum);

static void R_SetTessFogColor(unsigned char (*pcolor)[4], 
        uint32_t fnum, uint32_t nVerts);

#define VERTEX_CHUNK_SIZE   (768 * 1024)
#define INDEX_BUFFER_SIZE   (2 * 1024 * 1024)

#define XYZ_SIZE            (4 * VERTEX_CHUNK_SIZE)
#define COLOR_SIZE          (1 * VERTEX_CHUNK_SIZE)
#define ST0_SIZE            (2 * VERTEX_CHUNK_SIZE)
#define ST1_SIZE            (2 * VERTEX_CHUNK_SIZE)

#define XYZ_OFFSET          0
#define COLOR_OFFSET        (XYZ_OFFSET + XYZ_SIZE)
#define ST0_OFFSET          (COLOR_OFFSET + COLOR_SIZE)
#define ST1_OFFSET          (ST0_OFFSET + ST0_SIZE)

struct ShadingData_t
{
    // Buffers represent linear arrays of data which are used for various purposes
    // by binding them to a graphics or compute pipeline via descriptor sets or 
    // via certain commands,  or by directly specifying them as parameters to 
    // certain commands. Buffers are represented by VkBuffer handles:
	VkBuffer vertex_buffer;
	unsigned char* vertex_buffer_ptr ; // pointer to mapped vertex buffer
	uint32_t xyz_elements;
	uint32_t color_st_elements;

	VkBuffer index_buffer;
	unsigned char* index_buffer_ptr; // pointer to mapped index buffer
	uint32_t index_buffer_offset;

	// host visible memory that holds both vertex and index data
	VkDeviceMemory vertex_buffer_memory;
	VkDeviceMemory index_buffer_memory;
    VkDescriptorSet curDescriptorSets[2];

    // This flag is used to decide whether framebuffer's depth attachment should be cleared
    // with vmCmdClearAttachment (dirty_depth_attachment == true), or it have just been
    // cleared by render pass instance clear op (dirty_depth_attachment == false).

    VkBool32 s_depth_attachment_dirty;
};

struct ShadingData_t shadingDat;


VkBuffer vk_getIndexBuffer(void)
{
    return shadingDat.index_buffer;
}

VkBuffer vk_getVertexBuffer(void)
{
    return shadingDat.vertex_buffer;
}

static float s_modelview_matrix[16] QALIGN(16);


static float s_ProjectMat2d[16] QALIGN(16);


void R_Set2dProjectMatrix(float width, float height)
{
    s_ProjectMat2d[0] = 2.0f / width; 
    s_ProjectMat2d[1] = 0.0f; 
    s_ProjectMat2d[2] = 0.0f;
    s_ProjectMat2d[3] = 0.0f;

    s_ProjectMat2d[4] = 0.0f; 
    s_ProjectMat2d[5] = 2.0f / height; 
    s_ProjectMat2d[6] = 0.0f;
    s_ProjectMat2d[7] = 0.0f;

    s_ProjectMat2d[8] = 0.0f; 
    s_ProjectMat2d[9] = 0.0f; 
    s_ProjectMat2d[10] = 1.0f; 
    s_ProjectMat2d[11] = 0.0f;

    s_ProjectMat2d[12] = -1.0f; 
    s_ProjectMat2d[13] = -1.0f; 
    s_ProjectMat2d[14] = 0.0f;
    s_ProjectMat2d[15] = 1.0f;
}


void set_modelview_matrix(const float mv[16])
{
    memcpy(s_modelview_matrix, mv, 64);
}


const float * getptr_modelview_matrix()
{
    return s_modelview_matrix;
}


// Vulkan memory is broken up into two categories, host memory and device memory.
// Host memory is memory needed by the Vulkan implementation for 
// non-device-visible storage. Allocations returned by vkAllocateMemory
// are guaranteed to meet any alignment requirement of the implementation
//
// Host access to buffer must be externally synchronized

void vk_createVertexBuffer(void)
{
    ri.Printf(PRINT_ALL, " Create vertex buffer: shadingDat.vertex_buffer \n");

    vk_createBufferResource( XYZ_SIZE + COLOR_SIZE + ST0_SIZE + ST1_SIZE, 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            &shadingDat.vertex_buffer,
            &shadingDat.vertex_buffer_memory );

    void* data;
    VK_CHECK(qvkMapMemory(vk.device, shadingDat.vertex_buffer_memory, 0, VK_WHOLE_SIZE, 0, &data));
    shadingDat.vertex_buffer_ptr = (unsigned char*)data;
}



void vk_createIndexBuffer(void)
{
    ri.Printf(PRINT_ALL, " Create index buffer: shadingDat.index_buffer \n");

    vk_createBufferResource( INDEX_BUFFER_SIZE, 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            &shadingDat.index_buffer, &shadingDat.index_buffer_memory);

    void* data;
    VK_CHECK(qvkMapMemory(vk.device, shadingDat.index_buffer_memory, 0, VK_WHOLE_SIZE, 0, &data));
    shadingDat.index_buffer_ptr = (unsigned char*)data;
}


// Descriptors and Descriptor Sets
// A descriptor is a special opaque shader variable that shaders use to access buffer 
// and image resources in an indirect fashion. It can be thought of as a "pointer" to
// a resource.  The Vulkan API allows these variables to be changed between draw
// operations so that the shaders can access different resources for each draw.

// A descriptor set is called a "set" because it can refer to an array of homogenous
// resources that can be described with the same layout binding. one possible way to
// use multiple descriptors is to construct a descriptor set with two descriptors, 
// with each descriptor referencing a separate texture. Both textures are therefore
// available during a draw. A command in a command buffer could then select the texture
// to use by specifying the index of the desired texture. To describe a descriptor set,
// you use a descriptor set layout.

// Descriptor sets corresponding to bound texture images.

// outside of TR since it shouldn't be cleared during ref re-init
// the renderer front end should never modify glstate_t
//typedef struct {



void updateCurDescriptor( VkDescriptorSet curDesSet, uint32_t tmu)
{
    shadingDat.curDescriptorSets[tmu] = curDesSet;
}



void vk_shade_geometry(VkPipeline pipeline, VkBool32 multitexture, enum Vk_Depth_Range depRg, VkBool32 indexed)
{
	// configure vertex data stream
	VkBuffer bufs[3] = { shadingDat.vertex_buffer, shadingDat.vertex_buffer, shadingDat.vertex_buffer };
	VkDeviceSize offs[3] = {
		COLOR_OFFSET + shadingDat.color_st_elements * 4, // sizeof(color4ub_t)
		ST0_OFFSET   + shadingDat.color_st_elements * sizeof(vec2_t),
		ST1_OFFSET   + shadingDat.color_st_elements * sizeof(vec2_t)
	};

    // color, sizeof(color4ub_t)
    if ((shadingDat.color_st_elements + tess.numVertexes) * 4 > COLOR_SIZE)
        ri.Error(ERR_DROP, "vulkan: vertex buffer overflow (color) %d \n", 
                (shadingDat.color_st_elements + tess.numVertexes) * 4);

    unsigned char* dst_color = shadingDat.vertex_buffer_ptr + offs[0];
    memcpy(dst_color, tess.svars.colors, tess.numVertexes * 4);
    // st0

    unsigned char* dst_st0 = shadingDat.vertex_buffer_ptr + offs[1];
    memcpy(dst_st0, tess.svars.texcoords[0], tess.numVertexes * sizeof(vec2_t));

	// st1
	if (multitexture)
    {
		unsigned char* dst = shadingDat.vertex_buffer_ptr + offs[2];
		memcpy(dst, tess.svars.texcoords[1], tess.numVertexes * sizeof(vec2_t));
	}

	qvkCmdBindVertexBuffers(vk.command_buffer, 1, multitexture ? 3 : 2, bufs, offs);
	shadingDat.color_st_elements += tess.numVertexes;

	// bind descriptor sets

//    vkCmdBindDescriptorSets causes the sets numbered [firstSet.. firstSet+descriptorSetCount-1] to use
//    the bindings stored in pDescriptorSets[0..descriptorSetCount-1] for subsequent rendering commands 
//    (either compute or graphics, according to the pipelineBindPoint).
//    Any bindings that were previously applied via these sets are no longer valid.

	qvkCmdBindDescriptorSets(vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
        vk.pipeline_layout, 0, (multitexture ? 2 : 1), shadingDat.curDescriptorSets, 0, NULL);

    // bind pipeline
	qvkCmdBindPipeline(vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	// configure pipeline's dynamic state
    VkViewport viewport;

//    VkRect2D scissor = vk.renderArea; 
    //, VkRect2D* const pRect , &scissor
    //vk_setViewportScissor(backEnd.projection2D, depRg, &viewport);

	if (backEnd.projection2D)
	{
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = vk.renderArea.extent.width;
		viewport.height = vk.renderArea.extent.height;
	}
	else
	{
		int X = backEnd.viewParms.viewportX;
		int Y = backEnd.viewParms.viewportY;
		int W = backEnd.viewParms.viewportWidth;
		int H = backEnd.viewParms.viewportHeight;

        //pRect->offset.x = backEnd.viewParms.viewportX;
        //pRect->offset.y = backEnd.viewParms.viewportY;
        //pRect->extent.width = backEnd.viewParms.viewportWidth;
		//pRect->extent.height = backEnd.viewParms.viewportHeight;

        if ( X < 0)
		    X = 0;
        if (Y < 0)
		    Y = 0;
        if (X + W > vk.renderArea.extent.width)
		    W = vk.renderArea.extent.width - X;
	    if (Y + H > vk.renderArea.extent.height)
		    H = vk.renderArea.extent.height - Y;

        //pRect->offset.x = 
        viewport.x = X;
		//pRect->offset.y = 
        viewport.y = Y;
		//pRect->extent.width =
        viewport.width = W;
		//pRect->extent.height = 
        viewport.height = H;
	}

    switch(depRg)
    {
        case DEPTH_RANGE_NORMAL:
        {
        	viewport.minDepth = 0.0f;
		    viewport.maxDepth = 1.0f;
        }break;

        case DEPTH_RANGE_ZERO:
        {
		    viewport.minDepth = 0.0f;
		    viewport.maxDepth = 0.0f;
	    }break;
        
        case DEPTH_RANGE_ONE:
        {
		    viewport.minDepth = 1.0f;
		    viewport.maxDepth = 1.0f;
	    }break;

        case DEPTH_RANGE_WEAPON:
        {
            viewport.minDepth = 0.0f;
		    viewport.maxDepth = 0.3f;
        }break;
    }

    qvkCmdSetViewport(vk.command_buffer, 0, 1, &viewport);

//    qvkCmdSetScissor(vk.command_buffer, 0, 1, &scissor);


	if (tess.shader->polygonOffset) {
		qvkCmdSetDepthBias(vk.command_buffer, r_offsetUnits->value, 0.0f, r_offsetFactor->value);
	}

	// issue draw call
	if (indexed)
		qvkCmdDrawIndexed(vk.command_buffer, tess.numIndexes, 1, 0, 0, 0);
	else
		qvkCmdDraw(vk.command_buffer, tess.numVertexes, 1, 0, 0);
	
    shadingDat.s_depth_attachment_dirty = VK_TRUE;
}



void updateMVP(VkBool32 isPortal, VkBool32 is2D, const float mvMat4x4[16])
{
	if (isPortal)
    {
        // mvp transform + eye transform + clipping plane in eye space
        float push_constants[32] QALIGN(16);
    
        // Eye space transform.
        MatrixMultiply4x4_SSE(mvMat4x4, backEnd.viewParms.projectionMatrix, push_constants);

        // NOTE: backEnd.or.modelMatrix incorporates s_flipMatrix,
        // so it should be taken into account when computing clipping plane too.

		push_constants[16] = backEnd.or.modelMatrix[0];
		push_constants[17] = backEnd.or.modelMatrix[4];
		push_constants[18] = backEnd.or.modelMatrix[8];
		push_constants[19] = backEnd.or.modelMatrix[12];

		push_constants[20] = backEnd.or.modelMatrix[1];
		push_constants[21] = backEnd.or.modelMatrix[5];
		push_constants[22] = backEnd.or.modelMatrix[9];
		push_constants[23] = backEnd.or.modelMatrix[13];

		push_constants[24] = backEnd.or.modelMatrix[2];
		push_constants[25] = backEnd.or.modelMatrix[6];
		push_constants[26] = backEnd.or.modelMatrix[10];
		push_constants[27] = backEnd.or.modelMatrix[14];
	
        // Clipping plane in eye coordinates.
		struct rplane_s eye_plane;

        R_TransformPlane(backEnd.viewParms.or.axis, backEnd.viewParms.or.origin, &eye_plane);
        
        // Apply s_flipMatrix to be in the same coordinate system as push_constants.
        
        push_constants[28] = -eye_plane.normal[1];
		push_constants[29] =  eye_plane.normal[2];
		push_constants[30] = -eye_plane.normal[0];
		push_constants[31] =  eye_plane.dist;


        // As described above in section Pipeline Layouts, the pipeline layout defines shader push constants
        // which are updated via Vulkan commands rather than via writes to memory or copy commands.
        // Push constants represent a high speed path to modify constant data in pipelines
        // that is expected to outperform memory-backed resource updates.
	    NO_CHECK( qvkCmdPushConstants(vk.command_buffer, vk.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 128, push_constants) );
	}
    else
    {
        // push constants are another way of passing dynamic values to shaders

        if (is2D)
        {            
            NO_CHECK( qvkCmdPushConstants(vk.command_buffer, vk.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, s_ProjectMat2d) );
        }
        else
        {
		    // Specify push constants.
		    float mvp[16] QALIGN(16); // mvp transform + eye transform + clipping plane in eye space

            // update q3's proj matrix (opengl) to vulkan conventions:
            // z - [0, 1] instead of [-1, 1] and invert y direction
            MatrixMultiply4x4_SSE(mvMat4x4, backEnd.viewParms.projectionMatrix, mvp);
            // As described above in section Pipeline Layouts, the pipeline layout defines shader push constants
            // which are updated via Vulkan commands rather than via writes to memory or copy commands.
            // Push constants represent a high speed path to modify constant data in pipelines
            // that is expected to outperform memory-backed resource updates.
		    NO_CHECK( qvkCmdPushConstants(vk.command_buffer, vk.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, mvp) );
        }
    }
}


// =========================================================
// Vertex fetching is controlled via configurable state, 
// as a logically distinct graphics pipeline stage.
//  
//  Vertex Attributes
//
//  Vertex shaders can define input variables, which receive vertex attribute data
//  transferred from one or more VkBuffer(s) by drawing commands. Vertex shader 
//  input variables are bound to buffers via an indirect binding where the vertex 
//  shader associates a vertex input attribute number with each variable, vertex 
//  input attributes are associated to vertex input bindings on a per-pipeline basis, 
//  and vertex input bindings are associated with specific buffers on a per-draw basis
//  via the vkCmdBindVertexBuffers command. 
//
//  Vertex input attribute and vertex input binding descriptions also
//  contain format information controlling how data is extracted from
//  buffer memory and converted to the format expected by the vertex shader.
//
//  There are VkPhysicalDeviceLimits::maxVertexInputAttributes number of vertex
//  input attributes and VkPhysicalDeviceLimits::maxVertexInputBindings number of
//  vertex input bindings (each referred to by zero-based indices), where there 
//  are at least as many vertex input attributes as there are vertex input bindings.
//  Applications can store multiple vertex input attributes interleaved in a single 
//  buffer, and use a single vertex input binding to access those attributes.
//
//  In GLSL, vertex shaders associate input variables with a vertex input attribute
//  number using the location layout qualifier. The component layout qualifier
//  associates components of a vertex shader input variable with components of
//  a vertex input attribute.

void vk_UploadXYZI(float (*pXYZ)[4], uint32_t nVertex, uint32_t* pIdx, uint32_t nIndex)
{
	// xyz stream
	{
        const VkDeviceSize xyz_offset = XYZ_OFFSET + shadingDat.xyz_elements * sizeof(vec4_t);
		
        unsigned char* const vDst = shadingDat.vertex_buffer_ptr + xyz_offset;

        // 4 float in the array, with each 4 bytes.
		memcpy(vDst, pXYZ, nVertex * 16);

		NO_CHECK( qvkCmdBindVertexBuffers(vk.command_buffer, 0, 1, &shadingDat.vertex_buffer, &xyz_offset) );
		
        shadingDat.xyz_elements += tess.numVertexes;

        assert (shadingDat.xyz_elements * sizeof(vec4_t) < XYZ_SIZE);
	}

	// indexes stream
    if(nIndex != 0)
	{
		const uint32_t indexes_size = nIndex * sizeof(uint32_t);        

		unsigned char* iDst = shadingDat.index_buffer_ptr + shadingDat.index_buffer_offset;
		memcpy(iDst, pIdx, indexes_size);

		NO_CHECK( qvkCmdBindIndexBuffer(vk.command_buffer, shadingDat.index_buffer, shadingDat.index_buffer_offset, VK_INDEX_TYPE_UINT32) );
		
        shadingDat.index_buffer_offset += indexes_size;

        assert (shadingDat.index_buffer_offset < INDEX_BUFFER_SIZE);
	}
}


void vk_resetGeometryBuffer(void)
{
	// Reset geometry buffer's current offsets.
	shadingDat.xyz_elements = 0;
	shadingDat.color_st_elements = 0;
	shadingDat.index_buffer_offset = 0;
    shadingDat.s_depth_attachment_dirty = VK_FALSE;

    Mat4Identity(s_modelview_matrix);
}


void vk_destroy_shading_data(void)
{
    ri.Printf(PRINT_ALL, " Destroy vertex/index buffer: shadingDat.vertex_buffer shadingDat.index_buffer. \n");
    ri.Printf(PRINT_ALL, " Free device memory: vertex_buffer_memory index_buffer_memory. \n");

    NO_CHECK( qvkUnmapMemory(vk.device, shadingDat.vertex_buffer_memory) );
    NO_CHECK( qvkUnmapMemory(vk.device, shadingDat.index_buffer_memory) );
	
    NO_CHECK( qvkFreeMemory(vk.device, shadingDat.vertex_buffer_memory, NULL) );
	NO_CHECK( qvkFreeMemory(vk.device, shadingDat.index_buffer_memory, NULL) );

    NO_CHECK( qvkDestroyBuffer(vk.device, shadingDat.vertex_buffer, NULL) );
	NO_CHECK( qvkDestroyBuffer(vk.device, shadingDat.index_buffer, NULL) );

    memset(&shadingDat, 0, sizeof(shadingDat));
}



void vk_clearDepthStencilAttachments(void)
{
    if(shadingDat.s_depth_attachment_dirty)
    {
        VkClearAttachment attachments;
        memset(&attachments, 0, sizeof(VkClearAttachment));

        attachments.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        attachments.clearValue.depthStencil.depth = 1.0f;

        if (r_shadows->integer == 2) {
            attachments.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            attachments.clearValue.depthStencil.stencil = 0;
        }

        VkClearRect clear_rect;
        clear_rect.rect = vk.renderArea;
        clear_rect.baseArrayLayer = 0;
        clear_rect.layerCount = 1;

        //ri.Printf(PRINT_ALL, "(%d, %d, %d, %d)\n", 
        //        clear_rect.rect.offset.x, clear_rect.rect.offset.y, 
        //        clear_rect.rect.extent.width, clear_rect.rect.extent.height);


        NO_CHECK( qvkCmdClearAttachments(vk.command_buffer, 1, &attachments, 1, &clear_rect) );
        
        shadingDat.s_depth_attachment_dirty = VK_FALSE;
    }
}




void vk_clearColorAttachments(const float* color)
{

    VkClearAttachment attachments[1];
    memset(attachments, 0, sizeof(VkClearAttachment));
    
    // aspectMask is a mask selecting the color, depth and/or stencil aspects
    // of the attachment to be cleared. aspectMask can include 
    // VK_IMAGE_ASPECT_COLOR_BIT for color attachments,
    // VK_IMAGE_ASPECT_DEPTH_BIT for depth/stencil attachments with a depth
    // component, and VK_IMAGE_ASPECT_STENCIL_BIT for depth/stencil attachments
    // with a stencil component. If the subpass\A1\AFs depth/stencil attachment
    // is VK_ATTACHMENT_UNUSED, then the clear has no effect.

    attachments[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // colorAttachment is only meaningful if VK_IMAGE_ASPECT_COLOR_BIT
    // is set in aspectMask, in which case it is an index to 
    // the pColorAttachments array in the VkSubpassDescription structure
    // of the current subpass which selects the color attachment to clear.
    attachments[0].colorAttachment = 0;
    attachments[0].clearValue.color.float32[0] = color[0];
    attachments[0].clearValue.color.float32[1] = color[1];
    attachments[0].clearValue.color.float32[2] = color[2];
    attachments[0].clearValue.color.float32[3] = color[3];

/* 
	VkClearRect clear_rect[2];
	clear_rect[0].rect = get_scissor_rect();
	clear_rect[0].baseArrayLayer = 0;
	clear_rect[0].layerCount = 1;
	uint32_t rect_count = 1;
  
	// Split viewport rectangle into two non-overlapping rectangles.
	// It's a HACK to prevent Vulkan validation layer's performance warning:
	//		"vkCmdClearAttachments() issued on command buffer object XXX prior to any Draw Cmds.
	//		 It is recommended you use RenderPass LOAD_OP_CLEAR on Attachments prior to any Draw."
	// 
	// NOTE: we don't use LOAD_OP_CLEAR for color attachment when we begin renderpass
	// since at that point we don't know whether we need color buffer clear (usually we don't).
    uint32_t h = clear_rect[0].rect.extent.height / 2;
    clear_rect[0].rect.extent.height = h;
    clear_rect[1] = clear_rect[0];
    clear_rect[1].rect.offset.y = h;
    rect_count = 2;
*/

    VkClearRect clear_rect[1];
	clear_rect[0].rect = vk.renderArea;
	clear_rect[0].baseArrayLayer = 0;
	clear_rect[0].layerCount = 1;

    //ri.Printf(PRINT_ALL, "(%d, %d, %d, %d)\n", 
    //        clear_rect[0].rect.offset.x, clear_rect[0].rect.offset.y, 
    //        clear_rect[0].rect.extent.width, clear_rect[0].rect.extent.height);


    // CmdClearAttachments can clear multiple regions of each attachment
    // used in the current subpass of a render pass instance. This command
    // must be called only inside a render pass instance, and implicitly
    // selects the images to clear based on the current framebuffer 
    // attachments and the command parameters.

	NO_CHECK( qvkCmdClearAttachments(vk.command_buffer, 1, attachments, 1, clear_rect) );
}


/*
** RB_CalcDiffuseColor
**
** The basic vertex lighting calc
*/
static void RB_CalcDiffuseColor(unsigned char (*colors)[4])
{

    //trRefEntity_t* ent = backEnd.currentEntity;
    vec3_t abtLit;
    abtLit[0] = backEnd.currentEntity->ambientLight[0];
    abtLit[1] = backEnd.currentEntity->ambientLight[1];
    abtLit[2] = backEnd.currentEntity->ambientLight[2];

    vec3_t drtLit;
    drtLit[0] = backEnd.currentEntity->directedLight[0];
    drtLit[1] = backEnd.currentEntity->directedLight[1];
    drtLit[2] = backEnd.currentEntity->directedLight[2];

    vec3_t litDir;
    litDir[0] = backEnd.currentEntity->lightDir[0];
    litDir[1] = backEnd.currentEntity->lightDir[1];
    litDir[2] = backEnd.currentEntity->lightDir[2];

    unsigned char ambLitRGBA[4];  
    
    ambLitRGBA[0] = backEnd.currentEntity->ambientLightRGBA[0];
    ambLitRGBA[1] = backEnd.currentEntity->ambientLightRGBA[1];
    ambLitRGBA[2] = backEnd.currentEntity->ambientLightRGBA[2];
    ambLitRGBA[3] = backEnd.currentEntity->ambientLightRGBA[3];

    int numVertexes = tess.numVertexes;
	int	i;
	for (i = 0; i < numVertexes; i++)
    {
		float incoming = DotProduct(tess.normal[i], litDir);
		
        if( incoming <= 0 )
        {
			colors[i][0] = ambLitRGBA[0];
            colors[i][1] = ambLitRGBA[1];
			colors[i][2] = ambLitRGBA[2];
			colors[i][3] = ambLitRGBA[3];
		}
        else
        {
            int r = abtLit[0] + incoming * drtLit[0];
            int g = abtLit[1] + incoming * drtLit[1];
            int b = abtLit[2] + incoming * drtLit[2];

            colors[i][0] = (r <= 255 ? r : 255);
            colors[i][1] = (g <= 255 ? g : 255);
            colors[i][2] = (b <= 255 ? b : 255);
            colors[i][3] = 255;
        }
	}
}


// This fixed version comes from ZEQ2Lite
//Calculates specular coefficient and places it in the alpha channel
static void RB_CalcSpecularAlphaNew(unsigned char (*alphas)[4])
{
	int numVertexes = tess.numVertexes;
	
    int	i;
    for (i = 0 ; i < numVertexes; i++)
    {
        vec3_t lightDir, viewer, reflected;
		if ( backEnd.currentEntity == &tr.worldEntity )
        {
            // old compatibility with maps that use it on some models
            vec3_t lightOrigin = {-960, 1980, 96};		// FIXME: track dynamically
			VectorSubtract( lightOrigin, tess.xyz[i], lightDir );
        }
        else
        {
			VectorCopy( backEnd.currentEntity->lightDir, lightDir );
        }
		// calculate the specular color
		float d = 2*DotProduct(tess.normal[i], lightDir);

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		reflected[0] = tess.normal[i][0]*d - lightDir[0];
		reflected[1] = tess.normal[i][1]*d - lightDir[1];
		reflected[2] = tess.normal[i][2]*d - lightDir[2];

		VectorSubtract(backEnd.or.viewOrigin, tess.xyz[i], viewer);
		
        float l = DotProduct(reflected, viewer)/sqrtf(DotProduct(viewer, viewer));

		if(l < 0)
			alphas[i][3] = 0;
        else if(l >= 1)
			alphas[i][3] = 255;
        else
        {
			l = l*l;
            alphas[i][3] = l*l*255;
		}
	}
}

static void ComputeColors( shaderStage_t *pStage )
{
	int		i;
	//
	// rgbGen
	//
	switch ( pStage->rgbGen )
	{
		case CGEN_IDENTITY:
			memset( tess.svars.colors, 0xff, tess.numVertexes * 4 );
			break;
		default:
		case CGEN_IDENTITY_LIGHTING:
			memset( tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4 );
			break;
		case CGEN_LIGHTING_DIFFUSE:
			RB_CalcDiffuseColor( tess.svars.colors );
			break;
		case CGEN_EXACT_VERTEX:
			memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
			break;
		case CGEN_CONST:
        {
            
            uint32_t nVerts = tess.numVertexes;

			for ( i = 0; i < nVerts; ++i )
            {
				*(int *)tess.svars.colors[i] = *(int *)pStage->constantColor;
			}
        } break;
		case CGEN_VERTEX:
			if ( tr.identityLight == 1 )
			{
				memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
			}
			else
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = tess.vertexColors[i][0] * tr.identityLight;
					tess.svars.colors[i][1] = tess.vertexColors[i][1] * tr.identityLight;
					tess.svars.colors[i][2] = tess.vertexColors[i][2] * tr.identityLight;
					tess.svars.colors[i][3] = tess.vertexColors[i][3];
				}
			}
			break;
		case CGEN_ONE_MINUS_VERTEX:
			if ( tr.identityLight == 1 )
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
					tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
					tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
				}
			}
			else
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = ( 255 - tess.vertexColors[i][0] ) * tr.identityLight;
					tess.svars.colors[i][1] = ( 255 - tess.vertexColors[i][1] ) * tr.identityLight;
					tess.svars.colors[i][2] = ( 255 - tess.vertexColors[i][2] ) * tr.identityLight;
				}
			}
			break;
		case CGEN_FOG:
		{
            R_SetTessFogColor(tess.svars.colors, tess.fogNum, tess.numVertexes);
		}break;
		case CGEN_WAVEFORM:
			RB_CalcWaveColor( &pStage->rgbWave, tess.svars.colors );
			break;
		case CGEN_ENTITY:
			RB_CalcColorFromEntity( tess.svars.colors );
			break;
		case CGEN_ONE_MINUS_ENTITY:
			RB_CalcColorFromOneMinusEntity( tess.svars.colors );
			break;
	}

	//
	// alphaGen
	//
	switch ( pStage->alphaGen )
	{
	case AGEN_SKIP:
		break;
	case AGEN_IDENTITY:
		if ( pStage->rgbGen != CGEN_IDENTITY ) {
			if ( ( pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1 ) ||
				 pStage->rgbGen != CGEN_VERTEX ) {
				for ( i = 0; i < tess.numVertexes; i++ ) {
					tess.svars.colors[i][3] = 0xff;
				}
			}
		}
		break;
	case AGEN_CONST:
		if ( pStage->rgbGen != CGEN_CONST ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[i][3] = pStage->constantColor[3];
			}
		}
		break;
	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha( &pStage->alphaWave, ( unsigned char * ) tess.svars.colors );
		break;
	case AGEN_LIGHTING_SPECULAR:
		// RB_CalcSpecularAlpha( ( unsigned char * ) tess.svars.colors );
		RB_CalcSpecularAlphaNew(tess.svars.colors);
		break;
	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity( ( unsigned char * ) tess.svars.colors );
		break;
	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity( ( unsigned char * ) tess.svars.colors );
		break;
    case AGEN_VERTEX:
		if ( pStage->rgbGen != CGEN_VERTEX ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
        break;
    case AGEN_ONE_MINUS_VERTEX:
        for ( i = 0; i < tess.numVertexes; i++ )
        {
			tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
        }
        break;
	case AGEN_PORTAL:
		{
			unsigned char alpha;

			for ( i = 0; i < tess.numVertexes; i++ )
			{
				vec3_t v;

				VectorSubtract( tess.xyz[i], backEnd.viewParms.or.origin, v );
				float len = VectorLength( v );

				len /= tess.shader->portalRange;

				if ( len < 0 )
				{
					alpha = 0;
				}
				else if ( len > 1 )
				{
					alpha = 0xff;
				}
				else
				{
					alpha = len * 0xff;
				}

				tess.svars.colors[i][3] = alpha;
			}
		}
		break;
	}

	//
	// fog adjustment for colors to fade out as fog increases
	//
	if ( tess.fogNum )
	{
		switch ( pStage->adjustColorsForFog )
		{
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_NONE:
			break;
		}
	}
}

static void ComputeTexCoords( shaderStage_t * pStage )
{
	uint32_t i;
	uint32_t b;

	for ( b = 0; b < NUM_TEXTURE_BUNDLES; ++b )
    {
		int tm;

		//
		// generate the texture coordinates
		//
		switch ( pStage->bundle[b].tcGen )
		{
            case TCGEN_IDENTITY:
                memset( tess.svars.texcoords[b], 0, sizeof( float ) * 2 * tess.numVertexes );
                break;
            case TCGEN_TEXTURE:
                for ( i = 0 ; i < tess.numVertexes ; i++ ) {
                    tess.svars.texcoords[b][i][0] = tess.texCoords[i][0][0];
                    tess.svars.texcoords[b][i][1] = tess.texCoords[i][0][1];
                }
                break;
            case TCGEN_LIGHTMAP:
                for ( i = 0 ; i < tess.numVertexes ; i++ ) {
                    tess.svars.texcoords[b][i][0] = tess.texCoords[i][1][0];
                    tess.svars.texcoords[b][i][1] = tess.texCoords[i][1][1];
                }
                break;
            case TCGEN_VECTOR:
                for ( i = 0 ; i < tess.numVertexes ; i++ ) {
                    tess.svars.texcoords[b][i][0] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[0] );
                    tess.svars.texcoords[b][i][1] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[1] );
                }
                break;
            case TCGEN_FOG:
                RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
            case TCGEN_ENVIRONMENT_MAPPED:
                RB_CalcEnvironmentTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
           case TCGEN_ENVIRONMENT_MAPPED_WATER:
                RB_CalcEnvironmentTexCoordsJO( ( float * ) tess.svars.texcoords[b] ); 			
                break;
            case TCGEN_ENVIRONMENT_CELSHADE_MAPPED:
                RB_CalcEnvironmentCelShadeTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
            case TCGEN_ENVIRONMENT_CELSHADE_LEILEI:
                RB_CalcCelTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
            case TCGEN_BAD:
                return;
		}

		//
		// alter texture coordinates
		//
		for ( tm = 0; tm < pStage->bundle[b].numTexMods ; tm++ )
        {
			switch ( pStage->bundle[b].texMods[tm].type )
			{
			case TMOD_NONE:
				tm = TR_MAX_TEXMODS;		// break out of for loop
				break;

			case TMOD_TURBULENT:
				RB_CalcTurbulentTexCoords( &pStage->bundle[b].texMods[tm].wave, ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_ENTITY_TRANSLATE:
				RB_CalcScrollTexCoords( backEnd.currentEntity->e.shaderTexCoord, ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_SCROLL:
				RB_CalcScrollTexCoords( pStage->bundle[b].texMods[tm].scroll, ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_SCALE:
				RB_CalcScaleTexCoords( pStage->bundle[b].texMods[tm].scale, ( float * ) tess.svars.texcoords[b] );
				break;
			
			case TMOD_STRETCH:
				RB_CalcStretchTexCoords( &pStage->bundle[b].texMods[tm].wave, ( float * ) tess.svars.texcoords[b] );
				break;
			case TMOD_ATLAS:
				RB_CalcAtlasTexCoords(  &pStage->bundle[b].texMods[tm].atlas, ( float * ) tess.svars.texcoords[b] );
				break;
			case TMOD_TRANSFORM:
				RB_CalcTransformTexCoords( &pStage->bundle[b].texMods[tm], ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_ROTATE:
				RB_CalcRotateTexCoords( pStage->bundle[b].texMods[tm].rotateSpeed, ( float * ) tess.svars.texcoords[b] );
				break;

			default:
				ri.Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'\n", pStage->bundle[b].texMods[tm].type, tess.shader->name );
				break;
			}
		}
	}
}



/*
===================
Perform dynamic lighting with another rendering pass
===================
*/
static void ProjectDlightTexture( shaderCommands_t * const pTess, uint32_t num_dlights, struct dlight_s	* const pDlights)
{
	unsigned char clipBits[SHADER_MAX_VERTEXES];


    uint32_t l;
	for (l = 0; l < num_dlights; ++l)
    {
		//dlight_t	*dl;

		if ( !( pTess->dlightBits & ( 1 << l ) ) )
        {
			continue;	// this surface definately doesn't have any of this light
		}
		float* texCoords = pTess->svars.texcoords[0][0];
		//colors = tess.svars.colors[0];

		//dl = &backEnd.refdef.dlights[l];
        vec3_t origin;
		VectorCopy( pDlights[l].transformed, origin );


        float radius = pDlights[l].radius;
		float scale = 1.0f / radius;
    	float modulate;

        float floatColor[3] = {
		    pDlights[l].color[0] * 255.0f,
		    pDlights[l].color[1] * 255.0f,
		    pDlights[l].color[2] * 255.0f
        };

        uint32_t i;
		for ( i = 0 ; i < pTess->numVertexes ; ++i, texCoords += 2)
        {
			vec3_t dist;

			++backEnd.pc.c_dlightVertexes;

			VectorSubtract( origin, pTess->xyz[i], dist );
			texCoords[0] = 0.5f + dist[0] * scale;
			texCoords[1] = 0.5f + dist[1] * scale;

			uint32_t clip = 0;
			if ( texCoords[0] < 0.0f ) {
				clip |= 1;
			}
            else if ( texCoords[0] > 1.0f ) {
				clip |= 2;
			}

			if ( texCoords[1] < 0.0f ) {
				clip |= 4;
			}
            else if ( texCoords[1] > 1.0f ) {
				clip |= 8;
			}

			// modulate the strength based on the height and color
			if ( dist[2] > radius )
            {
				clip |= 16;
				modulate = 0.0f;
			}
            else if ( dist[2] < -radius )
            {
				clip |= 32;
				modulate = 0.0f;
			}
            else
            {
				dist[2] = fabs(dist[2]);
				if ( dist[2] < radius * 0.5f )
                {
					modulate = 1.0f;
				}
                else
                {
					modulate = 2.0f * (radius - dist[2]) * scale;
				}
			}
			clipBits[i] = clip;
            
            // += 4 
			pTess->svars.colors[i][0] = (floatColor[0] * modulate);
			pTess->svars.colors[i][1] = (floatColor[1] * modulate);
			pTess->svars.colors[i][2] = (floatColor[2] * modulate);
			pTess->svars.colors[i][3] = 255;
		}

      
		// build a list of triangles that need light
		uint32_t numIndexes = 0;
		for ( i = 0 ; i < pTess->numIndexes ; i += 3 )
        {
			uint32_t a, b, c;

			a = pTess->indexes[i];
			b = pTess->indexes[i+1];
			c = pTess->indexes[i+2];
			if ( clipBits[a] & clipBits[b] & clipBits[c] ) {
				continue;	// not lighted
			}
			numIndexes += 3;
		}

		if ( numIndexes == 0 ) {
			continue;
		}


		updateCurDescriptor( tr.dlightImage->descriptor_set, 0 );
		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light where they aren't rendered
		backEnd.pc.c_totalIndexes += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;

		// VULKAN

		vk_shade_geometry(g_globalPipelines.dlight_pipelines[pDlights[l].additive > 0 ? 1 : 0][pTess->shader->cullType][pTess->shader->polygonOffset],
                VK_FALSE, DEPTH_RANGE_NORMAL, VK_TRUE);

	}
}


/*
===================
RB_FogPass
Blends a fog texture on top of everything else
===================
*/
void R_SetTessFogColor(unsigned char (*pcolor)[4], uint32_t fnum, uint32_t nVerts)
{

    fog_t* pFogs = NULL;
    uint32_t nFogs = 0;
    
    // fog_t* fog = tr.world->fogs + fnum;

    R_GetFogArray(&pFogs, &nFogs);
    
    //ri.Printf(PRINT_ALL, "number of fog: %d, fnumL %d\n", nFogs, fnum);

    uint32_t i; 
    for (i = 0; i < nVerts; ++i)
    {
        pcolor[i][0] = pFogs[fnum].colorRGBA[0];
        pcolor[i][1] = pFogs[fnum].colorRGBA[1];
        pcolor[i][2] = pFogs[fnum].colorRGBA[2];
        pcolor[i][3] = pFogs[fnum].colorRGBA[3];
        // ri.Printf(PRINT_ALL, "nVerts: %d, pcolor %d, %d, %d, %d\n", 
        //        i, pcolor[i][0], pcolor[i][1], pcolor[i][2], pcolor[i][3]);
    }
    
}



static void RB_FogPass( shaderCommands_t * const pTess )
{

    R_SetTessFogColor(pTess->svars.colors, pTess->fogNum, pTess->numVertexes);

	RB_CalcFogTexCoords( ( float * ) pTess->svars.texcoords[0] );

	updateCurDescriptor( tr.fogImage->descriptor_set, 0);

	// VULKAN

    assert(pTess->shader->fogPass > 0);
    VkPipeline pipeline = g_globalPipelines.fog_pipelines[pTess->shader->fogPass - 1][pTess->shader->cullType][pTess->shader->polygonOffset];
    vk_shade_geometry(pipeline, VK_FALSE, DEPTH_RANGE_NORMAL, VK_TRUE);
}

///////////////////////////////////////////////////////////////////////////////////

static void R_BindAnimatedImage( textureBundle_t *bundle, int tmu)
{

	if ( bundle->isVideoMap ) {
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);
		return;
	}

	if ( bundle->numImageAnimations <= 1 ) {
		updateCurDescriptor( bundle->image[0]->descriptor_set, tmu );
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	int index = (int)( tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
	index >>= FUNCTABLE_SIZE2;

	if ( index < 0 ) {
		index = 0;	// may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

	updateCurDescriptor( bundle->image[ index ]->descriptor_set, tmu );
}

/////////////////////////////////////////////////////////////////////////////////

void RB_StageIteratorGeneric(shaderCommands_t * const pTess)
{
//	shaderCommands_t *input = &tess;

	RB_DeformTessGeometry(pTess);

	// call shader function
	//
	// VULKAN
   
    vk_UploadXYZI(pTess->xyz, pTess->numVertexes, pTess->indexes, pTess->numIndexes);

    updateMVP(backEnd.viewParms.isPortal, backEnd.projection2D, 
            getptr_modelview_matrix() );
    

    uint32_t stage = 0;

	for ( stage = 0; stage < MAX_SHADER_STAGES; ++stage )
	{
		if ( NULL == pTess->xstages[stage])
		{
			break;
		}

		ComputeColors( pTess->xstages[stage] );
		ComputeTexCoords( pTess->xstages[stage] );

        // base, set state
		R_BindAnimatedImage( &pTess->xstages[stage]->bundle[0], 0 );
		//
		// do multitexture
		//
        qboolean multitexture = (pTess->xstages[stage]->bundle[1].image[0] != NULL);

		if ( multitexture )
		{
            // DrawMultitextured( input, stage );
            // output = t0 * t1 or t0 + t1

            // t0 = most upstream according to spec
            // t1 = most downstream according to spec
            // this is an ugly hack to work around a GeForce driver
            // bug with multitexture and clip planes


            // lightmap/secondary pass

            R_BindAnimatedImage( &pTess->xstages[stage]->bundle[1], 1 );
            // disable texturing on TEXTURE1, then select TEXTURE0
		}

       
        enum Vk_Depth_Range depth_range = DEPTH_RANGE_NORMAL;
        if (pTess->shader->isSky)
        {
            depth_range = DEPTH_RANGE_ONE;
            if (r_showsky->integer)
                depth_range = DEPTH_RANGE_ZERO;
        }
        else if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
        {
            depth_range = DEPTH_RANGE_WEAPON;
        }


        if ( r_lightmap->integer && multitexture) {
            updateCurDescriptor( tr.whiteImage->descriptor_set, 0 );
        }

        if (backEnd.viewParms.isMirror)
        {
            vk_shade_geometry(pTess->xstages[stage]->vk_mirror_pipeline, multitexture, depth_range, VK_TRUE);
        }
        else if (backEnd.viewParms.isPortal)
        {
            vk_shade_geometry(pTess->xstages[stage]->vk_portal_pipeline, multitexture, depth_range, VK_TRUE);
        }
        else
        {
            vk_shade_geometry(pTess->xstages[stage]->vk_pipeline, multitexture, depth_range, VK_TRUE);
        }

                
		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pTess->xstages[stage]->bundle[0].isLightmap || pTess->xstages[stage]->bundle[1].isLightmap ) )
		{
			break;
		}
	}

	// 
	// now do any dynamic lighting needed
	//
	if ( pTess->dlightBits && 
         pTess->shader->sort <= SS_OPAQUE && 
            !(pTess->shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) )
    {
        if(backEnd.refdef.num_dlights != 0)
        {
	    	ProjectDlightTexture( pTess, backEnd.refdef.num_dlights, backEnd.refdef.dlights);
        }
	}

	//
	// now do fog
	//
	if ( pTess->fogNum && pTess->shader->fogPass ) {
		RB_FogPass(pTess);
	}
}
