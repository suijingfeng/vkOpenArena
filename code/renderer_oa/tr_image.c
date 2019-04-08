/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_image.c
#include "tr_local.h"
extern glconfig_t glConfig;

extern void (APIENTRYP qglActiveTextureARB) (GLenum texture);


static unsigned char s_gammatable[256];

static int gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
static int gl_filter_max = GL_LINEAR;

static int force32upload;		// leilei - hack to get bloom/post to always do 32bit texture

static cvar_t* r_texturebits;
static cvar_t* r_overBrightBits;

#define FILE_HASH_SIZE		1024
static image_t* hashTable[FILE_HASH_SIZE];



/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname )
{
	int		i = 0;
	long	hash = 0;
	char	letter;

	while (fname[i] != '\0')
    {
		letter = tolower(fname[i]);
		if (letter =='.')
            break;				// don't include extension
		if (letter =='\\')
            letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}



//=======================================================================

/*
================
ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function before or after.
================
*/
static void ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight )
{
	int		i, j;
	unsigned	*inrow, *inrow2;
	unsigned	p1[2048], p2[2048];
	unsigned char *pix1, *pix2, *pix3, *pix4;

	if (outwidth>2048)
		ri.Error(ERR_DROP, "ResampleTexture: max width");
								
	unsigned int fracstep = inwidth*0x10000/outwidth;

	unsigned int frac = fracstep>>2;
	for ( i=0 ; i<outwidth ; i++ )
    {
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for ( i=0 ; i<outwidth ; i++ )
    {
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth)
    {
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
		
        for (j=0 ; j<outwidth ; j++)
        {
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}



//
// Darkplaces texture resampling with lerping
// from Twilight/Darkplaces, code by LordHavoc (I AM ASSUMING)
//

static void Image_Resample32LerpLine (const unsigned char *in, unsigned char *out, int inwidth, int outwidth)
{
	int	j, xi,  f, lerp;
	int fstep = (int) (inwidth*65536.0f/outwidth);
    int oldx = 0;
	int endx = (inwidth-1);

	for (j = 0,f = 0; j < outwidth; j++, f += fstep)
	{
		xi = f >> 16;
		if (xi != oldx)
		{
			in += (xi - oldx) * 4;
			oldx = xi;
		}
		if (xi < endx)
		{
			lerp = f & 0xFFFF;
			*out++ = (unsigned char) ((((in[4] - in[0]) * lerp) >> 16) + in[0]);
			*out++ = (unsigned char) ((((in[5] - in[1]) * lerp) >> 16) + in[1]);
			*out++ = (unsigned char) ((((in[6] - in[2]) * lerp) >> 16) + in[2]);
			*out++ = (unsigned char) ((((in[7] - in[3]) * lerp) >> 16) + in[3]);
		}
		else // last pixel of the line has no pixel to lerp to
		{
			*out++ = in[0];
			*out++ = in[1];
			*out++ = in[2];
			*out++ = in[3];
		}
	}
}


static void Image_Resample32Lerp(const void *indata, int inwidth, int inheight, void *outdata, int outwidth, int outheight)
{
    #define LERPBYTE(i) r = resamplerow1[i];out[i] = (unsigned char) ((((resamplerow2[i] - r) * lerp) >> 16) + r)
	int i, j, r, yi, oldy, f, fstep, lerp, endy = (inheight - 1), inwidth4 = inwidth*4, outwidth4 = outwidth*4;
	unsigned char *out;
	const unsigned char *inrow;
	unsigned char *resamplerow1;
	unsigned char *resamplerow2;
	out = (unsigned char *)outdata;
	fstep = (int) (inheight*65536.0f/outheight);

	
	resamplerow1 = ri.Hunk_AllocateTempMemory(outwidth*4*2);
	resamplerow2 = resamplerow1 + outwidth*4;

	inrow = (const unsigned char *)indata;
	oldy = 0;
	Image_Resample32LerpLine (inrow, resamplerow1, inwidth, outwidth);
	Image_Resample32LerpLine (inrow + inwidth4, resamplerow2, inwidth, outwidth);
	for (i = 0, f = 0;i < outheight;i++,f += fstep)
	{
		yi = f >> 16;
		if (yi < endy)
		{
			lerp = f & 0xFFFF;
			if (yi != oldy)
			{
				inrow = (unsigned char *)indata + inwidth4*yi;
				if (yi == oldy+1)
					memcpy(resamplerow1, resamplerow2, outwidth4);
				else
					Image_Resample32LerpLine (inrow, resamplerow1, inwidth, outwidth);
				Image_Resample32LerpLine (inrow + inwidth4, resamplerow2, inwidth, outwidth);
				oldy = yi;
			}
			j = outwidth - 4;
			while(j >= 0)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				LERPBYTE( 4);
				LERPBYTE( 5);
				LERPBYTE( 6);
				LERPBYTE( 7);
				LERPBYTE( 8);
				LERPBYTE( 9);
				LERPBYTE(10);
				LERPBYTE(11);
				LERPBYTE(12);
				LERPBYTE(13);
				LERPBYTE(14);
				LERPBYTE(15);
				out += 16;
				resamplerow1 += 16;
				resamplerow2 += 16;
				j -= 4;
			}
			if (j & 2)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				LERPBYTE( 4);
				LERPBYTE( 5);
				LERPBYTE( 6);
				LERPBYTE( 7);
				out += 8;
				resamplerow1 += 8;
				resamplerow2 += 8;
			}
			if (j & 1)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				out += 4;
				resamplerow1 += 4;
				resamplerow2 += 4;
			}
			resamplerow1 -= outwidth4;
			resamplerow2 -= outwidth4;
		}
		else
		{
			if (yi != oldy)
			{
				inrow = (unsigned char *)indata + inwidth4*yi;
				if (yi == oldy+1)
					memcpy(resamplerow1, resamplerow2, outwidth4);
				else
					Image_Resample32LerpLine (inrow, resamplerow1, inwidth, outwidth);
				oldy = yi;
			}
			memcpy(out, resamplerow1, outwidth4);
		}
	}

	ri.Hunk_FreeTempMemory( resamplerow1 );
	resamplerow1 = NULL;
	resamplerow2 = NULL;
}



/*
================
R_MipMap2

Operates in place, quartering the size of the texture Proper linear filter
================
*/
static void R_MipMap2( unsigned *in, int inWidth, int inHeight )
{
	int			i, j, k;
	int			inWidthMask, inHeightMask;
	int			outWidth, outHeight;
	unsigned	*temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = ri.Hunk_AllocateTempMemory( outWidth * outHeight * 4 );

	inWidthMask = inWidth - 1;
	inHeightMask = inHeight - 1;

	for ( i = 0 ; i < outHeight ; i++ ) {
		for ( j = 0 ; j < outWidth ; j++ )
        {
			unsigned char* outpix = (unsigned char*) ( temp + i * outWidth + j );
			for ( k = 0 ; k < 4 ; k++ )
            {
				int total = 
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k];
				outpix[k] = total / 36;
			}
		}
	}

	memcpy( in, temp, outWidth * outHeight * 4 );
	ri.Hunk_FreeTempMemory( temp );
}

/*
================
R_MipMap: Operates in place, quartering the size of the texture
================
*/
static void R_MipMap(unsigned char *in, int width, int height)
{
	int	i, j;
	unsigned char *out;
	int	row;

	if ( !r_simpleMipMaps->integer ) {
		R_MipMap2( (unsigned *)in, width, height );
		return;
	}

	if ( width == 1 && height == 1 )
		return;

	row = width * 4;
	out = in;
	width >>= 1;
	height >>= 1;

	if ( width == 0 || height == 0 )
    {
		width += height;	// get largest
		for (i=0 ; i<width ; i++, out+=4, in+=8 )
        {
			out[0] = ( in[0] + in[4] )>>1;
			out[1] = ( in[1] + in[5] )>>1;
			out[2] = ( in[2] + in[6] )>>1;
			out[3] = ( in[3] + in[7] )>>1;
		}
		return;
	}

	for (i=0 ; i<height ; i++, in+=row)
    {
		for (j=0 ; j<width ; j++, out+=4, in+=8)
        {
			out[0] = (in[0] + in[4] + in[row+0] + in[row+4])>>2;
			out[1] = (in[1] + in[5] + in[row+1] + in[row+5])>>2;
			out[2] = (in[2] + in[6] + in[row+2] + in[row+6])>>2;
			out[3] = (in[3] + in[7] + in[row+3] + in[row+7])>>2;
		}
	}
}


/*
==================
R_BlendOverTexture: Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture(unsigned char *data, int pixelCount, const unsigned char blend[4])
{
	int	i;
	int	inverseAlpha = 255 - blend[3];

	for( i = 0; i < pixelCount; i+=4)
    {
		data[i] =   ( data[i]   * inverseAlpha + blend[0] * blend[3] ) >> 9;
		data[i+1] = ( data[i+1] * inverseAlpha + blend[1] * blend[3] ) >> 9;
		data[i+2] = ( data[i+2] * inverseAlpha + blend[2] * blend[3] ) >> 9;
	}
}



static void GL_CheckErrors( void )
{
	char s[64];

	int err = qglGetError();
	if ( err == GL_NO_ERROR )
		return;

	if ( r_ignoreGLErrors->integer ) {
		return;
	}
	switch( err )
    {
        case GL_INVALID_ENUM:
            strcpy( s, "GL_INVALID_ENUM" );
            break;
        case GL_INVALID_VALUE:
            strcpy( s, "GL_INVALID_VALUE" );
            break;
        case GL_INVALID_OPERATION:
            strcpy( s, "GL_INVALID_OPERATION" );
            break;
        case GL_STACK_OVERFLOW:
            strcpy( s, "GL_STACK_OVERFLOW" );
            break;
        case GL_STACK_UNDERFLOW:
            strcpy( s, "GL_STACK_UNDERFLOW" );
            break;
        case GL_OUT_OF_MEMORY:
            strcpy( s, "GL_OUT_OF_MEMORY" );
            break;
        default:
            snprintf( s, sizeof(s), "%i", err);
            break;
	}

	ri.Error( ERR_FATAL, "GL_CheckErrors: %s", s );
}


static void Upload32( unsigned *data, int width, int height, qboolean mipmap, qboolean picmip, qboolean lightMap, int *format, int *pUploadWidth, int *pUploadHeight )
{
	unsigned *resampledBuffer = NULL;
	GLenum	internalFormat = GL_RGB;

	int		forceBits = 0; 
	int		scaled_width, scaled_height;
	int		i;

	if (r_roundImagesDown->integer == 2)
	{
        scaled_width = width;
        scaled_height = height;

		if ( scaled_width != width || scaled_height != height )
        {
			resampledBuffer = ri.Hunk_AllocateTempMemory( scaled_width * scaled_height * 4 );
			ResampleTexture (data, width, height, resampledBuffer, scaled_width, scaled_height);
			data = resampledBuffer;
			width = scaled_width;
			height = scaled_height;
		}
	}
	else 
	{

		for (scaled_width = 1; scaled_width < width; scaled_width<<=1)
			;
		for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
			;
		if ( (r_roundImagesDown->integer == 1) && scaled_width > width )
			scaled_width >>= 1;
		
        if ( (r_roundImagesDown->integer == 1) && scaled_height > height )
			scaled_height >>= 1;

		if ( scaled_width != width || scaled_height != height )
        {
			resampledBuffer = ri.Hunk_AllocateTempMemory( scaled_width * scaled_height * 4 );
		    
            //leilei - high quality texture resampling, Currently 0 as there is an alignment issue I haven't fixed.
            if ( 0 )
			    Image_Resample32Lerp(data, width, height, resampledBuffer, scaled_width, scaled_height - 1);
			else
			    ResampleTexture (data, width, height, resampledBuffer, scaled_width, scaled_height);
			
            data = resampledBuffer;
			width = scaled_width;
			height = scaled_height;
		}
	}


	// perform optional picmip operation
	
    if( picmip )
    {
		scaled_width >>= r_picmip->integer;
		scaled_height >>= r_picmip->integer;
	}


	// clamp to minimum size
	if (scaled_width < 1)
    {
		scaled_width = 1;
	}
	if (scaled_height < 1)
    {
		scaled_height = 1;
	}

	// clamp to the current upper OpenGL limit scale both axis down equally
    // so we don't have to deal with a half mip resampling
	
	while( scaled_width > glConfig.maxTextureSize || scaled_height > glConfig.maxTextureSize )
    {
		scaled_width >>= 1;
		scaled_height >>= 1;
	}

	unsigned int *scaledBuffer = ri.Hunk_AllocateTempMemory( sizeof( unsigned int) * scaled_width * scaled_height );

	// scan the texture for each channel's max values and verify if the alpha channel is being used or not
	
	unsigned char *scan = (unsigned char *)data;
	int	samples = 3;


	if(lightMap)
	{
		internalFormat = GL_RGB;
	}
	else
	{
	    // here //
		for(i = 0; i <width*height; i++)
		{
			if(scan[i*4 + 3] != 255 ) 
			{
				samples = 4;
				break;
			}
		}
        
		// select proper internal format
		if ( samples == 3 )
		{
            if ( glConfig.textureCompression == TC_S3TC_ARB )
            {
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            }
            else if ( glConfig.textureCompression == TC_S3TC )
            {
                internalFormat = GL_RGB4_S3TC;
            }
            else if ( r_texturebits->integer == 32 || forceBits == 32)
            {
                internalFormat = GL_RGB8;
            }
            else
            {
                internalFormat = GL_RGB;
            }
            
            if (force32upload)
                internalFormat = GL_RGB8;   // leilei - gets bloom and postproc working on s3tc & 8bit & palettes
		}
		else if ( samples == 4 )
		{
            if ( glConfig.textureCompression == TC_S3TC_ARB )
            {
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; // leilei - this was missing
            }
            else if ( r_texturebits->integer == 32 || forceBits == 32)
            {
                internalFormat = GL_RGBA8;
            }
            else
            {
                internalFormat = GL_RGBA;
            }
            
            if (force32upload)
                internalFormat = GL_RGBA8;   // leilei - gets bloom and postproc working on s3tc & 8bit & palettes
		}
	}


	// copy or resample data as appropriate for first MIP level
	if ( ( scaled_width == width ) && ( scaled_height == height ) ) 
    {
		if (!mipmap)
		{
			qglTexImage2D (GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			*pUploadWidth = scaled_width;
			*pUploadHeight = scaled_height;
			*format = internalFormat;

			goto done;
		}

		memcpy(scaledBuffer, data, width*height*4);
	}
	else
	{
		// use the normal mip-mapping function to go down from here
		while ( width > scaled_width || height > scaled_height )
        {
			R_MipMap( (byte *)data, width, height );
			width >>= 1;
			height >>= 1;
			if ( width < 1 ) {
				width = 1;
			}
			if ( height < 1 ) {
				height = 1;
			}
		}
		memcpy(scaledBuffer, data, width * height * 4 );
	}

	*pUploadWidth = scaled_width;
	*pUploadHeight = scaled_height;
	*format = internalFormat;

	qglTexImage2D(GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );


	if (mipmap)
	{
        static const unsigned char mipBlendColors[16][4] =
        {
            {0,0,0,0},
            {255,0,0,128},
            {0,255,0,128},
            {0,0,255,128},
            {255,0,0,128},
            {0,255,0,128},
            {0,0,255,128},
            {255,0,0,128},
            {0,255,0,128},
            {0,0,255,128},
            {255,0,0,128},
            {0,255,0,128},
            {0,0,255,128},
            {255,0,0,128},
            {0,255,0,128},
            {0,0,255,128},
        };

		int	miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			R_MipMap( (unsigned char *)scaledBuffer, scaled_width, scaled_height );
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;

			if ( r_colorMipLevels->integer )
            {
				R_BlendOverTexture( (unsigned char *)scaledBuffer, scaled_width * scaled_height, mipBlendColors[miplevel] );
			}

			qglTexImage2D (GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
		}
	}
done:

	if (mipmap)
	{   
        qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
        qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	GL_CheckErrors();

	if ( scaledBuffer != 0 )
		ri.Hunk_FreeTempMemory( scaledBuffer );
	if ( resampledBuffer != 0 )
		ri.Hunk_FreeTempMemory( resampledBuffer );
}





//===================================================================
/*
 * Loads any of the supported image types into a cannonical 32 bit format.
*/
static void R_LoadImage(const char *name, unsigned char **pic, int *width, int *height )
{
    typedef struct
    {
        char *ext;
        void (*ImageLoader)( const char *, unsigned char **, int *, int * );
    } imageExtToLoaderMap_t;

    // Note that the ordering indicates the order of preference used
    // when there are multiple images of different formats available
    static const imageExtToLoaderMap_t imageLoaders[ ] =
    {
        { "tga",  R_LoadTGA },
        { "jpg",  R_LoadJPG },
        { "jpeg", R_LoadJPG },
        { "png",  R_LoadPNG },
        { "pcx",  R_LoadPCX },
        { "bmp",  R_LoadBMP }
    };

    static const int numImageLoaders = ARRAY_LEN( imageLoaders );
    
    // qboolean orgNameFailed = qfalse;
	int orgLoader = -1;
	int i;
	char localName[ MAX_QPATH ];

	*pic = NULL;
	*width = 0;
	*height = 0;

	Q_strncpyz( localName, name, MAX_QPATH );

	const char *ext = COM_GetExtension( localName );


	if( *ext )
	{
		// Look for the correct loader and use it
		for( i = 0; i < numImageLoaders; i++ )
		{
			if( !Q_stricmp( ext, imageLoaders[ i ].ext ) )
			{
				// Load
				imageLoaders[ i ].ImageLoader( localName, pic, width, height );
				break;
			}
		}

		// A loader was found
		if( i < numImageLoaders )
		{
			if( *pic == NULL )
			{
				// Loader failed, most likely because the file isn't there;
				// try again without the extension
				// orgNameFailed = qtrue;
				orgLoader = i;
				R_StripExtension( name, localName, MAX_QPATH );
			}
			else
			{
				// Something loaded
				return;
			}
		}
	}

	// Try and find a suitable match using all
	// the image formats supported
	for( i = 0; i < numImageLoaders; i++ )
	{
		if (i == orgLoader)
			continue;

		char *altName = va( "%s.%s", localName, imageLoaders[ i ].ext );

		// Load
		imageLoaders[ i ].ImageLoader( altName, pic, width, height );

		if( *pic )
		{
            /*
			if( orgNameFailed )
			{
				ri.Printf( PRINT_WARNING, "WARNING: %s not present, using %s instead\n",name, altName );
			}
            */
			break;
		}
	}
}



/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t* R_FindImageFile( const char *name, imgType_t type, imgFlags_t flags )
{
    image_t* image;
    int	width, height;
    unsigned char* pic;

    if (!name) {
        return NULL;
    }

    long hash = generateHashValue(name);

    //
    // see if the image is already loaded
    //
    for (image=hashTable[hash]; image; image=image->next)
    {
        if ( !strcmp( name, image->imgName ) )
        {
            // the white image can be used with any set of parms, but other mismatches are errors
            if ( strcmp( name, "*white" ) )
            {
                if ( image->flags != flags ) {
                    ri.Printf( PRINT_ALL, "WARNING: reused image %s with mixed flags (%i vs %i)\n", name, image->flags, flags );
                }
            }
            return image;
        }
    }

    //
    // load the pic from disk
    //
    R_LoadImage( name, &pic, &width, &height );
    if ( pic == NULL ) {
        return NULL;
    }

    image = R_CreateImage( ( char * ) name, pic, width, height, type, flags, 0 );
    ri.Free( pic );
    return image;
}



/*
================
R_CreateDlightImage
================
*/
static void R_CreateDlightImage( void )
{
    #define	DLIGHT_SIZE	16
	int	x,y;
	unsigned char data[DLIGHT_SIZE][DLIGHT_SIZE][4];

	// make a centered inverse-square falloff blob for dynamic lighting
	for (x=0 ; x<DLIGHT_SIZE ; x++)
    {
		for (y=0 ; y<DLIGHT_SIZE ; y++)
        {
			float d = ( DLIGHT_SIZE/2 - 0.5f - x ) * ( DLIGHT_SIZE/2 - 0.5f - x ) +
				( DLIGHT_SIZE/2 - 0.5f - y ) * ( DLIGHT_SIZE/2 - 0.5f - y );
			int b = 4000 / d;
			if (b > 255)
				b = 255;
            else if( b < 75 )
				b = 0;

			data[y][x][0] = data[y][x][1] = data[y][x][2] = b;
			data[y][x][3] = 255;			
		}
	}
	tr.dlightImage = R_CreateImage("*dlight", (unsigned char *)data, DLIGHT_SIZE, DLIGHT_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE, 0 );
}



static void R_CreateFogImage( void )
{
    #define	FOG_S	256
    #define	FOG_T	32

	int		x,y;
	float	borderColor[4];
	force32upload = 1;		// leilei - paletted fog fix
	unsigned char * data = ri.Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

	// S is distance, T is depth
	for (x=0 ; x<FOG_S ; x++)
    {
		for (y=0 ; y<FOG_T ; y++)
        {
			float d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			data[(y*FOG_S+x)*4+0] = data[(y*FOG_S+x)*4+1] = data[(y*FOG_S+x)*4+2] = 255;
			data[(y*FOG_S+x)*4+3] = 255*d;
		}
	}


	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does what we want.
	tr.fogImage = R_CreateImage("*fog", (unsigned char *)data, FOG_S, FOG_T, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE, 0 );
	ri.Hunk_FreeTempMemory( data );

	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
	force32upload = 0;		// leilei - paletted fog fix
}


static void R_CreateDefaultImage( void )
{
    const unsigned int DEFAULT_SIZE = 16;
	unsigned char data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	int	x;

	// the default image will be a box, to allow you to see the mapping coordinates
	memset( data, 32, sizeof( data ) );
	for ( x = 0 ; x < DEFAULT_SIZE ; x++ )
    {
		data[0][x][0] = data[0][x][1] =	data[0][x][2] =	data[0][x][3] = 255;

		data[x][0][0] =	data[x][0][1] =	data[x][0][2] =	data[x][0][3] = 255;

		data[DEFAULT_SIZE-1][x][0] = data[DEFAULT_SIZE-1][x][1] = data[DEFAULT_SIZE-1][x][2] = data[DEFAULT_SIZE-1][x][3] = 255;

		data[x][DEFAULT_SIZE-1][0] = data[x][DEFAULT_SIZE-1][1] = data[x][DEFAULT_SIZE-1][2] = data[x][DEFAULT_SIZE-1][3] = 255;
	}
	tr.defaultImage = R_CreateImage("*default", (unsigned char *)data, DEFAULT_SIZE, DEFAULT_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_MIPMAP, 0);
}


static void R_CreateBuiltinImages( void )
{
	int	x,y;
    const unsigned int DEFAULT_SIZE = 16;
	unsigned char data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	memset( data, 255, sizeof( data ) );
	
    tr.whiteImage = R_CreateImage("*white", (unsigned char *)data, 8, 8, IMGTYPE_COLORALPHA, IMGFLAG_NONE, 0);

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for (x=0 ; x<DEFAULT_SIZE ; x++)
    {
		for (y=0 ; y<DEFAULT_SIZE ; y++) {
			data[y][x][0] = data[y][x][1] = data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;			
		}
	}

	tr.identityLightImage = R_CreateImage("*identityLight", (unsigned char *)data, 8, 8, IMGTYPE_COLORALPHA, IMGFLAG_NONE, 0);


	for(x=0;x<32;x++)
    {
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage("*scratch", (unsigned char *)data, DEFAULT_SIZE, DEFAULT_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_PICMIP | IMGFLAG_CLAMPTOEDGE, 0);
	}


	R_CreateDlightImage();
	R_CreateFogImage();
	//tr.fogImage = R_FindImageFile( "gfx/engine/fog.tga", 0, IMGFLAG_CLAMPTOEDGE )
	//tr.dlightImage = R_FindImageFile( "gfx/engine/dlight.tga", 0, IMGFLAG_CLAMPTOEDGE );
}




/////////////////////////////////////////////////////////////////////////////////

void GL_TextureMode( const char *string )
{
    typedef struct {
	char *name;
	int	minimize, maximize;
    } textureMode_t;

    static const textureMode_t texModes[] = {
        {"GL_NEAREST", GL_NEAREST, GL_NEAREST},
        {"GL_LINEAR", GL_LINEAR, GL_LINEAR},
        {"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
        {"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
        {"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
        {"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
    };

	int	i;

	for( i=0 ; i< 6 ; i++ )
    {
		if ( !Q_stricmp( texModes[i].name, string ) ) {
			break;
		}
	}

	if ( i == 6 )
    {
		ri.Printf(PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = texModes[i].minimize;
	gl_filter_max = texModes[i].maximize;

	// change all the existing mipmap texture objects
	for( i = 0 ; i < tr.numImages ; i++ )
    {
		image_t	*glt = tr.images[i];
		if ( glt->flags & IMGFLAG_MIPMAP )
        {
			GL_Bind(glt);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}


int R_SumOfUsedImages( void )
{
	int	total = 0;
	int i;

	for( i = 0; i < tr.numImages; i++ )
    {
		if( tr.images[i]->frameUsed == tr.frameCount )
        {
			total += tr.images[i]->uploadWidth * tr.images[i]->uploadHeight;
		}
	}

	return total;
}



/*
================
R_CreateImage: This is the only way any image_t are created
================
*/
image_t *R_CreateImage(const char *name, unsigned char* pic, int width, int height, imgType_t type, imgFlags_t flags, int internalFormat)
{
	qboolean isLightmap = qfalse;
	int glWrapClampMode;
	
    if (strlen(name) >= MAX_QPATH )
    {
		ri.Error(ERR_DROP, "R_CreateImage: \"%s\" is too long", name);
	}
	if ( !strncmp( name, "*lightmap", 9 ) )
    {
		isLightmap = qtrue;
	}

	if ( tr.numImages >= MAX_DRAWIMAGES )
    {
		ri.Error( ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit");
	}

	image_t	*image = tr.images[tr.numImages] = ri.Hunk_Alloc( sizeof( image_t ), h_low );
	image->texnum = 1024 + tr.numImages;
	tr.numImages++;

	image->type = type;
	image->flags = flags;

	strcpy(image->imgName, name);

	image->width = width;
	image->height = height;


    if (flags & IMGFLAG_CLAMPTOEDGE)
        glWrapClampMode = GL_CLAMP_TO_EDGE; 
    else{
        glWrapClampMode = GL_REPEAT;
    }

	// lightmaps are always allocated on TMU 1

	if ( qglActiveTextureARB && isLightmap ) {
		image->TMU = 1;
	} else {
		image->TMU = 0;
	}

	if ( qglActiveTextureARB ) {
		GL_SelectTexture( image->TMU );
	}

	GL_Bind(image);

    Upload32( (unsigned *)pic, image->width, image->height, image->flags & IMGFLAG_MIPMAP,
	image->flags & IMGFLAG_PICMIP, isLightmap,
	&image->internalFormat, &image->uploadWidth, &image->uploadHeight );

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapClampMode );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapClampMode );

	// FIXME: this stops fog from setting border color?
	glState.currenttextures[glState.currenttmu] = 0;
	qglBindTexture( GL_TEXTURE_2D, 0 );

	if ( image->TMU == 1 ) {
		GL_SelectTexture( isLightmap );
	}

	long hash = generateHashValue(name);
	image->next = hashTable[hash];
	hashTable[hash] = image;

	return image;
}



void R_ImageList_f( void )
{
	int i;
	int estTotalSize = 0;

	ri.Printf(PRINT_ALL, "\n      -w-- -h-- type  -size- --name-------\n");

	for ( i = 0 ; i < tr.numImages ; i++ )
	{
		image_t *image = tr.images[i];
		char *format = "???? ";
		char *sizeSuffix;
		int estSize;
		int displaySize;

		estSize = image->uploadHeight * image->uploadWidth;

		switch(image->internalFormat)
		{
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
				format = "sDXT1";
				// 64 bits per 16 pixels, so 4 bits per pixel
				estSize /= 2;
				break;
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
				format = "sDXT5";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:
				format = "sBPTC";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
				format = "LATC ";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
				format = "DXT1 ";
				// 64 bits per 16 pixels, so 4 bits per pixel
				estSize /= 2;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				format = "DXT5 ";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
				format = "BPTC ";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_RGB4_S3TC:
				format = "S3TC ";
				// same as DXT1?
				estSize /= 2;
				break;
			case GL_RGBA4:
			case GL_RGBA8:
			case GL_RGBA:
				format = "RGBA ";
				// 4 bytes per pixel
				estSize *= 4;
				break;
			case GL_LUMINANCE8:
			case GL_LUMINANCE16:
			case GL_LUMINANCE:
				format = "L    ";
				// 1 byte per pixel?
				break;
			case GL_RGB5:
			case GL_RGB8:
			case GL_RGB:
				format = "RGB  ";
				// 3 bytes per pixel?
				estSize *= 3;
				break;
			case GL_LUMINANCE8_ALPHA8:
			case GL_LUMINANCE16_ALPHA16:
			case GL_LUMINANCE_ALPHA:
				format = "LA   ";
				// 2 bytes per pixel?
				estSize *= 2;
				break;
			case GL_SRGB_EXT:
			case GL_SRGB8_EXT:
				format = "sRGB ";
				// 3 bytes per pixel?
				estSize *= 3;
				break;
			case GL_SRGB_ALPHA_EXT:
			case GL_SRGB8_ALPHA8_EXT:
				format = "sRGBA";
				// 4 bytes per pixel?
				estSize *= 4;
				break;
			case GL_SLUMINANCE_EXT:
			case GL_SLUMINANCE8_EXT:
				format = "sL   ";
				// 1 byte per pixel?
				break;
			case GL_SLUMINANCE_ALPHA_EXT:
			case GL_SLUMINANCE8_ALPHA8_EXT:
				format = "sLA  ";
				// 2 byte per pixel?
				estSize *= 2;
				break;
		}

		// mipmap adds about 50%
		if (image->flags & IMGFLAG_MIPMAP)
			estSize += estSize / 2;

		sizeSuffix = "b ";
		displaySize = estSize;

		if (displaySize > 1024)
		{
			displaySize /= 1024;
			sizeSuffix = "kb";
		}

		if (displaySize > 1024)
		{
			displaySize /= 1024;
			sizeSuffix = "Mb";
		}

		if (displaySize > 1024)
		{
			displaySize /= 1024;
			sizeSuffix = "Gb";
		}

		ri.Printf(PRINT_ALL, "%4i: %4ix%4i %s %4i%s %s\n", i, image->uploadWidth, image->uploadHeight, format, displaySize, sizeSuffix, image->imgName);

		estTotalSize += estSize;
	}

	ri.Printf (PRINT_ALL, " ---------\n");
	ri.Printf (PRINT_ALL, " approx %i bytes\n", estTotalSize);
	ri.Printf (PRINT_ALL, " %i total images\n\n", tr.numImages );
}


void R_InitSkins( void )
{
    ri.Printf( PRINT_ALL, "---------- R_InitSkins ---------- \n");

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin_t* skin = tr.skins[0] = ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;

	skin->surfaces = ri.Hunk_Alloc( sizeof( skinSurface_t ), h_low );
	skin->surfaces[0].shader = tr.defaultShader;
}



void R_SetColorMappings(void)
{
	int	i, inf,	shift;
	if ( !glConfig.deviceSupportsGamma)
    {
		tr.overbrightBits = 0;		// need hardware gamma for overbright
	}

	// never overbright in windowed mode
	if( !glConfig.isFullscreen)
	{
		tr.overbrightBits = 0;
	}
    else
    {
    	// setup the overbright lighting
	    tr.overbrightBits = r_overBrightBits->integer;
    }

	// allow 2 overbright bits in 24 bit, but only 1 in 16 bit
	if( glConfig.colorBits > 16 )
    {
		if( tr.overbrightBits > 2 )
        {
			tr.overbrightBits = 2;
		}
	}
    else
    {
		if ( tr.overbrightBits > 1 )
			tr.overbrightBits = 1;
	}


	if( tr.overbrightBits < 0 )
    {
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / ( 1 << tr.overbrightBits );
	tr.identityLightByte = 255 * tr.identityLight;


	if( r_gamma->value < 0.4f )
    {
		ri.Cvar_Set( "r_gamma", "0.4" );
	}
    else if ( r_gamma->value > 4.0f )
    {
		ri.Cvar_Set( "r_gamma", "4.0" );
	}

	float g = r_gamma->value;

	shift = tr.overbrightBits;		// hardware gamma to work (if available) since we can't do alternate gamma via blends


	for ( i = 0; i < 256; i++ )
    {
		if ( g == 1 )
        {
			inf = i;
		}
        else
        {
			inf = 255 * pow ( i/255.0f, 1.0f / g ) + 0.5f;
		}
		inf <<= shift;
		if (inf < 0)
        {
			inf = 0;
		}
        else if (inf > 255) 
        {    
			inf = 255;
		}
		s_gammatable[i] = inf;
	}

	if ( glConfig.deviceSupportsGamma)
	{
		GLimp_SetGamma( s_gammatable, s_gammatable, s_gammatable );
	}
}


void R_GammaCorrect(unsigned char *buffer, int bufSize)
{
	int i;
	for( i = 0; i < bufSize; i++ )
    {
		buffer[i] = s_gammatable[buffer[i]];
	}
}




void R_InitFogTable( void )
{
	int	i;

	for( i = 0 ; i < FOG_TABLE_SIZE ; i++ )
    {
		tr.fogTable[i] = pow( i/(float)(FOG_TABLE_SIZE-1), 0.5);
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float R_FogFactor( float s, float t )
{
	float	d;

	s -= 1.0/512;
	if ( s < 0 ) {
		return 0;
	}
	if ( t < 1.0/32 ) {
		return 0;
	}
	if ( t < 31.0/32 ) {
		s *= (t - 1.0f/32.0f) / (30.0f/32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 ) {
		s = 1.0;
	}

	d = tr.fogTable[ (int)(s * (FOG_TABLE_SIZE-1)) ];

	return d;
}

void R_InitImages(void)
{
	memset(hashTable, 0, sizeof(hashTable));
	// build brightness translation tables
	
    r_texturebits = ri.Cvar_Get("r_texturebits", "32", CVAR_ARCHIVE | CVAR_LATCH);
    ri.Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );

    r_overBrightBits = ri.Cvar_Get ("r_overBrightBits", "1", CVAR_ARCHIVE | CVAR_LATCH );
    

    R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();
}
