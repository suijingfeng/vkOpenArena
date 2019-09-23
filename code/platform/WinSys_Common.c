#include "../client/client.h"
#include "sys_public.h"
#include "win_public.h"

static cvar_t* r_customwidth;
static cvar_t* r_customheight;

typedef struct vidmode_s
{
    const char *description;
    int         width, height;
} vidmode_t;


static const vidmode_t s_vidModes[] =
{
	{ "Mode  0: 320x240",		320,	240 },
	{ "Mode  1: 400x300",		400,	300 },
	{ "Mode  2: 512x384",		512,	384 },
	{ "Mode  3: 640x480 (480p)",640,	480 },
	{ "Mode  4: 800x600",		800,	600 },
	{ "Mode  5: 960x720",		960,	720 },
	{ "Mode  6: 1024x768",		1024,	768 },
	{ "Mode  7: 1152x864",		1152,	864 },
	{ "Mode  8: 1280x1024",		1280,	1024 },
	{ "Mode  9: 1600x1200",		1600,	1200 },
	{ "Mode 10: 2048x1536",		2048,	1536 },
	{ "Mode 11: 856x480",		856,	480 }, // Q3 MODES END HERE AND EXTENDED MODES BEGIN
	{ "Mode 12: 1280x720 (720p)",1280,	720 },
	{ "Mode 13: 1280x768",		1280,	768 },
	{ "Mode 14: 1280x800",		1280,	800 },
	{ "Mode 15: 1280x960",		1280,	960 },
	{ "Mode 16: 1360x768",		1360,	768 },
	{ "Mode 17: 1366x768",		1366,	768 }, // yes there are some out there on that extra 6
	{ "Mode 18: 1360x1024",		1360,	1024 },
	{ "Mode 19: 1400x1050",		1400,	1050 },
	{ "Mode 20: 1400x900",		1400,	900 },
	{ "Mode 21: 1600x900",		1600,	900 },
	{ "Mode 22: 1680x1050",		1680,	1050 },
	{ "Mode 23: 1920x1080 (1080p)",1920,1080 },
	{ "Mode 24: 1920x1200",		1920,	1200 },
	{ "Mode 25: 1920x1440",		1920,	1440 },
    { "Mode 26: 2560x1080",		2560,	1080 },
    { "Mode 27: 2560x1600",		2560,	1600 },
	{ "Mode 28: 3840x2160 (4K)",3840,	2160 }
};

static const int s_numVidModes = 29;



qboolean CL_GetModeInfo( int *width, int *height, int mode, int dw, int dh, qboolean fullscreen )
{
	if ( mode < -2 )
		return qfalse;

	if ( mode >= s_numVidModes )
		return qfalse;

	// fix unknown desktop resolution
	if ( mode == -2 && (dw == 0 || dh == 0) )
		mode = 3;

	if ( mode == -2 ) { // desktop resolution
		*width = dw;
		*height = dh;
	} else if ( mode == -1 ) { // custom resolution
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
	} else { // predefined resolution
		*width  = s_vidModes[ mode ].width;
		*height = s_vidModes[ mode ].height;
	}
	return qtrue;
}


void printDisplayMode_f( void )
{
	int i;

	Com_Printf("\n" );
	for ( i = 0; i < s_numVidModes; ++i )
	{
		Com_Printf( "%s\n", s_vidModes[i].description );
	}
	Com_Printf("\n" );
}


int R_GetDisplayMode(int mode, uint32_t * const pWidth, uint32_t * const pHeight)
{
	// corse error handle ...
	if (mode < 0 || mode >= s_numVidModes)
	{
		// just 640 * 480;
		*pWidth = 640;
		*pHeight = 480;
	}
	else
	{
		// should be fullscreen
		const vidmode_t * pVm = &s_vidModes[mode];
		*pWidth = pVm->width;
		*pHeight = pVm->height;
	}
	
	return mode;	
}



void WinSys_ConstructDislayModes(void)
{
	r_customwidth = Cvar_Get( "r_customwidth", "1920", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = Cvar_Get( "r_customheight", "1080", CVAR_ARCHIVE | CVAR_LATCH );

	Cmd_AddCommand( "printDisplayMode", printDisplayMode_f );

}

void WinSys_DestructDislayModes(void)
{
    
	Cmd_RemoveCommand( "printDisplayMode" );

}


void FileSys_Logging(const char * const comment )
{
/*   
	if ( glw_state.log_fp )
	{
		fprintf( glw_state.log_fp, "%s", comment );
	}
*/
}
