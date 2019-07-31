#include "VKimpl.h"
#include "vk_cmd.h"
#include "vk_instance.h"
#include "tr_globals.h"
#include "tr_shader.h"
#include "tr_cvar.h"
#include "vk_buffers.h"
#include "vk_image_sampler.h"
#include "vk_image.h"
#include "R_ImageProcess.h"
#include "R_SortAlgorithm.h"
#include "vk_descriptor_sets.h"
#include "ref_import.h" 

#define IMAGE_CHUNK_SIZE        (64 * 1024 * 1024)


struct ImageChunk_s {
    VkDeviceMemory block;
    uint32_t Used;
    // uint32_t typeIndex;
};


struct deviceLocalMemory_t {
    // One large device device local memory allocation, assigned to multiple images
	struct ImageChunk_s Chunks[8];
	uint32_t Index; // number of chunks used
};


static struct deviceLocalMemory_t devMemImg;


uint32_t R_GetGpuMemConsumedByImage(void)
{
    return (devMemImg.Index * IMAGE_CHUNK_SIZE / (1024 * 1024));
}


////////////////////////////////////////

uint32_t find_memory_type(uint32_t memory_type_bits, VkMemoryPropertyFlags properties)
{
    uint32_t i;
    for (i = 0; i < vk.devMemProperties.memoryTypeCount; ++i)
    {
        if ( ((memory_type_bits & (1 << i)) != 0) && 
                (vk.devMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    ri.Error(ERR_FATAL, "Vulkan: failed to find matching memory type with requested properties");
    return -1;
}




#define FILE_HASH_SIZE	1024
static struct image_s *	hashTable[FILE_HASH_SIZE];

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
        struct ImageChunk_s * const pChunk)
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
        struct ImageChunk_s * const pChunk)
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


static void vk_createDescriptorSet(struct image_s * const pImage)
{
    // Allocate a descriptor set from the pool. 
    vk_allocOneDescptrSet(&pImage->descriptor_set);

    //ri.Printf(PRINT_ALL, " Allocate Descriptor Sets \n");

	// the image will be bound for reading by shaders.
    // this layout is typically used when an image is going to
    // be used as a texture.

    VkDescriptorImageInfo image_info = {
		.sampler = vk_find_sampler(pImage->mipmap, pImage->wrapClampMode == GL_REPEAT),
		.imageView = pImage->view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkWriteDescriptorSet descriptor_info = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        // dstSet is the destination descriptor set to update
		.dstSet = pImage->descriptor_set,
        // dstBinding is the descriptor binding within that set
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.pNext = NULL,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &image_info,
		.pBufferInfo = NULL,
		.pTexelBufferView = NULL
	};

    // The operations described by pDescriptorWrites are performed first, 
    // followed by the operations described by pDescriptorCopies. Within
    // each array, the operations are performed in the order they appear
    // in the array.
    //
    // Each element in the pDescriptorWrites array describes an operation
    // updating the descriptor set using descriptors for resources specified
    // in the structure.
    //
    // Each element in the pDescriptorCopies array is a VkCopyDescriptorSet
    // structure describing an operation copying descriptors between sets.
    NO_CHECK( qvkUpdateDescriptorSets(vk.device, 1, &descriptor_info, 0, NULL) );

    // The above steps essentially copy the VkDescriptorBufferInfo
    // to the descriptor, which is likely in the device memory.
}



void vk_createImageResourceImpl(struct image_s * const pImage)
{
    vk_create2DImageHandle( VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, pImage);
    vk_bindImageHandleWithDeviceMemory(pImage->handle, &devMemImg.Index, devMemImg.Chunks);
    vk_createViewForImageHandle(pImage->handle, VK_FORMAT_R8G8B8A8_UNORM, &pImage->view);
    vk_createDescriptorSet(pImage);
}


// how can i make sure the incomming image dosen't have a alpha channel ?
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

    // perform optional picmip operation
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
        // ri.Printf( PRINT_WARNING, "curMipMapLevel: %d, base_width: %d, base_height:
		//  %d, buffer_size: %d, name: %s\n",
        //    curMipMapLevel, scaled_width, scaled_height, buffer_size, name);
    }

    /*
    vk_create2DImageHandle( VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, pImage);
    vk_bindImageHandleWithDeviceMemory(pImage->handle, &devMemImg.Index, devMemImg.Chunks);
    vk_createViewForImageHandle(pImage->handle, VK_FORMAT_R8G8B8A8_UNORM, &pImage->view);
    vk_createDescriptorSet(pImage);
    */
    vk_createImageResourceImpl(pImage);

    VK_UploadImageToStagBuffer(pUploadBuffer, buffer_size);

    vk_stagBufToDevLocal(pImage->handle, regions, pImage->mipLevels);
    
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


static void vk_destroySingleImage( struct image_s * const pImg )
{
   	// ri.Printf(PRINT_ALL, " Destroy Image: %s \n", pImg->imgName); 
/*
	if(pImg->descriptor_set != VK_NULL_HANDLE)
    {   
        //To free allocated descriptor sets
        NO_CHECK( qvkFreeDescriptorSets(vk.device, vk.descriptor_pool, 1, &pImg->descriptor_set) );
        pImg->descriptor_set = VK_NULL_HANDLE;
    }
*/
    
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

	if (name == NULL)
    {
        ri.Printf( PRINT_WARNING, "Find Image File: NULL\n");
		return NULL;
	}

	int hash = generateHashValue(name);

	// see if the image is already loaded
   	image_t* image;
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



static void R_CreateDefaultImage( void )
{
	#define	DEFAULT_SIZE 32

	unsigned char data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	memset( data, 32, sizeof( data ) );

	for (uint32_t x = 0; x < DEFAULT_SIZE; ++x )
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
    
	tr.defaultImage = R_CreateImage("*default", (unsigned char *)data, 
            DEFAULT_SIZE, DEFAULT_SIZE, qtrue, qfalse, GL_REPEAT);
    #undef DEFAULT_SIZE
}



static void R_CreateWhiteImage(void)
{
    #define	DEFAULT_SIZE 32
	// we use a solid white image instead of disabling texturing
	unsigned char data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage("*white", (unsigned char *)data, 
            DEFAULT_SIZE, DEFAULT_SIZE, qfalse, qfalse, GL_REPEAT);
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
	for (x=0 ; x<FOG_S; ++x)
    {
		for (y=0 ; y<FOG_T; ++y)
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

    #undef FOG_S
    #undef FOG_T
}



void R_InitImages( void )
{
    memset(hashTable, 0, sizeof(hashTable));

	// setup the overbright lighting

	tr.identityLight = 1.0f;
	tr.identityLightByte = 255 * tr.identityLight;


    // build brightness translation tables
    R_SetColorMappings(1.6f, r_gamma->value);

    // create default texture and white texture
    R_CreateDefaultImage();

    R_CreateWhiteImage();

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
    
    for (i = 0; i < devMemImg.Index; ++i)
    {
        NO_CHECK( qvkFreeMemory(vk.device, devMemImg.Chunks[i].block, NULL) );
        devMemImg.Chunks[i].Used = 0;
    }

    memset(&devMemImg, 0, sizeof(devMemImg));

    memset( tr.images, 0, sizeof( tr.images ) );
    
    tr.numImages = 0;

    memset(hashTable, 0, sizeof(hashTable));
}
