#include "vk_instance.h"
#include "vk_pipelines.h"
#include "tr_shader.h"
// The graphics pipeline is the sequence of operations that take the vertices
// and textures of your meshes all the way to the pixels in the render targets
//
// The input assembler collects the raw vertex data from the buffers you specify
// and may also use an index buffer to repeat cartain elements without having to
// duplicate the vertex data itself.
//
// The vertex shader is run for every vertex and generally applies transformations
// to turn vertex positions from model space to screen space. It also passes
// per-vertex data down the pipeline
//
// The tessllation shaders allow you to subdivide geometry based on certain rules
// to increase the mesh quality. This is often make surfaces like brick walls
// and staircases look less flat when they are nearby.
//
// The geometry shader is run on every primitive(triangle, line, point) and can
// discard it or output more primitives than came in. This is similar to the
// tessellation shader, but much more flexible.
//
// The rasterization stage discretizes the primitives into fragments. These are
// the pixel elements that they fill on the framebuffer. Any fragments that fall
// outside the screen are discarded and the attributes outputted by the vertex
// shader are interpolated across the fragments.
//
// The fragment shader is invoked for every fragment that servives and determines
// which framebuffer(s) the fragment are written to and with which color and depth
// values. It can do this using the interpolated data from the vertex shader,
// which can include things like texture coordinates and normals for lighting.
//
// The color blending stage applies operations to mix different fragments that
// map to the same pixel in the framebuffer. Fragments can simply overwrite
// each other, add up or be mixed based opon transparency.
//

#define MAX_VK_PIPELINES        256


struct PipelineParameter_t {
    VkPipeline pipeline; // saved a copy for destroy. 
    uint32_t state_bits; // GLS_XXX flags
	
    enum CullType_t face_culling;
    enum Vk_Shader_Type shader_type;
	enum Vk_Shadow_Phase shadow_phase;

    VkBool32 polygon_offset;
	VkBool32 clipping_plane;
	VkBool32 mirror;
	VkBool32 line_primitives;
};


struct pipeline_tree_s {

    struct PipelineParameter_t par;
    
	struct pipeline_tree_s * left; //left child
	struct pipeline_tree_s * right; //right child
};

static struct pipeline_tree_s * pPlRoot = NULL;

static struct pipeline_tree_s mem_alloced[MAX_VK_PIPELINES];

//static struct VK_PipelineMgr_t s_created_ppl[MAX_VK_PIPELINES];
static uint32_t s_numPipelines = 0;


static int32_t ComparepPplPar(const struct pipeline_tree_s * const pTree, 
        const struct PipelineParameter_t* const par2)
{

//    ri.Printf(PRINT_WARNING, "%d, %d \n",
//            pTree->par.state_bits, par2->state_bits);
    if(pTree->par.state_bits == par2->state_bits)
    {

    int32_t i1 = (pTree->par.face_culling << 8) |
        (pTree->par.shader_type << 6) |
        (pTree->par.shadow_phase << 4) | 
        (pTree->par.polygon_offset << 3) |
        (pTree->par.clipping_plane << 2) |
        (pTree->par.mirror << 1) | pTree->par.line_primitives ;

    int32_t i2 = (par2->face_culling << 8) |
        (par2->shader_type << 6) |
        (par2->shadow_phase << 4) | 
        (par2->polygon_offset << 3) |
        (par2->clipping_plane << 2) |
        (par2->mirror << 1) | par2->line_primitives;
        return i1 - i2;
    }
    else
        return (pTree->par.state_bits - par2->state_bits);
}



struct pipeline_tree_s *
InsertPipelineToTree(struct pipeline_tree_s * pTree, const struct PipelineParameter_t * const par)
{
    if( pTree == NULL)
	{
        // ri.Printf(PRINT_WARNING, "make new pipeline:%d\n", s_numPipelines);
        // pTree = (struct pipeline_tree_s *) malloc( sizeof(struct pipeline_tree_s) );

        pTree = &mem_alloced[s_numPipelines++];
        pTree->par = *par;
        pTree->left = pTree->right = NULL;

        // Create and return a one-node tree;
    }
    else
    {
        int32_t comp = ComparepPplPar(pTree, par);

        if( comp < 0 )
		    pTree->left = InsertPipelineToTree(pTree->left, par);
        else if( comp > 0 )
		    pTree->right = InsertPipelineToTree(pTree->right, par);

        // Else X is in the tree already,
    }

    return pTree;
}


void printPipelineTree(struct pipeline_tree_s * pTree)
{
	if(pTree != NULL)
	{
		printPipelineTree(pTree->left);

        ri.Printf(PRINT_ALL, "state bits:%d, face culling:%d\n", 
            pTree->par.state_bits, pTree->par.face_culling);

		printPipelineTree(pTree->right);
	}
}


struct pipeline_tree_s *
FindPipelineFromTree(struct pipeline_tree_s * pTree , const struct PipelineParameter_t * const par)
{
    if(pTree == NULL)
        return NULL;
    
    int32_t comp = ComparepPplPar(pTree, par);
    
    if( comp < 0 )
        return FindPipelineFromTree(pTree->left, par);
    else if(comp > 0)
        return FindPipelineFromTree(pTree->right, par);
    else
        return pTree;
}


void DestroySearchTree(struct pipeline_tree_s * pTree)
{
	if(pTree != NULL)
	{

        DestroySearchTree(pTree->left);
        // ri.Printf(PRINT_ALL, "free left child.\n");
        DestroySearchTree(pTree->right);
        // ri.Printf(PRINT_ALL, "free right child.\n");

        qvkDestroyPipeline(vk.device, pTree->par.pipeline, NULL);
        
        // free(pTree);
        // memset(pTree, 0, sizeof(struct pipeline_tree_s));
	}
}


void R_PipelineList_f(void)
{
    ri.Printf(PRINT_ALL, " Total pipeline created: %d\n", s_numPipelines);
    printPipelineTree(pPlRoot);
}


// uniform values in the shaders need to be specified during pipeline creation
// transformation matrix to the vertex shader, or to create texture samplers
// in the fragment shader.



void vk_createPipelineLayout(void)
{
    ri.Printf(PRINT_ALL, " Create ipeline layout. \n");
 
    VkPushConstantRange push_range;
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_range.offset = 0;
    push_range.size = 128; // 16 mvp floats + 16

    const VkDescriptorSetLayout setLayoutArray[2] = {vk.set_layout, vk.set_layout};

    VkPipelineLayoutCreateInfo desc;
    desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;

    // setLayoutCount: the number of descriptor sets included in the pipeline layout.
    // pSetLayouts: a pointer to an array of VkDescriptorSetLayout objects.
    // The maximum number of the descriptor sets that can be bound at once at least 4.
    desc.setLayoutCount = 2;
    desc.pSetLayouts = setLayoutArray;

    // pushConstantRangeCount is the number of push constant ranges 
    // included in the pipeline layout.
    //
    // pPushConstantRanges is a pointer to an array of VkPushConstantRange
    // structures defining a set of push constant ranges for use in 
    // a single pipeline layout.
    // 
    // In addition to descriptor set layouts, a pipeline layout also 
    // describes how many push constants can be accessed by each stage
    // of the pipeline.

    desc.pushConstantRangeCount = 1;
    desc.pPushConstantRanges = &push_range;

    // Access to descriptor sets from a pipeline is accomplished through
    // a pipeline layout. Zero or more descriptor set layouts and zero or
    // more push constant ranges are combined to form a pipeline layout 
    // object which describes the complete set of resources that can be
    // accessed by a pipeline. The pipeline layout represents a sequence
    // of descriptor sets with each having a specific layout. 
    // This sequence of layouts is used to determine the interface between
    // shader stages and shader resources. 
    //
    // Each pipeline is created using a pipeline layout.
    VK_CHECK( qvkCreatePipelineLayout(vk.device, &desc, NULL, &vk.pipeline_layout) );
}

void vk_destroy_pipeline_layout(void)
{
    qvkDestroyPipelineLayout(vk.device, vk.pipeline_layout, NULL);
}

struct Specialization_Data {
    int32_t alpha_test_func;
};


void vk_create_pipeline(
        uint32_t state_bits,
        enum Vk_Shader_Type shader_type,
        enum CullType_t face_culling,
        enum Vk_Shadow_Phase shadow_phase,
        VkBool32 clipping_plane,
        VkBool32 mirror,
        VkBool32 polygon_offset,
        VkBool32 isLine, 
        VkPipeline* pPipeLine)
{


    // Two stages: vs and fs
    VkPipelineShaderStageCreateInfo shaderStages[2];

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    //shaderStages[0].module = *vs_module;
    shaderStages[0].pName = "main";
    shaderStages[0].pNext = NULL;
    shaderStages[0].flags = 0;
	shaderStages[0].pSpecializationInfo = NULL;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    //shaderStages[1].module = *fs_module;
    shaderStages[1].pName = "main";
    shaderStages[1].pNext = NULL;
    shaderStages[1].flags = 0;
    shaderStages[1].pSpecializationInfo = NULL;

    
    struct Specialization_Data specialization_data;
    VkSpecializationMapEntry specialization_entries;
    VkSpecializationInfo specialization_info;

    if(state_bits & GLS_ATEST_BITS)
    {
        switch ( state_bits & GLS_ATEST_BITS )
        {
            case 0:
                specialization_data.alpha_test_func = 0;
                break;
            case GLS_ATEST_GT_0:
                specialization_data.alpha_test_func = 1;
                break;
            case GLS_ATEST_LT_80:
                specialization_data.alpha_test_func = 2;
                break;
            case GLS_ATEST_GE_80:
                specialization_data.alpha_test_func = 3;
                break;
            default:
                assert( 0 );
                break;
        }


        specialization_entries.constantID = 0;
        specialization_entries.offset = offsetof(struct Specialization_Data, alpha_test_func);
        specialization_entries.size = sizeof(int32_t);
        // This structure contains the information required to specialize a shader
        // which is the process of building a shader with some of its constants
        // compiled in. A typical Vulkan implementation will delay final code
        // generation for pipelines until vkCreatePipelines is called.
        // This allows the values of specialization constants to be considered
        // during the final passes of the optimization over the shader.
        specialization_info.mapEntryCount = 1;
        specialization_info.pMapEntries = &specialization_entries;
        specialization_info.dataSize = sizeof(struct Specialization_Data);
        specialization_info.pData = &specialization_data;


        // pSpecializationInfo allows you to specify values for shader constants,
        // you can use a single shader module where its behavior can be configured
        // at pipeline creation by specifying different values fot the constants
        // used in it. This is more effient than configuring the shader using 
        // variables at render time, because the compiler can do optimizations.
  
        shaderStages[1].pSpecializationInfo = &specialization_info;
    }


    vk_specifyShaderModule(shader_type, clipping_plane, &shaderStages[0].module, &shaderStages[1].module);

	// ============== Vertex Input Description =================
    // Applications specify vertex input attribute and vertex input binding
    // descriptions as part of graphics pipeline creation	
    // A vertex binding describes at which rate to load data
    // from memory throughout the vertices

    VkVertexInputBindingDescription bindings[4];
    // xyz array
    bindings[0].binding = 0;
    // The stride parameter specifies the number of bytes from one entry to the next
    bindings[0].stride = sizeof(vec4_t);
    // move to the next data entry after each vertex
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    // color array
    bindings[1].binding = 1;
    bindings[1].stride = 4; //sizeof(color4ub_t);
    bindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    // st0 array
    bindings[2].binding = 2;
    bindings[2].stride = sizeof(vec2_t);
    bindings[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    // st1 array
    bindings[3].binding = 3;
    bindings[3].stride = sizeof(vec2_t);
    bindings[3].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


    // Describes how to handle vertex input
    VkVertexInputAttributeDescription attribs[4];
    // xyz
    attribs[0].location = 0;
    attribs[0].binding = 0;
    attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attribs[0].offset = 0;
    // color
    attribs[1].location = 1;
    attribs[1].binding = 1;
    attribs[1].format = VK_FORMAT_R8G8B8A8_UNORM;
    attribs[1].offset = 0;
    // st0
    attribs[2].location = 2;
    attribs[2].binding = 2;
    attribs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribs[2].offset = 0;
    // st1
    attribs[3].location = 3;
    attribs[3].binding = 3;
    attribs[3].format = VK_FORMAT_R32G32_SFLOAT;
    attribs[3].offset = 0;


	VkPipelineVertexInputStateCreateInfo vertex_input_state;
	vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state.pNext = NULL;
	vertex_input_state.flags = 0;
	vertex_input_state.vertexBindingDescriptionCount = (shader_type == ST_SINGLE_TEXTURE) ? 3 : 4;
	vertex_input_state.pVertexBindingDescriptions = bindings;
	vertex_input_state.vertexAttributeDescriptionCount = (shader_type == ST_SINGLE_TEXTURE) ? 3 : 4;
	vertex_input_state.pVertexAttributeDescriptions = attribs;

	//
	// Primitive assembly.
	//
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.pNext = NULL;
	input_assembly_state.flags = 0;
	input_assembly_state.topology = isLine ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;

	//
	// Viewport.
	//
	VkPipelineViewportStateCreateInfo viewport_state;
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = NULL;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = NULL; // dynamic viewport state
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = NULL; // dynamic scissor state

	//
	// Rasterization.
	// The rasterizer takes the geometry that is shaped by the vertices
    // from the vertex shader and turns it into fragments to be colored
    // by the fragment shader. It also performs depth testing, face culling
    // and the scissor test.
	VkPipelineRasterizationStateCreateInfo rasterization_state;
	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.pNext = NULL;
	rasterization_state.flags = 0;
	rasterization_state.depthClampEnable = VK_FALSE;
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state.polygonMode = (state_bits & GLS_POLYMODE_LINE) ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;

    switch ( face_culling )
    {
        case CT_TWO_SIDED:
            rasterization_state.cullMode = VK_CULL_MODE_NONE;
            break;
        case CT_FRONT_SIDED:
            rasterization_state.cullMode = 
                (mirror ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT);
            break;
        case CT_BACK_SIDED:
            rasterization_state.cullMode = 
                (mirror ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_FRONT_BIT);
            break;
    }

    
    // how fragments are generated for geometry.
	rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE; // Q3 defaults to clockwise vertex order

	rasterization_state.depthBiasEnable = polygon_offset ? VK_TRUE : VK_FALSE;
	rasterization_state.depthBiasConstantFactor = 0.0f; // dynamic depth bias state
	rasterization_state.depthBiasClamp = 0.0f; // dynamic depth bias state
	rasterization_state.depthBiasSlopeFactor = 0.0f; // dynamic depth bias state
	rasterization_state.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample_state;
	multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state.pNext = NULL;
	multisample_state.flags = 0;
	multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_state.sampleShadingEnable = VK_FALSE;
	multisample_state.minSampleShading = 1.0f;
	multisample_state.pSampleMask = NULL;
	multisample_state.alphaToCoverageEnable = VK_FALSE;
	multisample_state.alphaToOneEnable = VK_FALSE;

    // If you are using a depth and/or stencil buffer, then you also need to configure
    // the depth and stencil tests.
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state.pNext = NULL;
	depth_stencil_state.flags = 0;
	depth_stencil_state.depthTestEnable = (state_bits & GLS_DEPTHTEST_DISABLE) ? VK_FALSE : VK_TRUE;
	depth_stencil_state.depthWriteEnable = (state_bits & GLS_DEPTHMASK_TRUE) ? VK_TRUE : VK_FALSE;
	depth_stencil_state.depthCompareOp = (state_bits & GLS_DEPTHFUNC_EQUAL) ? VK_COMPARE_OP_EQUAL : VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
	
    switch (shadow_phase) 
    {
        case SHADOWS_RENDERING_DISABLED:
        {
            depth_stencil_state.stencilTestEnable = VK_FALSE;
	    	memset(&depth_stencil_state.front, 0, sizeof(depth_stencil_state.front));
		    memset(&depth_stencil_state.back, 0, sizeof(depth_stencil_state.back));
        }break;
        case SHADOWS_RENDERING_EDGES:
        {
            depth_stencil_state.stencilTestEnable = VK_TRUE;

        	depth_stencil_state.front.failOp = VK_STENCIL_OP_KEEP;
            depth_stencil_state.front.passOp = ( face_culling == CT_FRONT_SIDED ? 
              VK_STENCIL_OP_INCREMENT_AND_CLAMP : VK_STENCIL_OP_DECREMENT_AND_CLAMP );
            depth_stencil_state.front.depthFailOp = VK_STENCIL_OP_KEEP;
            depth_stencil_state.front.compareOp = VK_COMPARE_OP_ALWAYS;
            depth_stencil_state.front.compareMask = 255;
            depth_stencil_state.front.writeMask = 255;
            depth_stencil_state.front.reference = 0;

            depth_stencil_state.back = depth_stencil_state.front;
        }break;
        
        case SHADOWS_RENDERING_FULLSCREEN_QUAD:
        {
            depth_stencil_state.stencilTestEnable = VK_TRUE;

            depth_stencil_state.front.failOp = VK_STENCIL_OP_KEEP;
            depth_stencil_state.front.passOp = VK_STENCIL_OP_KEEP;
            depth_stencil_state.front.depthFailOp = VK_STENCIL_OP_KEEP;
            depth_stencil_state.front.compareOp = VK_COMPARE_OP_NOT_EQUAL;
            depth_stencil_state.front.compareMask = 255;
            depth_stencil_state.front.writeMask = 255;
            depth_stencil_state.front.reference = 0;

            depth_stencil_state.back = depth_stencil_state.front;
        }break;
    }

	depth_stencil_state.minDepthBounds = 0.0;
	depth_stencil_state.maxDepthBounds = 0.0;


    //After a fragment shader has returned a color, it needs to be combined
    //with the color that is already in the framebuffer. This transformation
    //is known as color blending and there are two ways to do it
    //
    // 1) Mix the old and new value to produce a final color.
    // 2) combine the old and the new value using a bitwise operation.

    // contains the configuraturation per attached framebuffer
    
	VkPipelineColorBlendAttachmentState attachment_blend_state = {};
	attachment_blend_state.blendEnable = (state_bits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) ? VK_TRUE : VK_FALSE;

	if (shadow_phase == SHADOWS_RENDERING_EDGES)
		attachment_blend_state.colorWriteMask = 0;
	else
		attachment_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	
	if (attachment_blend_state.blendEnable)
    {
		switch (state_bits & GLS_SRCBLEND_BITS)
        {
			case GLS_SRCBLEND_ZERO:
				attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
				break;
			default:
				ri.Error( ERR_DROP, "create_pipeline: invalid src blend state bits\n" );
				break;
		}
		switch (state_bits & GLS_DSTBLEND_BITS)
        {
			case GLS_DSTBLEND_ZERO:
				attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
				break;
			default:
				ri.Error( ERR_DROP, "create_pipeline: invalid dst blend state bits\n" );
				break;
		}

		attachment_blend_state.srcAlphaBlendFactor = attachment_blend_state.srcColorBlendFactor;
		attachment_blend_state.dstAlphaBlendFactor = attachment_blend_state.dstColorBlendFactor;
		attachment_blend_state.colorBlendOp = VK_BLEND_OP_ADD;
		attachment_blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
	}

    // Contains the global color blending settings
	VkPipelineColorBlendStateCreateInfo blend_state;
	blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend_state.pNext = NULL;
	blend_state.flags = 0;
	blend_state.logicOpEnable = VK_FALSE;
	blend_state.logicOp = VK_LOGIC_OP_COPY;
	blend_state.attachmentCount = 1;
	blend_state.pAttachments = &attachment_blend_state;
	blend_state.blendConstants[0] = 0.0f;
	blend_state.blendConstants[1] = 0.0f;
	blend_state.blendConstants[2] = 0.0f;
	blend_state.blendConstants[3] = 0.0f;


    // A limited amount of the state that we've specified in the previous
    // structs can actually be changed without recreating the pipeline.
    // Examples are the size of the viewport, line width and blend constants
    // If we want to do that, we have to fill in a VkPipelineDynamicStateCreateInfo
    // structure like this.
	VkPipelineDynamicStateCreateInfo dynamic_state;
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pNext = NULL;
	dynamic_state.flags = 0;
	dynamic_state.dynamicStateCount = 3;
	const VkDynamicState dynamic_state_array[3] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
	dynamic_state.pDynamicStates = dynamic_state_array;


	VkGraphicsPipelineCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // pNext is NULL or a pointer to an extension-specific structure.
	create_info.pNext = NULL;
    // flags is a bitmask of VkPipelineCreateFlagBits
    // specifying how the pipeline will be generated.
	create_info.flags = 0;
    // stageCount is the number of entries in the pStages array.
	create_info.stageCount = 2;
    // pStages is an array of size stageCount structures of type
    // VkPipelineShaderStageCreateInfo describing the set of the 
    // shader stages to be included in the graphics pipeline.
	create_info.pStages = shaderStages;
    // pVertexInputState is a pointer to an instance of the
    // VkPipelineVertexInputStateCreateInfo structure.
	create_info.pVertexInputState = &vertex_input_state;
    // pInputAssemblyState is a pointer to an instance of the 
    // VkPipelineInputAssemblyStateCreateInfo structure which
    // determines input assembly behavior, as described in Drawing Commands.
	create_info.pInputAssemblyState = &input_assembly_state;
    // pTessellationState is a pointer to an instance of the 
    // VkPipelineTessellationStateCreateInfo structure, and is ignored 
    // if the pipeline does not include a tessellation control shader 
    // stage and tessellation evaluation shader stage
	create_info.pTessellationState = NULL;
    // pViewportState is a pointer to an instance of the
    // VkPipelineViewportStateCreateInfo structure, and 
    // is ignored if the pipeline has rasterization disabled.
	create_info.pViewportState = &viewport_state;
    // pRasterizationState is a pointer to an instance of the
    // VkPipelineRasterizationStateCreateInfo structure.
	create_info.pRasterizationState = &rasterization_state;
    // pMultisampleState is a pointer to an instance of the 
    // VkPipelineMultisampleStateCreateInfo, and is ignored
    // if the pipeline has rasterization disabled.
	create_info.pMultisampleState = &multisample_state;

    // pDepthStencilState is a pointer to an instance of the
    // VkPipelineDepthStencilStateCreateInfe structure, and is ignored
    // if the pipeline has rasterization disabled or 
    // if the subpass of the render pass the pipeline is created
    // against does not use a depth/stencil attachment.
	create_info.pDepthStencilState = &depth_stencil_state;

    // pColorBlendState is a pointer to an instance of the 
    // VkPipelineColorBlendStateCreateInfo structure, and is ignored 
    // if the pipeline has rasterization disabled or if the subpass of
    // the render pass the pipeline is created against does not use 
    // any color attachments.
	create_info.pColorBlendState = &blend_state;

    // pDynamicState is a pointer to VkPipelineDynamicStateCreateInfo and
    // is used to indicate which properties of the pipeline state object 
    // are dynamic and can be changed independently of the pipeline state.
    // This can be NULL, which means no state in the pipeline is considered dynamic.
	create_info.pDynamicState = &dynamic_state;
    // layout is the description of binding locations used 
    // by both the pipeline and descriptor sets used with the pipeline.
	create_info.layout = vk.pipeline_layout;
    // renderPass is a handle to a render pass object describing the environment
    // in which the pipeline will be used; the pipeline must only be used with
    // an instance of any render pass compatible with the one provided. 
    // See Render Pass Compatibility for more information.
	create_info.renderPass = vk.render_pass;

    // A pipeline derivative is a child pipeline created from a parent pipeline,
    // where the child and parent are expected to have much commonality. 
    // The goal of derivative pipelines is that they be cheaper to create 
    // using the parent as a starting point, and that it be more efficient
    // on either host or device to switch/bind between children of the same
    // parent.
    // subpass is the index of the subpass in the render pass 
    // where this pipeline will be used.
	create_info.subpass = 0;
    // basePipelineHandle is a pipeline to derive from.
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex = -1;

    // Graphics pipelines consist of multiple shader stages, 
    // multiple fixed-function pipeline stages, and a pipeline layout.
    // To create graphics pipelines
    // VK_NULL_HANDLE indicating that pipeline caching is disabled; 
    // TODO: provide the handle of a valid pipeline cache object, 
    // 1 is the length of the pCreateInfos and pPipelines arrays.
    //
	VK_CHECK(qvkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &create_info, NULL, pPipeLine));
}


void vk_create_shader_stage_pipelines(shaderStage_t *pStage, shader_t* pShader)
{
    // ri.Printf(PRINT_ALL, " Create shader stage pipeline for %s. \n", pShader->name);
    
    enum Vk_Shader_Type def_shader_type = ST_SINGLE_TEXTURE;
 
    if (pStage->bundle[1].image[0] == NULL)
        def_shader_type = ST_SINGLE_TEXTURE;
    else if (pShader->multitextureEnv == GL_MODULATE)
        def_shader_type = ST_MULTI_TEXURE_MUL;
    else if (pShader->multitextureEnv == GL_ADD)
        def_shader_type = ST_MULTI_TEXURE_ADD;
    else
        ri.Error(ERR_FATAL, "Vulkan: could not create pipelines for q3 shader '%s'\n", pShader->name);

    struct PipelineParameter_t plPar;
    struct pipeline_tree_s * plTr = NULL; 
    

    plPar.state_bits = pStage->stateBits; 
    plPar.face_culling = pShader->cullType;
    plPar.shader_type = def_shader_type; 
    plPar.shadow_phase = SHADOWS_RENDERING_DISABLED;
    plPar.line_primitives = VK_FALSE;
    plPar.polygon_offset = pShader->polygonOffset;

    plPar.clipping_plane = VK_FALSE;
    plPar.mirror = VK_FALSE; 

/*
    vk_find_pipeline( pStage->stateBits, def_shader_type, pShader->cullType, SHADOWS_RENDERING_DISABLED,
            VK_FALSE, VK_FALSE, pShader->polygonOffset, VK_FALSE, &pStage->vk_pipeline);
*/  
    plTr = FindPipelineFromTree(pPlRoot, &plPar);
    
    if(plTr == NULL)
    {
        vk_create_pipeline( 
         plPar.state_bits, plPar.shader_type, plPar.face_culling, plPar.shadow_phase,
         plPar.clipping_plane, plPar.mirror, plPar.polygon_offset, plPar.line_primitives, 
         &plPar.pipeline );

        InsertPipelineToTree(pPlRoot, &plPar);

        pStage->vk_pipeline = plPar.pipeline;
    }
    else 
    {
        pStage->vk_pipeline = plTr->par.pipeline;
    }

    plPar.clipping_plane = VK_TRUE;
/*
    vk_find_pipeline( pStage->stateBits, def_shader_type, pShader->cullType, SHADOWS_RENDERING_DISABLED,
            VK_TRUE, VK_FALSE, pShader->polygonOffset, VK_FALSE, &pStage->vk_portal_pipeline);
*/
    plTr = FindPipelineFromTree(pPlRoot, &plPar);
    
    if(plTr == NULL)
    {
        vk_create_pipeline( 
         plPar.state_bits, plPar.shader_type, plPar.face_culling, plPar.shadow_phase,
         plPar.clipping_plane, plPar.mirror, plPar.polygon_offset, plPar.line_primitives, 
         &plPar.pipeline );

        InsertPipelineToTree(pPlRoot, &plPar);

        pStage->vk_portal_pipeline = plPar.pipeline;
    }
    else 
    {
        pStage->vk_portal_pipeline = plTr->par.pipeline;
    }
 
    plPar.mirror = VK_TRUE;
/*
    vk_find_pipeline( pStage->stateBits, def_shader_type, pShader->cullType, SHADOWS_RENDERING_DISABLED,
            VK_TRUE, VK_TRUE, pShader->polygonOffset, VK_FALSE, &pStage->vk_mirror_pipeline);
*/
    plTr = FindPipelineFromTree(pPlRoot, &plPar);
    
    if(plTr == NULL)
    {
        vk_create_pipeline( 
         plPar.state_bits, plPar.shader_type, plPar.face_culling, plPar.shadow_phase,
         plPar.clipping_plane, plPar.mirror, plPar.polygon_offset, plPar.line_primitives, 
         &plPar.pipeline );

        InsertPipelineToTree(pPlRoot, &plPar);

        pStage->vk_mirror_pipeline = plPar.pipeline;
    }
    else 
    {
        pStage->vk_mirror_pipeline = plTr->par.pipeline;
    }
}


void vk_destroyShaderStagePipeline(void)
{
    ri.Printf(PRINT_ALL, " Destroy %d shader stage pipeline. \n", s_numPipelines);
    qvkDeviceWaitIdle(vk.device);

    uint32_t i;
    for(i = 0; i < s_numPipelines; ++i)
        if(mem_alloced[i].par.pipeline != VK_NULL_HANDLE) {
            qvkDestroyPipeline(vk.device, mem_alloced[i].par.pipeline, NULL);
            mem_alloced[i].par.pipeline = VK_NULL_HANDLE;
        }

    memset(mem_alloced, 0, s_numPipelines);
    s_numPipelines = 0;

    if(pPlRoot != NULL)
    {
        DestroySearchTree(pPlRoot);
        pPlRoot = NULL;
    }
}


void vk_InitShaderStagePipeline(void)
{

    uint32_t i;
    for(i = 0; i < s_numPipelines; ++i)
        if(mem_alloced[i].par.pipeline != VK_NULL_HANDLE) {
            qvkDestroyPipeline(vk.device, mem_alloced[i].par.pipeline, NULL);
            mem_alloced[i].par.pipeline = VK_NULL_HANDLE;
        }

    memset(mem_alloced, 0, s_numPipelines);
    s_numPipelines = 0;

    if(pPlRoot != NULL)
    {
        DestroySearchTree(pPlRoot);
        pPlRoot = NULL;
    }
    
    struct PipelineParameter_t plPar;
    
    plPar.state_bits = 256; 
    plPar.face_culling = CT_FRONT_SIDED;
    plPar.shader_type = ST_SINGLE_TEXTURE; 
    plPar.shadow_phase = SHADOWS_RENDERING_DISABLED;
    plPar.line_primitives = VK_FALSE;
    plPar.polygon_offset = VK_FALSE;
    plPar.clipping_plane = VK_FALSE;
    plPar.mirror = VK_FALSE;

    vk_create_pipeline( 
            plPar.state_bits, plPar.shader_type, plPar.face_culling, plPar.shadow_phase,
            plPar.clipping_plane, plPar.mirror, plPar.polygon_offset, plPar.line_primitives, 
            &plPar.pipeline );

    pPlRoot = InsertPipelineToTree(NULL, &plPar);
}
