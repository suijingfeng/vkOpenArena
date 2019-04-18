#include "R_LoadImage.h"
#include "ref_import.h"
// Description:  Loads any of the supported image types into
// a cannonical 32 bit format.

enum IMAGE_EXT_TYPE_t { 
    IMG_EXT_TGA = 0,
    IMG_EXT_JPG = 1,
    IMG_EXT_PNG = 2,
    IMG_EXT_PCX = 3,
    IMG_EXT_BMP = 4,
    IMG_EXT_CNT = 5
};


const static void (* fnImgLdrs[5])(const char *, unsigned char **, int *, int * ) = {
    R_LoadTGA,
    R_LoadJPG,
    R_LoadBMP,
    R_LoadPNG,
    R_LoadPCX,
};


// Load the image without a extention
static void
R_LoadNSE(const char * const pName, unsigned char **pic, int *width, int *height)
{
    *pic = NULL;

    static const char * ExTable[5]={".tga",".jpg",".png",".pcx",".bmp"};

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
        // strip ext
    }
    else
    {
        pPt = pSrc;
        // end of str
    }
    // no ext now


    uint32_t i; 
    for(i = 0; i < IMG_EXT_CNT; ++i)
    {
        strncpy(pPt, ExTable[i], 5); 
        ri.Printf( PRINT_WARNING, " Loading %s \n", localName);
        fnImgLdrs[i](localName, pic, width, height );

        if(*pic != NULL)
            return;
    }

    ri.Printf( PRINT_ERROR, " Failed loading %s. \n", pName);
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
            break;
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
            ri.Printf( PRINT_WARNING, " Unsupported image extension: %s.\n", pName);
        }
    }

    // Loading failed, the image just dont exist;
    // Try all ext supported, this means we have to try all 5 loader fun
    return R_LoadNSE(pName, pic, width, height);
}
