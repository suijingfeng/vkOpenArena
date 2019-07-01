#include "vk_buffers.h"
#include "vk_instance.h"
#include "ref_import.h"
#include "vk_image.h"

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
		hDevMem = VK_NULL_HANDLE;
    }

    if (hBuf != VK_NULL_HANDLE)
    {
        NO_CHECK( qvkDestroyBuffer(vk.device, hBuf, NULL) );
        hBuf = VK_NULL_HANDLE;
    }
}
