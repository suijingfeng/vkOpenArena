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


#include "sdl_icon.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#include "glimp.h"

#ifdef _WIN32
	#include "../SDL2/include/SDL.h"
#else
	#include <SDL2/SDL.h>
#endif

#include "tr_public.h"

extern refimport_t	ri;

typedef enum
{
	RSERR_OK,
	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,
	RSERR_UNKNOWN
} rserr_t;

static cvar_t* r_fullscreen;
cvar_t* r_availableModes;


SDL_Window *SDL_window = NULL;
static SDL_GLContext SDL_glContext = NULL;



static cvar_t* r_allowResize; // make window resizable
static cvar_t* r_customwidth;
static cvar_t* r_customheight;
static cvar_t* r_customPixelAspect;
static cvar_t* r_mode;
static cvar_t* r_stencilbits;
static cvar_t* r_colorbits;
static cvar_t* r_depthbits;

// display refresh rate
static cvar_t* r_displayRefresh;

// not used cvar, keep it for backward compatibility
static cvar_t *r_sdlDriver;
static cvar_t* r_displayIndex;

typedef struct vidmode_s {
	const char *description;
	int width, height;
	float pixelAspect;		// pixel width / height
} vidmode_t;

static const vidmode_t r_vidModes[] = {
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

static void R_ModeList_f( void )
{
	int i;

	ri.Printf(PRINT_ALL,  "\n" );
	for ( i = 0; i < s_numVidModes; i++ )
    {
		ri.Printf(PRINT_ALL, "%s\n", r_vidModes[i].description );
	}
	ri.Printf(PRINT_ALL,  "\n" );
}



/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown( void )
{
    ri.Cmd_RemoveCommand("modelist");
	ri.Cmd_RemoveCommand("in_restart");

    GLimp_DeleteGLContext();
    GLimp_DestroyWindow();

	ri.IN_Shutdown();
	SDL_QuitSubSystem( SDL_INIT_VIDEO );

    ri.Printf(PRINT_ALL, "...Delete GL Window and Context...\n");
}

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize( void )
{
	SDL_MinimizeWindow( SDL_window );
}


void GLimp_LogComment( char *comment )
{
}

void GLimp_DeleteGLContext(void)
{
    SDL_GL_DeleteContext( SDL_glContext );
    SDL_glContext = NULL;
}

void GLimp_DestroyWindow(void)
{
    SDL_DestroyWindow( SDL_window );
    SDL_window = NULL;
}


void* GLimp_GetProcAddress(const char* fun)
{
    return SDL_GL_GetProcAddress(fun);
}


static void GLimp_DetectAvailableModes(void)
{
	int i, j;
	char buf[ MAX_STRING_CHARS ] = { 0 };

	SDL_DisplayMode windowMode;
    
	// If a window exists, note its display index
	if( SDL_window != NULL )
	{
		r_displayIndex->integer = SDL_GetWindowDisplayIndex( SDL_window );
		if( r_displayIndex->integer < 0 )
		{
			ri.Printf(PRINT_ALL, "SDL_GetWindowDisplayIndex() failed: %s\n", SDL_GetError() );
            return;
		}
	}

	int numSDLModes = SDL_GetNumDisplayModes( r_displayIndex->integer );

	if( SDL_GetWindowDisplayMode( SDL_window, &windowMode ) < 0 || numSDLModes <= 0 )
	{
		ri.Printf(PRINT_ALL, "Couldn't get window display mode, no resolutions detected: %s\n", SDL_GetError() );
		return;
	}

	int numModes = 0;
	SDL_Rect* modes = SDL_calloc(numSDLModes, sizeof( SDL_Rect ));
	if ( !modes )
	{
        ////////////////////////////////////
		Com_Error(ERR_FATAL, "Out of memory" );
        ////////////////////////////////////
	}

	for( i = 0; i < numSDLModes; i++ )
	{
		SDL_DisplayMode mode;

		if( SDL_GetDisplayMode( r_displayIndex->integer, i, &mode ) < 0 )
			continue;

		if( !mode.w || !mode.h )
		{
			ri.Printf(PRINT_ALL,  "Display supports any resolution\n" );
			SDL_free( modes );
			return;
		}

		if( windowMode.format != mode.format )
			continue;

		// SDL can give the same resolution with different refresh rates.
		// Only list resolution once.
		for( j = 0; j < numModes; j++ )
		{
			if( (mode.w == modes[ j ].w) && (mode.h == modes[ j ].h) )
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
			ri.Printf(PRINT_ALL,  "Skipping mode %ux%u, buffer too small\n", modes[ i ].w, modes[ i ].h );
	}

	if( *buf )
	{
		buf[ strlen( buf ) - 1 ] = 0;
		ri.Printf(PRINT_ALL, "Available modes: '%s'\n", buf );
		ri.Cvar_Set( "r_availableModes", buf );
	}
	SDL_free( modes );
}


/*
===============
GLimp_SetMode
===============
*/
static int GLimp_SetMode(int mode, qboolean fullscreen, glconfig_t *glCfg, qboolean coreContext)
{
	SDL_DisplayMode desktopMode;

    int samples = 0;

	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;

	ri.Printf(PRINT_ALL,  "Initializing OpenGL display\n");

	if ( r_allowResize->integer )
		flags |= SDL_WINDOW_RESIZABLE;
#ifdef USE_ICON
SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(
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


	//Let SDL_GetDisplayMode handle this
	//SDL_Init(SDL_INIT_VIDEO);
	//ri.Printf(PRINT_ALL, "SDL_GetNumVideoDisplays(): %d\n", SDL_GetNumVideoDisplays());
    SDL_GetNumVideoDisplays();

	int display_mode_count = SDL_GetNumDisplayModes(r_displayIndex->integer);
	if (display_mode_count < 1)
	{
		ri.Printf(PRINT_ALL, "SDL_GetNumDisplayModes failed: %s", SDL_GetError());
	}


    int tmp = SDL_GetDesktopDisplayMode(r_displayIndex->integer, &desktopMode);
	if( (tmp == 0) && (desktopMode.h > 0) )
    {
    	Uint32 f = desktopMode.format;
        ri.Printf(PRINT_ALL, "bpp %i\t%s\t%i x %i, refresh_rate: %dHz\n", SDL_BITSPERPIXEL(f), SDL_GetPixelFormatName(f), desktopMode.w, desktopMode.h, desktopMode.refresh_rate);
    }
    else if (SDL_GetDisplayMode(r_displayIndex->integer, 0, &desktopMode) != 0)
	{
    	//mode = 0: use the first display mode SDL return;
        ri.Printf(PRINT_ALL, "SDL_GetDisplayMode failed: %s\n", SDL_GetError());
	}


	glCfg->refresh_rate = desktopMode.refresh_rate;


	if (mode == -2)
	{
        // use desktop video resolution
        glCfg->vidWidth = desktopMode.w;
        glCfg->vidHeight = desktopMode.h;
        glCfg->windowAspect = (float)desktopMode.w / (float)desktopMode.h;
        glCfg->refresh_rate = desktopMode.refresh_rate;
    }
	else if(mode == -1)
	{
		// custom. 
		glCfg->vidWidth = r_customwidth->integer;
		glCfg->vidHeight = r_customheight->integer;
		glCfg->windowAspect = glCfg->vidWidth / (float)glCfg->vidHeight;
        glCfg->refresh_rate = r_displayRefresh->integer;
	}
	else if((mode >= 0) && (mode < s_numVidModes))
	{
        glCfg->vidWidth = r_vidModes[mode].width;
        glCfg->vidHeight = r_vidModes[mode].height;
        glCfg->windowAspect = (float)glCfg->vidWidth * r_vidModes[mode].pixelAspect / (float)glCfg->vidHeight;
    }
    else
    {
        glCfg->vidWidth = 640;
        glCfg->vidHeight = 480;
        glCfg->windowAspect = (float)glCfg->vidWidth / (float)glCfg->vidHeight;
        glCfg->refresh_rate = 60;
    }
    
    ri.Printf(PRINT_ALL, "Display mode: %d\n", mode);


	// Center window
	if(!fullscreen)
	{
		x = ( desktopMode.w / 2 ) - ( glCfg->vidWidth / 2 );
		y = ( desktopMode.h / 2 ) - ( glCfg->vidHeight / 2 );
	}

	// Destroy existing state if it exists
	if( SDL_glContext != NULL )
	{
        SDL_GL_DeleteContext( SDL_glContext );
		SDL_glContext = NULL;
	}

	if( SDL_window != NULL )
	{
		SDL_GetWindowPosition( SDL_window, &x, &y );
		ri.Printf(PRINT_ALL,  "Existing window at %dx%d before being destroyed\n", x, y );
		SDL_DestroyWindow( SDL_window );
		SDL_window = NULL;
	}

	if( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		flags |= SDL_WINDOW_BORDERLESS;
		glCfg->isFullscreen = qtrue;
	}
	else
	{
		glCfg->isFullscreen = qfalse;
	}


	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0 );

	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );

    SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
	
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );


	SDL_window = SDL_CreateWindow( CLIENT_WINDOW_TITLE, x, y,
						glCfg->vidWidth, glCfg->vidHeight, flags );
	if( SDL_window == NULL )
		Com_Error(ERR_FATAL,"SDL_CreateWindow failed: %s\n", SDL_GetError( ) );
    else{
        ri.Printf(PRINT_ALL, "SDL_CreateWindow successed.\n");
    }
#ifdef USE_ICON
	SDL_SetWindowIcon( SDL_window, icon );
#endif

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

        SDL_glContext = SDL_GL_CreateContext(SDL_window);

		if (SDL_glContext == NULL)
		{
			ri.Printf(PRINT_ALL, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
			
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profileMask);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
		}
		else
		{
			ri.Printf(PRINT_ALL, "SDL_GL_CreateContext succeeded.\n");
		}
	}
	else
	{
		SDL_glContext = NULL;
	}


    if(!SDL_glContext )
    {
        SDL_glContext = SDL_GL_CreateContext( SDL_window );
        if(SDL_glContext == NULL)
        {
            ri.Printf(PRINT_ALL, "SDL_GL_CreateContext failed: %s\n", SDL_GetError( ));
            SDL_DestroyWindow(SDL_window);
            SDL_window = NULL;
        }
        else
        {
            ri.Printf(PRINT_ALL, "Create fixed pipelined Context succeeded.\n");
        }
    }


    #define SWAP_INTERVAL   0
	if( SDL_GL_SetSwapInterval( SWAP_INTERVAL ) == -1 )
	{
		ri.Printf(PRINT_ALL, "SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError( ) );
	}
	int realColorBits[3];
	SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &realColorBits[0] );
	SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &realColorBits[1] );
	SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &realColorBits[2] );
	SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &glCfg->depthBits );
	SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &glCfg->stencilBits );

	glCfg->colorBits = realColorBits[0] + realColorBits[1] + realColorBits[2];

	ri.Printf(PRINT_ALL,  "Using %d color bits, %d depth, %d stencil display.\n",
			glCfg->colorBits, glCfg->depthBits, glCfg->stencilBits );


#ifdef USE_ICON
    SDL_FreeSurface( icon );
#endif

	if( SDL_window )
    {
        GLimp_DetectAvailableModes();
        return RSERR_OK;
    }
    else
	{
		ri.Printf(PRINT_ALL, "Couldn't get a visual\n" );
	}

    return RSERR_INVALID_MODE;
}


/*
 * This routine is responsible for initializing the OS specific portions of OpenGL
 */
void GLimp_Init(glconfig_t *glCfg, qboolean coreContext)
{
    ri.Printf(PRINT_ALL, "-------- Glimp_Init(%d) started! --------\n", coreContext);
    const char *fsstrings[] = {
		"windowed",
		"fullscreen"
	};

	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH );

	r_mode = ri.Cvar_Get( "r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH ); // leilei - -2 is so convenient for modern day PCs
	
	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
	r_fullscreen = ri.Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE );
	r_customwidth = ri.Cvar_Get( "r_customwidth", "1920", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = ri.Cvar_Get( "r_customheight", "1080", CVAR_ARCHIVE | CVAR_LATCH );
	r_customPixelAspect = ri.Cvar_Get( "r_customPixelAspect", "1.78", CVAR_ARCHIVE | CVAR_LATCH );

	r_stencilbits = ri.Cvar_Get( "r_stencilbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorbits = ri.Cvar_Get( "r_colorbits", "32", CVAR_ARCHIVE | CVAR_LATCH );
	r_depthbits = ri.Cvar_Get( "r_depthbits", "24", CVAR_ARCHIVE | CVAR_LATCH );

	r_displayIndex = ri.Cvar_Get( "r_displayIndex", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_displayRefresh = ri.Cvar_Get( "r_displayRefresh", "60", CVAR_LATCH );
	ri.Cvar_CheckRange( r_displayRefresh, 0, 200, qtrue );

	if( ri.Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		ri.Cvar_Set( "r_mode", "3"); // R_MODE_FALLBACK
		ri.Cvar_Set( "r_fullscreen", "0" );
		ri.Cvar_Set( "com_abnormalExit", "0" );
	}

    ri.Cmd_AddCommand("modelist", R_ModeList_f);
	ri.Cmd_AddCommand("in_restart", ri.IN_Restart);
	// Create the window and set up the context
    
	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
        const char *driverName;
		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			ri.Printf(PRINT_ALL, "SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n", SDL_GetError());
		}
        else
        {
    		ri.Printf(PRINT_ALL, " SDL using driver \"%s\"\n", SDL_GetCurrentVideoDriver( ));
        }
        
        driverName = SDL_GetCurrentVideoDriver( );
        ri.Printf(PRINT_ALL,  "SDL using driver \"%s\"\n", driverName );
		ri.Cvar_Set( "r_sdlDriver", driverName );
    }


    
	if( RSERR_OK == GLimp_SetMode(r_mode->integer, r_fullscreen->integer, glCfg, coreContext) )
	{
        goto success;
	}
    else
    {
        ri.Printf(PRINT_ALL, "Setting r_mode=%d, r_fullscreen=%d failed, falling back on r_mode=%d\n",
                r_mode->integer, r_fullscreen->integer, 3 );

        if( RSERR_OK == GLimp_SetMode(3, qfalse, glCfg, coreContext) )
        {
            goto success;
        }
        else
        {
            Com_Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem" );
        }
    }


success:

    // These values force the UI to disable driver selection
    glCfg->driverType = GLDRV_ICD;
    glCfg->hardwareType = GLHW_GENERIC;

    // Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
    glCfg->deviceSupportsGamma = qtrue ;


	ri.Printf(PRINT_ALL,  "MODE: %s, %d x %d, refresh rate: %dhz\n", fsstrings[r_fullscreen->integer == 1], glCfg->vidWidth, glCfg->vidHeight, glCfg->refresh_rate);

    
	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init(SDL_window, 0);
}


/*
 * GLimp_EndFrame() Responsible for doing a swapbuffers
 */
void GLimp_EndFrame( void )
{
	SDL_GL_SwapWindow( SDL_window );

	if( r_fullscreen->modified )
	{
		// qboolean sdlToggled = qfalse;

		// Find out the current state
		int fullscreen = !!( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_FULLSCREEN );

		// Is the state we want different from the current state?
		qboolean needToToggle = !!r_fullscreen->integer != fullscreen;

		if( needToToggle )
		{
			//sdlToggled = SDL_SetWindowFullscreen( SDL_window, r_fullscreen->integer ) >= 0;

			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			//if( !sdlToggled )
			//	ri.Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");

			ri.IN_Init(SDL_window, 0);
		}

		r_fullscreen->modified = qfalse;
	}
}




void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	Uint16 table[3][256];
	int i, j;

	for (i = 0; i < 256; i++)
	{
		table[0][i] = ( ( ( Uint16 ) red[i] ) << 8 ) | red[i];
		table[1][i] = ( ( ( Uint16 ) green[i] ) << 8 ) | green[i];
		table[2][i] = ( ( ( Uint16 ) blue[i] ) << 8 ) | blue[i];
	}


	// enforce constantly increasing
	for (j = 0; j < 3; j++)
	{
		for (i = 1; i < 256; i++)
		{
			if (table[j][i] < table[j][i-1])
				table[j][i] = table[j][i-1];
		}
	}

	if (SDL_SetWindowGammaRamp(SDL_window, table[0], table[1], table[2]) < 0)
	{
		ri.Printf(PRINT_ALL, "SDL_SetWindowGammaRamp() failed: %s\n", SDL_GetError() );
	}
}







//////////////////////////////////////////////////////

void GLimp_RenderThreadWrapper( void *stub ) {}
qboolean GLimp_SpawnRenderThread( void (*function)( void ) )
{
	ri.Printf(PRINT_ALL, "ERROR: SMP support was disabled at compile time\n");
  return qfalse;
}
void *GLimp_RendererSleep( void ) {
  return NULL;
}
void GLimp_FrontEndSleep( void ) {}
void GLimp_WakeRenderer( void *data ) {}
