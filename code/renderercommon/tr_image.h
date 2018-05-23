#ifndef TR_IMHGE_H_
#define TR_IMAGE_H_

#include "../qcommon/q_shared.h"

////////////////////// image_t  ////////////////////////////// 
typedef enum
{
	IMGTYPE_COLORALPHA, // for color, lightmap, diffuse, and specular
	IMGTYPE_NORMAL,
	IMGTYPE_NORMALHEIGHT,
	IMGTYPE_DELUXE, // normals are swizzled, deluxe are not
} imgType_t;

typedef enum
{
	IMGFLAG_NONE           = 0x0000,
	IMGFLAG_MIPMAP         = 0x0001,
	IMGFLAG_PICMIP         = 0x0002,
	IMGFLAG_CUBEMAP        = 0x0004,
	IMGFLAG_NO_COMPRESSION = 0x0010,
	IMGFLAG_NOLIGHTSCALE   = 0x0020,
	IMGFLAG_CLAMPTOEDGE    = 0x0040,
	IMGFLAG_SRGB           = 0x0080,
	IMGFLAG_GENNORMALMAP   = 0x0100,
} imgFlags_t;

typedef struct image_s {
	char		imgName[MAX_QPATH];		// game path, including extension
    struct image_s*	next;
	
    int			width;
    int         height;				// source image
	int			uploadWidth;
    int         uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
	unsigned int texnum;					// gl texture binding
	int			TMU;
	int			frameUsed;			// for texture usage in frame statistics
	int			internalFormat;
	imgType_t   type;
	imgFlags_t  flags;
} image_t;


void R_LoadBMP( const char *name, unsigned char** pic, int *width, int *height );
void R_LoadJPG( const char *name, unsigned char** pic, int *width, int *height );
void R_LoadPCX( const char *name, unsigned char** pic, int *width, int *height );
void R_LoadPNG( const char *name, unsigned char** pic, int *width, int *height );
void R_LoadTGA( const char *name, unsigned char** pic, int *width, int *height );

//image_t *R_FindImageFile( const char *name, imgType_t type, imgFlags_t flags );
//image_t *R_CreateImage(const char *name, unsigned char* pic, int width, int height, imgType_t type, imgFlags_t flags, int internalFormat);

#endif
