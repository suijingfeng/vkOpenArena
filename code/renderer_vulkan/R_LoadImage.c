#include "R_LoadImage.h"
#include "ref_import.h"
// Description:  Loads any of the supported image types into
// a cannonical 32 bit format.

enum IMAGE_EXT_TYPE_t { 
    IMG_EXT_TGA = 0,
    IMG_EXT_JPG = 1,
    IMG_EXT_BMP = 2,
    IMG_EXT_PNG = 3,
    IMG_EXT_PCX = 4,
    IMG_EXT_CNT = 5
};

static const char * ExTable[5]={".tga",".jpg",".bmp",".png",".pcx"};

typedef void (* pFnImageLoader_t)(const char *, unsigned char **, int *, int *);

//TODO : STB SUPPORT
#ifdef USE_STB_IMAGE_LIB

const static pFnImageLoader_t fnImgLdrs[5] = {
    STB_LoadTGA,
    STB_LoadJPG,
    STB_LoadBMP,
    STB_LoadPNG,
    STB_LoadPCX
};

#else

const static pFnImageLoader_t fnImgLdrs[5] = {
    R_LoadTGA,
    R_LoadJPG,
    R_LoadBMP,
    R_LoadPNG,
    R_LoadPCX
};

#endif

// Load the image without a extention
static void
R_LoadNSE(const char * const pName, unsigned char **pic, int *width, int *height)
{
    *pic = NULL;

	char localName[128];
    strncpy(localName, pName, 128);
    char* pPt = NULL;
    char* pSrc = localName;
    for( ; *pSrc != 0; ++pSrc)
    {
        // find the last point

        if( *pSrc == '.')
        {
            // have extention
            pPt = pSrc;
        }
    }

    if(pPt != NULL)
    {
        *pPt = 0;
    }
    else
    {
        pPt = pSrc;
    }
    // no ext now


    uint32_t i; 
    for(i = 0; i < IMG_EXT_CNT; ++i)
    {
        // strncpy(pPt, ExTable[i], 5); 
        pPt[0] = ExTable[i][0];
        pPt[1] = ExTable[i][1];
        pPt[2] = ExTable[i][2];
        pPt[3] = ExTable[i][3];
        pPt[4] = ExTable[i][4];

        // ri.Printf( PRINT_WARNING, " Loading %s \n", localName);
        
        fnImgLdrs[i](localName, pic, width, height );

        if(*pic != NULL)
            return;
    }

//    ri.Printf( PRINT_WARNING, " Failed loading %s. \n", pName);
}


// pName must not be null;
void R_LoadImage(const char * pName, unsigned char **pic, int *width, int *height )
{
    const char* pExt = NULL;

    const char* pSrc = pName;
    *pic = NULL;
    for( ; *pSrc != 0; ++pSrc)
    {
        // find the last point
        if( *pSrc == '.')
        {
            // have extention
            pExt = pSrc + 1;
            // break;
        }
    }

    if( pExt != NULL )
    {
        if( ( (pExt[0] == 't') && (pExt[1] == 'g') && (pExt[2] == 'a') ) ||
                ( (pExt[0] == 'T') && (pExt[1] == 'G') && (pExt[2] == 'A') ) )
        {
            R_LoadTGA( pName, pic, width, height );
            if(*pic != NULL )
                return;
        }
        else if( ( ( pExt[0] == 'j' ) && ( pExt[1] == 'p' ) && 
                    ( ( pExt[2] == 'g' ) || ( ( pExt[2] == 'e' ) && (pExt[3] == 'g') ) ) )
                || ( ( pExt[0] == 'J' ) && ( pExt[1] == 'P') &&
                    ( ( pExt[2] == 'G' ) || ( ( pExt[2] == 'E' ) && (pExt[3] == 'G') ) ) )
               ) {
            R_LoadJPG( pName, pic, width, height );
            if(*pic != NULL )
                return;
        }
        else if( ( (pExt[0] == 'b') && (pExt[1] == 'm') && (pExt[2] == 'p') ) ||
                ( (pExt[0] == 'B') && (pExt[1] == 'M') && (pExt[2] == 'P') ) )
        {   
            R_LoadBMP( pName, pic, width, height );
            if(*pic != NULL )
                return;
        }
        else if( ( (pExt[0] == 'p') && (pExt[1] == 'n') && (pExt[2] == 'g') ) ||
                ( (pExt[0] == 'P') && (pExt[1] == 'N') && (pExt[2] == 'G') ) )
        {   
            R_LoadPNG( pName, pic, width, height );
            if(*pic != NULL )
                return;
        }
        else if( ( (pExt[0] == 'p') && (pExt[1] == 'c') && (pExt[2] == 'x') ) ||
                ( (pExt[0] == 'P') && (pExt[1] == 'C') && (pExt[2] == 'X') ) )
        {   
            R_LoadPCX( pName, pic, width, height );
            if(*pic != NULL )
                return;
        }
        else {
            ri.Printf( PRINT_WARNING, " Unsupported image extension: %s \n", pName);
        }
    }

    // Loading failed, the image just dont exist;
    // Try all ext supported, this means we have to try all 5 loader fun
    return R_LoadNSE(pName, pic, width, height);
}


#ifdef USE_STB_IMAGE_LIB

static void* q3_stbi_malloc(size_t size)
{
    return ri.Malloc((int)size);
}

static void q3_stbi_free(void* p)
{
    ri.Free(p);
}

static void* q3_stbi_realloc(void* p_old, size_t old_size, size_t new_size)
{
    if (p_old == NULL)
        return ri.Malloc((int)new_size);

    void* p_new;
    
    if (old_size < new_size)
    {
        p_new = q3_stbi_malloc(new_size);
        memcpy(p_new, p_old, old_size);
        q3_stbi_free(p_old);
    }
    else
    {
        p_new = p_old;
    }
    return p_new;
}

#define STBI_MALLOC q3_stbi_malloc
#define STBI_FREE q3_stbi_free
#define STBI_REALLOC_SIZED q3_stbi_realloc
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



// ====================================== //
// ====================================== //

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;



void STB_LoadJPG( const char* name, unsigned char** pic, uint32_t* width, uint32_t* height)
{
    char* fbuffer;
    int len = ri.FS_ReadFile(name, &fbuffer);
    if (!fbuffer) {
        return;
    }
  
    int components;
    *pic = stbi_load_from_memory((unsigned char*)fbuffer, len, (int*)width, (int*)height, &components, STBI_rgb_alpha);
    if (*pic == NULL) {
        ri.FS_FreeFile(fbuffer);
        return;
    }

    // clear all the alphas to 255
    {
        unsigned int i;
        unsigned char* buf = *pic;

        unsigned int nBytes = 4 * (*width) * (*height);
        for (i = 3; i < nBytes; i += 4)
        {
            buf[i] = 255;
        }
    }
    ri.FS_FreeFile(fbuffer);
}

#endif
