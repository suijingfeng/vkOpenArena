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
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"
#include "../sdl/sdl_glimp.h"

extern shaderCommands_t tess;
extern backEndData_t* backEndData;	// the second one may not be allocated
extern trGlobals_t	tr;
extern backEndState_t backEnd;


glconfig_t  glConfig;
glstate_t	glState;
refimport_t	ri;

//extern cvar_t	*r_verbose;			// used for verbose debug spew
//extern cvar_t	*r_vertexLight;		// vertex lighting mode for better performance
//extern cvar_t	*r_logFile;		    // number of frames to emit GL logs

cvar_t* r_maxpolys;
cvar_t* r_maxpolyverts;


static cvar_t* r_textureMode;
static cvar_t* r_aviMotionJpegQuality;
static cvar_t* r_screenshotJpegQuality;

/*

struct vidmode_s {
	const char *description;
	int width, height;
	float pixelAspect;		// pixel width / height
};


static const struct vidmode_s r_vidModes[] = {
	{ "Mode  0: 320x240",		320,	240,	1 },
	{ "Mode  1: 400x300",		400,	300,	1 },
	{ "Mode  2: 512x384",		512,	384,	1 },
	{ "Mode  3: 640x480 (480p)",	640,	480,	1 },
	{ "Mode  4: 800x600",		800,	600,	1 },
	{ "Mode  5: 960x720",		960,	720,	1 },
	{ "Mode  6: 1024x768",		1024,	768,	1 },
	{ "Mode  7: 1152x864",		1152,	864,	1 },
	{ "Mode  8: 1280x1024",		1280,	1024,	1 },
	{ "Mode  9: 1600x1200",		1600,	1200,	1 },
	{ "Mode 10: 2048x1536",		2048,	1536,	1 },
	{ "Mode 11: 856x480",		856,	480,	1 },		// Q3 MODES END HERE AND EXTENDED MODES BEGIN
	{ "Mode 12: 1280x720 (720p)",	1280,	720,	1 },
	{ "Mode 13: 1280x768",		1280,	768,	1 },
	{ "Mode 14: 1280x800",		1280,	800,	1 },
	{ "Mode 15: 1280x960",		1280,	960,	1 },
	{ "Mode 16: 1360x768",		1360,	768,	1 },
	{ "Mode 17: 1366x768",		1366,	768,	1 }, // yes there are some out there on that extra 6
	{ "Mode 18: 1360x1024",		1360,	1024,	1 },
	{ "Mode 19: 1400x1050",		1400,	1050,	1 },
	{ "Mode 20: 1400x900",		1400,	900,	1 },
	{ "Mode 21: 1600x900",		1600,	900,	1 },
	{ "Mode 22: 1680x1050",		1680,	1050,	1 },
	{ "Mode 23: 1920x1080 (1080p)",	1920,	1080,	1 },
	{ "Mode 24: 1920x1200",		1920,	1200,	1 },
	{ "Mode 25: 1920x1440",		1920,	1440,	1 },
	{ "Mode 26: 2560x1600",		2560,	1600,	1 },
	{ "Mode 27: 3840x2160 (4K)",	3840,	2160,	1 }
};
static const int s_numVidModes = ARRAY_LEN( r_vidModes );
*/

static void GL_SetDefaultState(void)
{
	qglClearDepth( 1.0f );

	qglCullFace(GL_FRONT);

	qglColor4f(1,1,1,1);

	// initialize downstream texture unit if we're running in a multitexture environment
	if ( qglActiveTextureARB )
    {
		GL_SelectTexture( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		GL_SelectTexture( 0 );
	}

	qglEnable(GL_TEXTURE_2D);
	GL_TextureMode( r_textureMode->string );
	GL_TexEnv( GL_MODULATE );

	qglShadeModel( GL_SMOOTH );
	qglDepthFunc( GL_LEQUAL );

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState(GL_VERTEX_ARRAY);

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );
}


/*
** Workaround for ri.Printf's 1024 characters buffer limit.
static void R_PrintLongString(const char *string)
{
	char buffer[1024];
	int size = strlen(string);
	const char *p = string;
	
    while(size > 0)
    {
		Q_strncpyz(buffer, p, sizeof(buffer) );
		ri.Printf(PRINT_ALL, "%s", buffer);
		p += 1023;
		size -= 1023;
	}
}
*/


static void GfxInfo_f( void )
{
	const char *enablestrings[] =
    {
		"disabled",
		"enabled"
	};

	ri.Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	ri.Printf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	ri.Printf( PRINT_ALL, "GL_EXTENSIONS: " );
//	R_PrintLongString( glConfig.extensions_string );
    ri.Printf( PRINT_ALL, "\n" );
	ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.numTextureUnits );
	ri.Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );


	if( glConfig.deviceSupportsGamma )
		ri.Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	else
		ri.Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );


	ri.Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
	ri.Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
	ri.Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	ri.Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
	ri.Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression!=TC_NONE] );

  
}


/*
** This function is responsible for initializing a valid OpenGL subsystem. 
** This is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config to the user.
*/
static void InitOpenGL(void)
{
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_mode
	//		- r_(color|depth|stencil)bits

    GLimp_Init(qfalse);
    // GLimp_InitExtraExtensions();

    // OpenGL driver constants
    qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize );

    // stubbed or broken drivers may have reported 0...
    if( glConfig.maxTextureSize < 0 )
    {
        glConfig.maxTextureSize = 0;
    }

	// set default state
	GL_SetDefaultState();
    
    // print info
	GfxInfo_f();
}


/*
static void R_ModeList_f( void )
{
	ri.Printf( PRINT_ALL, "\n" );
    int i;
	for ( i = 0; i < s_numVidModes; i++ )
    {
		ri.Printf( PRINT_ALL, "%s\n", r_vidModes[i].description );
	}
	ri.Printf( PRINT_ALL, "\n" );
}
*/


/*
==================
RB_ReadPixels: Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from integer stored at pointer offset. 
When the function has returned the actual offset was written back to address offset. 
This address will always have an alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with ri.Hunk_FreeTempMemory()
==================
*/

static unsigned char *RB_ReadPixels(int x, int y, int width, int height, size_t *offset, int *padlen)
{
	GLint packAlign;
	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	int linelen = width * 3;
	int padwidth = PAD(linelen, packAlign);

	// Allocate a few more bytes so that we can choose an alignment we like
	unsigned char* buffer = ri.Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1);

	unsigned char* bufstart = PADP((intptr_t) buffer + *offset, packAlign);
	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);

	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;

	return buffer;
}



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

==============================================================================
*/
static void RB_TakeScreenshot(int x, int y, int width, int height, char *fileName)
{
	unsigned char *destptr, *endline;
	
	int padlen;
	size_t offset = 18, memcount;

	unsigned char* allbuf = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	unsigned char* buffer = allbuf + offset - 18;

	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	// swap rgb to bgr and remove padding from line endings
	int linelen = width * 3;

	unsigned char* srcptr = destptr = allbuf + offset;
	unsigned char* endmem = srcptr + (linelen + padlen) * height;

	while(srcptr < endmem)
    {
		endline = srcptr + linelen;

		while(srcptr < endline)
        {
			unsigned char temp = srcptr[0];
			*destptr++ = srcptr[2];
			*destptr++ = srcptr[1];
			*destptr++ = temp;

			srcptr += 3;
		}

		// Skip the pad
		srcptr += padlen;
	}

	memcount = linelen * height;

	// gamma correct
	if ( glConfig.deviceSupportsGamma ) {
		R_GammaCorrect(allbuf + offset, memcount);
	}

	ri.FS_WriteFile(fileName, buffer, memcount + 18);

	ri.Hunk_FreeTempMemory(allbuf);
}


static void RB_TakeScreenshotJPEG(int x, int y, int width, int height, char *fileName)
{
	size_t offset = 0;
	int padlen;
	unsigned char* buffer = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	size_t memcount = (width * 3 + padlen) * height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(buffer + offset, memcount);

	RE_SaveJPG(fileName, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen);
	ri.Hunk_FreeTempMemory(buffer);
}


const void *RB_TakeScreenshotCmd( const void *data )
{
	const screenshotCommand_t *cmd = (const screenshotCommand_t *)data;

	if (cmd->jpeg)
		RB_TakeScreenshotJPEG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	else
		RB_TakeScreenshot( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);

	return (const void *)(cmd + 1);
}


static void R_TakeScreenshot( int x, int y, int width, int height, char *name, qboolean jpeg )
{
	static char	fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
	screenshotCommand_t	*cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	Q_strncpyz( fileName, name, sizeof(fileName) );
	cmd->fileName = fileName;
	cmd->jpeg = jpeg;
}


static void R_ScreenshotFilename( int lastNumber, char *fileName )
{
	if ( lastNumber < 0 || lastNumber > 9999 )
    {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.tga" );
		return;
	}

	int a = lastNumber / 1000;
	lastNumber -= a*1000;
	int b = lastNumber / 100;
	lastNumber -= b*100;
	int c = lastNumber / 10;
	lastNumber -= c*10;
	int d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga", a, b, c, d );
}


static void R_ScreenshotFilenameJPEG( int lastNumber, char *fileName )
{
	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.jpg" );
		return;
	}

	int a = lastNumber / 1000;
	lastNumber -= a*1000;
	int b = lastNumber / 100;
	lastNumber -= b*100;
	int c = lastNumber / 10;
	lastNumber -= c*10;
	int d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg", a, b, c, d );
}

/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for the menu system,
sampled down from full screen distorted images
====================
*/
static void R_LevelShot( void )
{
	char		checkname[MAX_OSPATH];
	unsigned char* src;
    unsigned char* dst;
	size_t			offset = 0;
	int			padlen;
	int			x, y;
	int			r, g, b;
	int			xx, yy;

	Com_sprintf(checkname, sizeof(checkname), "levelshots/%s.tga", tr.world->baseName);

	unsigned char* allsource = RB_ReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, &offset, &padlen);
	unsigned char* source = allsource + offset;
	unsigned char* buffer = ri.Hunk_AllocateTempMemory(128 * 128*3 + 18);

    memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = 128;
	buffer[14] = 128;
	buffer[16] = 24;	// pixel size

	// resample from source
	float xScale = glConfig.vidWidth / 512.0f;
	float yScale = glConfig.vidHeight / 384.0f;
	for ( y = 0 ; y < 128 ; y++ )
    {
		for ( x = 0 ; x < 128 ; x++ )
        {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ )
            {
				for ( xx = 0 ; xx < 4 ; xx++ )
                {
					src = source + (3 * glConfig.vidWidth + padlen) * (int)((y*3 + yy) * yScale) +
					      3 * (int) ((x*4 + xx) * xScale);
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

	// gamma correct
	if ( glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, 128 * 128 * 3 );
	}

	ri.FS_WriteFile( checkname, buffer, 128 * 128*3 + 18 );

	ri.Hunk_FreeTempMemory(buffer);
	ri.Hunk_FreeTempMemory(allsource);

	ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
}


static void R_ScreenShotJPEG_f (void)
{
	char checkname[MAX_OSPATH];
	static int	lastNumber = -1;
	qboolean silent;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	}
	else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.jpg", ri.Cmd_Argv( 1 ) );
	}
	else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilenameJPEG( lastNumber, checkname );

			if (!ri.FS_FileExists( checkname )) {
				break; // file doesn't exist
			}
		}

		if ( lastNumber == 10000 ) {
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qtrue );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
}


//============================================================================

const void *RB_TakeVideoFrameCmd( const void *data )
{
	const videoFrameCommand_t* cmd = (const videoFrameCommand_t *)data;
	GLint packAlign;
	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	size_t linelen = cmd->width * 3;

	// Alignment stuff for glReadPixels
	int padwidth = PAD(linelen, packAlign);
	int padlen = padwidth - linelen;
	// AVI line padding
	int avipadwidth = PAD(linelen, AVI_LINE_PADDING);
	int avipadlen = avipadwidth - linelen;

	unsigned char* cBuf = PADP(cmd->captureBuffer, packAlign);

	qglReadPixels(0, 0, cmd->width, cmd->height, GL_RGB, GL_UNSIGNED_BYTE, cBuf);

	size_t memcount = padwidth * cmd->height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(cBuf, memcount);

	if(cmd->motionJpeg)
    {
		memcount = RE_SaveJPGToBuffer(cmd->encodeBuffer, linelen * cmd->height, r_aviMotionJpegQuality->integer, cmd->width, cmd->height, cBuf, padlen);
		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, memcount);
	}
	else
    {
		unsigned char* lineend;

		unsigned char* srcptr = cBuf;
		unsigned char* destptr = cmd->encodeBuffer;
		unsigned char* memend = srcptr + memcount;

		// swap R and B and remove line paddings
		while(srcptr < memend)
        {
			lineend = srcptr + linelen;
			while(srcptr < lineend)
            {
				*destptr++ = srcptr[2];
				*destptr++ = srcptr[1];
				*destptr++ = srcptr[0];
				srcptr += 3;
			}

			memset(destptr, '\0', avipadlen);
			destptr += avipadlen;

			srcptr += padlen;
		}

		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, avipadwidth * cmd->height);
	}

	return (const void *)(cmd + 1);
}

//============================================================================


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
static void R_ScreenShot_f(void)
{
	char	checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) )
    {
		R_LevelShot();
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
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", ri.Cmd_Argv( 1 ) );
	}
	else
    {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilename( lastNumber, checkname );

			if (!ri.FS_FileExists( checkname )) {
				break; // file doesn't exist
			}
		}

		if ( lastNumber >= 9999 ) {
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qfalse );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
}


static void R_SkinList_f( void )
{
	ri.Printf(PRINT_ALL, "------------------\n");
	int	i, j;
	for ( i = 0 ; i < tr.numSkins ; i++ )
    {
		skin_t *skin = tr.skins[i];

		ri.Printf( PRINT_ALL, "%3i:%s (%d surfaces)\n", i, skin->name, skin->numSurfaces );
		for ( j = 0 ; j < skin->numSurfaces ; j++ )
        {
			ri.Printf( PRINT_ALL, " %s = %s\n", skin->surfaces[j].name, skin->surfaces[j].shader->name );
		}
	}
	ri.Printf (PRINT_ALL, "------------------\n");
}



//===========================================================================
//                        SKINS
//===========================================================================


/*
 * CommaParse: This is unfortunate, 
 * the skin files aren't compatable with our normal parsing rules.
 */
static char *CommaParse( char **data_p )
{
	int c = 0, len = 0;
	char *data = *data_p;;
	static char com_token[MAX_TOKEN_CHARS] = {0};

	// make sure incoming data is valid
	if ( !data )
    {
		*data_p = NULL;
		return com_token;
	}

	while( 1 )
    {
		// skip whitespace
		while( (c = *data) <= ' ')
        {
			if( !c )
            {
				break;
			}
			data++;
		}
        
		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32 && c != ',' );

	if (len == MAX_TOKEN_CHARS)
	{
//		ri.Printf (PRINT_DEVELOPER, "Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}



static qhandle_t RE_RegisterSkin( const char *name )
{
    
    skinSurface_t parseSurfaces[MAX_SKIN_SURFACES];
	qhandle_t	hSkin;
	skin_t* skin;
	skinSurface_t* surf;
	union {
		char *c;
		void *v;
	} text;
	char* text_p;
	char* token;
	char surfName[MAX_QPATH];

	if ( !name || !name[0] )
    {
		ri.Printf( PRINT_ALL, "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH )
    {
		ri.Printf( PRINT_ALL, "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}


	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ )
    {
		skin = tr.skins[hSkin];
		if ( !Q_stricmp( skin->name, name ) )
        {
			if( skin->numSurfaces == 0 ) {
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS )
    {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}
	tr.numSkins++;
	
    skin = ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	Q_strncpyz( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces = 0;

	R_IssuePendingRenderCommands();

	// If not a .skin file, load as a single shader
	if( strcmp( name + strlen( name ) - 5, ".skin" ) )
    {
		skin->numSurfaces = 1;
		skin->surfaces = ri.Hunk_Alloc( sizeof( skinSurface_t ), h_low );
		skin->surfaces[0].shader = R_FindShader( name, LIGHTMAP_NONE, qtrue );
		return hSkin;
	}

	// load and parse the skin file
    ri.FS_ReadFile( name, &text.v );
	if ( !text.c ) {
		return 0;
	}

	text_p = text.c;
	while ( text_p && *text_p )
    {
		// get surface name
		token = CommaParse( &text_p );
		Q_strncpyz( surfName, token, sizeof( surfName ) );

		if ( !token[0] ) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr( surfName );

		if ( *text_p == ',' ) {
			text_p++;
		}

		if ( strstr( token, "tag_" ) ) {
			continue;
		}
		
		// parse the shader name
		token = CommaParse( &text_p );

        surf = &parseSurfaces[skin->numSurfaces];
		Q_strncpyz( surf->name, surfName, sizeof( surf->name ) );
		surf->shader = R_FindShader( token, LIGHTMAP_NONE, qtrue );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile( text.v );


	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 ) {
		return 0;		// use default skin
	}

	skin->surfaces = ri.Hunk_Alloc( skin->numSurfaces * sizeof( skinSurface_t ), h_low );
	memcpy( skin->surfaces, parseSurfaces, skin->numSurfaces * sizeof( skinSurface_t ) );

	return hSkin;
}


/*
 * Touch all images to make sure they are resident
 */
static void RE_EndRegistration( void )
{
	if (!ri.Sys_LowPhysicalMemory())
    {
		RB_ShowImages();
	}
}


static void R_DeleteTextures( void )
{
	int	i;

	for(i=0; i<tr.numImages ; i++)
    {
		qglDeleteTextures( 1, &tr.images[i]->texnum );
	}
	memset( tr.images, 0, sizeof( tr.images ) );

	tr.numImages = 0;

	memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	if ( qglActiveTextureARB )
    {
		GL_SelectTexture( 1 );
		qglBindTexture( GL_TEXTURE_2D, 0 );
		GL_SelectTexture( 0 );
		qglBindTexture( GL_TEXTURE_2D, 0 );
	}
    else
    {
		qglBindTexture( GL_TEXTURE_2D, 0 );
	}
}


void RE_Shutdown( qboolean destroyWindow )
{
	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshotJPEG");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("imagelistmaponly");
	ri.Cmd_RemoveCommand("shaderlist");
	ri.Cmd_RemoveCommand("skinlist");
	ri.Cmd_RemoveCommand("gfxinfo");
	ri.Cmd_RemoveCommand("minimize");
//	ri.Cmd_RemoveCommand("modelist");
	ri.Cmd_RemoveCommand("shaderstate");

	if ( tr.registered )
    {
		R_IssuePendingRenderCommands();
		R_DeleteTextures();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if( destroyWindow )
    {
		GLimp_Shutdown();

		memset(&glConfig, 0, sizeof( glConfig ));
		memset(&glState, 0, sizeof( glState ));
	}

	tr.registered = qfalse;

    ri.Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );
}



// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define	MAX_POLYS		600
#define	MAX_POLYVERTS	3000

void R_Init(void)
{
	ri.Printf( PRINT_ALL, "-------- R_Init --------\n" );

	// clear all our internal state
	memset( &tr, 0, sizeof( tr ) );
	memset( &backEnd, 0, sizeof( backEnd ) );
	memset( &tess, 0, sizeof( tess ) );

	if(sizeof(glconfig_t) != 11332)
		ri.Error( ERR_FATAL, "Mod ABI incompatible: sizeof(glconfig_t) == %u != 11332", (unsigned int) sizeof(glconfig_t));

	if( (intptr_t)tess.xyz & 15 )
		ri.Printf( PRINT_WARNING, "tess.xyz not 16 byte aligned\n" );

	memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );

	// init function tables
    int i;
	for( i = 0; i < FUNCTABLE_SIZE; i++ )
    {
		tr.sinTable[i] = sin( 2*M_PI*i / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) );
		tr.squareTable[i] = ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];
        
        if ( i < FUNCTABLE_SIZE / 4 )
            tr.triangleTable[i] = (float) i / ( FUNCTABLE_SIZE / 4 );
        else if (i >= FUNCTABLE_SIZE / 4 && i < FUNCTABLE_SIZE / 2 )
            tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
		else
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
	}

	R_InitFogTable();

	R_NoiseInit();


#if defined( _WIN32 )
	// leilei -  Get some version info first, code torn from quake
	OSVERSIONINFO vinfo;
	vinfo.dwOSVersionInfoSize = sizeof(vinfo);
#endif

	//
	// latched and archived variables
	// temporary latched variables that can only change over a restart
	// temporary variables that can change at any time
	// archived variables that can change at any time
	//

	r_textureMode = ri.Cvar_Get( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE | CVAR_LATCH );

	r_aviMotionJpegQuality = ri.Cvar_Get("r_aviMotionJpegQuality", "90", CVAR_ARCHIVE);
	r_screenshotJpegQuality = ri.Cvar_Get("r_screenshotJpegQuality", "90", CVAR_ARCHIVE);

    r_maxpolys = ri.Cvar_Get( "r_maxpolys", va("%d", MAX_POLYS), 0);
	r_maxpolyverts = ri.Cvar_Get( "r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);


	unsigned char *ptr = ri.Hunk_Alloc(sizeof( *backEndData ) + sizeof(srfPoly_t) * r_maxpolys->integer + sizeof(polyVert_t) * r_maxpolyverts->integer, h_low);

	backEndData = (backEndData_t *) ptr;
	backEndData->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData ));
	backEndData->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData ) + sizeof(srfPoly_t) * r_maxpolys->integer);
	R_InitNextFrame();

	InitOpenGL();

	R_InitBSP();
	R_InitImages();

    R_InitSkins();
	R_ModelInit();
	R_InitFlares();
    R_InitCloudAndSky();
    R_InitCurve();

    R_InitSurface();
    R_InitScene();
    R_InitShade();
    R_InitDLight();
    R_InitBackend();
    R_InitShaders();
    R_InitAnimation();
	R_InitFreeType();
    R_InitMarks();
    R_InitMain();
    R_InitWorld();

    int err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri.Printf( PRINT_ALL, "glGetError() = 0x%x\n", err);


	// make sure all the commands added here are also removed in R_Shutdown
	ri.Cmd_AddCommand( "imagelist", R_ImageList_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri.Cmd_AddCommand( "skinlist", R_SkinList_f );
	ri.Cmd_AddCommand( "modellist", R_Modellist_f );
//	ri.Cmd_AddCommand( "modelist", R_ModeList_f );
	ri.Cmd_AddCommand( "imagelistmaponly", R_ImageListMapOnly_f );
	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	ri.Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
	ri.Cmd_AddCommand( "gfxinfo", GfxInfo_f );
	ri.Cmd_AddCommand( "minimize", GLimp_Minimize );


	ri.Printf( PRINT_ALL, "------- R_Init() finished -------\n\n");
}



void RE_BeginRegistration(glconfig_t *glconfigOut)
{
	R_Init();

	*glconfigOut = glConfig;

	R_IssuePendingRenderCommands();

	tr.viewCluster = -1; // force markleafs to regenerate

	RE_ClearScene();

	tr.registered = qtrue;
}



/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
#ifdef USE_RENDERER_DLOPEN
Q_EXPORT refexport_t* QDECL GetRefAPI( int apiVersion, refimport_t *rimp )
{
#else
refexport_t* GetRefAPI(int apiVersion, refimport_t *rimp)
{
#endif

	ri = *rimp;

	if( apiVersion != REF_API_VERSION )
    {
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion );
		return NULL;
	}

	static refexport_t re;
	memset(&re, 0, sizeof(re));

    
	// the RE_ functions are Renderer Entry points
	re.Shutdown = RE_Shutdown;
	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel = RE_RegisterModel;
	re.RegisterSkin = RE_RegisterSkin;
	re.RegisterShader = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.LoadWorld = RE_LoadWorldMap;
	re.SetWorldVisData = RE_SetWorldVisData;
	re.EndRegistration = RE_EndRegistration;
    re.ClearScene = RE_ClearScene;
    re.AddRefEntityToScene = RE_AddRefEntityToScene;
    re.AddPolyToScene = RE_AddPolyToScene;
	re.LightForPoint = R_LightForPoint;
    re.AddLightToScene = RE_AddLightToScene;
    re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;

	re.RenderScene = RE_RenderScene;
	re.SetColor = RE_SetColor;
	re.DrawStretchPic = RE_StretchPic;
	re.DrawStretchRaw = RE_StretchRaw;
	re.UploadCinematic = RE_UploadCinematic;
    
	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;
	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;
	re.RegisterFont = RE_RegisterFont;
	re.RemapShader = R_RemapShader;
	re.GetEntityToken = R_GetEntityToken;
	re.inPVS = R_inPVS;
	re.TakeVideoFrame = RE_TakeVideoFrame;

	return &re;
}
