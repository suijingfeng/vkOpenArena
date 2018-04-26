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

#ifdef USE_LOCAL_HEADERS
#include "../SDL2/include/SDL.h"
#else
#include <SDL.h>
#endif

#include "../qcommon/q_shared.h"
#include "sdl_glimp.h"
#include "sdl_icon.h"
#include "qgl.h"

#include "tr_public.h"
extern refimport_t ri;
extern glconfig_t glConfig;



typedef enum
{
    RSERR_OK,
    RSERR_INVALID_FULLSCREEN,
    RSERR_INVALID_MODE,
    RSERR_UNKNOWN
} rserr_t;

static cvar_t* r_allowResize; // make window resizable
static cvar_t* r_customwidth;
static cvar_t* r_customheight;
static cvar_t* r_customPixelAspect;

static cvar_t* r_noborder;
static cvar_t* r_fullscreen;
static cvar_t* r_ignorehwgamma;		// overrides hardware gamma capabilities

static cvar_t* r_stencilbits;
static cvar_t* r_depthbits;
static cvar_t* r_colorbits;

static cvar_t* r_ext_compiled_vertex_array;


//
// cvars
//
//extern cvar_t *r_ext_multisample;
										// 0 = use framebuffer depth
										// 16 = use 16-bit textures
										// 32 = use 32-bit textures
										// all else = error


cvar_t* r_ext_texture_filter_anisotropic;
cvar_t* r_ext_max_anisotropy;
cvar_t* r_ext_compressed_textures;
cvar_t	*r_drawBuffer;
SDL_Window *SDL_window = NULL;


int qglMajorVersion, qglMinorVersion;
int qglesMajorVersion, qglesMinorVersion;

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);

void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
void (APIENTRYP qglUnlockArraysEXT) (void);

#define GLE(ret, name, ...) name##proc * qgl##name;
QGL_1_1_PROCS;
QGL_DESKTOP_1_1_PROCS;
QGL_ES_1_1_PROCS;
QGL_3_0_PROCS;
#undef GLE


void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256])
{
	uint16_t table[3][256];
	int i;

	for (i = 0; i < 256; i++)
	{
		table[0][i] = ( ( ( Uint16 ) red[i] ) << 8 ) | red[i];
		table[1][i] = ( ( ( Uint16 ) green[i] ) << 8 ) | green[i];
		table[2][i] = ( ( ( Uint16 ) blue[i] ) << 8 ) | blue[i];
	}

    if (SDL_SetWindowGammaRamp(SDL_window, table[0], table[1], table[2]) < 0)
    {
        ri.Printf( PRINT_ALL, "SDL_SetWindowGammaRamp() failed: %s\n", SDL_GetError() );
    }
            
    ri.Printf( PRINT_ALL, " GLimp_SetGamma() is called. \n");
}


void GLimp_Shutdown( void )
{
	ri.IN_Shutdown();

	SDL_QuitSubSystem( SDL_INIT_VIDEO );

    ri.Printf(PRINT_ALL, " GLimp_Shutdown. \n");
}


void GLimp_Minimize( void )
{
	SDL_MinimizeWindow( SDL_window );
}

void GLimp_LogComment( char *comment )
{
}

/*
===============
GLimp_GetProcAddresses

Get addresses for OpenGL functions.
===============
*/
static qboolean GLimp_GetProcAddresses( void )
{
	qboolean success = qtrue;
	const char *version;

#ifdef __SDL_NOGETPROCADDR__
#define GLE( ret, name, ... ) qgl##name = gl#name;
#else
#define GLE( ret, name, ... ) qgl##name = (name##proc *) SDL_GL_GetProcAddress("gl" #name); \
	if ( qgl##name == NULL ) { \
		ri.Printf( PRINT_ALL, "ERROR: Missing OpenGL function %s\n", "gl" #name ); \
		success = qfalse; \
	}
#endif

	// OpenGL 1.0 and OpenGL ES 1.0
	GLE(const GLubyte *, GetString, GLenum name)

	if ( !qglGetString )
        ri.Error(ERR_FATAL, "glGetString is NULL\n" );


	version = (const char *)qglGetString( GL_VERSION );

	if( !version )
        ri.Error(ERR_FATAL, "GL_VERSION is NULL\n" );

	if ( Q_stricmpn( "OpenGL ES", version, 9 ) == 0 ) {
		char profile[6]; // ES, ES-CM, or ES-CL
		sscanf( version, "OpenGL %5s %d.%d", profile, &qglesMajorVersion, &qglesMinorVersion );
		// common lite profile (no floating point) is not supported
		if ( Q_stricmp( profile, "ES-CL" ) == 0 ) {
			qglesMajorVersion = 0;
			qglesMinorVersion = 0;
		}
	} else {
		sscanf( version, "%d.%d", &qglMajorVersion, &qglMinorVersion );
	}

	if ( QGL_VERSION_ATLEAST( 1, 1 ) ) {
		QGL_1_1_PROCS;
		QGL_DESKTOP_1_1_PROCS;
	} else if ( qglesMajorVersion == 1 && qglesMinorVersion >= 1 ) {
		// OpenGL ES 1.1 (2.0 is not backward compatible)
		QGL_1_1_PROCS;
		QGL_ES_1_1_PROCS;
		// error so this doesn't segfault due to NULL desktop GL functions being used
		ri.Error( ERR_FATAL, "Unsupported OpenGL Version: %s\n", version );
	} else {
		ri.Error( ERR_FATAL, "Unsupported OpenGL Version: %s\n", version );
	}

	if ( QGL_VERSION_ATLEAST( 3, 0 ) || QGLES_VERSION_ATLEAST( 3, 0 ) ) {
		QGL_3_0_PROCS;
	}

#undef GLE

	return success;
}

/*
===============
GLimp_ClearProcAddresses

Clear addresses for OpenGL functions.
===============
*/
static void GLimp_ClearProcAddresses( void )
{
#define GLE( ret, name, ... ) qgl##name = NULL;

	qglMajorVersion = 0;
	qglMinorVersion = 0;
	qglesMajorVersion = 0;
	qglesMinorVersion = 0;

	QGL_1_1_PROCS;
	QGL_DESKTOP_1_1_PROCS;
	QGL_ES_1_1_PROCS;
	QGL_3_0_PROCS;

#undef GLE
}



static void GLimp_DetectAvailableModes(void)
{
	int i, j;
	char buf[ MAX_STRING_CHARS ] = { 0 };
	int numSDLModes;

	int numModes = 0;

	SDL_DisplayMode windowMode;
	int display = SDL_GetWindowDisplayIndex( SDL_window );
	if( display < 0 )
	{
		ri.Printf( PRINT_WARNING, "Couldn't get window display index, no resolutions detected: %s\n", SDL_GetError() );
		return;
	}
	numSDLModes = SDL_GetNumDisplayModes( display );

	if( SDL_GetWindowDisplayMode( SDL_window, &windowMode ) < 0 || numSDLModes <= 0 )
	{
		ri.Printf( PRINT_WARNING, "Couldn't get window display mode, no resolutions detected: %s\n", SDL_GetError() );
		return;
	}

	SDL_Rect *modes = SDL_calloc(numSDLModes, sizeof( SDL_Rect ));
	if ( !modes )
	{
		ri.Error( ERR_FATAL, "Out of memory" );
	}

	for( i = 0; i < numSDLModes; i++ )
	{
		SDL_DisplayMode mode;

		if( SDL_GetDisplayMode( display, i, &mode ) < 0 )
			continue;

		if( !mode.w || !mode.h )
		{
			ri.Printf( PRINT_ALL, "Display supports any resolution\n" );
			SDL_free( modes );
			return;
		}

		if( windowMode.format != mode.format )
			continue;

		// SDL can give the same resolution with different refresh rates.
		// Only list resolution once.
		for( j = 0; j < numModes; j++ )
		{
			if( mode.w == modes[ j ].w && mode.h == modes[ j ].h )
				break;
		}

		if( j != numModes )
			continue;

		modes[ numModes ].w = mode.w;
		modes[ numModes ].h = mode.h;
		numModes++;
	}

	for( i = 0; i < numModes; i++ )
	{
		const char *newModeString = va( "%ux%u ", modes[ i ].w, modes[ i ].h );

		if( strlen( newModeString ) < (int)sizeof( buf ) - strlen( buf ) )
			Q_strcat( buf, sizeof( buf ), newModeString );
		else
			ri.Printf( PRINT_WARNING, "Skipping mode %ux%u, buffer too small\n", modes[ i ].w, modes[ i ].h );
	}

	if( *buf )
	{
		buf[ strlen( buf ) - 1 ] = 0;
		ri.Printf( PRINT_ALL, " Available modes: '%s'\n", buf );
		ri.Cvar_Set( "r_availableModes", buf );
	}
/*  
        SDL_DisplayMode mode;

        mode.format = SDL_PIXELFORMAT_RGB24;
        mode.w = glConfig.vidWidth;
        mode.h = glConfig.vidHeight;
        mode.driverdata = NULL;

        if( SDL_SetWindowDisplayMode( SDL_window, &mode ) < 0 )
        {
            ri.Printf( PRINT_ALL, "SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError( ) );
        }
*/
	SDL_free( modes );
}


static int GLimp_SetMode(qboolean fullscreen, qboolean noborder, qboolean coreContext)
{
	int perChannelColorBits;
	int colorBits, depthBits, stencilBits;
	int samples = 0;
	int i = 0;
	SDL_Surface *icon = NULL;
	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	SDL_DisplayMode desktopMode;
	int display = 0;
	int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;
    static SDL_GLContext SDL_glContext = NULL;
	ri.Printf( PRINT_ALL, " Initializing OpenGL display\n");

	if ( r_allowResize->integer )
		flags |= SDL_WINDOW_RESIZABLE;

#ifdef USE_ICON
	icon = SDL_CreateRGBSurfaceFrom(
			(void *)CLIENT_WINDOW_ICON.pixel_data,
			CLIENT_WINDOW_ICON.width,
			CLIENT_WINDOW_ICON.height,
			CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
			CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
			0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
			);
#endif

	// If a window exists, note its display index
	if( SDL_window != NULL )
	{
		display = SDL_GetWindowDisplayIndex( SDL_window );
		if( display < 0 )
		{
			ri.Printf( PRINT_ALL, "SDL_GetWindowDisplayIndex() failed: %s\n", SDL_GetError() );
		}
	}

	if( SDL_GetDesktopDisplayMode( display, &desktopMode ) == 0 )
	{
        glConfig.refresh_rate = desktopMode.refresh_rate;
	}
	else
	{
		memset( &desktopMode, 0, sizeof( SDL_DisplayMode ) );

		ri.Printf( PRINT_ALL, "Cannot determine display aspect, assuming 1.333\n" );
	}

    // use desktop video resolution
    if( desktopMode.h > 0 )
    {
        glConfig.vidWidth = desktopMode.w;
        glConfig.vidHeight = desktopMode.h;
    }
    else
    {
    
        glConfig.vidWidth = r_customwidth->integer;
        glConfig.vidHeight = r_customheight->integer;
        glConfig.windowAspect = r_customPixelAspect->value;
        
        /*
        glConfig.vidWidth = 640;
        glConfig.vidHeight = 480;
        */
        // ri.Printf( PRINT_ALL, "Cannot determine display resolution, assuming 640x480\n" );
        ri.Printf( PRINT_ALL, "Cannot determine display resolution, assuming r_customwidth x r_customheight\n" );

    }

    glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
    


	// Destroy existing state if it exists
	if( SDL_glContext != NULL )
	{
		GLimp_ClearProcAddresses();
		SDL_GL_DeleteContext( SDL_glContext );
		SDL_glContext = NULL;
	}

	if( SDL_window != NULL )
	{
		SDL_GetWindowPosition( SDL_window, &x, &y );
		ri.Printf( PRINT_DEVELOPER, "Existing window at %dx%d before being destroyed\n", x, y );
		SDL_DestroyWindow( SDL_window );
		SDL_window = NULL;
	}

	if( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		glConfig.isFullscreen = qtrue;
	}
	else
	{
		if( noborder )
			flags |= SDL_WINDOW_BORDERLESS;

		glConfig.isFullscreen = qfalse;
	}

	colorBits = r_colorbits->value;
	if ((!colorBits) || (colorBits >= 32))
		colorBits = 24;

	if (!r_depthbits->value)
		depthBits = 24;
	else
		depthBits = r_depthbits->value;

	stencilBits = r_stencilbits->value;

	for (i = 0; i < 16; i++)
	{
		int testColorBits, testDepthBits, testStencilBits;

		// 0 - default
		// 1 - minus colorBits
		// 2 - minus depthBits
		// 3 - minus stencil
		if ((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
				case 2 :
					if (colorBits == 24)
						colorBits = 16;
					break;
				case 1 :
					if (depthBits == 24)
						depthBits = 16;
					else if (depthBits == 16)
						depthBits = 8;
				case 3 :
					if (stencilBits == 24)
						stencilBits = 16;
					else if (stencilBits == 16)
						stencilBits = 8;
			}
		}

		testColorBits = colorBits;
		testDepthBits = depthBits;
		testStencilBits = stencilBits;

		if ((i % 4) == 3)
		{ // reduce colorBits
			if (testColorBits == 24)
				testColorBits = 16;
		}

		if ((i % 4) == 2)
		{ // reduce depthBits
			if (testDepthBits == 24)
				testDepthBits = 16;
			else if (testDepthBits == 16)
				testDepthBits = 8;
		}

		if ((i % 4) == 1)
		{ // reduce stencilBits
			if (testStencilBits == 24)
				testStencilBits = 16;
			else if (testStencilBits == 16)
				testStencilBits = 8;
			else
				testStencilBits = 0;
		}

		if (testColorBits == 24)
			perChannelColorBits = 8;
		else
			perChannelColorBits = 4;


		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, testDepthBits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, testStencilBits );

		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );

        SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
		
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
        
        SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );

		if( ( SDL_window = SDL_CreateWindow( CLIENT_WINDOW_TITLE, x, y,	glConfig.vidWidth, glConfig.vidHeight, flags ) ) == NULL )
		{
			ri.Printf( PRINT_ALL, "SDL_CreateWindow failed: %s\n", SDL_GetError( ) );
			continue;
		}

		if( fullscreen )
		{
			SDL_DisplayMode mode;

			switch( testColorBits )
			{
				case 16: mode.format = SDL_PIXELFORMAT_RGB565; break;
				case 24: mode.format = SDL_PIXELFORMAT_RGB24;  break;
				default: ri.Printf( PRINT_DEVELOPER, "testColorBits is %d, can't fullscreen\n", testColorBits ); continue;
			}

			mode.w = glConfig.vidWidth;
			mode.h = glConfig.vidHeight;
			mode.refresh_rate = glConfig.refresh_rate;
			mode.driverdata = NULL;

			if( SDL_SetWindowDisplayMode( SDL_window, &mode ) < 0 )
			{
				ri.Printf( PRINT_ALL, "SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError( ) );
				continue;
			}
		}

		SDL_SetWindowIcon( SDL_window, icon );



		if (coreContext)
		{
			int profileMask, majorVersion, minorVersion;
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profileMask);
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion);
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion);

			ri.Printf(PRINT_ALL, "Trying to get an OpenGL 3.2 core context\n");
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
			if ((SDL_glContext = SDL_GL_CreateContext(SDL_window)) == NULL)
			{
				ri.Printf(PRINT_ALL, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
				ri.Printf(PRINT_ALL, "Reverting to default context\n");

				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profileMask);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
			}
			else
			{
				const char *renderer;

				ri.Printf(PRINT_ALL, "SDL_GL_CreateContext succeeded.\n");

				if ( GLimp_GetProcAddresses() )
				{
					renderer = (const char *)qglGetString(GL_RENDERER);
				}
				else
				{
					ri.Printf( PRINT_ALL, "GLimp_GetProcAddresses() failed for OpenGL 3.2 core context\n" );
					renderer = NULL;
				}

				if (!renderer || (strstr(renderer, "Software Renderer") || strstr(renderer, "Software Rasterizer")))
				{
					if ( renderer )
						ri.Printf(PRINT_ALL, "GL_RENDERER is %s, rejecting context\n", renderer);

					GLimp_ClearProcAddresses();
					SDL_GL_DeleteContext(SDL_glContext);
					SDL_glContext = NULL;

					SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profileMask);
					SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
					SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
				}
			}
		}
		else
		{
			SDL_glContext = NULL;
		}

		if ( !SDL_glContext )
		{
			if( ( SDL_glContext = SDL_GL_CreateContext( SDL_window ) ) == NULL )
			{
				ri.Printf( PRINT_DEVELOPER, "SDL_GL_CreateContext failed: %s\n", SDL_GetError( ) );
				continue;
			}

			if ( !GLimp_GetProcAddresses() )
			{
				ri.Printf( PRINT_ALL, "GLimp_GetProcAddresses() failed\n" );
				GLimp_ClearProcAddresses();
				SDL_GL_DeleteContext( SDL_glContext );
				SDL_glContext = NULL;
				SDL_DestroyWindow( SDL_window );
				SDL_window = NULL;
				continue;
			}
		}

		qglClearColor( 0, 0, 0, 1 );
		qglClear( GL_COLOR_BUFFER_BIT );
		SDL_GL_SwapWindow( SDL_window );

		if( SDL_GL_SetSwapInterval( 0 ) == -1 )
		{
			ri.Printf( PRINT_DEVELOPER, "SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError( ) );
		}

		glConfig.colorBits = testColorBits;
		glConfig.depthBits = testDepthBits;
		glConfig.stencilBits = testStencilBits;

		ri.Printf( PRINT_ALL, " Using %d color bits, %d depth, %d stencil display.\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
		break;
	}

	SDL_FreeSurface( icon );

	if( !SDL_window )
	{
		ri.Printf( PRINT_ALL, "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}

	GLimp_DetectAvailableModes();

	return RSERR_OK;
}



static qboolean GLimp_StartDriverAndSetMode(qboolean fullscreen, qboolean noborder, qboolean gl3Core)
{
	rserr_t err;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		const char *driverName;

		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			ri.Printf( PRINT_ALL, "SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n", SDL_GetError());
			return qfalse;
		}

		driverName = SDL_GetCurrentVideoDriver( );
		ri.Printf( PRINT_ALL, " SDL using driver \"%s\"\n", driverName );
	}

	err = GLimp_SetMode(fullscreen, noborder, gl3Core);

	switch ( err )
	{
		case RSERR_INVALID_FULLSCREEN:
			ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
			return qfalse;
		case RSERR_INVALID_MODE:
			ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode \n");
			return qfalse;
		default:
			break;
	}

	return qtrue;
}



static void GLimp_InitExtensions( void )
{
	ri.Printf( PRINT_ALL, "\n...Initializing OpenGL extensions\n" );

	glConfig.textureCompression = TC_NONE;

	// GL_EXT_texture_compression_s3tc
    if( SDL_GL_ExtensionSupported( "GL_ARB_texture_compression" ) && SDL_GL_ExtensionSupported( "GL_EXT_texture_compression_s3tc" ) )
	{
		if ( r_ext_compressed_textures->value )
		{
			glConfig.textureCompression = TC_S3TC_ARB;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n" );
	}

	// GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
	if(glConfig.textureCompression == TC_NONE)
	{
		if( SDL_GL_ExtensionSupported( "GL_S3_s3tc" ) )
		{
			if ( r_ext_compressed_textures->value )
			{
				glConfig.textureCompression = TC_S3TC;
				ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
			}
			else
			{
				ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...GL_S3_s3tc not found\n" );
		}
	}


	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( SDL_GL_ExtensionSupported( "EXT_texture_env_add" ) )
	{
		if ( SDL_GL_ExtensionSupported( "GL_EXT_texture_env_add" ) )
		{
			glConfig.textureEnvAddAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( SDL_GL_ExtensionSupported( "GL_ARB_multitexture" ) )
	{

        qglMultiTexCoord2fARB = SDL_GL_GetProcAddress( "glMultiTexCoord2fARB" );
        qglActiveTextureARB = SDL_GL_GetProcAddress( "glActiveTextureARB" );
        qglClientActiveTextureARB = SDL_GL_GetProcAddress( "glClientActiveTextureARB" );

        if ( qglActiveTextureARB )
        {
            GLint glint = 0;
            qglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glint );
            glConfig.numTextureUnits = (int) glint;
            if ( glConfig.numTextureUnits > 1 )
            {
                ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
            }
            else
            {
                qglMultiTexCoord2fARB = NULL;
                qglActiveTextureARB = NULL;
                qglClientActiveTextureARB = NULL;
                ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
            }
        }

	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
	}

	// GL_EXT_compiled_vertex_array
	if ( SDL_GL_ExtensionSupported( "GL_EXT_compiled_vertex_array" ) )
	{
		if ( r_ext_compiled_vertex_array->value )
		{
			ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
			qglLockArraysEXT = ( void ( APIENTRY * )( GLint, GLint ) ) SDL_GL_GetProcAddress( "glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) SDL_GL_GetProcAddress( "glUnlockArraysEXT" );
			if (!qglLockArraysEXT || !qglUnlockArraysEXT)
			{
				ri.Error (ERR_FATAL, "bad getprocaddress");
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
	}

	if ( SDL_GL_ExtensionSupported( "GL_EXT_texture_filter_anisotropic" ) )
	{
		if ( r_ext_texture_filter_anisotropic->integer )
        {
            int maxAnisotropy = 0;
            char target_string[4] = {0};
			qglGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint *)&maxAnisotropy );
			if ( maxAnisotropy <= 0 ) {
				ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not properly supported!\n" );
				maxAnisotropy = 0;
			}
			else
			{
                sprintf(target_string, "%d", maxAnisotropy);
				ri.Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic (max: %i)\n", maxAnisotropy );
                ri.Cvar_Set( "r_ext_max_anisotropy", target_string);

			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
	}
}


#define R_MODE_FALLBACK 3 // 640 * 480

/*
 * This routine is responsible for initializing the OS specific portions of OpenGL
 */
void GLimp_Init(qboolean coreContext)
{
    ri.Printf(PRINT_ALL, "\n-------- Glimp_Init() started! --------\n");
    const char *fsstrings[] = {
		"windowed",
		"fullscreen"
	};

	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH );


	r_ignorehwgamma = ri.Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_fullscreen = ri.Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE );
	r_noborder = ri.Cvar_Get("r_noborder", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_customwidth = ri.Cvar_Get( "r_customwidth", "1920", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = ri.Cvar_Get( "r_customheight", "1080", CVAR_ARCHIVE | CVAR_LATCH );
	r_customPixelAspect = ri.Cvar_Get( "r_customPixelAspect", "1.78", CVAR_ARCHIVE | CVAR_LATCH );

	r_colorbits = ri.Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH );	// use desktop
	r_stencilbits = ri.Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH );
	r_depthbits = ri.Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH );


	r_ext_compressed_textures = ri.Cvar_Get( "r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compiled_vertex_array = ri.Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_filter_anisotropic = ri.Cvar_Get( "r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_max_anisotropy = ri.Cvar_Get( "r_ext_max_anisotropy", "2", CVAR_ARCHIVE | CVAR_LATCH );
	
    r_drawBuffer = ri.Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );

    if( ri.Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		ri.Cvar_Set( "r_fullscreen", "0" );
		ri.Cvar_Set( "r_centerWindow", "0" );
		ri.Cvar_Set( "com_abnormalExit", "0" );
	}


	// Create the window and set up the context
	if(GLimp_StartDriverAndSetMode(r_fullscreen->integer, r_noborder->integer, coreContext))
		goto success;

	if(GLimp_StartDriverAndSetMode(r_fullscreen->integer, qfalse, coreContext))
		goto success;

	// Finally, try the default screen resolution
	
    ri.Printf( PRINT_ALL, "Setting mode failed, trying fall back mode.\n");

    if(GLimp_StartDriverAndSetMode(qfalse, qfalse, coreContext))
        goto success;


	// Nothing worked, give up
	ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem" );

success:

	// These values force the UI to disable driver selection
	glConfig.driverType = GLDRV_ICD;

 	// Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
	glConfig.deviceSupportsGamma = (!r_ignorehwgamma->integer) && (SDL_SetWindowBrightness( SDL_window, 1.0f ) >= 0);

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, (char *) qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (char *) qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz( glConfig.version_string, (char *) qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
    
	// manually create extension list if using OpenGL 3
	if( qglGetStringi )
	{
		int i, numExtensions, extensionLength, listLength;
		const char *extension;

		qglGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
		listLength = 0;

		for ( i = 0; i < numExtensions; i++ )
		{
			extension = (char *) qglGetStringi( GL_EXTENSIONS, i );
			extensionLength = strlen( extension );

			if ( ( listLength + extensionLength + 1 ) >= sizeof( glConfig.extensions_string ) )
				break;

			if ( i > 0 ) {
				Q_strcat( glConfig.extensions_string, sizeof( glConfig.extensions_string ), " " );
				listLength++;
			}

			Q_strcat( glConfig.extensions_string, sizeof( glConfig.extensions_string ), extension );
			listLength += extensionLength;
		}
	}
	else
	{
		Q_strncpyz( glConfig.extensions_string, (char *)qglGetString(GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );
	}

    // initialize extensions
	GLimp_InitExtensions();

	ri.Printf( PRINT_ALL, " MODE: %s, %d x %d, refresh rate: %dhz\n", fsstrings[r_fullscreen->integer == 1], glConfig.vidWidth, glConfig.vidHeight, glConfig.refresh_rate);


	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init(SDL_window);
#if defined( _WIN32 ) && defined( USE_CONSOLE_WINDOW )
		// leilei - hide our console window
	Sys_ShowConsole(0, 0 );
#endif
    ri.Printf(PRINT_ALL, "\n-------- Glimp_Init() finished! --------\n");
}


/*
 * GLimp_EndFrame() Responsible for doing a swapbuffers
 */
void GLimp_EndFrame( void )
{
	// don't flip if drawing to front buffer
	if( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		SDL_GL_SwapWindow( SDL_window );
	}

	if( r_fullscreen->modified )
	{
		qboolean sdlToggled = qfalse;

		// Find out the current state
		int fullscreen = !!( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_FULLSCREEN );

		// Is the state we want different from the current state?
		qboolean needToToggle = !!r_fullscreen->integer != fullscreen;

		if( needToToggle )
		{
			sdlToggled = SDL_SetWindowFullscreen( SDL_window, r_fullscreen->integer ) >= 0;

			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if( !sdlToggled )
				ri.Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");

			ri.IN_Restart();
		}

		r_fullscreen->modified = qfalse;
	}
}

