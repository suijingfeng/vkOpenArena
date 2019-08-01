#include "vk_instance.h"
#include "tr_globals.h"
#include "tr_cvar.h"
#include "tr_backend.h"
#include "tr_light.h"
#include "tr_shader.h"
#include "tr_shade.h"

#include "vk_buffers.h"
#include "vk_pipelines.h"

#include "ref_import.h" 
#include "matrix_multiplication.h"
#include "R_PortalPlane.h"
#include "RB_DeformGeometry.h"
#include "vk_shade_geometry.h"

// projection: the transformation of lines, points, and polygons
// form eye coordinates to clipping coordinates on screeen.
//
// perspective divide: the transformation applied to homogeneous
// vectors to move them from clip space into normalized device
// coordinates by dividing them through by their w components.
//
// normalized device coordinate: the coordinate space produced by
// taking a homogeneous position and deviding it through by its
// own w component.
//
// model-view matrix: the matrix that transforms position vectors
// from model(or object) space to view(or eye) space.
//
// eye coordinates: The coordinate system based on the position of
// the viewer. The viewer's position is placed along the positive
// z axis, looking down the negative z axis.
//
// clip coordinates: The 2D geometry coordinates that result from
// the model view and projection transformation.
//
// clip distance: A distance value assigned by a shader that is 
// used by fixed function clipping to allow primitives to be
// clipped against an arbitrary set of planes before rasterization.
//
// clipping: The elimination of a portion of a single primitive
// or group of the primitives. The points that outside the clipping
// region or volume would not be rendered. The clipping volume
// is generally specified by the projection matrix.
//
// ambient light: Light in a scene that doesn't come from any
// specific point source or direction. Ambient light illuminates
// all surfaces evenly and on all side.
// 
static QALIGN(16) float s_modelview_matrix[16];
static QALIGN(16) float s_ProjectMat2d[16];

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
	uint8_t * vertex_buffer_ptr ; // pointer to mapped vertex buffer
	uint32_t xyz_elements;
	uint32_t colorElemCount;

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
};

static struct ShadingData_t shadingDat;

static VkBool32 s_depth_attachment_dirty;

VkBuffer vk_getIndexBuffer(void)
{
    return shadingDat.index_buffer;
}

VkBuffer vk_getVertexBuffer(void)
{
    return shadingDat.vertex_buffer;
}



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
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &shadingDat.vertex_buffer,
            &shadingDat.vertex_buffer_memory );

    void* data;
    VK_CHECK( qvkMapMemory(vk.device, shadingDat.vertex_buffer_memory, 
                0, VK_WHOLE_SIZE, 0, &data) );
    shadingDat.vertex_buffer_ptr = (unsigned char*)data;
}



void vk_createIndexBuffer(void)
{
    ri.Printf(PRINT_ALL, " Create index buffer: shadingDat.index_buffer \n");

    vk_createBufferResource( INDEX_BUFFER_SIZE, 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            &shadingDat.index_buffer, &shadingDat.index_buffer_memory);

    void* data;
    VK_CHECK( qvkMapMemory(vk.device, shadingDat.index_buffer_memory,
                0, VK_WHOLE_SIZE, 0, &data));
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


// During the rendering process, the GPU will write to resources
// e.g the back buffer, the depth/stencil buffer, and read from
// resources (textures that describe the appearance of the surfaces,
// buffers that store the 3D positions of geometry), before we 
// issue a draw command, we need to bind/link the resources to the 
// rendering pipeline that are going to be referenced in the draw call.
// some of the resources may change per draw call, so we need to
// update the bindings per draw call if necessary. However, a GPU
// resources are not bound directly, Instand, a resource is referenced
// through a descriptor object, which can be thought of as a lightweight
// structure that describes the resource to the GPU. 
//
// Why go to this extra level of indirection with descriptor ?
// The reason is that resources are essentially generic chunks of
// memory. Resources are kept generic so they can be used in different
// stages of the rendering pipeline; a common example is to use 
// texture as a render target, draws into texture and letter as
// a shader resource which will be simpled and served as input data
// for a shader. A resource itself does not say if it is being used
// as a render taget, depth/stencil buffer or shader resource.
// moreover a resource can be created with a typeless format, so
// GPU may not even know the format of the GPU resource, This is
// where descripor come in. In addition to identifying the resource
// data, they also describes how the resource is going to be use
//


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

/*
static void vk_cmdInsertLoadingVertexBarrier(VkCommandBuffer HCmdBuffer)
{
    // Ensur/e visibility of geometry buffers writes.
    VkBufferMemoryBarrier barrier1;
    barrier1.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier1.pNext = NULL;
    barrier1.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    // VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT specifies read access 
    // to a vertex buffer as part of a drawing command, bound by
    // vkCmdBindVertexBuffers.
    barrier1.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier1.offset = 0;
    barrier1.size = VK_WHOLE_SIZE;
    barrier1.buffer = vk_getVertexBuffer();
    // 
    NO_CHECK( qvkCmdPipelineBarrier(HCmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 1, &barrier1, 0, NULL) );
}


static void vk_cmdInsertLoadingIndexBarrier(VkCommandBuffer HCmdBuffer)
{
    // Ensur/e visibility of geometry buffers writes.
    VkBufferMemoryBarrier barrier2;

    barrier2.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier2.pNext = NULL;
    barrier2.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        // VK_ACCESS_INDEX_READ_BIT specifies read access to an index buffer 
    // as part of an indexed drawing command, bound by vkCmdBindIndexBuffer.
    barrier2.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
    barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.offset = 0;
    barrier2.size = VK_WHOLE_SIZE;
    barrier2.buffer = vk_getIndexBuffer();

    NO_CHECK( qvkCmdPipelineBarrier(HCmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 1, &barrier2, 0, NULL) );

}
*/


void vk_UploadXYZI(const float (* const pXYZ)[4], uint32_t nVertex, 
        const uint32_t * const pIdx, uint32_t nIndex)
{
	// xyz stream
    const VkDeviceSize xyz_offset = XYZ_OFFSET + shadingDat.xyz_elements * sizeof(vec4_t);

    // 4 float in the array, with each 4 bytes.
    memcpy(shadingDat.vertex_buffer_ptr + xyz_offset, pXYZ, nVertex * sizeof(vec4_t));

    NO_CHECK( qvkCmdBindVertexBuffers(vk.command_buffer, 0, 1, &shadingDat.vertex_buffer, &xyz_offset) );

    shadingDat.xyz_elements += nVertex;

    assert (shadingDat.xyz_elements * sizeof(vec4_t) < XYZ_SIZE);

	// ri.Printf(PRINT_ALL, "nvert: %d\n", nVertex);

	// indexes stream
    if(nIndex != 0)
	{
		const uint32_t indexes_size = nIndex * sizeof(uint32_t);        

		memcpy( shadingDat.index_buffer_ptr + shadingDat.index_buffer_offset, pIdx, indexes_size);

		NO_CHECK( qvkCmdBindIndexBuffer(vk.command_buffer, shadingDat.index_buffer,
                    shadingDat.index_buffer_offset, VK_INDEX_TYPE_UINT32) );
		
        shadingDat.index_buffer_offset += indexes_size;

        assert (shadingDat.index_buffer_offset < INDEX_BUFFER_SIZE);
	}
}


void vk_rcdUpdateViewport(VkBool32 is2D, enum Vk_Depth_Range depRg)
{
    VkViewport viewport;

    if (is2D)
    {
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = vk.renderArea.extent.width;
        viewport.height = vk.renderArea.extent.height;
    }
    else
    {
        int32_t X = backEnd.viewParms.viewportX;
        int32_t Y = backEnd.viewParms.viewportY;
        int32_t W = backEnd.viewParms.viewportWidth;
        int32_t H = backEnd.viewParms.viewportHeight;

        // viewport.x = backEnd.viewParms.viewportX;
        // viewport.y = backEnd.viewParms.viewportY;
        // viewport.width = backEnd.viewParms.viewportWidth;
        // viewport.height = backEnd.viewParms.viewportHeight;
        
        // why this could happend ???
        if ( X < 0)
            X = 0;
        if (Y < 0)
            Y = 0;
        if (X + W > vk.renderArea.extent.width)
            W = vk.renderArea.extent.width - X;
        if (Y + H > vk.renderArea.extent.height)
            H = vk.renderArea.extent.height - Y;

        viewport.x = X;
        viewport.y = Y;
        viewport.width = W;
        viewport.height = H;

        // ri.Printf(PRINT_ALL, "X:%d,Y:%d,W:%d,H:%d\n", X,Y,W,H);
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

    NO_CHECK( qvkCmdSetViewport(vk.command_buffer, 0, 1, &viewport) );
}


void updateMVP(VkBool32 isPortal, VkBool32 is2D, const float mvMat4x4[16])
{
    // push constants are another way of passing dynamic values to shaders
    if (is2D)
    {            
        NO_CHECK( qvkCmdPushConstants(vk.command_buffer, vk.pipeline_layout,
                    VK_SHADER_STAGE_VERTEX_BIT, 0, 64, s_ProjectMat2d) );
    }
    else
    {
        // 3D, mvp transform + eye transform + clipping plane in eye space
        float QALIGN(16) push_constants[32];
        // update q3's proj matrix (opengl) to vulkan conventions:
        // z - [0, 1] instead of [-1, 1] and invert y direction
        // Eye space transform.
        uint32_t push_size = 64;
        MatrixMultiply4x4_SSE(mvMat4x4, backEnd.viewParms.projectionMatrix, push_constants);
        // As described above in section Pipeline Layouts, the pipeline layout defines shader push constants
        // which are updated via Vulkan commands rather than via writes to memory or copy commands.
        // Push constants represent a high speed path to modify constant data in pipelines
        // that is expected to outperform memory-backed resource updates.

        if (isPortal)
        {
            push_size += 64;
            // NOTE: backEnd.or.modelMatrix incorporates s_flipMatrix,
            // so it should be taken into account when computing clipping plane too.
            
            R_SetPushConstForPortal(&push_constants[16]);
        }

        // As described above in section Pipeline Layouts, the pipeline layout defines shader push constants
        // which are updated via Vulkan commands rather than via writes to memory or copy commands.
        // Push constants represent a high speed path to modify constant data in pipelines
        // that is expected to outperform memory-backed resource updates.
        NO_CHECK( qvkCmdPushConstants(vk.command_buffer, vk.pipeline_layout,
                    VK_SHADER_STAGE_VERTEX_BIT, 0, push_size, push_constants) );
    }
}


void vk_shade(VkPipeline pipeline, struct shaderCommands_s * const pTess,
        VkDescriptorSet* const pDesSet, VkBool32 multitexture, VkBool32 indexed)
{
    // configure vertex data stream
    const VkBuffer bufHandleArray[3] = {
      shadingDat.vertex_buffer, shadingDat.vertex_buffer, shadingDat.vertex_buffer };
    

    const VkDeviceSize offsetsArray[3] = {
        COLOR_OFFSET + shadingDat.colorElemCount * 4, // sizeof(color4ub_t)
        ST0_OFFSET   + shadingDat.colorElemCount * sizeof(vec2_t),
        ST1_OFFSET   + shadingDat.colorElemCount * sizeof(vec2_t)
    };


    const uint32_t Size_Color = pTess->numVertexes * 4; // 4 = sizeof(color4ub_t)
    const uint32_t Size_ST =  pTess->numVertexes * sizeof(vec2_t);

    shadingDat.colorElemCount += pTess->numVertexes;
    
    if ( shadingDat.colorElemCount * 4 > COLOR_SIZE)
    {
        ri.Error( ERR_DROP, "vulkan: vertex buffer overflow (color) %d \n", 
                shadingDat.colorElemCount * 4 );
    }

    memcpy(shadingDat.vertex_buffer_ptr + offsetsArray[0], 
             pTess->svars.colors, Size_Color);
    
    // st0
    memcpy( shadingDat.vertex_buffer_ptr + offsetsArray[1], 
             pTess->svars.texcoords[0], Size_ST );
    
    // st1
    if (multitexture)
    {
        memcpy( shadingDat.vertex_buffer_ptr + offsetsArray[2],
                 pTess->svars.texcoords[1], Size_ST);
    }

    // Once bound, a pipeline binding affects subsequent graphics
    // or compute commands in the command buffer until a different
    // pipeline is bound to the bind point. When a pipeline object
    // is bound, any pipeline object state that is not specified 
    // as dynamic is applied to the command buffer state

    NO_CHECK( qvkCmdBindPipeline(vk.command_buffer, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline) );
    
    // It is perfectly reasonable to bind the same buffer object with 
    // different offsets to a command buffer, simply include the same
    // VkBuffer handle multiple times in the pBuffers array. cache ?
    NO_CHECK( qvkCmdBindVertexBuffers(vk.command_buffer, 1, multitexture ? 3 : 2,
                bufHandleArray, offsetsArray) );
    
    // bind descriptor sets: vkCmdBindDescriptorSets causes the sets 
    // numbered [firstSet.. firstSet+descriptorSetCount-1] to use the
    // bindings stored in pDescriptorSets[0..descriptorSetCount-1] 
    // for subsequent rendering commands (either compute or graphics,
    // according to the pipelineBindPoint). Any bindings that were 
    // previously applied via these sets are no longer valid.
    
    NO_CHECK( qvkCmdBindDescriptorSets( vk.command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                vk.pipeline_layout, 0, 1 + multitexture, 
                pDesSet, 0, NULL) );


    
    // issue draw call
    if (indexed)
    {
        NO_CHECK( qvkCmdDrawIndexed(vk.command_buffer,  pTess->numIndexes, 1, 0, 0, 0) );
    }
    else
    {
        NO_CHECK( qvkCmdDraw(vk.command_buffer,  pTess->numVertexes, 1, 0, 0) );
    }

    s_depth_attachment_dirty = VK_TRUE;
}



void vk_resetGeometryBuffer(void)
{
	// Reset geometry buffer's current offsets.
	shadingDat.xyz_elements = 0;
	shadingDat.colorElemCount = 0;
	shadingDat.index_buffer_offset = 0;
    
    s_depth_attachment_dirty = VK_FALSE;

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
    if(s_depth_attachment_dirty)
    {
        VkClearAttachment attachments = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .clearValue.depthStencil.depth = 1.0f};

        VkClearRect clear_rect = {
            .rect = vk.renderArea,
            .baseArrayLayer = 0,
            .layerCount = 1};

        //ri.Printf(PRINT_ALL, "(%d, %d, %d, %d)\n", 
        //        clear_rect.rect.offset.x, clear_rect.rect.offset.y, 
        //        clear_rect.rect.extent.width, clear_rect.rect.extent.height);

        NO_CHECK( qvkCmdClearAttachments(vk.command_buffer, 1, &attachments, 1, &clear_rect) );
        
        s_depth_attachment_dirty = VK_FALSE;
    }
}

/*
===================
Perform dynamic lighting with another rendering pass
===================
*/
static void RB_ProjectDlightTexture( struct shaderCommands_s * const pTess, 
        float (* const pTexcoords)[2], uint8_t (* const pColors)[4],
        const uint32_t nDlights, struct dlight_s* const pDlights)
{
	unsigned char clipBits[SHADER_MAX_VERTEXES];

    uint32_t l;
	for (l = 0; l < nDlights; ++l)
    {
		//dlight_t	*dl;

		if ( ( pTess->dlightBits & ( 1 << l ) ) == 0)
        {
			continue;	// this surface definately doesn't have any of this light
		}
        

        uint32_t isAdditive = pDlights[l].additive > 0 ? 1 : 0;
        uint32_t cullType = pTess->shader->cullType;
        uint32_t polyOffset = pTess->shader->polygonOffset;

        // float (* const pTexcoords)[2] = pTess->svars.texcoords[0];
        // uint8_t (* const pColors)[4] = pTess->svars.colors;

        vec3_t origin;
		VectorCopy( pDlights[l].transformed, origin );


        float radius = pDlights[l].radius;
		
        float scale = 1.0f / radius;
    	float modulate = 0.0f;

        float floatColor[3] = {
		    pDlights[l].color[0] * 255.0f,
		    pDlights[l].color[1] * 255.0f,
		    pDlights[l].color[2] * 255.0f
        };

        uint32_t i;
		for ( i = 0 ; i < pTess->numVertexes; ++i)
        {
			vec3_t dist;

			VectorSubtract( origin, pTess->xyz[i], dist );

            float u = 0.5f + dist[0] * scale;
            float v = 0.5f + dist[1] * scale;
			
            pTexcoords[i][0] = u;
			pTexcoords[i][1] = v;

			uint32_t clip = 0;
			if ( u < 0.0f ) {
				clip |= 1;
			}
            else if ( u > 1.0f ) {
				clip |= 2;
			}

			if ( v < 0.0f ) {
				clip |= 4;
			}
            else if ( v > 1.0f ) {
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
			pColors[i][0] = (floatColor[0] * modulate);
			pColors[i][1] = (floatColor[1] * modulate);
			pColors[i][2] = (floatColor[2] * modulate);
			pColors[i][3] = 255;
		}

      
		// build a list of triangles that need light
		uint32_t numIndexes = 0;
		for ( i = 0 ; i < pTess->numIndexes; i += 3 )
        {
			if ( clipBits[pTess->indexes[i]] & clipBits[pTess->indexes[i+1]] & clipBits[pTess->indexes[i+2]] )
            {
				continue;	// not lighted
			}
			numIndexes += 3;
		}

		if ( numIndexes == 0 ) {
			continue;
		}
        
        R_UpdatePerformanceCounters( pTess->numVertexes, numIndexes, 0);
		
        		
        // include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces
        // don't add light where they aren't rendered


        vk_shade( g_globalPipelines.dlight_pipelines[isAdditive][cullType][polyOffset], 
             pTess, &tr.dlightImage->descriptor_set, VK_FALSE, VK_TRUE);
	}
}



static void RB_FogPass( struct shaderCommands_s * const pTess, struct shader_s * const pShader)
{

    RB_SetTessFogColor(pTess->svars.colors, pTess->fogNum, pTess->numVertexes);

	RB_CalcFogTexCoords( pTess->svars.texcoords[0], pTess->numVertexes);

    // ri.Printf(PRINT_ALL, "isFog: %d. \n", pTess->shader->fogPass);
    
    vk_shade(g_globalPipelines.fog_pipelines[pShader->fogPass - 1][pShader->cullType][pShader->polygonOffset], 
            pTess, &tr.fogImage->descriptor_set, VK_FALSE, VK_TRUE);

}

///////////////////////////////////////////////////////////////////////////////////

static void R_BindAnimatedImage( textureBundle_t *bundle, int tmu, float time)
{

	if ( bundle->isVideoMap ) {
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);
		return;
	}

	if ( bundle->numImageAnimations <= 1 )
    {
        shadingDat.curDescriptorSets[tmu] = bundle->image[0]->descriptor_set;
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	int index = (int)( time * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
	index >>= FUNCTABLE_SIZE2;

	if ( index < 0 ) {
		index = 0;	// may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

    shadingDat.curDescriptorSets[tmu] = bundle->image[index]->descriptor_set;
}

/////////////////////////////////////////////////////////////////////////////////

void RB_StageIteratorGeneric(struct shaderCommands_s * const pTess, VkBool32 isPortal, VkBool32 is2D)
{

	RB_DeformTessGeometry(pTess);

	// call shader function
	//

	vk_UploadXYZI(pTess->xyz, pTess->numVertexes, pTess->indexes, pTess->numIndexes);
    
    updateMVP(isPortal, is2D, getptr_modelview_matrix() );


    // r_showsky will let all the sky blocks be drawn in
	// front of everything to allow developers to see how
	// much sky is getting sucked in.
   	// VULKAN

    if(pTess->shader->isSky)
    {
        vk_rcdUpdateViewport(is2D, DEPTH_RANGE_ONE);
    }
    else if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
    {
        vk_rcdUpdateViewport(is2D, DEPTH_RANGE_WEAPON);
    }
    else
    {
        vk_rcdUpdateViewport(is2D, DEPTH_RANGE_NORMAL);
    }
    
    uint32_t i = 0;


	for (  i = 0; pTess->xstages[i] && (i < MAX_SHADER_STAGES); ++i )
	{
        struct shaderStage_s * const pCurShader = pTess->xstages[i];
		RB_ComputeColors( pCurShader );
		RB_ComputeTexCoords( pCurShader );

        // base, set state
		R_BindAnimatedImage( &pCurShader->bundle[0], 0, pTess->shaderTime );
		//
		// do multitexture
		//
        qboolean multitexture = (pCurShader->bundle[1].image[0] != NULL);

		if ( multitexture )
		{
            // DrawMultitextured( input, stage );
            // output = t0 * t1 or t0 + t1

            // t0 = most upstream according to spec
            // t1 = most downstream according to spec
            // this is an ugly hack to work around a GeForce driver
            // bug with multitexture and clip planes


            // lightmap/secondary pass

            R_BindAnimatedImage( &pCurShader->bundle[1], 1, pTess->shaderTime );
            // disable texturing on TEXTURE1, then select TEXTURE0
		}

		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer )
        {
            if( pCurShader->bundle[0].isLightmap || pCurShader->bundle[1].isLightmap )
		    {
                shadingDat.curDescriptorSets[0] = tr.whiteImage->descriptor_set;
            }
		}


        if (backEnd.viewParms.isMirror)
        {
            vk_shade(pCurShader->vk_mirror_pipeline, pTess, shadingDat.curDescriptorSets, multitexture, VK_TRUE);
        }
        else if (isPortal)
        {
            vk_shade(pCurShader->vk_portal_pipeline, pTess, shadingDat.curDescriptorSets, multitexture, VK_TRUE);
        }
        else
        {
            vk_shade(pCurShader->vk_pipeline, pTess, shadingDat.curDescriptorSets, multitexture, VK_TRUE);
        }                 
	}


	// 
	// now do any dynamic lighting needed
	//
	if ( pTess->dlightBits && pTess->shader->sort <= SS_OPAQUE && 
            !(pTess->shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) )
    {
	    RB_ProjectDlightTexture( pTess, pTess->svars.texcoords[0], pTess->svars.colors,
                    backEnd.refdef.num_dlights, backEnd.refdef.dlights);
	}

	//
	// now do fog
	//
	if ( pTess->fogNum && pTess->shader->fogPass )
    {
		RB_FogPass(pTess, pTess->shader);
	}
}
