#include "VKimpl.h"
#include "vk_instance.h"
#include "tr_shader.h"
#include "vk_image.h"
#include "ref_import.h" 
#include "render_export.h"
#include "vk_image_sampler.h"
// #include "tr_globals.h"

#define NUM_CINEMATIC   1

// #define IMAGE_MEMORY_SIZE   4 * 1024 * 1024
// Host visible memory used to copy image data to device local memory.

static VkDeviceMemory s_mappableMemory;

/*
struct SingleMappableImage_s {

	char		name[MAX_QPATH];		// game path, including extension
	uint32_t	uploadWidth, uploadHeight;	// after power of two 
    
    VkDeviceMemory mappableMemory;

	VkImage handle;
    // To use any VkImage, including those in the swap chain, int the render pipeline
    // we have to create a VkImageView object. An image view is quite literally a
    // view into image. It describe how to access the image and witch part of the
    // image to access, if it should be treated as a 2D texture depth texture without
    // any mipmapping levels.
    
    VkImageView view;

    // Descriptor set that contains single descriptor used to access the given image.
	// It is updated only once during image initialization.
	VkDescriptorSet descriptor_set;
};
*/
 
static struct image_s tr_scratchImage[NUM_CINEMATIC];
static struct shader_s * tr_cinematicShader;


struct image_s * R_GetScratchImageHandle(int idx)
{
	ri.Printf (PRINT_ALL, " R_GetScratchImageHandle: %i\n", idx);

    return &tr_scratchImage[idx];
}


void R_SetCinematicShader( struct shader_s * const pShader)
{
    ri.Printf (PRINT_ALL, " R_SetCinematicShader \n");

    tr_cinematicShader = pShader;
}


void vk_initScratchImage(void)
{
    ri.Printf (PRINT_ALL, " Init Scratch Image. \n");

    memset( tr_scratchImage, 0, sizeof( tr_scratchImage ) );
    tr_cinematicShader = NULL;
}


void vk_destroyScratchImage(void)
{
    struct image_s * const pImg = tr_scratchImage;
    uint32_t i;
	for (i = 0; i < NUM_CINEMATIC; ++i)
	{
  
        // vk_destroySingleImage(&tr_scratchImage[i]);
        if(pImg[i].descriptor_set != VK_NULL_HANDLE)
        {   
            //To free allocated descriptor sets
            NO_CHECK( qvkFreeDescriptorSets(vk.device, vk.descriptor_pool, 1, &pImg[i].descriptor_set) );
            pImg[i].descriptor_set = VK_NULL_HANDLE;
        }

        
        if (pImg[i].view != VK_NULL_HANDLE)
        {
            NO_CHECK( qvkDestroyImageView(vk.device, pImg[i].view, NULL) );
            pImg[i].view = VK_NULL_HANDLE; 
        }

        if (s_mappableMemory != VK_NULL_HANDLE)
        {
            NO_CHECK( qvkFreeMemory(vk.device, s_mappableMemory, NULL) );
            s_mappableMemory = VK_NULL_HANDLE;
        }

        if (pImg[i].handle != VK_NULL_HANDLE)
        {
            NO_CHECK( qvkDestroyImage(vk.device, pImg[i].handle, NULL) );
            pImg->handle = VK_NULL_HANDLE;
        }
	}

    memset( pImg, 0, NUM_CINEMATIC * sizeof( struct image_s ) );
    
    tr_cinematicShader = NULL;
}

static void vk_uploadImageDataReal(VkImage imgHandle, const unsigned char * const pData, uint32_t size)
{
    void* pDst;
    VK_CHECK( qvkMapMemory(vk.device, s_mappableMemory, 0, VK_WHOLE_SIZE, 0, &pDst) );
    
    memcpy(pDst, pData, size);

    NO_CHECK( qvkUnmapMemory(vk.device, s_mappableMemory) );
}


void RE_UploadCinematic(int w, int h, int cols, int rows, const unsigned char * data, int client, VkBool32 dirty)
{

    struct image_s* const pImage = &tr_scratchImage[client];
    
    // the image may not even created, and it image data may not even uploaded.
    // if the scratchImage isn't in the format we want, specify it as a new texture
    if ( (cols != pImage->uploadWidth) || (rows != pImage->uploadHeight) )
    {
        ri.Printf(PRINT_ALL, "w=%d, h=%d, cols=%d, rows=%d, client=%d, prtImage->width=%d, prtImage->height=%d\n", 
           w, h, cols, rows, client, pImage->uploadWidth, pImage->uploadHeight);

        // VULKAN
        // if already created, we will destroy it.
        if(pImage->descriptor_set != VK_NULL_HANDLE)
        {   
            //To free allocated descriptor sets
            NO_CHECK( qvkFreeDescriptorSets(vk.device, vk.descriptor_pool, 1, &pImage->descriptor_set) );
            pImage->descriptor_set = VK_NULL_HANDLE;
        }
        
        if (pImage->view != VK_NULL_HANDLE)
        {
            NO_CHECK( qvkDestroyImageView(vk.device, pImage->view, NULL) );
            pImage->view = VK_NULL_HANDLE; 
        }

        if (s_mappableMemory != VK_NULL_HANDLE)
        {
            NO_CHECK( qvkFreeMemory(vk.device, s_mappableMemory, NULL) );
            s_mappableMemory = VK_NULL_HANDLE;
        }

        if (pImage->handle != VK_NULL_HANDLE)
        {
            NO_CHECK( qvkDestroyImage(vk.device, pImage->handle, NULL) );
            pImage->handle = VK_NULL_HANDLE;
        }

        strncpy (pImage->imgName, "*scratch", 10);
        pImage->uploadWidth = cols;
        pImage->uploadHeight = rows;


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
        imgDesc.tiling = VK_IMAGE_TILING_LINEAR;
            // VK_IMAGE_TILING_OPTIMAL;
        ////
        imgDesc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT ;
        imgDesc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imgDesc.queueFamilyIndexCount = 0;
        imgDesc.pQueueFamilyIndices = NULL;
        // However, images must initially be created in either 
        // VK_IMAGE_LAYOUT_UNDEFINED or 
        imgDesc.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

        VK_CHECK( qvkCreateImage(vk.device, &imgDesc, NULL, &pImage->handle) );

        ri.Printf(PRINT_ALL, " Create Image For Cinematic. \n");

        //vk_create2DImageHandle( VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, pImage);
        
        VkMemoryRequirements memory_reqs;
        NO_CHECK( qvkGetImageMemoryRequirements(vk.device, pImage->handle, &memory_reqs) );
        
        // vk_allocDeviceLocalMemory(memory_requirements.memoryTypeBits, 0, pChunk);

        VkMemoryAllocateInfo alloc_info;
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = NULL;
        alloc_info.allocationSize = memory_reqs.size;
        alloc_info.memoryTypeIndex = find_memory_type( 
                memory_reqs.memoryTypeBits, 
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Allocate 4MB mappable coherent memory for the cinematic images
        VK_CHECK( qvkAllocateMemory(vk.device, &alloc_info, NULL, &s_mappableMemory) );
    
        ri.Printf(PRINT_ALL, "Allocate Device local memory, Size: %ld KB, Type Index: %d. \n",
            (memory_reqs.size >> 10), alloc_info.memoryTypeIndex);

        VK_CHECK( qvkBindImageMemory(vk.device, pImage->handle, s_mappableMemory, 0) );


        // vk_createViewForImageHandle(prtImage->handle, VK_FORMAT_R8G8B8A8_UNORM, &prtImage->view);
        VkImageViewCreateInfo desc;
        desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        desc.pNext = NULL;
        desc.flags = 0;
        desc.image = pImage->handle;
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
 
        VK_CHECK( qvkCreateImageView(vk.device, &desc, NULL, &pImage->view) );
        
        // vk_createDescriptorSet(prtImage);
        //
        // Allocate a descriptor set from the pool. 
        // Note that we have to provide the descriptor set layout that 
        // This layout describes how the descriptor set is to be allocated.

        // vk_allocOneDescptrSet(&pImage->descriptor_set);

        VkDescriptorSetAllocateInfo descSetAllocInfo;
        descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descSetAllocInfo.pNext = NULL;
        descSetAllocInfo.descriptorPool = vk.descriptor_pool;
        descSetAllocInfo.descriptorSetCount = 1;
        descSetAllocInfo.pSetLayouts = &vk.set_layout;
        VK_CHECK( qvkAllocateDescriptorSets(vk.device, &descSetAllocInfo, &pImage->descriptor_set) );

        //ri.Printf(PRINT_ALL, " Allocate Descriptor Sets \n");
        VkWriteDescriptorSet descriptor_write;

        VkDescriptorImageInfo image_info;
        image_info.sampler = vk_find_sampler(VK_FALSE, VK_FALSE);
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

        vk_uploadImageDataReal(pImage->handle, data, cols * rows * 4);
    }
    else if (dirty)
    {
        // otherwise, just subimage upload it so that
        // drivers can tell we are going to be changing
        // it and don't try and do a texture compression       
        vk_uploadImageDataReal(pImage->handle, data, cols * rows * 4);
    }
}



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
	int	i, j;

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

    tr_cinematicShader->stages[0]->bundle[0].image[0] = &tr_scratchImage[client];
    
    
    RE_StretchPic(x, y, w, h,  0.5f / cols, 0.5f / rows,
            1.0f - 0.5f / cols, 1.0f - 0.5 / rows, tr_cinematicShader->index);
}
