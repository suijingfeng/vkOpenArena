#include "VKimpl.h"
#include "vk_cmd.h"
#include "vk_instance.h"
#include "tr_globals.h"
#include "tr_shader.h"
#include "tr_cvar.h"
#include "tr_fog.h"
#include "vk_image_sampler.h"
#include "vk_image.h"
#include "R_ImageProcess.h"
#include "R_SortAlgorithm.h"
#include "vk_descriptor_sets.h"
#include "ref_import.h" 
#include "render_export.h"

#define IMAGE_CHUNK_SIZE        (64 * 1024 * 1024)


// An application can copy buffer and image data using several methods
// depending on the type of data transfer. Data can be copied between
// buffer objects with vkCmdCopyBuffer and a portion of an image can 
// be copied to another image with vkCmdCopyImage. 
//
// Image data can also be copied to and from buffer memory using
// vkCmdCopyImageToBuffer and vkCmdCopyBufferToImage.
//
// Image data can be blitted (with or without scaling and filtering) 
// with vkCmdBlitImage. Multisampled images can be resolved to a 
// non-multisampled image with vkCmdResolveImage.

struct StagingBuffer_t
{
    // Vulkan supports two primary resource types: buffers and images. 
    // Resources are views of memory with associated formatting and dimensionality.
    // Buffers are essentially unformatted arrays of bytes whereas images contain
    // format information, can be multidimensional and may have associated metadata.
    //
    // Buffers represent linear arrays of data which are used for various purposes
    // by binding them to a graphics or compute pipeline via descriptor sets or via
    // certain commands, or by directly specifying them as parameters to certain commands.
    VkBuffer buff;
    // Host visible memory used to copy image data to device local memory.
    VkDeviceMemory mappableMem;
    //uint32_t Used;
};


struct ImageChunk_t {
    VkDeviceMemory block;
    uint32_t Used;
    // uint32_t typeIndex;
};


struct deviceLocalMemory_t {
    // One large device device local memory allocation, assigned to multiple images
	struct ImageChunk_t Chunks[8];
	uint32_t Index; // number of chunks used
};


static struct StagingBuffer_t StagBuf;
static struct deviceLocalMemory_t devMemImg;


void gpuMemUsageInfo_f(void)
{
    // approm	 for debug info
    ri.Printf(PRINT_ALL, "Number of image created: %d, GPU memory used for store those image: %d MB \n", 
           tr.numImages, devMemImg.Index * (IMAGE_CHUNK_SIZE>>20) );
}


////////////////////////////////////////

uint32_t find_memory_type(uint32_t memory_type_bits, VkMemoryPropertyFlags properties)
{
    uint32_t i;
    for (i = 0; i < vk.devMemProperties.memoryTypeCount; i++)
    {
        if ( ((memory_type_bits & (1 << i)) != 0) && (vk.devMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    ri.Error(ERR_FATAL, "Vulkan: failed to find matching memory type with requested properties");
    return -1;
}


void vk_createBufferResource(uint32_t Size, VkBufferUsageFlags Usage, 
        VkBuffer * const pBuf, VkDeviceMemory * const pDevMem )
{
    memset(&StagBuf, 0, sizeof(StagBuf));

    VkBufferCreateInfo buffer_desc;
    buffer_desc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_desc.pNext = NULL;
    // flags is a bitmask of VkBufferCreateFlagBits specifying additional parameters of the buffer.
    buffer_desc.flags = 0;
    buffer_desc.size = Size;
    // VK_BUFFER_USAGE_TRANSFER_SRC_BIT specifies that the buffer
    // can be used as the source of a transfer command
    buffer_desc.usage = Usage;

    // sharingMode is a VkSharingMode value specifying the sharing mode of the buffer 
    // when it will be accessed by multiple queue families.
    // queueFamilyIndexCount is the number of entries in the pQueueFamilyIndices array.
    // pQueueFamilyIndices is a list of queue families that will access this buffer,
    // (ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT).
    buffer_desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_desc.queueFamilyIndexCount = 0;
    buffer_desc.pQueueFamilyIndices = NULL;

    VK_CHECK( qvkCreateBuffer(vk.device, &buffer_desc, NULL, pBuf) );


    // To determine the memory requirements for a buffer resource
    //
    //  typedef struct VkMemoryRequirements {
    //  VkDeviceSize size;
    //  VkDeviceSize alignment;
    //  uint32_t memoryTypeBits;
    //  } VkMemoryRequirements;

    VkMemoryRequirements memory_requirements;
    NO_CHECK( qvkGetBufferMemoryRequirements(vk.device, *pBuf, &memory_requirements) );

    VkMemoryAllocateInfo alloc_info;
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = memory_requirements.size;
    
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT bit specifies that memory allocated with
    // this type can be mapped for host access using vkMapMemory.
    //
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit specifies that the host cache
    // management commands vkFlushMappedMemoryRanges and vkInvalidateMappedMemoryRanges
    // are not needed to flush host writes to the device or make device writes visible
    // to the host, respectively.
    
    alloc_info.memoryTypeIndex = find_memory_type( 
        memory_requirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    VK_CHECK( qvkAllocateMemory(vk.device, &alloc_info, NULL, pDevMem) );

    VK_CHECK( qvkBindBufferMemory(vk.device, *pBuf, *pDevMem, 0) );

    ri.Printf(PRINT_ALL, " Create Buffer: alignment: %ld, memoryTypeBits: 0x%x, Type Index: %d. \n",
            memory_requirements.alignment, memory_requirements.memoryTypeBits, alloc_info.memoryTypeIndex);
}


void vk_destroyBufferResource(VkBuffer hBuf, VkDeviceMemory hDevMem)
{
    if (hDevMem != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkFreeMemory(vk.device, hDevMem, NULL) );
		hDevMem = VK_NULL_HANDLE;
    }

    if (hBuf != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkDestroyBuffer(vk.device, hBuf, NULL) );
        hBuf = VK_NULL_HANDLE;
    }
}




static void record_image_layout_transition( 
        VkCommandBuffer cmdBuf,
        VkImage image,
        VkImageAspectFlags image_aspect_flags,
        VkAccessFlags src_access_flags,
        VkImageLayout old_layout,
        VkAccessFlags dst_access_flags,
        VkImageLayout new_layout )
{

	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.srcAccessMask = src_access_flags;
	barrier.dstAccessMask = dst_access_flags;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = image_aspect_flags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    // vkCmdPipelineBarrier is a synchronization command that inserts
    // a dependency between commands submitted to the same queue, or 
    // between commands in the same subpass. When vkCmdPipelineBarrier
    // is submitted to a queue, it defines a memory dependency between
    // commands that were submitted before it, and those submitted 
    // after it.
    
    // cmdBuf is the command buffer into which the command is recorded.
	NO_CHECK( qvkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,	0, NULL, 0, NULL, 1, &barrier) );
}


static void vk_stagBufferToDeviceLocalMem(VkImage image, VkBufferImageCopy* pRegion, uint32_t num_region)
{

    vk_beginRecordCmds( vk.tmpRecordBuffer );

    // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : The image is the
    // destination of copy operations. 
    record_image_layout_transition( vk.tmpRecordBuffer, image, 
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


    // To copy data from a buffer object to an image object
    // StagBuf.buff is the source buffer.
    // image is the destination image.
    // dstImageLayout is the layout of the destination image subresources.
    // curLevel is the number of regions to copy.
    // pRegions is a pointer to an array of VkBufferImageCopy structures
    // specifying the regions to copy.
    NO_CHECK( qvkCmdCopyBufferToImage( vk.tmpRecordBuffer, StagBuf.buff, image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, num_region, pRegion) );

    record_image_layout_transition(vk.tmpRecordBuffer, image,
            VK_IMAGE_ASPECT_COLOR_BIT, 
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vk_commitRecordedCmds(vk.tmpRecordBuffer);
}


#define FILE_HASH_SIZE	1024
static image_t*	hashTable[FILE_HASH_SIZE];

static int generateHashValue( const char *fname )
{
    uint32_t i = 0;
    int	hash = 0;

    while (fname[i] != '\0')
    {
        // char letter = tolower(fname[i]);
        char letter = fname[i];
        if (letter =='.')
            break;		// don't include extension
        if (letter =='\\')
            letter = '/';	// damn path names
        hash+=(int)(letter)*(i+119);
        ++i;
    }

    return hash & (FILE_HASH_SIZE-1);
}


void printImageHashTable_f(void)
{
    uint32_t i = 0;

    int32_t tmpTab[FILE_HASH_SIZE] = {0};
    ri.Printf(PRINT_ALL, "\n\n-----------------------------------------------------\n"); 
    for(i = 0; i < FILE_HASH_SIZE; ++i)
    {
        image_t * pImg = hashTable[i];

        while(pImg != NULL)
        {
            
            ri.Printf(PRINT_ALL, "[%d] mipLevels: %d,  size: %dx%d  %s\n", 
                    i, pImg->mipLevels, pImg->width, pImg->height, pImg->imgName);
            
            ++tmpTab[i];
            
            pImg = pImg->next;
        }
    }

    quicksort(tmpTab, 0, FILE_HASH_SIZE-1);

    int count = 0;
    int total = 0;

    for(i = 0; i < FILE_HASH_SIZE; i++)
    {
        if(tmpTab[i]) {
            ++count;
            total += tmpTab[i];
        }
    }
    
    ri.Printf(PRINT_ALL, "\n Total %d images, hash Table used: %d/%d\n",
            total, count, FILE_HASH_SIZE);
    
    ri.Printf(PRINT_ALL, "\n Top 10 Collision: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
        tmpTab[0], tmpTab[1], tmpTab[2], tmpTab[3], tmpTab[4],
        tmpTab[5], tmpTab[6], tmpTab[7], tmpTab[8], tmpTab[9]);

    ri.Printf(PRINT_ALL, "-----------------------------------------------------\n\n"); 
}


static void vk_create2DImageHandle(VkImageUsageFlags imgUsage, image_t* const pImg)
{
    VkImageCreateInfo desc;
    desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.imageType = VK_IMAGE_TYPE_2D;
    desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    desc.extent.width = pImg->uploadWidth;
    desc.extent.height = pImg->uploadHeight;
    desc.extent.depth = 1;
    desc.mipLevels = pImg->mipLevels;
    desc.arrayLayers = 1;
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.tiling = VK_IMAGE_TILING_OPTIMAL;
    desc.usage = imgUsage;
    desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    desc.queueFamilyIndexCount = 0;
    desc.pQueueFamilyIndices = NULL;
    // However, images must initially be created in either 
    // VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK( qvkCreateImage(vk.device, &desc, NULL, &pImg->handle) );
}


static void vk_allocDeviceLocalMemory(uint32_t memType, uint32_t const idx,
        struct ImageChunk_t* const pChunk)
{
    VkMemoryAllocateInfo alloc_info;
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = IMAGE_CHUNK_SIZE;
    alloc_info.memoryTypeIndex = find_memory_type( 
        memType, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    // Allocate a new chunk
    VK_CHECK( qvkAllocateMemory(vk.device, &alloc_info, NULL, &pChunk[idx].block) );
    
    ri.Printf(PRINT_ALL, "Allocate Device local memory, Size: %d MB, Type Index: %d. \n",
            (IMAGE_CHUNK_SIZE >> 20), alloc_info.memoryTypeIndex);
}


static void vk_bindImageHandleWithDeviceMemory(VkImage hImg, uint32_t * const pIdx_uplimit,
        struct ImageChunk_t* const pChunk)
{

    VkMemoryRequirements memory_requirements;
    NO_CHECK( qvkGetImageMemoryRequirements(vk.device, hImg, &memory_requirements) );
    
    if(*pIdx_uplimit == 0)
    {
        // allocate memory ...
        vk_allocDeviceLocalMemory(memory_requirements.memoryTypeBits, 0, pChunk);
        ++*pIdx_uplimit;
    }

    uint32_t i = *pIdx_uplimit - 1;
    // ensure that memory region has proper alignment
    uint32_t mask = (memory_requirements.alignment - 1);
    uint32_t offset_aligned = (pChunk[i].Used + mask) & (~mask);
    uint32_t end = offset_aligned + memory_requirements.size;
    
    if(end <= IMAGE_CHUNK_SIZE)
    {
        VK_CHECK( qvkBindImageMemory(vk.device, hImg, pChunk[i].block, offset_aligned) );
        pChunk[i].Used = end;
    }
    else
    {
        // space not enough, allocate a new chunk ...
        vk_allocDeviceLocalMemory(memory_requirements.memoryTypeBits, *pIdx_uplimit, pChunk);
        VK_CHECK( qvkBindImageMemory(vk.device, hImg, pChunk[*pIdx_uplimit].block, 0) );
        pChunk[*pIdx_uplimit].Used = memory_requirements.size;
        ++*pIdx_uplimit;
    }
}

void vk_createViewForImageHandle(VkImage Handle, VkFormat Fmt, VkImageView* const pView)
{
    // In many cases, the image resource cannot be used directly, 
    // as more information about it is needed than is included in
    // the resource itself. For example, you cannot use an image
    // resource directly as an attacnment to a framebuffer or
    // bind an image in to a descriptor set in order to sample it
    // in a shader. To satisfy these additional requirements,
    // you must create an image view, which is essentically a 
    // collecton of properties and a reference to a parent image
    // resource.

    VkImageViewCreateInfo desc;
    desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.image = Handle;
    desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
    // format is a VkFormat describing the format and type used 
    // to interpret data elements in the image.
    desc.format = Fmt;

    // the components field allows you to swizzle the color channels
    // around. VK_COMPONENT_SWIZZLE_IDENTITY indicates that the data
    // in the child image should be read from the corresponding 
    // channel in the parent image.
    desc.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    desc.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    desc.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    desc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // The subresourceRange field describes what the image's purpose is
    // and which part of the image should be accessed. 
    //
    // selecting the set of mipmap levels and array layers to be accessible to the view.
    desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    desc.subresourceRange.baseMipLevel = 0;
    desc.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    desc.subresourceRange.baseArrayLayer = 0;
    desc.subresourceRange.layerCount = 1;
    // Some of the image creation parameters are inherited by the view.
    // In particular, image view creation inherits the implicit parameter
    // usage specifying the allowed usages of the image view that, 
    // by default, takes the value of the corresponding usage parameter
    // specified in VkImageCreateInfo at image creation time.
    //
    // This implicit parameter can be overriden by chaining a 
    // VkImageViewUsageCreateInfo structure through the pNext member to
    // VkImageViewCreateInfo.
    //
    // The resulting view of the parent image must have the same dimensions
    // as the parent. The format of the parent and child images must be
    // compatible, which usually means that they have the same number of
    // bits per pixel.
    VK_CHECK( qvkCreateImageView(vk.device, &desc, NULL, pView) );
}


static void vk_createDescriptorSet(image_t * const pImage)
{
    // Allocate a descriptor set from the pool. 
    // Note that we have to provide the descriptor set layout that 
    // This layout describes how the descriptor set is to be allocated.

    vk_allocOneDescptrSet(&pImage->descriptor_set);

    //ri.Printf(PRINT_ALL, " Allocate Descriptor Sets \n");
    VkWriteDescriptorSet descriptor_write;

    VkDescriptorImageInfo image_info;
    image_info.sampler = vk_find_sampler(pImage->mipmap, pImage->wrapClampMode == GL_REPEAT);
    image_info.imageView = pImage->view;
    // the image will be bound for reading by shaders.
    // this layout is typically used when an image is going to
    // be used as a texture.
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = pImage->descriptor_set;
    descriptor_write.dstBinding = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pNext = NULL;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.pImageInfo = &image_info;
    descriptor_write.pBufferInfo = NULL;
    descriptor_write.pTexelBufferView = NULL;


    NO_CHECK( qvkUpdateDescriptorSets(vk.device, 1, &descriptor_write, 0, NULL) );

    // The above steps essentially copy the VkDescriptorBufferInfo
    // to the descriptor, which is likely in the device memory.
}



image_t* R_CreateImage( const char *name, unsigned char* pic, const uint32_t width, const uint32_t height,
						VkBool32 isMipMap, VkBool32 allowPicmip, int glWrapClampMode)
{
    if (strlen(name) >= MAX_QPATH ) {
        ri.Error (ERR_DROP, "CreateImage: \"%s\" is too long\n", name);
    }


    // ri.Printf( PRINT_ALL, " Create Image: %s\n", name);
    
    // Create image_t object.

    image_t* pImage = (image_t*) ri.Hunk_Alloc( sizeof( image_t ), h_low );

    strncpy (pImage->imgName, name, sizeof(pImage->imgName));
    pImage->index = tr.numImages;
    pImage->mipmap = isMipMap;
    pImage->mipLevels = 1;
    pImage->allowPicmip = allowPicmip;
    pImage->wrapClampMode = glWrapClampMode;
    pImage->width = width;
    pImage->height = height;
    pImage->isLightmap = (strncmp(name, "*lightmap", 9) == 0);
    // Create corresponding GPU resource, lightmaps are always allocated on TMU 1 .
    // A texture mapping unit (TMU) is a component in modern graphics processing units (GPUs). 
    // Historically it was a separate physical processor. A TMU is able to rotate, resize, 
    // and distort a bitmap image (performing texture sampling), to be placed onto an arbitrary
    // plane of a given 3D model as a texture. This process is called texture mapping. 
    // In modern graphics cards it is implemented as a discrete stage in a graphics pipeline, 
    // whereas when first introduced it was implemented as a separate processor, 
    // e.g. as seen on the Voodoo2 graphics card. 
    //
    // The TMU came about due to the compute demands of sampling and transforming a flat
    // image (as the texture map) to the correct angle and perspective it would need to
    // be in 3D space. The compute operation is a large matrix multiply, 
    // which CPUs of the time (early Pentiums) could not cope with at acceptable performance.
    //
    // Today (2013), TMUs are part of the shader pipeline and decoupled from the
    // Render Output Pipelines (ROPs). For example, in AMD's Cypress GPU, 
    // each shader pipeline (of which there are 20) has four TMUs, giving the GPU 80 TMUs.
    // This is done by chip designers to closely couple shaders and the texture engines
    // they will be working with. 
    //
    // 3D scenes are generally composed of two things: 3D geometry, and the textures 
    // that cover that geometry. Texture units in a video card take a texture and 'map' it
    // to a piece of geometry. That is, they wrap the texture around the geometry and 
    // produce textured pixels which can then be written to the screen. 
    //
    // Textures can be an actual image, a lightmap, or even normal maps for advanced 
    // surface lighting effects. 


    // convert to exact power of 2 sizes
  
    const unsigned int max_texture_size = 2048;
    
    unsigned int scaled_width, scaled_height;

    for(scaled_width = max_texture_size; scaled_width > width; scaled_width>>=1)
        ;
    
    for (scaled_height = max_texture_size; scaled_height > height; scaled_height>>=1)
        ;


    if ( allowPicmip )
    {
        scaled_width >>= r_picmip->integer;
        scaled_height >>= r_picmip->integer;
    }

    pImage->uploadWidth = scaled_width;
    pImage->uploadHeight = scaled_height;
    
    uint32_t buffer_size = 4 * pImage->uploadWidth * pImage->uploadHeight;
    unsigned char * const pUploadBuffer = (unsigned char*) malloc ( 2 * buffer_size);

    if ((scaled_width != width) || (scaled_height != height) )
    {
        // just info
        // ri.Printf( PRINT_WARNING, "ResampleTexture: inwidth: %d, inheight: %d, outwidth: %d, outheight: %d\n",
        //        width, height, scaled_width, scaled_height );
        
        //go down from [width, height] to [scaled_width, scaled_height]
        ResampleTexture (pUploadBuffer, width, height, pic, scaled_width, scaled_height);
    }
    else
    {
        memcpy(pUploadBuffer, pic, buffer_size);
    }


    // perform optional picmip operation


    ////////////////////////////////////////////////////////////////////
    // 2^12 = 4096
    // The set of all bytes bound to all the source regions must not overlap
    // the set of all bytes bound to the destination regions.
    //
    // The set of all bytes bound to each destination region must not overlap
    // the set of all bytes bound to another destination region.

    VkBufferImageCopy regions[12];

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
    regions[0].imageExtent.width = pImage->uploadWidth;
    regions[0].imageExtent.height = pImage->uploadHeight;
    regions[0].imageExtent.depth = 1;

    if(isMipMap)
    {
        uint32_t curMipMapLevel = 1; 
        uint32_t base_width = pImage->uploadWidth;
        uint32_t base_height = pImage->uploadHeight;

        unsigned char* in_ptr = pUploadBuffer;
        unsigned char* dst_ptr = in_ptr + buffer_size;

        R_LightScaleTexture(pUploadBuffer, pUploadBuffer, buffer_size);

        // Use the normal mip-mapping to go down from [scaled_width, scaled_height] to [1,1] dimensions.

        while (1)
        {

            if ( r_simpleMipMaps->integer )
            {
                R_MipMap(in_ptr, base_width, base_height, dst_ptr);
            }
            else
            {
                R_MipMap2(in_ptr, base_width, base_height, dst_ptr);
            }


            if ((base_width == 1) && (base_height == 1))
                break;

            base_width >>= 1;
            if (base_width == 0) 
                base_width = 1;

            base_height >>= 1;
            if (base_height == 0)
                base_height = 1;

            regions[curMipMapLevel].bufferOffset = buffer_size;
            regions[curMipMapLevel].bufferRowLength = 0;
            regions[curMipMapLevel].bufferImageHeight = 0;
            regions[curMipMapLevel].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            regions[curMipMapLevel].imageSubresource.mipLevel = curMipMapLevel;
            regions[curMipMapLevel].imageSubresource.baseArrayLayer = 0;
            regions[curMipMapLevel].imageSubresource.layerCount = 1;
            regions[curMipMapLevel].imageOffset.x = 0;
            regions[curMipMapLevel].imageOffset.y = 0;
            regions[curMipMapLevel].imageOffset.z = 0;

            regions[curMipMapLevel].imageExtent.width = base_width;
            regions[curMipMapLevel].imageExtent.height = base_height;
            regions[curMipMapLevel].imageExtent.depth = 1;
            

            uint32_t curLevelSize = base_width * base_height * 4;

            buffer_size += curLevelSize;
            
            // Regions must not extend outside the bounds of the buffer or image level,
            // except that regions of compressed images can extend as far as the
            // dimension of the image level rounded up to a complete compressed texel block.

            assert(buffer_size <= IMAGE_CHUNK_SIZE);

            if ( r_colorMipLevels->integer ) {
                R_BlendOverTexture( in_ptr, base_width * base_height, curMipMapLevel );
            }


            ++curMipMapLevel;

            in_ptr = dst_ptr;
            dst_ptr += curLevelSize; 
        }
        pImage->mipLevels = curMipMapLevel; 
        // ri.Printf( PRINT_WARNING, "curMipMapLevel: %d, base_width: %d, base_height: %d, buffer_size: %d, name: %s\n",
        //    curMipMapLevel, scaled_width, scaled_height, buffer_size, name);
    }

    
    vk_create2DImageHandle( VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, pImage);
    vk_bindImageHandleWithDeviceMemory(pImage->handle, &devMemImg.Index, devMemImg.Chunks);
    vk_createViewForImageHandle(pImage->handle, VK_FORMAT_R8G8B8A8_UNORM, &pImage->view);
    vk_createDescriptorSet(pImage);


    void* data;
    VK_CHECK( qvkMapMemory(vk.device, StagBuf.mappableMem, 0, VK_WHOLE_SIZE, 0, &data) );
    memcpy(data, pUploadBuffer, buffer_size);
    vk_stagBufferToDeviceLocalMem(pImage->handle, regions, pImage->mipLevels);
    NO_CHECK( qvkUnmapMemory(vk.device, StagBuf.mappableMem) );

    free(pUploadBuffer);


    const int hash = generateHashValue(name);
    pImage->next = hashTable[hash];
    hashTable[hash] = pImage;

    tr.images[tr.numImages] = pImage;
    if ( ++tr.numImages >= MAX_DRAWIMAGES )
    {
        ri.Error( ERR_DROP, "CreateImage: MAX_DRAWIMAGES hit\n");
    }

    return pImage;
}


static void vk_destroySingleImage( image_t* pImg )
{
   	// ri.Printf(PRINT_ALL, " Destroy Image: %s \n", pImg->imgName); 
    if(pImg->descriptor_set != VK_NULL_HANDLE)
    {   
        //To free allocated descriptor sets
        NO_CHECK( qvkFreeDescriptorSets(vk.device, vk.descriptor_pool, 1, &pImg->descriptor_set) );
        pImg->descriptor_set = VK_NULL_HANDLE;
    }

    if (pImg->view != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkDestroyImageView(vk.device, pImg->view, NULL) );
        pImg->view = VK_NULL_HANDLE; 
    }

    
    if (pImg->handle != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkDestroyImage(vk.device, pImg->handle, NULL) );
        pImg->handle = VK_NULL_HANDLE;
    }
}



image_t* R_FindImageFile(const char *name, VkBool32 mipmap, VkBool32 allowPicmip, int glWrapClampMode)
{
   	image_t* image;

	if (name == NULL)
    {
        ri.Printf( PRINT_WARNING, "Find Image File: NULL\n");
		return NULL;
	}

	int hash = generateHashValue(name);

	// see if the image is already loaded

	for (image=hashTable[hash]; image; image=image->next)
	{
		if ( !strcmp( name, image->imgName ) )
		{
			// the white image can be used with any set of parms,
			// but other mismatches are errors
			if ( strcmp( name, "*white" ) )
			{
				if ( image->mipmap != mipmap ) {
					ri.Printf( PRINT_WARNING, "WARNING: reused image %s with mixed mipmap parm\n", name );
				}
				if ( image->wrapClampMode != glWrapClampMode ) {
					ri.Printf( PRINT_WARNING, "WARNING: reused image %s with mixed glWrapClampMode parm\n", name );
				}
			}
			return image;
		}
	}

    //
	// Not find from already loadied, load the pic from disk
    //
    uint32_t width = 0, height = 0;
    unsigned char* pic = NULL;
    
    R_LoadImage( name, &pic, &width, &height );

    if (pic == NULL)
    {
        ri.Printf( PRINT_WARNING, "R_FindImageFile: Fail loading %s the from disk\n", name);
        return NULL;
    }

    image = R_CreateImage( name, pic, width, height, mipmap, allowPicmip, glWrapClampMode);

    ri.Free( pic );
    
    return image;
}


image_t	* tr_scratchImage[16];
struct shader_s * tr_cinematicShader;

void RE_UploadCinematic (int w, int h, int cols, int rows, const unsigned char *data, int client, VkBool32 dirty)
{

    image_t* const prtImage = tr_scratchImage[client];
    
    // if the scratchImage isn't in the format we want, specify it as a new texture
    if ( (cols != prtImage->uploadWidth) || (rows != prtImage->uploadHeight) )
    {
        ri.Printf(PRINT_ALL, "w=%d, h=%d, cols=%d, rows=%d, client=%d, prtImage->width=%d, prtImage->height=%d\n", 
           w, h, cols, rows, client, prtImage->uploadWidth, prtImage->uploadHeight);

        // VULKAN

        vk_destroySingleImage(prtImage);

        prtImage->uploadWidth = cols;
        prtImage->uploadHeight = rows;
        prtImage->mipLevels = 1;

        // vk_createImageAndBindWithMemory(prtImage);
        vk_create2DImageHandle( VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, prtImage);
        vk_bindImageHandleWithDeviceMemory(prtImage->handle, &devMemImg.Index, devMemImg.Chunks);
        vk_createViewForImageHandle(prtImage->handle, VK_FORMAT_R8G8B8A8_UNORM, &prtImage->view);
        vk_createDescriptorSet(prtImage);


        VkBufferImageCopy region;
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset.x = 0;
        region.imageOffset.y = 0;
        region.imageOffset.z = 0;
        region.imageExtent.width = cols;
        region.imageExtent.height = rows;
        region.imageExtent.depth = 1;

        const uint32_t buffer_size = cols * rows * 4;

        void* pDat;
        VK_CHECK( qvkMapMemory(vk.device, StagBuf.mappableMem, 0, VK_WHOLE_SIZE, 0, &pDat) );
        memcpy(pDat, data, buffer_size);
        vk_stagBufferToDeviceLocalMem(tr_scratchImage[client]->handle, &region, 1);
        
        NO_CHECK( qvkUnmapMemory(vk.device, StagBuf.mappableMem) );
    }
    else if (dirty)
    {
        // otherwise, just subimage upload it so that
        // drivers can tell we are going to be changing
        // it and don't try and do a texture compression       
        // vk_uploadSingleImage(prtImage->handle, cols, rows, data);

        VkBufferImageCopy region;
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset.x = 0;
        region.imageOffset.y = 0;
        region.imageOffset.z = 0;
        region.imageExtent.width = cols;
        region.imageExtent.height = rows;
        region.imageExtent.depth = 1;

        const uint32_t buffer_size = cols * rows * 4;

        void* pDat;
        VK_CHECK( qvkMapMemory(vk.device, StagBuf.mappableMem, 0, VK_WHOLE_SIZE, 0, &pDat));
        memcpy(pDat, data, buffer_size);
        vk_stagBufferToDeviceLocalMem(tr_scratchImage[client]->handle, &region, 1);
        NO_CHECK( qvkUnmapMemory(vk.device, StagBuf.mappableMem) );
    }
}


/*
=============
FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const unsigned char *data, int client, qboolean dirty)
{
	int	i, j;

	if ( !tr.registered ) {
		return;
	}
	
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
	for ( i = 1 ; ( 1 << i ) < cols ; ++i )
    {
        ;
	}
	for ( j = 1 ; ( 1 << j ) < rows ; ++j )
    {
        ;
	}
    
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows) {
		ri.Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

    RE_UploadCinematic(w, h, cols, rows, data, client, dirty);

    tr_cinematicShader->stages[0]->bundle[0].image[0] = tr_scratchImage[client];
    
    
    RE_StretchPic(x, y, w, h,  0.5f / cols, 0.5f / rows,  1.0f - 0.5f / cols, 1.0f - 0.5 / rows, tr_cinematicShader->index);
}



image_t * R_GetScratchImageHandle(int idx)
{
	ri.Printf (PRINT_ALL, " R_GetScratchImageHandle: %i\n", idx);

    return tr_scratchImage[idx];
}

void R_SetCinematicShader( struct shader_s * pShader)
{
    ri.Printf (PRINT_ALL, " R_SetCinematicShader \n");

    tr_cinematicShader = pShader;
}


static void R_CreateDefaultImage( void )
{
	#define	DEFAULT_SIZE 32

	unsigned char data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	memset( data, 32, sizeof( data ) );

	uint32_t x;
	for ( x = 0; x < DEFAULT_SIZE; ++x )
	{
		data[0][x][0] =
			data[0][x][1] =
			data[0][x][2] =
			data[0][x][3] = 255;

		data[x][0][0] =
			data[x][0][1] =
			data[x][0][2] =
			data[x][0][3] = 255;

		data[DEFAULT_SIZE-1][x][0] =
			data[DEFAULT_SIZE-1][x][1] =
			data[DEFAULT_SIZE-1][x][2] =
			data[DEFAULT_SIZE-1][x][3] = 255;

		data[x][DEFAULT_SIZE-1][0] =
			data[x][DEFAULT_SIZE-1][1] =
			data[x][DEFAULT_SIZE-1][2] =
			data[x][DEFAULT_SIZE-1][3] = 255;
	}
	tr.defaultImage = R_CreateImage("*default", (unsigned char *)data, DEFAULT_SIZE, DEFAULT_SIZE, qtrue, qfalse, GL_REPEAT);
    #undef DEFAULT_SIZE
}



static void R_CreateWhiteImage(void)
{
    #define	DEFAULT_SIZE 32
	// we use a solid white image instead of disabling texturing
	unsigned char data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage("*white", (unsigned char *)data, DEFAULT_SIZE, DEFAULT_SIZE, qfalse, qfalse, GL_REPEAT);
    #undef DEFAULT_SIZE
}


static void R_CreateIdentityLightImage(void)
{
    #define	DEFAULT_SIZE 64
    uint32_t x,y;
	unsigned char data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for (x=0 ; x<DEFAULT_SIZE ; ++x)
    {
		for (y=0; y<DEFAULT_SIZE; ++y)
        {
			data[y][x][0] = 
			data[y][x][1] = 
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}
	tr.identityLightImage = R_CreateImage("*identityLight", (unsigned char *)data, DEFAULT_SIZE, DEFAULT_SIZE,
            qfalse, qfalse, GL_REPEAT);
    #undef DEFAULT_SIZE
}


static void R_CreateDlightImage( void )
{
    #define	DLIGHT_SIZE	32
	uint32_t x,y;
	unsigned char data[DLIGHT_SIZE][DLIGHT_SIZE][4];

	// make a centered inverse-square falloff blob for dynamic lighting
	for (x=0; x<DLIGHT_SIZE; ++x)
    {
		for (y=0; y<DLIGHT_SIZE; ++y)
        {
            float w = DLIGHT_SIZE/2 - 0.5f - x;
            float h = DLIGHT_SIZE/2 - 0.5f - y;
			float d = w * w + h * h;
			int b = 16000 / d;
			if (b > 255) {
				b = 255;
			} else if ( b < 95 ) {
				b = 0;
			}

			data[x][y][0] = 
			data[x][y][1] = 
			data[x][y][2] = b;
			data[x][y][3] = 255;			
		}
	}
	tr.dlightImage = R_CreateImage("*dlight", (unsigned char*) data, DLIGHT_SIZE, DLIGHT_SIZE,
            qfalse, qfalse, GL_CLAMP);
    #undef DEFAULT_SIZE
}


static void R_CreateFogImage( void )
{
    #define	FOG_S	256
    #define	FOG_T	32

	unsigned int x,y;

	unsigned char* const data = (unsigned char*) malloc( FOG_S * FOG_T * 4 );

	// S is distance, T is depth
	for (x=0 ; x<FOG_S ; x++)
    {
		for (y=0 ; y<FOG_T ; y++)
        {
			float d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

            unsigned int index = (y*FOG_S+x)*4;
			data[index ] = 
			data[index+1] = 
			data[index+2] = 255;
			data[index+3] = 255*d;
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage("*fog", (unsigned char *)data, FOG_S, FOG_T, qfalse, qfalse, GL_CLAMP);
	
    free( data );
}



image_t* R_CreateImageForCinematic( const char *name, unsigned char* pic, const uint32_t width, const uint32_t height)
{
    // ri.Printf( PRINT_ALL, " Create Image: %s\n", name);
    
    image_t* pImage = (image_t*) ri.Hunk_Alloc( sizeof( image_t ), h_low );

    strncpy (pImage->imgName, name, sizeof(pImage->imgName));
    pImage->index = tr.numImages;
    pImage->mipmap = 0; 
    pImage->mipLevels = 1; 
    pImage->allowPicmip = 1; //
    pImage->wrapClampMode = GL_CLAMP; //
    pImage->width = width;
    pImage->height = height;
    pImage->isLightmap = 0; //

  
    const unsigned int max_texture_size = 2048;
    
    unsigned int scaled_width, scaled_height;

    for(scaled_width = max_texture_size; scaled_width > width; scaled_width>>=1)
        ;
    
    for (scaled_height = max_texture_size; scaled_height > height; scaled_height>>=1)
        ;
    
    pImage->uploadWidth = scaled_width;
    pImage->uploadHeight = scaled_height;
    
    uint32_t buffer_size = 4 * scaled_width * scaled_height;
    unsigned char * const pUploadBuffer = (unsigned char*) malloc ( 2 * buffer_size);

    if ((scaled_width != width) || (scaled_height != height) )
    {
        // just info
        // ri.Printf( PRINT_WARNING, "ResampleTexture: inwidth: %d, inheight: %d, outwidth: %d, outheight: %d\n",
        //        width, height, scaled_width, scaled_height );
        
        //go down from [width, height] to [scaled_width, scaled_height]
        ResampleTexture (pUploadBuffer, width, height, pic, scaled_width, scaled_height);
    }
    else
    {
        memcpy(pUploadBuffer, pic, buffer_size);
    }


    // perform optional picmip operation


    ////////////////////////////////////////////////////////////////////
    // 2^12 = 4096
    // The set of all bytes bound to all the source regions must not overlap
    // the set of all bytes bound to the destination regions.
    //
    // The set of all bytes bound to each destination region must not overlap
    // the set of all bytes bound to another destination region.

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
    regions[0].imageExtent.width = pImage->uploadWidth;
    regions[0].imageExtent.height = pImage->uploadHeight;
    regions[0].imageExtent.depth = 1;

    
    vk_create2DImageHandle( VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, pImage);
    
    vk_bindImageHandleWithDeviceMemory(pImage->handle, &devMemImg.Index, devMemImg.Chunks);

    vk_createViewForImageHandle(pImage->handle, VK_FORMAT_R8G8B8A8_UNORM, &pImage->view);
    vk_createDescriptorSet(pImage);


    void* data;
    VK_CHECK( qvkMapMemory(vk.device, StagBuf.mappableMem, 0, VK_WHOLE_SIZE, 0, &data) );
    memcpy(data, pUploadBuffer, buffer_size);
    NO_CHECK( qvkUnmapMemory(vk.device, StagBuf.mappableMem) );
    vk_stagBufferToDeviceLocalMem(pImage->handle, regions, pImage->mipLevels);
    
    free(pUploadBuffer);

    return pImage;
}


static void R_CreateScratchImage(void)
{
    #define DEFAULT_SIZE 512

    uint32_t x;
    
    unsigned char data[DEFAULT_SIZE][DEFAULT_SIZE][4];

    for(x=0; x<16; ++x)
    {
        // scratchimage is usually used for cinematic drawing
        tr_scratchImage[x] = R_CreateImageForCinematic("*scratch", (unsigned char *)data, 
                DEFAULT_SIZE, DEFAULT_SIZE);
    }
    #undef DEFAULT_SIZE
}



void R_InitImages( void )
{
    memset(hashTable, 0, sizeof(hashTable));

    memset( tr_scratchImage, 0, sizeof( tr_scratchImage ) );
    tr_cinematicShader = NULL;

    ri.Printf(PRINT_ALL, " Create staging buffer (8 MB) \n");

    // StagBuf.buff must have been created with VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    // usage flag for vkCmdCopyBufferToImage.
    vk_createBufferResource( 8 * 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
             &StagBuf.buff, &StagBuf.mappableMem );

	// setup the overbright lighting

	tr.identityLight = 1.0f;
	tr.identityLightByte = 255 * tr.identityLight;


    // build brightness translation tables
    R_SetColorMappings(r_brightness->value, r_gamma->value);

    // create default texture and white texture
    // R_CreateBuiltinImages();

    R_CreateDefaultImage();

    R_CreateWhiteImage();

    // R_CreateIdentityLightImage();

    R_CreateScratchImage();
    
    R_CreateDlightImage();
    
    R_CreateFogImage();
}





void vk_destroyImageRes(void)
{
    ri.Printf(PRINT_ALL, " vk_destroyImageRes. \n");
	vk_free_sampler();

    uint32_t i = 0;

	for (i = 0; i < tr.numImages; ++i)
	{
        vk_destroySingleImage(tr.images[i]);
	}

	for (i = 0; i < 16; ++i)
	{
        vk_destroySingleImage(tr_scratchImage[i]);
	}
    memset( tr_scratchImage, 0, sizeof( tr_scratchImage ) );
    tr_cinematicShader = NULL;

    for (i = 0; i < devMemImg.Index; ++i)
    {
        NO_CHECK( qvkFreeMemory(vk.device, devMemImg.Chunks[i].block, NULL) );
        devMemImg.Chunks[i].Used = 0;
    }

    memset(&devMemImg, 0, sizeof(devMemImg));

    ri.Printf(PRINT_ALL, " Destroy staging buffer res: StagBuf.buff, StagBuf.mappableMem.\n");
    vk_destroyBufferResource(StagBuf.buff, StagBuf.mappableMem);
    memset(&StagBuf, 0, sizeof(StagBuf));

    
    // Destroying a pool object implicitly frees all objects allocated from that pool. 
    // Specifically, destroying VkCommandPool frees all VkCommandBuffer objects that 
    // were allocated from it, and destroying VkDescriptorPool frees all 
    // VkDescriptorSet objects that were allocated from it.
    VK_CHECK( qvkResetDescriptorPool(vk.device, vk.descriptor_pool, 0) );

    memset( tr.images, 0, sizeof( tr.images ) );
    
    tr.numImages = 0;

    memset(hashTable, 0, sizeof(hashTable));
}
