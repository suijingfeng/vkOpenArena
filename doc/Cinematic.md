
### history code for create image for cinamatic

I think we don't hurry to create image in advance,
we do it when needed. it's non performance part of
the engine.

Additional learning code reposite or material

* https://github.com/brugal/wolfcamql

* https://github.com/mightycow/uberdemotools

```
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
    unsigned char * const pUploadBuffer = (unsigned char*) malloc ( buffer_size);

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
        R_CreateImageForCinematic("*scratch", (unsigned char *)data, 
                DEFAULT_SIZE, DEFAULT_SIZE);
    }
    #undef DEFAULT_SIZE
}

```
