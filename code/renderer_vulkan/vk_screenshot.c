#include "tr_globals.h"
#include "vk_instance.h"
#include "vk_buffers.h"
#include "vk_cmd.h"
#include "vk_screenshot.h"

#include "R_ImageProcess.h"
#include "R_ImageJPG.h"
#include "ref_import.h"


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
extern void RE_SaveJPG(char * filename, int quality, int image_width, int image_height,
        unsigned char *image_buffer, int padding);


static void imgFlipY(unsigned char * pBuf, const uint32_t w, const uint32_t h)
{
    const uint32_t a_row = w * 4;
    const uint32_t nLines = h / 2;

    unsigned char* pTmp = (unsigned char*) malloc( a_row );
    unsigned char *pSrc = pBuf;
    unsigned char *pDst = pBuf + w * (h - 1) * 4;

    uint32_t j = 0;
    for (j = 0; j < nLines; j++)
    {
        memcpy(pTmp, pSrc, a_row );
        memcpy(pSrc, pDst, a_row );
        memcpy(pDst, pTmp, a_row );

        pSrc += a_row;
        pDst -= a_row;
	}

    free(pTmp);
}


// Just reading the pixels for the GPU MEM, don't care about swizzling
static void vk_read_pixels(unsigned char* const pBuf, uint32_t W, uint32_t H)
{
	// NO_CHECK( qvkDeviceWaitIdle(vk.device) );
    VkBuffer screenShotBuffer;
    VkDeviceMemory screenShotMemory;

	// Create image in host visible memory to serve as a destination
    // for framebuffer pixels.
  
    const uint32_t sizeFB = W * H * 4;

    // GPU-to-CPU Data Flow

    // Memory objects created with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    // are considered mappable. Memory objects must be mappable in order
    // to be successfully mapped on the host.  
    //
    // Use HOST_VISIBLE with HOST_COHERENT and HOST_CACHED. This is the
    // only Memory Type which supports cached reads by the CPU. Great
    // for cases like recording screen-captures, feeding back
    // Hierarchical Z-Buffer occlusion tests, etc. ---from AMD webset.
    
    vk_createBufferResource( sizeFB,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | 
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT, 
             &screenShotBuffer, &screenShotMemory );

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

    // Image memory barriers only apply to memory accesses involving a specific image subresource
    // range. That is, a memory dependency formed from an image memory barrier is scoped to access
    // via the specified image subresource range. Image memory barriers can also be used to define
    // image layout transitions or a queue family ownership transfer for the specified image 
    // subresource range.

    VkImageMemoryBarrier image_barrier;
    
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.pNext = NULL;
    image_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
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

    // read pixel with command buffer

    NO_CHECK( qvkCmdPipelineBarrier(vk.tmpRecordBuffer, 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL, 0, NULL, 1, &image_barrier) );
    
    NO_CHECK( qvkCmdCopyImageToBuffer(vk.tmpRecordBuffer, 
        vk.swapchain_images_array[vk.idx_swapchain_image], 
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, screenShotBuffer, 1, &image_copy) );

    vk_commitRecordedCmds(vk.tmpRecordBuffer);

//    VK_CHECK( qvkQueueWaitIdle(vk.queue) );
    

    // If the memory mapping was made using a memory object allocated from
    // a memory type that exposes the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    // property then the mapping between the host and device is always coherent.
    // That is, the host and device communicate in order to ensure that 
    // their respective caches are synchronized and that any reads or writes
    // to shared memory are seen by the other peer.
    //
    // If memory is NOT allocated from a memory type exposing the 
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT porperty, then you must execute 
    // a pipeline barrier to move the resource into a host-readable state. 
    // To do this, make sure that the destination access mask includes
    // VK_ACCESS_HOST_READ_BIT.
    //
    // Memory barriers are used to explicitly control access to buffer and
    // image subresource ranges. Memory barriers are used to transfer ownership
    // between queue families, change image layouts, and define availability 
    // and visibility operations. They explicitly define the access types and
    // buffer and image subresource ranges that are included in the access 
    // scopes of a memory dependency that is created by a synchronization 
    // command that includes them. 
    unsigned char* data;    
    // we use VK_MEMORY_PROPERTY_HOST_COHERENT_BIT

    VK_CHECK( qvkMapMemory(vk.device, screenShotMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data) );
    // To retrieve a host virtual address pointer to a region of a mappable memory object
    memcpy(pBuf, data, sizeFB);
    
    NO_CHECK( qvkUnmapMemory(vk.device, screenShotMemory) );

    vk_destroyBufferResource(screenShotBuffer, screenShotMemory);
    ri.Printf(PRINT_ALL, " Destroy screenshot buffer resources. \n");
}



void RB_TakeScreenshot( uint32_t width, uint32_t height, char * const fileName, VkBool32 isJpeg)
{
    ri.Printf(PRINT_ALL, "read %dx%d pixels from GPU\n", width, height);
    const uint32_t cnPixels = width * height; 

    if(isJpeg)
    {
        unsigned char* const pImg = (unsigned char*) malloc ( cnPixels * 4);

        vk_read_pixels(pImg, width, height);
       
        // but why this is need ? why the readed image got fliped about Y ???
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

        RE_SaveJPG(fileName, 90, width, height, pImg, 0);

        free( pImg );

        //bufSize = RE_SaveJPGToBuffer(out, bufSize, 90, width, height, pImg, padding);
        //ri.FS_WriteFile(filename, out, bufSize);
    }
    else
    {

        //const uint32_t cnPixels = width * height;
        const uint32_t imgSize = 18 + cnPixels * 3;

        unsigned char* const pBuffer = (unsigned char*) malloc ( imgSize + cnPixels * 4 );
        unsigned char* const buffer_ptr = pBuffer + 18;
        unsigned char* const pImg = pBuffer + imgSize;
        
        vk_read_pixels(pImg, width, height);

        // why the readed image got fliped about Y ???
        imgFlipY(pImg, width, height);

        memset (pBuffer, 0, 18);
        pBuffer[2] = 2;		// uncompressed type
        pBuffer[12] = width & 255;
        pBuffer[13] = width >> 8;
        pBuffer[14] = height & 255;
        pBuffer[15] = height >> 8;
        pBuffer[16] = 24;	// pixel size


        uint32_t i;
        if (0)
        {
            for (i = 0; i < cnPixels; ++i)
            {
                buffer_ptr[i*3]   = *(pImg + i*4 + 2);
                buffer_ptr[i*3+1] = *(pImg + i*4 + 1);
                buffer_ptr[i*3+2] = *(pImg + i*4 );
            }
        }
        else
        {
            for (i = 0; i < cnPixels; ++i)
            {
                buffer_ptr[i*3]   = *(pImg + i*4 );
                buffer_ptr[i*3+1] = *(pImg + i*4 + 1);
                buffer_ptr[i*3+2] = *(pImg + i*4 + 2);
            }
        }
        
        ri.FS_WriteFile( fileName, pBuffer, imgSize);
        
        free( pBuffer );
    }
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
	for ( y = 0 ; y < 128 ; y++ ) {
		for ( x = 0 ; x < 128 ; x++ ) {
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

/* 
================== 
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
================== 
*/  
void R_ScreenShot_f (void)
{
	char	checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

    int W = vk.renderArea.extent.width;
    int H = vk.renderArea.extent.height;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) )
    {
		R_LevelShot(W, H);
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) )
    {
		silent = qtrue;
	}
    else
    {
		silent = qfalse;
	}


	if ( ri.Cmd_Argc() == 2 && !silent )
    {
		// explicit filename
		snprintf( checkname, sizeof(checkname), "screenshots/%s.tga", ri.Cmd_Argv( 1 ) );
	}
    else
    {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan again, 
        // because recording demo avis can involve thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ )
        {
			//R_ScreenshotFilename( lastNumber, checkname );
            
            int	a,b,c,d;

            a = lastNumber / 1000;
            b = lastNumber % 1000 / 100;
            c = lastNumber % 100  / 10;
            d = lastNumber % 10;

            snprintf( checkname, sizeof(checkname), "screenshots/shot%i%i%i%i.tga", a, b, c, d );

            if (!ri.FS_FileExists( checkname ))
            {
                break; // file doesn't exist
            }
		}

		if ( lastNumber >= 9999 )
        {
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n"); 
			return;
 		}

		lastNumber++;
	}

	RB_TakeScreenshot( W, H, checkname, qfalse );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
} 


void R_ScreenShotJPEG_f(void)
{
	char		checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

    int W = vk.renderArea.extent.width;
    int H = vk.renderArea.extent.height;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot(W, H);
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		snprintf( checkname, sizeof(checkname), "screenshots/%s.jpg", ri.Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ )
        {
            int	a,b,c,d;

            a = lastNumber / 1000;
            lastNumber -= a*1000;
            b = lastNumber / 100;
            lastNumber -= b*100;
            c = lastNumber / 10;
            lastNumber -= c*10;
            d = lastNumber;

            snprintf( checkname, sizeof(checkname), "screenshots/shot%i%i%i%i.jpg"
                    , a, b, c, d );

            if (!ri.FS_FileExists( checkname ))
            {
                break; // file doesn't exist
            }
		}

		lastNumber++;
	}

	RB_TakeScreenshot( W, H, checkname, qtrue );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
}


void RE_TakeVideoFrame( const int Width, const int Height, 
        unsigned char *captureBuffer, unsigned char *encodeBuffer, qboolean motionJpeg )
{		
    unsigned char* const pImg = (unsigned char*) malloc ( Width * Height * 4);
    
    vk_read_pixels(pImg, Width, Height);

    imgFlipY(pImg, Width, Height);



	// AVI line padding
	int avipadwidth = PAD( (Width * 3), 4);
	
	// size_t memcount = avipadwidth * Height;

    unsigned char* pSrc = captureBuffer;
    unsigned char* pDst = pImg;
    // const uint32_t cnPixels = Width * Height;
    uint32_t j;
    for(j = 0; j < Height; ++j)
    {
        uint32_t i;
        //unsigned char* pSrc = captureBuffer + avipadwidth * j;
        //unsigned char* pDst = pImg + Width * j * 4; 
        for (i = 0; i < Width; ++i)
        {
            *(pSrc + i*3 + 0) = *( pDst + i*4 + 2 );
            *(pSrc + i*3 + 1) = *( pDst + i*4 + 1 );
            *(pSrc + i*3 + 2) = *( pDst + i*4 + 0 );
        }
        pSrc += avipadwidth;
        pDst += Width * 4;
    }

    int memcount = RE_SaveJPGToBuffer(encodeBuffer, Width * Height * 3,
            75,	Width, Height, captureBuffer, (avipadwidth - Width * 3) );

    ri.CL_WriteAVIVideoFrame(encodeBuffer, memcount);

    free(pImg);
}
