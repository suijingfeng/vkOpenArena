/*
 * =====================================================================================
 *
 *       Filename:  loadImage.c
 *
 *    Description:  Loads any of the supported image types into a cannonical 32 bit format.
 *
 *        Version:  1.0
 *        Created:  2018年09月23日 22时13分19秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sui Jingfeng , jingfengsui@gmail.com
 *   Organization:  CASIA(2014-2017)
 *
 * =====================================================================================
 */

#include "tr_local.h"
#include "../renderercommon/image_loader.h"

typedef struct
{
    char *ext;
    void (*ImageLoader)( const char *, unsigned char **, int *, int * );
} imageExtToLoaderMap_t;

// Note that the ordering indicates the order of preference used
// when there are multiple images of different formats available
const static imageExtToLoaderMap_t imageLoaders[6] =
{
    { "tga",  R_LoadTGA },
    { "jpg",  R_LoadJPG },
    { "jpeg", R_LoadJPG },
    { "png",  R_LoadPNG },
    { "pcx",  R_LoadPCX },
    { "bmp",  R_LoadBMP }
};

const static int numImageLoaders = ARRAY_LEN( imageLoaders );


void R_LoadImage(const char *name, unsigned char **pic, int *width, int *height )
{

	qboolean orgNameFailed = qfalse;
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
				orgNameFailed = qtrue;
				orgLoader = i;
				stripExtension( name, localName, MAX_QPATH );
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
			if( orgNameFailed )
			{
				ri.Printf( PRINT_DEVELOPER, "WARNING: %s not present, using %s instead\n",name, altName );
			}

			break;
		}
	}
}

/*
		// if we dont get a successful load
	char altname[MAX_QPATH];                              // copy the name
    strcpy( altname, name );                              //
    int len = (int)strlen( altname );                              // 
    altname[len-3] = toupper(altname[len-3]);             // and try upper case extension for unix systems
    altname[len-2] = toupper(altname[len-2]);             //
    altname[len-1] = toupper(altname[len-1]);             //
	ri.Printf( PRINT_ALL, "trying %s...\n", altname );    // 
	R_LoadImage( altname, &pic, &width, &height );        //
*/
