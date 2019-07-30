#include "vk_buffers.h"
#include "vk_instance.h"
#include "ref_import.h"
#include "vk_image.h"
#include "vk_cmd.h"

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
    uint32_t capacity; // size in bytes
};


static struct StagingBuffer_t StagBuf;

uint32_t R_GetStagingBufferSize(void)
{
    return StagBuf.capacity;
}

void vk_stagBufToDevLocal(VkImage hImage, VkBufferImageCopy* const pRegion, const uint32_t nRegion)
{
    vk_beginRecordCmds( vk.tmpRecordBuffer );
	
    VkImageMemoryBarrier barrier ;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = hImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	
    NO_CHECK( qvkCmdPipelineBarrier(vk.tmpRecordBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,	0, NULL, 0, NULL, 1, &barrier) );


    NO_CHECK( qvkCmdCopyBufferToImage( vk.tmpRecordBuffer, StagBuf.buff, hImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, nRegion, pRegion) );

    VkImageMemoryBarrier barrier2 ;
	barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier2.pNext = NULL;
	barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier2.image = hImage;
	barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier2.subresourceRange.baseMipLevel = 0;
	barrier2.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier2.subresourceRange.baseArrayLayer = 0;
	barrier2.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	
    NO_CHECK( qvkCmdPipelineBarrier(vk.tmpRecordBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier2) );

    vk_commitRecordedCmds(vk.tmpRecordBuffer);
}



void vk_createBufferResource(const uint32_t Size, VkBufferUsageFlags Usage,
        VkMemoryPropertyFlagBits MemTypePrefered,
        VkBuffer * const pBuf, VkDeviceMemory * const pDevMem )
{
    // memset(&StagBuf, 0, sizeof(StagBuf));

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
        memory_requirements.memoryTypeBits, MemTypePrefered );

    VK_CHECK( qvkAllocateMemory(vk.device, &alloc_info, NULL, pDevMem) );

    VK_CHECK( qvkBindBufferMemory(vk.device, *pBuf, *pDevMem, 0) );

#ifndef NDEBUG  
    ri.Printf(PRINT_ALL, " Create Buffer, Size: %d bytes, Alignment: %ld, Type Index: %d. \n",
       Size, memory_requirements.alignment, alloc_info.memoryTypeIndex);
#endif
}


void vk_destroyBufferResource(VkBuffer hBuf, VkDeviceMemory hDevMem)
{
    if (hDevMem != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkFreeMemory(vk.device, hDevMem, NULL) );
    }

    if (hBuf != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkDestroyBuffer(vk.device, hBuf, NULL) );
    }
}


void vk_createStagingBuffer(uint32_t size)
{
    // ri.Printf(PRINT_ALL, " Create Staging Buffer, Size: %d KB. \n", size / 1024);
    StagBuf.capacity = size;
    vk_createBufferResource( size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,  &StagBuf.buff, &StagBuf.mappableMem );
}


void vk_destroyStagingBuffer(void)
{
    ri.Printf(PRINT_ALL, " Destroy staging buffer res: StagBuf.buff, StagBuf.mappableMem.\n");
    
	if (StagBuf.mappableMem != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkFreeMemory(vk.device, StagBuf.mappableMem, NULL) );
    }

    if (StagBuf.buff != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkDestroyBuffer(vk.device, StagBuf.buff, NULL) );
    }

    memset(&StagBuf, 0, sizeof(StagBuf));
}


void VK_UploadImageToStagBuffer(const unsigned char * const pUploadBuffer, uint32_t buffer_size)
{
    void* data;
    
    if( StagBuf.capacity < buffer_size)
    {
        // reallocate memory
        ri.Printf(PRINT_ALL, " Create Staging Buffer, Size: %d KB. \n", buffer_size / 1024);
        vk_destroyStagingBuffer();
        vk_createStagingBuffer(buffer_size);
    }

    VK_CHECK( qvkMapMemory(vk.device, StagBuf.mappableMem, 0, buffer_size, 0, &data) );
    memcpy(data, pUploadBuffer, buffer_size);
    NO_CHECK( qvkUnmapMemory(vk.device, StagBuf.mappableMem) );
}

