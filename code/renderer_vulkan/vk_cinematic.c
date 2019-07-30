#include "VKimpl.h"
#include "vk_instance.h"
#include "tr_shader.h"
#include "vk_image.h"
#include "ref_import.h" 
#include "render_export.h"
#include "vk_image_sampler.h"
#include "vk_buffers.h"


static VkDeviceMemory s_mappableMemory;
static struct image_s tr_scratchImage;
static struct shader_s * tr_cinematicShader;


struct image_s * R_GetScratchImageHandle(int idx)
{
	ri.Printf (PRINT_ALL, " R_GetScratchImageHandle: %i\n", idx);

    return &tr_scratchImage;
}


void R_SetCinematicShader( struct shader_s * const pShader)
{
    ri.Printf (PRINT_ALL, " R_SetCinematicShader \n");

    tr_cinematicShader = pShader;
}


void vk_initScratchImage(void)
{
    ri.Printf (PRINT_ALL, " Init Scratch Image. \n");

    memset( &tr_scratchImage, 0, sizeof( tr_scratchImage ) );
    tr_cinematicShader = NULL;
}


void vk_destroyScratchImage(void)
{
/*
    if(tr_scratchImage.descriptor_set != VK_NULL_HANDLE)
    {   
        //To free allocated descriptor sets
		ri.Printf (PRINT_ALL, " To free tr_scratchImage descriptor sets. \n");
        NO_CHECK( qvkFreeDescriptorSets(vk.device, vk.descriptor_pool, 1, &tr_scratchImage.descriptor_set) );
        tr_scratchImage.descriptor_set = VK_NULL_HANDLE;
    }
*/
    if (tr_scratchImage.view != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkDestroyImageView(vk.device, tr_scratchImage.view, NULL) );
        tr_scratchImage.view = VK_NULL_HANDLE; 
    }

    if (s_mappableMemory != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkFreeMemory(vk.device, s_mappableMemory, NULL) );
        s_mappableMemory = VK_NULL_HANDLE;
    }

    if (tr_scratchImage.handle != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkDestroyImage(vk.device, tr_scratchImage.handle, NULL) );
         tr_scratchImage.handle = VK_NULL_HANDLE;
    }

    memset( &tr_scratchImage, 0, sizeof( tr_scratchImage ) );

    ri.Printf (PRINT_ALL, " Destroy Scratch Image. \n");
}



void RE_UploadCinematic(int w, int h, int cols, int rows, const unsigned char * data, int client, VkBool32 dirty)
{

    // the image may not even created, and it image data may not even uploaded.
    // if the scratchImage isn't in the format we want, specify it as a new texture
    if ( (cols != tr_scratchImage.uploadWidth) || (rows != tr_scratchImage.uploadHeight) )
    {
        ri.Printf(PRINT_ALL, "w=%d, h=%d, cols=%d, rows=%d, client=%d, prtImage->width=%d, prtImage->height=%d\n", 
           w, h, cols, rows, client, tr_scratchImage.uploadWidth, tr_scratchImage.uploadHeight);

        // VULKAN
        // if already created, we will destroy it.
        
		vk_destroyScratchImage();

        strncpy (tr_scratchImage.imgName, "*scratch", 10);
        tr_scratchImage.uploadWidth = cols;
        tr_scratchImage.uploadHeight = rows;

        {
            VkImageCreateInfo imgDesc;
            imgDesc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imgDesc.pNext = NULL;
            imgDesc.flags = 0;
            imgDesc.imageType = VK_IMAGE_TYPE_2D;
            imgDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
            imgDesc.extent.width = rows;
            imgDesc.extent.height = cols;
            imgDesc.extent.depth = 1;
            imgDesc.mipLevels = 1;
            imgDesc.arrayLayers = 1;
            imgDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            imgDesc.tiling = VK_IMAGE_TILING_OPTIMAL;
            ////
            imgDesc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            imgDesc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imgDesc.queueFamilyIndexCount = 0;
            imgDesc.pQueueFamilyIndices = NULL;
            // However, images must initially be created in either 
            // VK_IMAGE_LAYOUT_UNDEFINED or 
            imgDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VK_CHECK( qvkCreateImage(vk.device, &imgDesc, NULL, &tr_scratchImage.handle) );

            ri.Printf(PRINT_ALL, " Create Image For Cinematic. \n");
        }
        //vk_create2DImageHandle( VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, pImage);
        
        VkMemoryRequirements memory_reqs;
        NO_CHECK( qvkGetImageMemoryRequirements(vk.device, tr_scratchImage.handle, &memory_reqs) );
        

        {
        // Allocate Device Local memory for the cinematic images
        VkMemoryAllocateInfo alloc_info;
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = NULL;
        alloc_info.allocationSize = memory_reqs.size;
        alloc_info.memoryTypeIndex = find_memory_type( 
                memory_reqs.memoryTypeBits, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK( qvkAllocateMemory(vk.device, &alloc_info, NULL, &s_mappableMemory) );
        
        // (int) for prevent anoy warnings
        ri.Printf(PRINT_ALL, "Allocate Device local memory, Size: %d KB, Type Index: %d. \n",
            (int)(memory_reqs.size >> 10), alloc_info.memoryTypeIndex);
        }

        VK_CHECK( qvkBindImageMemory(vk.device, tr_scratchImage.handle, s_mappableMemory, 0) );


        // vk_createViewForImageHandle(prtImage->handle, VK_FORMAT_R8G8B8A8_UNORM, &prtImage->view);
        VkImageViewCreateInfo desc;
        desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        desc.pNext = NULL;
        desc.flags = 0;
        desc.image = tr_scratchImage.handle;
        desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
        // format is a VkFormat describing the format and type used 
        // to interpret data elements in the image.
        desc.format = VK_FORMAT_R8G8B8A8_UNORM;
        desc.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        desc.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        desc.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        desc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        desc.subresourceRange.baseMipLevel = 0;
        desc.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        desc.subresourceRange.baseArrayLayer = 0;
        desc.subresourceRange.layerCount = 1;
 
        VK_CHECK( qvkCreateImageView(vk.device, &desc, NULL, &tr_scratchImage.view) );
        
        // vk_createDescriptorSet(prtImage);
        //
        // Allocate a descriptor set from the pool. 
        // Note that we have to provide the descriptor set layout that 
        // This layout describes how the descriptor set is to be allocated.

        // vk_allocOneDescptrSet(&tr_scratchImage.descriptor_set);

        VkDescriptorSetAllocateInfo descSetAllocInfo;
        descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descSetAllocInfo.pNext = NULL;
        descSetAllocInfo.descriptorPool = vk.descriptor_pool;
        descSetAllocInfo.descriptorSetCount = 1;
        descSetAllocInfo.pSetLayouts = &vk.set_layout;
        VK_CHECK( qvkAllocateDescriptorSets(vk.device, &descSetAllocInfo, &tr_scratchImage.descriptor_set) );

        //ri.Printf(PRINT_ALL, " Allocate Descriptor Sets \n");
        VkWriteDescriptorSet descriptor_write;

        VkDescriptorImageInfo image_info;
        image_info.sampler = vk_find_sampler(VK_FALSE, VK_FALSE);
        image_info.imageView = tr_scratchImage.view;
        // the image will be bound for reading by shaders.
        // this layout is typically used when an image is going to
        // be used as a texture.
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = tr_scratchImage.descriptor_set;
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pNext = NULL;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_write.pImageInfo = &image_info;
        descriptor_write.pBufferInfo = NULL;
        descriptor_write.pTexelBufferView = NULL;

        NO_CHECK( qvkUpdateDescriptorSets(vk.device, 1, &descriptor_write, 0, NULL) );
        
        dirty = 1;
    }
    
    if (dirty)
    {
        // otherwise, just subimage upload it so that
        // drivers can tell we are going to be changing
        // it and don't try and do a texture compression       

        VK_UploadImageToStagBuffer(data, cols * rows * 4);

        VkBufferImageCopy regions[1];

        regions[0].bufferOffset = 0;
        regions[0].bufferRowLength = 0;
        regions[0].bufferImageHeight = 0;
        regions[0].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        regions[0].imageSubresource.mipLevel = 0;
        regions[0].imageSubresource.baseArrayLayer = 0;
        regions[0].imageSubresource.layerCount = 1;
        regions[0].imageOffset.x = 0;
        regions[0].imageOffset.y = 0;
        regions[0].imageOffset.z = 0;
        regions[0].imageExtent.width = rows;
        regions[0].imageExtent.height = cols;
        regions[0].imageExtent.depth = 1;

        vk_stagBufToDevLocal(tr_scratchImage.handle, regions, 1);
    }
}


inline static int isPowerOfTwo(int number)
{
    return ((number & (number - 1)) == 0);
}


extern void RB_StretchPic(float x, float y, float w, float h, 
				  float s1, float t1, float s2, float t2, shader_t * pShader );

/*
=============
FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, 
        const unsigned char *data, int client, qboolean dirty )
{

    // SCR_AdjustFrom640( &x, &y, &w, &h );
/*
    float xscale = vk.renderArea.extent.width / 640.0f;
    float yscale = vk.renderArea.extent.height / 480.0f;

    x *= xscale;
    y *= yscale;
    w *= xscale;
    h *= yscale;
*/

    // make sure rows and cols are powers of 2
	if ( isPowerOfTwo( cols ) && isPowerOfTwo( rows ))
    {
        RE_UploadCinematic(w, h, cols, rows, data, client, dirty);

        tr_cinematicShader->stages[0]->bundle[0].image[0] = &tr_scratchImage;
    
        RB_StretchPic(x, y, w, h, 
				  0.5f / cols, 0.5f / rows, 1.0f - 0.5f / cols, 1.0f - 0.5 / rows, tr_cinematicShader );
	}
    else
    {
        ri.Printf(PRINT_WARNING, "Draw_StretchRaw: size not a power of 2: %i by %i",
                cols, rows);
    }
}


//
// memory barriers are for cache flushing. That's it. Execution dependencies handle the rest.
//
// ===================================================================================================
// 
// Synchronization of access to resources is primarily the responsibility of the application in Vulkan.
//
// The order of execution of commands with respect to the host and 
// other commands on the device has few implicit guarantees, and 
// needs to be explicitly specified. 
// 
// Memory caches and other optimizations are also explicitly managed, 
// requiring that the flow of data through the system is largely under
// application control.
//
// Whilst some implicit guarantees exist between commands, 
// five explicit synchronization mechanisms are exposed by Vulkan:
//
//                           Fences
//
// Fences can be used to communicate to the host that execution of
// some task on the device has completed.
//
//                         Semaphores
//
// Semaphores can be used to control resource access across multiple queues.
//                          
//                           Events
//
// Events provide a fine-grained synchronization primitive which can
// be signaled either within a command buffer or by the host, and
// can be waited upon within a command buffer or queried on the host.
//
//                      Pipeline Barriers
//
// Pipeline barriers also provide synchronization control within a command buffer,
// but at a single point, rather than with separate signal and wait operations.
//
//
//                         Render Passes
//
// Render passes provide a useful synchronization framework for most rendering tasks,
// built upon the concepts in this chapter. Many cases that would otherwise need an
// application to use other synchronization primitives can be expressed more efficiently
// as part of a render pass.
//
//
// ==================================================== 
//    Execution and Memory Dependencies
// ====================================================
//
// An operation is an arbitrary amount of work to be executed on the host, a device, 
// or an external entity such as a presentation engine. Synchronization commands 
// introduce explicit execution dependencies, and memory dependencies between 
// two sets of operations defined by the command’s two synchronization scopes.
//
// The synchronization scopes define which other operations a synchronization command
// is able to create execution dependencies with. 
//
// Any type of operation that is not in a synchronization command’s synchronization
// scopes will not be included in the resulting dependency. 
//
// For example, for many synchronization commands, the synchronization scopes can be
// limited to just operations executing in specific pipeline stages, which allows 
// other pipeline stages to be excluded from a dependency. Other scoping options are
// possible, depending on the particular command.
//
// An "execution dependency" is a guarantee that for two sets of operations, 
// the first set must happen-before the second set. If an operation happens-before
// another operation, then the first operation must complete before the second
// operation is initiated. More precisely:
// 
// -> Let A and B be separate sets of operations.
// -> Let S be a synchronization command.
// -> Let A_s and B_s be the synchronization scopes of S.
// -> Let A' be the intersection of sets A and A_s .
// -> Let B' be the intersection of sets B and B_s .
// 
// Submitting A, S and B for execution, in that order, 
// will result in execution dependency E between A' and B'.
// Execution dependency E guarantees that A' happens-before B'.
//
// An "execution dependency chain" is a sequence of execution dependencies
// that form a happens-before relation between the first dependency’s A' 
// and the final dependency’s B'. For each consecutive pair of execution 
// dependencies, a chain exists if the intersection of B_s in the first dependency
// and A_s in the second dependency is not an empty set. The formation of a single
// execution dependency from an execution dependency chain can be described by
// substituting the following in the description of execution dependencies:
//
// Execution dependencies alone are not sufficient to guarantee that 
// values resulting from writes in one set of operations can be read
// from another set of operations. Three additional types of operation
// are used to control memory access.
// 
// Availability operations cause the values generated by specified memory
// write accesses to become available to a memory domain for future access. 
// Any available value remains available until a subsequent write to the
// same memory location occurs (whether it is made available or not) or 
// the memory is freed. Memory domain operations cause writes that are 
// available to a source memory domain to become available to a destination
// memory domain (an example of this is making writes available to the host
// domain available to the device domain). Visibility operations cause values
// available to a memory domain to become visible to specified memory accesses.
//
// A memory dependency is an execution dependency which includes availability
// and visibility operations such that:
// 
// The first set of operations happens-before the availability operation.
// The availability operation happens-before the visibility operation.
// The visibility operation happens-before the second set of operations.
//
//
// Once written values are made visible to a particular type of memory access,
// they can be read or written by that type of memory access. Most synchronization
// commands in Vulkan define a memory dependency.
//
//
// The specific memory accesses that are made available and visible are defined
// by the access scopes of a memory dependency. Any type of access that is in a
// memory dependency’s first access scope and occurs in A' is made available. 
// Any type of access that is in a memory dependency’s second access scope and
// occurs in B' has any available writes made visible to it. Any type of operation
// that is not in a synchronization command’s access scopes will not be included
// in the resulting dependency.
//
// A memory dependency enforces availability and visibility of memory accesses
// and execution order between two sets of operations.
// 
// that read and write operations occur in a well-defined order. Write-after-read
// hazards can be solved with just an execution dependency, but read-after-write and
// write-after-write hazards need appropriate memory dependencies to be included
// between them. If an application does not include dependencies to solve these
// hazards, the results and execution orders of memory accesses are undefined.
//
//
// Image subresources can be transitioned from one layout to another as part of
// a memory dependency (e.g. by using an image memory barrier). When a layout 
// transition is specified in a memory dependency, it happens-after the 
// availability operations in the memory dependency, and happens-before the
// visibility operations. Image layout transitions may perform read and write
// accesses on all memory bound to the image subresource range, so applications
// must ensure that all memory writes have been made available before a layout
// transition is executed. Available memory is automatically made visible to a
// layout transition, and writes performed by a layout transition are automatically
// made available.
//
// Layout transitions always apply to a particular image subresource range, 
// and specify both an old layout and new layout. If the old layout does not
// match the new layout, a transition occurs. The old layout must match the 
// current layout of the image subresource range, with one exception. The old
// layout can always be specified as VK_IMAGE_LAYOUT_UNDEFINED, 
// though doing so invalidates the contents of the image subresource range.
// 
// Image layout transitions may perform read and write accesses on the memory
// bound to the image.
//
// Setting the old layout to VK_IMAGE_LAYOUT_UNDEFINED implies that the contents
// of the image subresource need not be preserved. Implementations may use this
// information to avoid performing expensive data transition operations.
//
// Applications must ensure that layout transitions happen after all operations
// accessing the image with the old layout, and happen before any operations that
// will access the image with the new layout. Layout transitions are potentially
// read/write operations, so not defining appropriate memory dependencies to
// guarantee this will result in a data race.
//
// Drawing commands, dispatching commands, copy commands, clear commands, 
// and synchronization commands all execute in different sets of pipeline stages.
// Synchronization commands do not execute in a defined pipeline, but do execute
// VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT and VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT.
//
// Operations performed by synchronization commands ( availability and
// visibility operations) are not executed by a defined pipeline stage. 
// However other commands can still synchronize VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
// and with them via the VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT pipeline stages.
//
// Execution of operations across pipeline stages must adhere to implicit ordering
// guarantees, particularly including pipeline stage order. Otherwise, execution 
// across pipeline stages may overlap or execute out of order with regards to other
// stages, unless otherwise enforced by an execution dependency.
