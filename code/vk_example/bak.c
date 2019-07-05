/* Convert ppm image data from header file into RGBA texture image */
#include "lunarg_ppm.h"

static bool loadTexture(const char *filename, uint8_t *rgba_data, VkSubresourceLayout *layout, 
		int32_t * pWidth, int32_t * pHeight)
{
    (void)filename;
    
    const unsigned char * const lunarg_ppm = getPtr_ppm();
    char * cPtr = ( char * ) lunarg_ppm;
    const unsigned int lunarg_ppm_len = getLen_ppm();
    
    uint32_t width, height;

    if ((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) || strncmp(cPtr, "P6\n", 3))
    {
        return false;
    }
    
    while (strncmp(cPtr++, "\n", 1))
        ;
    sscanf(cPtr, "%u %u", &width, &height);
    
    *pWidth = width;
    *pHeight = height;

    if (rgba_data == NULL) {
        return true;
    }
    
    while (strncmp(cPtr++, "\n", 1))
        ;
    
    if ((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) || strncmp(cPtr, "255\n", 4)) {
        return false;
    }
    
    while (strncmp(cPtr++, "\n", 1))
        ;
    
//	R_SaveToJPEG(cPtr, width, height, "img_lunarg.jpg");

    for (int y = 0; y < height; ++y)
    {
        uint8_t *rowPtr = rgba_data;
        // add alpha channal
        for (int x = 0; x < width; ++x)
        {
            rowPtr[0] = cPtr[0];
            rowPtr[1] = cPtr[1];
            rowPtr[2] = cPtr[2];
            rowPtr[3] = 255; /* Alpha of 1 */
            rowPtr += 4;
            cPtr += 3;
        }
        
        rgba_data += layout->rowPitch;
    }
    return true;
}

