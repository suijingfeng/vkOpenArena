#include "vk_instance.h"
#include "vk_buffers.h"
#include "vk_cmd.h"
#include "vk_screenshot.h"

#include "R_ImageProcess.h"
#include "ref_import.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
/* 
============================================================================== 
 
						SCREEN SHOTS 

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes


Images created with tiling equal to VK_IMAGE_TILING_LINEAR have further restrictions on their
limits and capabilities compared to images created with tiling equal to VK_IMAGE_TILING_OPTIMAL.
Creation of images with tiling VK_IMAGE_TILING_LINEAR may not be supported unless other parameters
meetall of the constraints:
* imageType is VK_IMAGE_TYPE_2D
* format is not a depth/stencil format
* mipLevels is 1
* arrayLayers is 1
* samples is VK_SAMPLE_COUNT_1_BIT
* usage only includes VK_IMAGE_USAGE_TRANSFER_SRC_BIT and/or VK_IMAGE_USAGE_TRANSFER_DST_BIT
Implementations may support additional limits and capabilities beyond those listed above.

============================================================================== 
*/


extern void R_GetWorldBaseName(char* checkname);
extern size_t RE_SaveJPGToBuffer(byte *buffer, size_t bufSize, int quality,
    int image_width, int image_height, byte *image_buffer, int padding);


static uint32_t s_lastNumber = 0;

static uint32_t getLastnumber(void)
{
    return s_lastNumber++;
}

// channal = 4 components default, RGBA
struct vkScreenShotManager_s {
    uint32_t width;
    uint32_t height;
    void * pData;
    VkBuffer hBuffer;
    VkDeviceMemory hMemory;
    VkBool32 initialized;
};

static struct vkScreenShotManager_s scnShotMgr;

uint32_t R_GetScreenShotBufferSizeKB(void)
{
    return 4 * scnShotMgr.width * scnShotMgr.height / 1024;
}

void vk_clearScreenShotManager(void)
{
    
    if(scnShotMgr.initialized)
    {
        NO_CHECK( qvkUnmapMemory(vk.device, scnShotMgr.hMemory) );
        vk_destroyBufferResource(scnShotMgr.hBuffer, scnShotMgr.hMemory);
        memset(&scnShotMgr, 0, sizeof(scnShotMgr)); 
    }

    ri.Printf(PRINT_ALL, " Destroy screenshot buffer resources. \n");
}


static uint32_t vk_createScreenShotManager(const uint32_t w, const uint32_t h)
{
    const uint32_t szBytes = w * h * 4;
    if((w == 0) || (h == 0) || (w > vk_getWinWidth()) || (h > vk_getWinHeight()))
    {
        // trivial error check ...
        ri.Printf(PRINT_WARNING, "error screen shot parameter size. \n");
        return 0;
    }
    else if((w == scnShotMgr.width) && (h == scnShotMgr.height) && scnShotMgr.initialized)
    {
        // already created, and size match, just return 
        return szBytes;
    }
    else
    {
        // recreate ...
        vk_clearScreenShotManager();
    }
    
    scnShotMgr.width = w;
    scnShotMgr.height = h;
    // scnShotMgr->pData = (unsigned char * ) malloc( w * h * 4 );

    // Memory objects created with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    // are considered mappable. Memory objects must be mappable in order
    // to be successfully mapped on the host.  
    //
    // Use HOST_VISIBLE with HOST_COHERENT and HOST_CACHED. This is the
    // only Memory Type which supports cached reads by the CPU. Great
    // for cases like recording screen-captures, feeding back
    // Hierarchical Z-Buffer occlusion tests, etc. --- from AMD webset.

    vk_createBufferResource( szBytes,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | 
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT, 
             &scnShotMgr.hBuffer, &scnShotMgr.hMemory );

    // If the memory mapping was made using a memory object allocated from a memory type
    // that exposes the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property then the mapping 
    // between the host and device is always coherent. That is, the host and device 
    // communicate in order to ensure that their respective caches are synchronized 
    // and that any reads or writes to shared memory are seen by the other peer.

    VK_CHECK( qvkMapMemory(vk.device, scnShotMgr.hMemory, 0, VK_WHOLE_SIZE, 0, &scnShotMgr.pData) );

    scnShotMgr.initialized = VK_TRUE;

    ri.Printf(PRINT_ALL, " Create a buffer for screenshot, Size: %d bytes. \n", szBytes);
    return szBytes;
}



static void imgFlipY(unsigned char * pBuf, const uint32_t w, const uint32_t h)
{
    const uint32_t bytesInOneRow = w * 4;
    const uint32_t nLines = h / 2;

    unsigned char* const pTmp = (unsigned char*) malloc( bytesInOneRow );
    unsigned char* pSrc = pBuf;
    unsigned char* pDst = pBuf + w * (h - 1) * 4;

    for (uint32_t j = 0; j < nLines; ++j)
    {
        memcpy(pTmp, pSrc, bytesInOneRow );
        memcpy(pSrc, pDst, bytesInOneRow );
        memcpy(pDst, pTmp, bytesInOneRow );

        pSrc += bytesInOneRow;
        pDst -= bytesInOneRow;
	}

    free(pTmp);
}


// Just reading the pixels for the GPU MEM, don't care about swizzling
static void vk_read_pixels(unsigned char* const pBuf, uint32_t W, uint32_t H)
{
	// NO_CHECK( qvkDeviceWaitIdle(vk.device) );

	// Create image in host visible memory to serve as a destination
    // for framebuffer pixels.
    vk_createScreenShotManager(W, H);

    // GPU-to-CPU Data Flow
    // Access Types doc
    // 
    // Memory in Vulkan can be accessed from within shader invocations 
    // and via some fixed-function stages of the pipeline. The access 
    // type is a function of the descriptor type used, or how a fixed-
    // function stage accesses memory. Each access type corresponds to
    // a bit flag in VkAccessFlagBits.
    //
    // Some synchronization commands take sets of access types as 
    // parameters to define the access scopes of a memory dependency. 
    // If a synchronization command includes a source access mask, 
    // its first access scope only includes accesses via the access 
    // types specified in that mask. Similarly, if a synchronization 
    // command includes a destination access mask, its second access 
    // scope only includes accesses via the access types specified 
    // in that mask.
    // 
    // Certain access types are only performed by a subset of pipeline stages.
    // Any synchronization command that takes both stage masks and access masks
    // uses both to define the access scopes - only the specified access types 
    // performed by the specified stages are included in the access scope. 
    //
    // An application must not specify an access flag in a synchronization command
    // if it does not include a pipeline stage in the corresponding stage mask that
    // is able to perform accesses of that type. 
    //
    // VK_ACCESS_TRANSFER_READ_BIT              VK_PIPELINE_STAGE_TRANSFER_BIT
    // VK_ACCESS_TRANSFER_WRITE_BIT             VK_PIPELINE_STAGE_TRANSFER_BIT
    // VK_ACCESS_COLOR_ATTACHMENT_READ_BIT      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    // VK_ACCESS_TRANSFER_READ_BIT specifies read access to an image
    // or buffer in a copy operation.
    //////////////////////////////////////////////////////////
    vk_beginRecordCmds(vk.tmpRecordBuffer);

    VkBufferImageCopy image_copy;
    image_copy.bufferOffset = 0;
    image_copy.bufferRowLength = W;
    image_copy.bufferImageHeight = H;
    image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.imageSubresource.layerCount = 1;
    image_copy.imageSubresource.mipLevel = 0;
    image_copy.imageSubresource.baseArrayLayer = 0;
    image_copy.imageOffset.x = 0;
    image_copy.imageOffset.y = 0;
    image_copy.imageOffset.z = 0;
    image_copy.imageExtent.width = W;
    image_copy.imageExtent.height = H;
    image_copy.imageExtent.depth = 1;

    // Image memory barriers only apply to memory accesses 
    // involving a specific image subresource range. That is,
    // a memory dependency formed from an image memory barrier
    // is scoped to access via the specified image subresource range. 
    // 
    // Image memory barriers can also be used to define image
    // layout transitions or a queue family ownership transfer
    // for the specified image  subresource range.

    // Memory barriers are used to change image layouts here ...
    VkImageMemoryBarrier image_barrier;
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.pNext = NULL;
    // srcAccessMask is a bitmask of VkAccessFlagBits specifying a source access mask.
    image_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    // VK_ACCESS_TRANSFER_READ_BIT ?
    // dstAccessMask is a bitmask of VkAccessFlagBits specifying a destination access mask.
    image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL must only be used as a
    // source image of a transfer command (see the definition of 
    // VK_PIPELINE_STAGE_TRANSFER_BIT). This layout is valid only 
    // for image subresources of images created with the 
    // VK_IMAGE_USAGE_TRANSFER_SRC_BIT usage bit enabled.
    image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = vk.swapchain_images_array[vk.idx_swapchain_image];
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;

    NO_CHECK( qvkCmdPipelineBarrier(vk.tmpRecordBuffer, 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL, 0, NULL, 1, &image_barrier) );
    
    NO_CHECK( qvkCmdCopyImageToBuffer(vk.tmpRecordBuffer, 
        vk.swapchain_images_array[vk.idx_swapchain_image], 
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, scnShotMgr.hBuffer, 1, &image_copy) );

    vk_commitRecordedCmds(vk.tmpRecordBuffer);

    // VK_CHECK( qvkQueueWaitIdle(vk.queue) );
   	// NO_CHECK( qvkDeviceWaitIdle(vk.device) ); 

    memcpy(pBuf, scnShotMgr.pData, W * H * 4);
}



extern size_t RE_SaveJPGToBuffer(byte *buffer, size_t bufSize, int quality,
    int image_width, int image_height, byte *image_buffer, int padding);


static void RB_TakeScreenshotJPEG(unsigned char * const pImg, uint32_t width, uint32_t height, char * const fileName,
        const char * const path )
{
    const uint32_t cnPixels = width * height; 

    imgFlipY(pImg, width, height);

    // Remove alpha channel and rbg <-> bgr
    {
        unsigned char* pSrc = pImg;
        unsigned char* pDst = pImg;

        uint32_t i;
        for (i = 0; i < cnPixels; ++i)
        {
            pSrc[0] = pDst[2];
            pSrc[1] = pDst[1];
            pSrc[2] = pDst[0];
            pSrc += 3;
            pDst += 4;
        }
    }

    uint32_t bufSize = cnPixels * 3;

    unsigned char* const out = (unsigned char *) malloc(bufSize);

    bufSize = RE_SaveJPGToBuffer(out, bufSize, 80, width, height, pImg, 0);

    char pathname[128] = {0}; 

    snprintf( pathname, 128, "%s/%s", path, fileName );
   
    if( ri.FS_FileExists( pathname ) )
        ri.Printf(PRINT_WARNING, " %s already exist, overwrite \n", pathname);

    ri.FS_WriteFile(pathname, out, bufSize);
    
    ri.Printf(PRINT_ALL, "write %s success! \n", pathname);

    free(out);
}



///////////////
//
struct ImageWriteBuffer_s
{
	uint8_t * const pData;
	uint32_t szBytesUsed;
    const uint32_t szCapacity;
};

static void fnImageWriteToBufferCallback(void *context, void *data, int size)
{
    struct ImageWriteBuffer_s * const pCtx = (struct ImageWriteBuffer_s *) context;

	if (pCtx->szBytesUsed + size > pCtx->szCapacity )
	{
		// pBuffer->data->resize(pBuffer->bytesWritten + size);
        ri.Error(ERR_FATAL, "fnImageWriteToBufferCallback: buffer overflow. ");
	}

	memcpy(pCtx->pData + pCtx->szBytesUsed, data, size);

    pCtx->szBytesUsed += size;
}


// Yet another impl using stbi ...
static void RB_TakeScreenshotJPG(unsigned char* const pImg, uint32_t width, uint32_t height, char * const fileName )
{
    const uint32_t cnPixels = width * height; 


    //  bgr to rgb, split merely because the first pixel ... 
    unsigned char* pSrc = pImg;

    for (uint32_t i = 0; i < cnPixels; ++i, pSrc += 4)
    {
        unsigned char b0 = pSrc[0];
        unsigned char b2 = pSrc[2];
        pSrc[0] = b2;
        pSrc[2] = b0;
    }

    // Remove alpha channel ...
    {
        const unsigned char* pSrc = pImg;
        unsigned char* pDst = pImg;

        for (uint32_t i = 0; i < cnPixels; ++i, pSrc += 4, pDst += 3)
        {
            pDst[0] = pSrc[0];
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[2];
        }
    }
    int error = 0;

#ifndef STBI_WRITE_NO_STDIO
	error = stbi_write_jpg (fileName, width, height, 3, pImg, 90);    
#else

    uint32_t bufSize = cnPixels * 3;

    struct ImageWriteBuffer_s ctx = {
        .pData = (unsigned char *) malloc( bufSize ),
        .szCapacity = bufSize,
        .szBytesUsed = 0 };
    
    // bufSize = RE_SaveJPGToBuffer(out, bufSize, 80, width, height, pImg, 0);
    //stbi_write_jpg_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data, int quality);

    int error = stbi_write_jpg_to_func( fnImageWriteToBufferCallback, &ctx, width, height, 3, pImg, 90);

    ri.FS_WriteFile(fileName, ctx.pData, ctx.szBytesUsed);

    free(ctx.pData);
#endif

    if(error == 0)
    {
        ri.Printf(PRINT_WARNING, "failed writing %s to the disk. \n", fileName);
    }
    else
    {
        ri.Printf(PRINT_ALL, "write to %s success! \n", fileName);
    }  
}



static void RB_TakeScreenshotTGA(unsigned char * const pImg, uint32_t width, uint32_t height, 
        const char * const fileName, const char * const path )
{

    const uint32_t cnPixels = width * height;

    const uint32_t imgSize = 18 + cnPixels * 3;

    unsigned char* const pBuffer = (unsigned char*) malloc ( imgSize );
    
    memset (pBuffer, 0, 18);
    pBuffer[2] = 2;		// uncompressed type
    pBuffer[12] = width & 255;
    pBuffer[13] = width >> 8;
    pBuffer[14] = height & 255;
    pBuffer[15] = height >> 8;
    pBuffer[16] = 24;	// pixel size

    // why the readed image got fliped about Y ???
    imgFlipY(pImg, width, height);


    // Remove alpha channel ...
    {
        const unsigned char* pSrc = pImg;
        unsigned char* pDst = pBuffer + 18;

        for (uint32_t i = 0; i < cnPixels; ++i, pSrc += 4, pDst += 3)
        {
            pDst[0] = pSrc[0];
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[2];
        }
    }

    char pathname[128] = {0}; 

    snprintf( pathname, 128, "%s/%s", path, fileName );

    ri.FS_WriteFile( pathname, pBuffer, imgSize);
   

    ri.Printf( PRINT_ALL, "%s to %s success! \n",
            (ri.FS_FileExists( pathname ) ? "write" : "overwrite"), pathname );

    free( pBuffer );
}


static void R_NameTheImage(char * const pName, uint32_t max_name_len, uint32_t w, uint32_t h, const char * const ext)
{
    // scan for a free number
    uint32_t lastNumber = getLastnumber();
    uint32_t b = lastNumber / 100;
    uint32_t remain = lastNumber - b*100;
    uint32_t c = remain / 10;
    uint32_t d = remain - c*10;

    snprintf( pName, max_name_len, "shot%ix%i_%i%i%i.%s", w, h, b, c, d, ext );
}


void R_ScreenShotPNG_f( void )
{
    const uint32_t width = vk_getWinWidth();
    const uint32_t height = vk_getWinHeight();

    const uint32_t cnPixels = width * height;
    unsigned char* const pImg = (unsigned char*) malloc ( cnPixels * 4 );
    
    vk_read_pixels(pImg, width, height);

    uint32_t i;

    // rbg <-> bgr 
    {
        unsigned char* pSrc = pImg;
        for (i = 0; i < cnPixels; ++i)
        {
            unsigned char tmp = pSrc[0];
            pSrc[0] = pSrc[2];
            pSrc[2] = tmp;
            pSrc += 4;
        }
    }
    
    // Remove alpha channel ...
    {
        const unsigned char* pSrc = pImg;
        unsigned char* pDst = pImg;

        for (uint32_t i = 0; i < cnPixels; ++i, pSrc += 4, pDst += 3)
        {
            pDst[0] = pSrc[0];
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[2];
        }
    }

	char checkname[MAX_OSPATH];

    R_NameTheImage(checkname, sizeof(checkname), width, height, "png");

    // For PNG, "stride_in_bytes" is the distance in bytes from the first byte of
    // a row of pixels to the first byte of the next row of pixels.
    stbi_write_png(checkname, width, height, 3, pImg, width*3);

    ri.Printf(PRINT_ALL, "write to %s success! \n", checkname);

    free( pImg );
}


void R_ScreenShotBMP_f( void )
{

    uint32_t width = vk_getWinWidth();
    uint32_t height = vk_getWinHeight();

    const uint32_t cnPixels = width * height;
    unsigned char* const pImg = (unsigned char*) malloc ( cnPixels * 4 );
    vk_read_pixels(pImg, width, height);


    uint32_t i;

    // rbg <-> bgr 
    {
        unsigned char* pSrc = pImg;
        for (i = 0; i < cnPixels; ++i)
        {
            unsigned char tmp = pSrc[0];
            pSrc[0] = pSrc[2];
            pSrc[2] = tmp;
            pSrc += 4;
        }
    }
    
    // Remove alpha channel ...
    {
        const unsigned char* pSrc = pImg;
        unsigned char* pDst = pImg;

        for (uint32_t i = 0; i < cnPixels; ++i, pSrc += 4, pDst += 3)
        {
            pDst[0] = pSrc[0];
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[2];
        }
    }

    // For PNG, "stride_in_bytes" is the distance in bytes from the first byte of
    // a row of pixels to the first byte of the next row of pixels.
	char checkname[MAX_OSPATH];
    // scan for a free number
    // we dont care about overwrite, does it matter ? 
    R_NameTheImage(checkname, sizeof(checkname), width, height, "bmp");
    
    if( stbi_write_bmp(checkname, width, height, 3, pImg) )
        ri.Printf(PRINT_ALL, "write to %s success! \n", checkname);

    free( pImg );
}



/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for the 
menu system, sampled down from full screen distorted images
====================
*/
static void R_LevelShot( int W, int H )
{
	char checkname[MAX_OSPATH];
	unsigned char* buffer;
	unsigned char* source;
	unsigned char* src;
	unsigned char* dst;
	int			x, y;
	int			r, g, b;
	float		xScale, yScale;
	int			xx, yy;
    int i = 0;

    R_GetWorldBaseName(checkname);

	source = (unsigned char*) ri.Hunk_AllocateTempMemory( W * H * 3 );

	buffer = (unsigned char*) ri.Hunk_AllocateTempMemory( 128 * 128*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = 128;
	buffer[14] = 128;
	buffer[16] = 24;	// pixel size

    {
        unsigned char* buffer2 = (unsigned char*) malloc (W * H * 4);
        vk_read_pixels(buffer2, W, H);

        unsigned char* buffer_ptr = source;
        unsigned char* buffer2_ptr = buffer2;
        for (i = 0; i < W * H; i++)
        {
            buffer_ptr[0] = buffer2_ptr[0];
            buffer_ptr[1] = buffer2_ptr[1];
            buffer_ptr[2] = buffer2_ptr[2];
            buffer_ptr += 3;
            buffer2_ptr += 4;
        }
        free(buffer2);
    }

	// resample from source
	xScale = W / 512.0f;
	yScale = H / 384.0f;
	for ( y = 0 ; y < 128 ; y++ )
    {
		for ( x = 0 ; x < 128 ; x++ )
        {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ ) {
				for ( xx = 0 ; xx < 4 ; xx++ ) {
					src = source + 3 * ( W * (int)( (y*3+yy)*yScale ) + (int)( (x*4+xx)*xScale ) );
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst = buffer + 18 + 3 * ( y * 128 + x );
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	ri.FS_WriteFile( checkname, buffer, 128 * 128*3 + 18 );

	ri.Hunk_FreeTempMemory( buffer );
	ri.Hunk_FreeTempMemory( source );

	ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
}


void R_LevelShot_f (void)
{

    uint32_t width = vk_getWinWidth();
    uint32_t height = vk_getWinHeight();

	R_LevelShot(width, height);
}




/* 
================== 
R_ScreenShot_f

screenshot
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
================== 
*/  
void R_ScreenShotTGA_f( void )
{
	char checkname[MAX_OSPATH];

    uint32_t W = vk_getWinWidth();
    uint32_t H = vk_getWinHeight();

    unsigned char* const pImg = (unsigned char*) malloc ( W * H * 4);

    vk_read_pixels(pImg, W, H);

	if ( ri.Cmd_Argc() == 2 )
    {
		// explicit filename
		snprintf( checkname, sizeof(checkname), "%s.tga", ri.Cmd_Argv( 1 ) );
	}
    else
    {
        R_NameTheImage(checkname, sizeof(checkname), W, H, "tga");
	}

	RB_TakeScreenshotTGA(pImg, W, H, checkname, "screenshots");


    free(pImg);
} 




void R_ScreenShotJPEG_f(void)
{
	char checkname[MAX_OSPATH];

    const uint32_t W = vk_getWinWidth();
    const uint32_t H = vk_getWinHeight();

    unsigned char* const pImg = (unsigned char*) malloc ( W * H * 4);

    vk_read_pixels(pImg, W, H);

    // we dont care about overwrite, does it matter ? 
    R_NameTheImage(checkname, sizeof(checkname), W, H, "jpg");
    
    RB_TakeScreenshotJPEG(pImg, W, H, checkname, "screenshots" );

    free(pImg);
}


void R_ScreenShotJPG_f(void)
{
    char imgname[MAX_OSPATH];

    const uint32_t W = vk_getWinWidth();
    const uint32_t H = vk_getWinHeight();

    unsigned char* const pImg = (unsigned char*) malloc ( W * H * 4);

    vk_read_pixels(pImg, W, H);

    // we dont care about overwrite, does it matter ?
    // it just write to where the execute locate 
    R_NameTheImage(imgname, sizeof(imgname), W, H, "jpg");
    RB_TakeScreenshotJPG(pImg, W, H, imgname );

    free(pImg);
}

void R_ScreenShot_f(void)
{
    R_ScreenShotJPG_f();
}

void RE_TakeVideoFrame( const int Width, const int Height, 
        unsigned char *captureBuffer, unsigned char *encodeBuffer, qboolean motionJpeg )
{		
    unsigned char* const pImg = (unsigned char*) malloc ( Width * Height * 4);
    
    vk_read_pixels(pImg, Width, Height);

    imgFlipY(pImg, Width, Height);


	// AVI line padding
	const int avipadwidth = PAD( (Width * 3), 4);
	
    const unsigned char* pSrc = pImg;
    unsigned char* pDstRow = captureBuffer;

    for(uint32_t j = 0; j < Height; ++j, pDstRow += avipadwidth )
    {   
        unsigned char* pDst = pDstRow;
        for (uint32_t i = 0; i < Width; ++i)
        {
            *(pDst + 0) = *( pSrc + 2 );
            *(pDst + 1) = *( pSrc + 1 );
            *(pDst + 2) = *( pSrc + 0 );
            pDst += 3;
            pSrc += 4; 
        }
    }

    int memcount = RE_SaveJPGToBuffer(encodeBuffer, Width * Height * 3,
            75, Width, Height, captureBuffer, (avipadwidth - Width * 3) );

    ri.CL_WriteAVIVideoFrame(encodeBuffer, memcount);


////////////////////
// failed replace 
/*
    struct ImageWriteBuffer_s ctx = {
        .pData = (unsigned char *) encodeBuffer,
        .szCapacity = avipadwidth * Height * 3,
        .szBytesUsed = 0 };
    
    // bufSize = RE_SaveJPGToBuffer(out, bufSize, 80, width, height, pImg, 0);
    //stbi_write_jpg_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data, int quality);

    stbi_write_jpg_to_func( fnImageWriteToBufferCallback, &ctx, avipadwidth, Height, 3, pImg, 75);

    ri.CL_WriteAVIVideoFrame(ctx.pData, ctx.szBytesUsed);
*/
////////////////////////

    free(pImg);
}
