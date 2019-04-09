#ifndef IMAGE_H_
#define IMAGE_H_

extern refimport_t ri;

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
	IMGFLAG_GENNORMALMAP   = 0x0080,
} imgFlags_t;


typedef struct image_s {
	char		imgName[MAX_QPATH];		// game path, including extension
	int			width, height;				// source image
	int			uploadWidth, uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
	uint32_t texnum;					// gl texture binding

	int			frameUsed;			// for texture usage in frame statistics

	int			internalFormat;
	int			TMU;				// only needed for voodoo2
    int         index;
	imgType_t   type;
	imgFlags_t  flags;

    int			wrapClampMode;		// GL_CLAMP or GL_REPEAT, for vulkan
    qboolean    mipmap;             // for vulkan
    qboolean    allowPicmip;        // for vulkan
	struct image_s*	next;
} image_t;

/*
=============================================================

IMAGE LOADERS

=============================================================
*/

void R_LoadBMP( const char *name, byte **pic, int *width, int *height );
void R_LoadJPG( const char *name, byte **pic, int *width, int *height );
void R_LoadPCX( const char *name, byte **pic, int *width, int *height );
void R_LoadPNG( const char *name, byte **pic, int *width, int *height );
void R_LoadTGA( const char *name, byte **pic, int *width, int *height );


#endif
