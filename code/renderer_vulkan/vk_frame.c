#include "tr_cvar.h"
#include "vk_instance.h"
#include "vk_shade_geometry.h"
#include "vk_swapchain.h"
#include "R_GetMicroSeconds.h"
#include "vk_image.h"
#include "vk_frame.h"
#include "vk_cmd.h"
#include "ref_import.h" 
//  Synchronization of access to resources is primarily the responsibility
//  of the application in Vulkan. The order of execution of commands with
//  respect to the host and other commands on the device has few implicit
//  guarantees, and needs to be explicitly specified. Memory caches and 
//  other optimizations are also explicitly managed, requiring that the
//  flow of data through the system is largely under application control.
//  Whilst some implicit guarantees exist between commands, five explicit
//  synchronization mechanisms are exposed by Vulkan:
//
//
//                          Fences
//
//  Fences can be used to communicate to the host that execution of some 
//  task on the device has completed. 
//
//  Fences are a synchronization primitive that can be used to insert a dependency
//  from a queue to the host. Fences have two states - signaled and unsignaled.
//  A fence can be signaled as part of the execution of a queue submission command. 
//  Fences can be unsignaled on the host with vkResetFences. Fences can be waited
//  on by the host with the vkWaitForFences command, and the current state can be
//  queried with vkGetFenceStatus.
//
//
//                          Semaphores
//
//  Semaphores can be used to control resource access across multiple queues.
//
//                          Events
//
//  Events provide a fine-grained synchronization primitive which can be
//  signaled either within a command buffer or by the host, and can be 
//  waited upon within a command buffer or queried on the host.
//
//                          Pipeline Barriers
//
//  Pipeline barriers also provide synchronization control within a command buffer,
//  but at a single point, rather than with separate signal and wait operations.
//
//                          Render Passes
//
//  Render passes provide a useful synchronization framework for most rendering tasks,
//  built upon the concepts in this chapter. Many cases that would otherwise need an
//  application to use other synchronization primitives can be expressed more 
//  efficiently as part of a render pass.
//
//

VkSemaphore sema_imageAvailable;
VkSemaphore sema_renderFinished;
VkFence fence_renderFinished;

/*
   Host access to fence must be externally synchronized.

   When a fence is submitted to a queue as part of a queue submission command, 
   it defines a memory dependency on the batches that were submitted as part
   of that command, and defines a fence signal operation which sets the fence
   to the signaled state.
*/

// static uint64_t t_frame_start = 0;

static void vk_createRenderFinishedFence(VkFence* const pFence)
{
    ri.Printf(PRINT_ALL, " Create render finished fence. \n");
    
    VkFenceCreateInfo fence_desc;
    fence_desc.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_desc.pNext = NULL;
    // VK_FENCE_CREATE_SIGNALED_BIT specifies that the fence object
    // is created in the signaled state. Otherwise, it is created 
    // in the unsignaled state.
    fence_desc.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // vk.device is the logical device that creates the fence.
    // fence_desc is an instance of the VkFenceCreateInfo structure
    // pAllocator controls host memory allocation as described
    // in the Memory Allocation chapter. which contains information
    // about how the fence is to be created.
    // "fence_renderFinished" is a handle in which the resulting
    // fence object is returned.

    VK_CHECK( qvkCreateFence(vk.device, &fence_desc, NULL, pFence) );
}


/* 
 * Semaphores represent flags that can be atomically set or reset
 * by the hardware, the views of which are coherent across queues
 * when you are setting the semaphone, the device will wait for it
 * to be unset, set it, and then return control to the caller.
 * Likewise, when reseting the semaphone, the device will waits for
 * the semaphone to be set, resets it, and then return control to
 * the caller.
 * Semaphones cannot be be explicitly signaled and waited on by
 * the device. Rather, they are signaled and waited on by the
 * queue operations such as vkQueueSubmit(). You use these objects
 * to synchronization access to resources across queues and form
 * an integral part of submission of work to the device.
 * We need one semaphone to signal that an image has been acquired
   and is ready for rendering; and another one to signal that 
   rendering has finished and presentation can happen.
*/
static void vk_createSyncSemaphores(VkSemaphore * const pImgAvailable,
        VkSemaphore * const pRenderFinished)
{
    ri.Printf(PRINT_ALL, " Create Semaphores: sema_imageAvailable, sema_renderFinished. \n");

    // Semaphores represent flags that can be automically set or
    // reset by the hardware, the views of which are coherent across
    // queues. when you are setting the semaphone, the device will
    // wait for it to be unset, set it, and then retunrn control
    // to the caller. Likewise, when resetting the semaphone, the 
    // device waits for the semaphore to be set, resets it, and the
    // return to the caller. This all happenas atomically. 
    //
    // Semaphones cannot be explicitly signaled or waited on by the
    // device. Rather, they are signaled and waited on by queue 
    // operations such as vkQueueSubmit().
    //
    VkSemaphoreCreateInfo desc;
    desc.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;

    // vk.device is the logical device that creates the semaphore.
    // &desc is a pointer to an instance of the VkSemaphoreCreateInfo structure
    // which contains information about how the semaphore is to be created.
    // When created, the semaphore is in the unsignaled state.
    VK_CHECK( qvkCreateSemaphore(vk.device, &desc, NULL, pImgAvailable) );
    VK_CHECK( qvkCreateSemaphore(vk.device, &desc, NULL, pRenderFinished) );
}

void vk_create_sync_primitives(void)
{
    vk_createSyncSemaphores(&sema_imageAvailable, &sema_renderFinished);
    vk_createRenderFinishedFence(&fence_renderFinished);
}


void vk_destroy_sync_primitives(void)
{
    ri.Printf(PRINT_ALL, " Destroy sema_imageAvailable sema_renderFinished fence_renderFinished\n");

    NO_CHECK( qvkDestroySemaphore(vk.device, sema_imageAvailable, NULL) );
	NO_CHECK( qvkDestroySemaphore(vk.device, sema_renderFinished, NULL) );

    // To destroy a fence, 
	NO_CHECK( qvkDestroyFence(vk.device, fence_renderFinished, NULL) );
}



// =====================================================================
// 
//            Renderpass.
// 
// =====================================================================
// 
// A render pass represents a collection of attachments, subpasses,
// and dependencies between the subpasses, and describes how the 
// attachments are used over the course of the subpasses. The use
// of a render pass in a command buffer is a render pass instance.
//
// An attachment description describes the properties of an attachment
// including its format, sample count, and how its contents are treated
// at the beginning and end of each render pass instance.
//
// A subpass represents a phase of rendering that reads and writes
// a subset of the attachments in a render pass. Rendering commands
// are recorded into a particular subpass of a render pass instance.
//
// A subpass description describes the subset of attachments that 
// is involved in the execution of a subpass. Each subpass can read
// from some attachments as input attachments, write to some as
// color attachments or depth/stencil attachments, and perform
// multisample resolve operations to resolve attachments. 
//
// A subpass description can also include a set of preserve attachments, 
// which are attachments that are not read or written by the subpass
// but whose contents must be preserved throughout the subpass.
//
// A subpass uses an attachment if the attachment is a color, 
// depth/stencil, resolve, or input attachment for that subpass
// (as determined by the pColorAttachments, pDepthStencilAttachment,
// pResolveAttachments, and pInputAttachments members of 
// VkSubpassDescription, respectively). A subpass does not use an
// attachment if that attachment is preserved by the subpass.
// The first use of an attachment is in the lowest numbered subpass
// that uses that attachment. Similarly, the last use of an attachment
// is in the highest numbered subpass that uses that attachment.
//
// The subpasses in a render pass all render to the same dimensions, 
// and fragments for pixel (x,y,layer) in one subpass can only read
// attachment contents written by previous subpasses at that same
// (x,y,layer) location.
//
// By describing a complete set of subpasses in advance, render passes
// provide the implementation an opportunity to optimize the storage 
// and transfer of attachment data between subpasses. In practice, 
// this means that subpasses with a simple framebuffer-space dependency
// may be merged into a single tiled rendering pass, keeping the
// attachment data on-chip for the duration of a render pass instance. 
// However, it is also quite common for a render pass to only contain
// a single subpass.
//
// Subpass dependencies describe execution and memory dependencies 
// between subpasses. A subpass dependency chain is a sequence of
// subpass dependencies in a render pass, where the source subpass
// of each subpass dependency (after the first) equals the destination
// subpass of the previous dependency.
//
// Execution of subpasses may overlap or execute out of order with
// regards to other subpasses, unless otherwise enforced by an 
// execution dependency. Each subpass only respects submission order
// for commands recorded in the same subpass, and the vkCmdBeginRenderPass
// and vkCmdEndRenderPass commands that delimit the render pass - 
// commands within other subpasses are not included. This affects 
// most other implicit ordering guarantees.
//
// A render pass describes the structure of subpasses and attachments
// independent of any specific image views for the attachments. 
// The specific image views that will be used for the attachments,
// and their dimensions, are specified in VkFramebuffer objects.
// ========================================================================

void vk_createRenderPass(VkDevice device, VkFormat colorFormat, 
        VkFormat depthFormat, VkRenderPass * const pRenderPassObj)
{
    // In complex graphics applications, the picture is built up over
    // many passes where each pass is responsible for producing a different
    // part of the scene, applying full-frame effects such as postprocessing
    // or composition, rendering usr interface elements, and so on. Such pass
    // can be represented in Vulkan using a renderpass object. A single pass
    // encapsulates multiple passes or rendering phases over a single set of
    // output images. Each pass within the renderpass is known as a subpass.
    // Renderpass objects can contains many subpasses. Renderpass object 
    // contains information about the output image.
    // 
    // All drawing must be contained inside a renderpass. Further, graphics
    // pipelines need to know where they're rendering to; therefore, its
    // necessary to create a renderpass object before creating a graphics
    // pipeline so that we can tell the pipeline about the images it'll
    // be processing. 
    //
    // Before we can finish creating the pipeline, we need to tell vulkan
    // about the framebuffer attachment that will be used while rendering.
    // We need to specify how many color and depth buffers there will be,
    // how many samples to use for each of them and how their contents 
    // should be handled throughout the rendering operations.
    //
    // Textueres and framebuffers in Vulkan are represented by VkImage
    // objects with a certain pixel format. however the layout of the
    // pixels in memory can change based on what you're trying to do
    // with an image.

    ri.Printf(PRINT_ALL, " Create RenderPass. \n");

    // Each VkAttachmentDescription structures define the attachments
    // associated with the renderpass. Each of these structures defines
    // a single image that is to be used as an input, output, or both
    // within one of more of the subpass in the renderpass.
	VkAttachmentDescription attachmentsArray[2];
	attachmentsArray[0].flags = 0;
    // The format of the color attachment should match the format
    // of the images used as attachment. 
	attachmentsArray[0].format = colorFormat;
    // indicate the number of samples in the image, for multisampling
	attachmentsArray[0].samples = VK_SAMPLE_COUNT_1_BIT;

    // The next four fields specify what to do with the attachment
    // at the beginning and end of the renderpass.
    //
    // VK_ATTACHMENT_LOAD_OP_DONT_CARE indicates that you don't care
    // about the content of the attachment at the beginning of the
    // renderpass and that Vulkan is free to do whatever it wishes
    // with it. You can use this if you plan to explicitly clear the
    // attachment or if you know that you'll replace the content of
    // the attachment inside the renderpass.
	
    attachmentsArray[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // attachmentsArray[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // VK_ATTACHMENT_STORE_OP_STORE indicates that you want Vulkan
    // to keep the contents of the attachment for later use, which
    // usually means that it should write them out into memory.
    // This is the usually the case for images you want display to
    // user, read from later, or use as an attachment in another
    // renderpass.
	attachmentsArray[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // If the attachment is a combined depth-stentil attachment,
    // then the stencilLoadOp and stencilStoreOp fields tell
    // Vulkan what to do with the stencil part of the attachment,
    // which can be different from the depth part. The regular 
    // loadOp and storeOp fields specify what should happen to
    // the depth part of the attachment.
	attachmentsArray[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentsArray[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// The layout to expect the image to be in when the renderpass
    // begins, the renderpass do not automatically move the images
    // into the initial layout.
    attachmentsArray[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// What layout to leave it in when the renderpass ends. 
    // renderpass move the image to the final layout when it's done.
    attachmentsArray[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachmentsArray[1].flags = 0;
	attachmentsArray[1].format = depthFormat;
	attachmentsArray[1].samples = VK_SAMPLE_COUNT_1_BIT;

    // Attachments can also be cleared at the beginning of a render pass
    // instance by setting loadOp/stencilLoadOp of VkAttachmentDescription
    // to VK_ATTACHMENT_LOAD_OP_CLEAR, as described for vkCreateRenderPass.
    // loadOp and stencilLoadOp, specifying how the contents of the 
    // attachment are treated before rendering and after rendering.
	attachmentsArray[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // VK_ATTACHMENT_STORE_OP_DONT_CARE indicates that you don't need the
    // content after the renderpass has ended.
	attachmentsArray[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // Specifies that the contents within the render area will be cleared to
    // a uniform value, which is specified when a render pass instance is begun    
	attachmentsArray[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentsArray[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    
    // You can use barriers to explicitly move images from layout to layout
    // but where possible, its best to try to move images from layout to
    // layout inside renderpass. This gives Vulkan the best opportunity
    // to choose the right layout for each part fo the renderpass and even
    // perform any operations required to move images between layouts in
    // parallel with other rendering.
	attachmentsArray[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentsArray[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    
    // Each attachment reference is a simple structure containing an
    // index into the array of attachments in attachment and the image
    // layout that the attachment is expected to be in at this subpass.

	VkAttachmentReference color_attachment_ref;
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref;
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    // Each subpass references a number of attachments from the array
    // passed in pAttachments as inputs or outputs. Those descriptions
    // are specified in an array of VkSubpassDescription structures,
    // one for each subpass in the renderpass.
	VkSubpassDescription subpassDesc;
	subpassDesc.flags = 0;
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // The remaining field describe the attachments used by the subpass.
    // Each subpass can have a number of input attachments, which are
    // attachments from which you can read in your fragment shaders.
    // The primary difference between an input attachment and a normal
    // texture bound into a descriptor set is that when you read from
    // an input attachment, you read from current fragment.
	subpassDesc.inputAttachmentCount = 0;
	subpassDesc.pInputAttachments = NULL;
    // Color attachments, which are attachments to which its outputs are
    // written, the maximum number of color attachments that a single 
    // subpass can render to is guaranteed to be at least 4.
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &color_attachment_ref;
	subpassDesc.pResolveAttachments = NULL;
    // There is only one depth-stencil attachment, so this parameter is not
    // an array and has no associated count.
	subpassDesc.pDepthStencilAttachment = &depth_attachment_ref;
	// are the attachments to which multisample image data is resolved.
    // if there are attachments that you want to live across a subpass
    // but that are not directly referenced by the subpass, you should
    // reference them in the pPreserveAttachments array. This reference
    // will prevent Vulkan from making any optimizations that may disdurb
    // the contents of those attachments.
    subpassDesc.preserveAttachmentCount = 0;
	subpassDesc.pPreserveAttachments = NULL;

/*
	VkSubpassDependency subpass_dependencies[1];
	subpass_dependencies[0].srcSubpass = 0;
	subpass_dependencies[0].dstSubpass = 1;
	subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpass_dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependencies[0].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
*/
    
	VkRenderPassCreateInfo desc;
	
    desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
    // Each of these structures defines a single image that is to be used 
    // as an input, output, or both winth one or more of the subpasses in
    // the renderpass. almost all graphics praphics rendering will use at
    // least one attachment.
    desc.attachmentCount = 2;
	desc.pAttachments = attachmentsArray;
    // Each subpass references a number of attachments
    desc.subpassCount = 1;
	desc.pSubpasses = &subpassDesc;
    // Subpass dependencies
    // Remember that the subpasses in a render pass automatically take care of
    // image layout transitions. These transitions are controlled by subpass
    // dependensies, which specify memory and execution dependencies between
    // subpasses. Operations right before and right after this subpass also
    // count as implicit "subpasses".
	desc.dependencyCount = 0;
	desc.pDependencies = NULL;

	VK_CHECK( qvkCreateRenderPass(device, &desc, NULL, pRenderPassObj) );
}


void vk_destroyRenderPass(VkRenderPass hRenderPassObj)
{
    ri.Printf(PRINT_ALL, " Destroy vk.render_pass.\n");

    NO_CHECK( qvkDestroyRenderPass(vk.device, hRenderPassObj, NULL) );
}


//==================================================================
//                   Render Pass Compatibility
//===================================================================
//  Framebuffers and graphics pipelines are created based on
//  a specific render pass object. They must only be used with
//  that render pass object, or one compatible with it.
//
//  Two attachment references are compatible if they have matching
//  format and sample count, or are both VK_ATTACHMENT_UNUSED
//  or the pointer that would contain the reference is NULL.
//
//  Two arrays of attachment references are compatible if all
//  corresponding pairs of attachments are compatible. If the arrays
//  are of different lengths, attachment references not present in
//  the smaller array are treated as VK_ATTACHMENT_UNUSED.
//
//  Two render passes are compatible if their corresponding color,
//  input, resolve, and depth/stencil attachment references are 
//  compatible and if they are otherwise identical except for:
//  1) Initial and final image layout in attachment descriptions
//  2) Load and store operations in attachment descriptions
//  3) Image layout in attachment references
//
//  A framebuffer is compatible with a render pass if it was created
//  using the same render pass or a compatible render pass.
// ===================================================================


void vk_createColorAttachment(VkDevice lgDev, const VkSwapchainKHR HSwapChain, 
        VkFormat surFmt, uint32_t * const pSwapchainLen,
        VkImageView * const pSwapChainImgViews)
{

    // To obtain the actual number of presentable images for swapchain
    // Because when you create the swapchain, you get to specify only
    // the minimum number of the images in the swap chain, not the exact
    // number of images, you need to use vkGetSwapchainImagesKHR to determind
    // how many images there really are in a swap chain, even if you just
    // created it.

    uint32_t swapchainLen = 0;

    VK_CHECK( qvkGetSwapchainImagesKHR(lgDev, HSwapChain, &swapchainLen, NULL) );
 
    ri.Printf(PRINT_ALL, " Actual Number of Swapchain image: %d, format: %d \n",
            swapchainLen, surFmt );

    // To obtain presentable image handles associated with a swapchain
    VK_CHECK( qvkGetSwapchainImagesKHR(lgDev, HSwapChain, &swapchainLen, vk.swapchain_images_array) );

    uint32_t i;
    for (i = 0; i < swapchainLen; ++i)
    {
        VkImageViewCreateInfo desc;
        desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        desc.pNext = NULL;
        desc.flags = 0;
        desc.image = vk.swapchain_images_array[i];
        desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
        desc.format = surFmt;
        desc.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        desc.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        desc.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        desc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        desc.subresourceRange.baseMipLevel = 0;
        desc.subresourceRange.levelCount = 1;
        desc.subresourceRange.baseArrayLayer = 0;
        desc.subresourceRange.layerCount = 1;
        VK_CHECK( qvkCreateImageView(lgDev, &desc, NULL, &pSwapChainImgViews[i]) );
    }

    *pSwapchainLen = swapchainLen;
}


void vk_destroyColorAttachment(void)
{
    ri.Printf(PRINT_ALL, " Destroy vk.color_image_views.\n");
    
    uint32_t i;
	for (i = 0; i < vk.swapchain_image_count; ++i)
    {
		NO_CHECK( qvkDestroyImageView(vk.device, vk.color_image_views[i], NULL) );
        // NO_CHECK( qvkDestroyImage(vk.device, vk.swapchain_images_array[i], NULL) );
    }
}


void vk_createDepthAttachment(int Width, int Height, VkFormat depthFmt)
{
    // A depth attachment is based on an image, just like the color attachment
    // The difference is that the swap chain will not automatically create
    // depth image for us. We need only a single depth image, because only
    // one draw operation is running at once.
    ri.Printf(PRINT_ALL, " Create depth image: vk.depth_image, %d x %d. \n", Width, Height);

    VkImageCreateInfo desc;
    desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.imageType = VK_IMAGE_TYPE_2D;
    desc.format = depthFmt;
    desc.extent.width = Width;
    desc.extent.height = Height;
    desc.extent.depth = 1;
    desc.mipLevels = 1;
    desc.arrayLayers = 1;
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.tiling = VK_IMAGE_TILING_OPTIMAL;
    desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    desc.queueFamilyIndexCount = 0;
    desc.pQueueFamilyIndices = NULL;
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK( qvkCreateImage(vk.device, &desc, NULL, &vk.depth_image) );


    //
    //
    ri.Printf(PRINT_ALL, " Allocate device local memory for depth image. \n");
    VkMemoryRequirements memory_requirements;
    NO_CHECK( qvkGetImageMemoryRequirements(vk.device, vk.depth_image, &memory_requirements) );

    VkMemoryAllocateInfo alloc_info;
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(
        memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    // = vk.idx_depthImgMem;
    VK_CHECK( qvkAllocateMemory(vk.device, &alloc_info, NULL, &vk.depth_image_memory) );
    VK_CHECK( qvkBindImageMemory(vk.device, vk.depth_image, vk.depth_image_memory, 0) );


    //
    //
    //
    ri.Printf(PRINT_ALL, " Create image view for depth image. \n");
    VkImageViewCreateInfo imgViewDesc;
    imgViewDesc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imgViewDesc.pNext = NULL;
    imgViewDesc.flags = 0;
    imgViewDesc.image = vk.depth_image;
    imgViewDesc.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imgViewDesc.format = vk.fmt_DepthStencil;
    imgViewDesc.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imgViewDesc.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imgViewDesc.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imgViewDesc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imgViewDesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    imgViewDesc.subresourceRange.baseMipLevel = 0;
    imgViewDesc.subresourceRange.levelCount = 1;
    imgViewDesc.subresourceRange.baseArrayLayer = 0;
    imgViewDesc.subresourceRange.layerCount = 1;
    VK_CHECK( qvkCreateImageView(vk.device, &imgViewDesc, NULL, &vk.depth_image_view) );

/*

    VkImageAspectFlags image_aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    VkCommandBufferAllocateInfo cmdAllocInfo;
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.pNext = NULL;
    cmdAllocInfo.commandPool = vk.command_pool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer pCB;
    VK_CHECK( qvkAllocateCommandBuffers(vk.device, &cmdAllocInfo, &pCB) );

    VkCommandBufferBeginInfo begin_info;
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = NULL;

    VK_CHECK( qvkBeginCommandBuffer(pCB, &begin_info) );

    image_layout_transition(pCB, vk.depth_image, 
            image_aspect_flags, 0, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | 
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, 
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


    VK_CHECK( qvkEndCommandBuffer(pCB) );

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &pCB;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    VK_CHECK( qvkQueueSubmit(vk.queue, 1, &submit_info, VK_NULL_HANDLE));
    VK_CHECK( qvkQueueWaitIdle(vk.queue) );

    NO_CHECK( qvkFreeCommandBuffers(vk.device, vk.command_pool, 1, &pCB) );
*/
}


void vk_destroyDepthAttachment(void)
{
    ri.Printf(PRINT_ALL, " Destroy vk.depth_image_view vk.depth_image_memory vk.depth_image.\n");

    NO_CHECK( qvkDestroyImageView(vk.device, vk.depth_image_view, NULL) );
	NO_CHECK( qvkFreeMemory(vk.device, vk.depth_image_memory, NULL) );
    NO_CHECK( qvkDestroyImage(vk.device, vk.depth_image, NULL) );
}



void vk_createFrameBuffers(uint32_t w, uint32_t h, VkRenderPass h_rpass,
        uint32_t fbCount, VkFramebuffer* const pFrameBuffers ) 
{

    // The framebuffer is an object that reprensents the set of images
    // that graphic pipelines will render into. These affect the last
    // few stages(depth and stencil tests, blending, multisampling) 
    // in the pipeline. 
    //
    // A framebuffer object is created by using a reference to a 
    // renderpass and can be used with any renderpass that has a 
    // similar arrangement of attachment.
    ri.Printf(PRINT_ALL, " Create vk.framebuffers \n");

    // Framebuffers are created with respect to a specific render pass
    // that the framebuffer is compatible with. Collectively, a render
    // pass and a framebuffer define the complete render target state 
    // for one or more subpasses as well as the algorithmic dependencies
    // between the subpasses.
    //
    // The various pipeline stages of the drawing commands for a given
    // subpass may execute concurrently and/or out of order, both within 
    // and across drawing commands, whilst still respecting pipeline order.
    // However for a given (x,y,layer,sample) sample location, certain
    // per-sample operations are performed in rasterization order.


    // Framebuffers for each swapchain image.
    // The attachments specified during render pass creation are bound
    // by wrapping them into a VkFramebuffer object. A framebuffer object
    // references all of the VkImageView objects that represent the attachments
    

    // Render passes operate in conjunction with framebuffers. 
    // Framebuffers represent a collection of specific memory
    // attachments that a render pass instance uses.

    uint32_t i;
    for (i = 0; i < fbCount; ++i)
    {
        // set color and depth attachment
        // The image that we have to use as attachment depends on which image
        // the the swap chain returns when we retrieve one for presentation
        // this means that we have to create a framebuffer for all of the images
        // in the swap chain and use the one that corresponds to the retrieved
        // image at draw time.

        VkImageView attachmentsArray[2] = {
            vk.color_image_views[i], vk.depth_image_view };
        
        VkFramebufferCreateInfo desc;
        desc.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        desc.pNext = NULL;
        desc.flags = 0;
        // renderPass is a render pass that defines what render
        // passes the framebuffer will be compatible with. 
        desc.renderPass = h_rpass;
        desc.attachmentCount = 2;
        // pAttachments is an array of VkImageView handles, each 
        // of which will be used as the corresponding attachment
        // in a render pass instance.
        desc.pAttachments = attachmentsArray;
        desc.width = w;
        desc.height = h;
        desc.layers = 1;

        VK_CHECK( qvkCreateFramebuffer(vk.device, &desc, NULL, &pFrameBuffers[i]) );

        // Applications must ensure that all accesses to memory that backs
        // image subresources used as attachments in a given renderpass instance 
        // either happen-before the load operations for those attachments,
        // or happen-after the store operations for those attachments.
        //
        // For depth/stencil attachments, each aspect can be used separately
        // as attachments and non-attachments as long as the non-attachment 
        // accesses are also via an image subresource in either the
        // VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL layout
        // or the VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
        // layout, and the attachment resource uses whichever of those two 
        // layouts the image accesses do not. Use of non-attachment aspects
        // in this case is only well defined if the attachment is used in the
        // subpass where the non-attachment access is being made, or the layout
        // of the image subresource is constant throughout the entire render
        // pass instance, including the initialLayout and finalLayout.
        //
        // These restrictions mean that the render pass has full knowledge of
        // all uses of all of the attachments, so that the implementation is
        // able to make correct decisions about when and how to perform layout
        // transitions, when to overlap execution of subpasses, etc.
    }
}


void vk_destroyFrameBuffers(VkFramebuffer* const pFrameBuffers )
{
    ri.Printf(PRINT_ALL, " Destroy vk.framebuffers.\n");
//  Destroying a framebuffer object does not affect any of the images attached
//  to the framebuffer. Images can be attached to multiple framebuffers
//  at the same time and can be used with multiple ways at the same time
    uint32_t i;
	for (i = 0; i < vk.swapchain_image_count; ++i)
    {
		NO_CHECK( qvkDestroyFramebuffer(vk.device, pFrameBuffers[i], NULL) );
    }
}


// Applications have control over which layout each image subresource uses,
// and can transition an image subresource from one layout to another. 
// Transitions can happen with an image memory barrier

void vk_insertLoadingVertexBarrier(VkCommandBuffer HCmdBuffer)
{
    // Ensur/e visibility of geometry buffers writes.
    VkBufferMemoryBarrier barrier1;
    VkBufferMemoryBarrier barrier2;

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


    // To record a pipeline barrier
    // vkCmdPipelineBarrier is a synchronization command that inserts 
    // a dependency between commands submitted to the same queue, or 
    // between commands in the same subpass. When vkCmdPipelineBarrier
    // is submitted to a queue, it defines a memory dependency between
    // commands that were submitted before it, and those submitted
    // after it.

    // VK_PIPELINE_STAGE_HOST_BIT: This pipeline stage corresponds to 
    // access from the host.
    //
    // VK_PIPELINE_STAGE_TRANSFER_BIT: Any pending transfors triggered
    // as a result of calls to vkCmdCopyImage() or vkCmdCopyBuffer(),
    //
    // VK_PIPELINE_STAGE_VERTEX_INPUT_BI: This is the stage where vertex
    // attributes are fetched from their respective buffers. After this
    // content of vertex buffers can be overwritten, even if the resulting
    // vertex shaders have not yet completed execution.
    // 
    NO_CHECK( qvkCmdPipelineBarrier(HCmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 1, &barrier1, 0, NULL) );

    NO_CHECK( qvkCmdPipelineBarrier(HCmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 1, &barrier2, 0, NULL) );

    // 
}


// =====================================

void vk_clearColorAttachments(const float* const color)
{
    // NOTE: we don't use LOAD_OP_CLEAR for color attachment
    // when we begin renderpass since at that point we don't
    // know whether we need color buffer clear, usually we don't.
    VkClearAttachment attachments[1];
    
    // aspectMask is a mask selecting the color, depth and/or 
    // stencil aspects of the attachment to be cleared. aspectMask
    // can include VK_IMAGE_ASPECT_COLOR_BIT for color attachments,
    // VK_IMAGE_ASPECT_DEPTH_BIT for depth/stencil attachments with
    // a depth component, and VK_IMAGE_ASPECT_STENCIL_BIT for 
    // depth/stencil attachments with a stencil component. 
    // If the subpass's depth/stencil attachment is 
    // VK_ATTACHMENT_UNUSED, then the clear has no effect.

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


    VkClearRect clear_rect[1];
	clear_rect[0].rect = vk.renderArea;
	clear_rect[0].baseArrayLayer = 0;
	clear_rect[0].layerCount = 1;

    // CmdClearAttachments can clear multiple regions of each attachment
    // used in the current subpass of a render pass instance. This command
    // must be called only inside a render pass instance, and implicitly
    // selects the images to clear based on the current framebuffer 
    // attachments and the command parameters.

	NO_CHECK( qvkCmdClearAttachments(vk.command_buffer, 1, attachments, 1, clear_rect) );

}


void vk_begin_frame(void)
{
    // t_frame_start = R_GetTimeMicroSeconds() ;

    //  User could call method vkWaitForFences to wait for completion. 
    //  A fence is a very heavyweight synchronization primitive as it 
    //  requires the GPU to flush all caches at least, and potentially
    //  some additional synchronization. Due to those costs, fences 
    //  should be used sparingly. In particular, try to group per-frame
    //  resources and track them together. To wait for one or more fences
    //  to enter the signaled state on the host, call qvkWaitForFences.


    //  fence_renderFinished is an optional handle to a fence to be 
    //  signaled once all submitted command buffers have completed 
    //  execution. If the condition is satisfied when vkWaitForFences
    //  is called, then vkWaitForFences returns immediately. If the
    //  condition is not satisfied at the time vkWaitForFences is 
    //  called, then vkWaitForFences will block and wait up to timeout
    //  nanoseconds for the condition to become satisfied.

    VK_CHECK( qvkWaitForFences(vk.device, 1, &fence_renderFinished, VK_FALSE, 1e9) );

    //  To set the state of fences to unsignaled from the host
    //  "1" is the number of fences to reset. 
    //  "fence_renderFinished" is the fence handle to reset.
    VK_CHECK( qvkResetFences(vk.device, 1, &fence_renderFinished) );


    // begin_info is an instance of the VkCommandBufferBeginInfo structure,
    // which defines additional information about how the command buffer 
    // begins recording.
    static const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT specifies that
        // each recording of the command buffer will only be submitted
        // once, and the command buffer will be reset and recorded again
        // between each submission.
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL,
    };

    // To begin recording a command buffer
    VK_CHECK( qvkBeginCommandBuffer(vk.command_buffer, &begin_info) );

    // vk_beginRecordCmds(vk.command_buffer);
    //  commandBuffer must not be in the recording or pending state.
    
    vk_insertLoadingVertexBarrier(vk.command_buffer);

    // An application can acquire use of a presentable image with 
    // vkAcquireNextImageKHR. After acquiring a presentable image
    // and before modifying it, the application must use a synch-
    // ronization primitive to ensure that the presentation engine
    // has finished reading from the image. vkAcquireNextImageKHR
    // will block until an image is acquired or an error occurs.
    //
    // Use of a presentable image must occur only after the image is
    // returned by vkAcquireNextImageKHR, and before it is presented
    // by vkQueuePresentKHR. This includes transitioning the image
    // layout and rendering commands.
    
    // The presentation engine is an abstraction for the platform's
    // compositor or display engine. The presentation engine controls
    // the order in which presentable images are acquired for use by
    // the application. This allows the platform to handle situations
    // which require out-of-order return of images after presentation.
    // At the same time, it allows the application to generate command
    // buffers referencing all of the images in the swapchain at 
    // initialization time, rather than in its main loop.

    // An application must wait until either the semaphore or fence
    // is signaled before accessing the image's data.
    // If timeout is UINT64_MAX, the timeout period is treated as infinite
    VK_CHECK( qvkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX,
                sema_imageAvailable, VK_NULL_HANDLE, &vk.idx_swapchain_image) );

    //
    // Begin render pass.
    //
    VkClearValue pClearValues[2];
    
    pClearValues[0].color.float32[0] = 1.0f;
    pClearValues[0].color.float32[1] = 1.0f;
    pClearValues[0].color.float32[2] = 1.0f;
    pClearValues[0].color.float32[3] = 1.0f;
    
    // ignore clear_values[0] which corresponds to color attachment
    pClearValues[1].depthStencil.depth = 1.0f;
    pClearValues[1].depthStencil.stencil = 0.0f;

    VkRenderPassBeginInfo renderPass_beginInfo;
    renderPass_beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPass_beginInfo.pNext = NULL;
    renderPass_beginInfo.renderPass = vk.render_pass;
    renderPass_beginInfo.framebuffer = vk.framebuffers[vk.idx_swapchain_image];
    renderPass_beginInfo.renderArea = vk.renderArea;
    renderPass_beginInfo.clearValueCount = 2;
    // If any of the attachments in the renderpass have a load operation
    // of VK_ATTACHMENT_LOAD_OP_CLEAR, then the colors or values that you
    // want to clear them to are specified in an array of VkClearValue
    // unions. The index of each attachment is used to index into the 
    // array of VkClearValue unions. This means that if only some of the
    // attachments have a load operation of VK_ATTACHMENT_LOAD_OP_CLEAR,
    // then there could be unused entries in the array. There must be at
    // least as many entries in pClearValues array as the highest-indexed
    // attachment with a load operation of VK_ATTACHMENT_LOAD_OP_CLEAR.
    
    renderPass_beginInfo.pClearValues = pClearValues;

    NO_CHECK( qvkCmdBeginRenderPass(vk.command_buffer, &renderPass_beginInfo, VK_SUBPASS_CONTENTS_INLINE) );
}


void vk_end_frame(void)
{
	NO_CHECK( qvkCmdEndRenderPass(vk.command_buffer) );
	
    NO_CHECK( qvkEndCommandBuffer(vk.command_buffer) );


	VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    
    // Queue submission and synchronization
	VkSubmitInfo submit_info;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vk.command_buffer;
	submit_info.signalSemaphoreCount = 1;
    // specify which semaphones to signal once the command buffers
    // have finished execution
	submit_info.pSignalSemaphores = &sema_renderFinished;
	
    // Before executing the commands in pCommandBuffers, the queue
    // will wait for all of the semaphores in pWaitSemaphores. In
    // doing so, it will take ownership of the semaphores. It will
    // then execute the commands contained in each of the command
    // buffers, and when it is done, it will signal each of the
    // the semaphores contained in pSignalSemaphores.
    submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &sema_imageAvailable;

    //  queue is the queue that the command buffers will be submitted to.
    //  1 is the number of elements in the pSubmits array.
    //  pSubmits is a pointer to an array of VkSubmitInfo structures,
    //  each specifying a command buffer submission batch.

    //  vkQueueSubmit is a queue submission command, with each batch defined
    //  by an element of pSubmits as an instance of the VkSubmitInfo structure.
    //
    //  Fence and semaphore operations submitted with vkQueueSubmit 
    //  have additional ordering constraints compared to other 
    //  submission commands, with dependencies involving previous and
    //  subsequent queue operations. 
    //
    //  The order that batches appear in pSubmits is used to determine
    //  submission order, and thus all the implicit ordering guarantees
    //  that respect it. Other than these implicit ordering guarantees
    //  and any explicit synchronization primitives, these batches may
    //  overlap or otherwise execute out of order. If any command buffer
    //  submitted to this queue is in the executable state, it is moved
    //  to the pending state. Once execution of all submissions of a 
    //  command buffer complete, it moves from the pending state,
    //  back to the executable state.
    //
    //  If a command buffer was recorded with the 
    //  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT flag,
    //  it instead moves back to the invalid state.


    // Generally, commands executed inside command buffers submitted
    // to a queue produce the images that are to be presented. 
    // So those images should be shown to the user only when the 
    // rendering operations that created them have complated.
    // Submission can be a high overhead operation, and applications 
    // should attempt to batch work together into as few calls to 
    // vkQueueSubmit as possible.

    VK_CHECK( qvkQueueSubmit(vk.queue, 1, &submit_info, fence_renderFinished) );

    
    VkPresentInfoKHR present_info;
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &sema_renderFinished;

    // specify the swap chains to present images to
	present_info.swapchainCount = 1;
    // Each element of pSwapchains member of pPresentInfo must be a 
    // swapchain that is created for a surface for which presentation
    // is supported from queue.
	present_info.pSwapchains = &vk.swapchain;
    // specify the index of the image for each swap chain
	present_info.pImageIndices = &vk.idx_swapchain_image;
	present_info.pResults = NULL;
    
    // After queueing all rendering commands and transitioning the
    // image to the correct layout, to queue an image for presentation.
    // queue is a queue that is capable of presentation to the target 
    // surface's platform on the same device as the image's swapchain.
    VkResult result = qvkQueuePresentKHR(vk.queue, &present_info);
    if(result == VK_SUCCESS)
    {
        // ri.Printf(PRINT_ALL, "frame time: %ld us \n", R_GetTimeMicroSeconds() - t_frame_start);
        return;
    }
    else if( (result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_ERROR_SURFACE_LOST_KHR))
    {
        // we first call vkDeviceWaitIdle because we 
        // shouldn't touch resources that still be in use
        NO_CHECK( qvkDeviceWaitIdle(vk.device) );
        // recreate the objects that depend on the swap chain and the window size

        vk_recreateSwapChain();
    }
}
