#include "../client/client.h"

/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
	const char * description;
	int         width, height;
} vidmode_t;

static const vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",		320,	240},
	{ "Mode  1: 400x300",		400,	300},
	{ "Mode  2: 512x384",		512,	384},
	{ "Mode  3: 640x480",		640,	480},
	{ "Mode  4: 800x600",		800,	600},
	{ "Mode  5: 960x720",		960,	720},
	{ "Mode  6: 1024x768",		1024,	768},
	{ "Mode  7: 1152x864",		1152,	864},
	{ "Mode  8: 1280x1024",		1280,	1024},
	{ "Mode  9: 1600x1200",		1600,	1200},
	{ "Mode 10: 2048x1536",		2048,	1536},
	{ "Mode 11: 856x480 (wide)",856,	480},
	{ "Mode 12: 1280x720",		1280,	720},
	{ "Mode 13: 1280x768",		1280,	768},
	{ "Mode 14: 1280x800",		1280,	800},
	{ "Mode 15: 1280x960",		1280,	960},
	{ "Mode 16: 1360x768",		1360,	768},
	{ "Mode 17: 1366x768",		1366,	768}, // yes there are some out there on that extra 6
	{ "Mode 18: 1360x1024",		1360,	1024},
	{ "Mode 19: 1400x1050",		1400,	1050},
	{ "Mode 20: 1400x900",		1400,	900},
	{ "Mode 21: 1600x900",		1600,	900},
	{ "Mode 22: 1680x1050",		1680,	1050},
	{ "Mode 23: 1920x1080",		1920,	1080},
	{ "Mode 24: 1920x1200",		1920,	1200},
	{ "Mode 25: 1920x1440",		1920,	1440},
	{ "Mode 26: 2560x1080",		2560,	1080},
	{ "Mode 27: 2560x1600",		2560,	1600},
	{ "Mode 28: 3840x2160 (4K)",3840,	2160}
};

const static int s_numVidModes = (sizeof(r_vidModes) / sizeof(r_vidModes[0]));


// always returu a valid mode ...
int R_GetModeInfo(int * const width, int * const height, int mode, const int desktopWidth, const int desktopHeight)
{
	// corse error handle,
	if (mode < 0 || mode >= s_numVidModes)
	{
		// just 640 * 480;
		*width = 640;
		*height = 480;
		return 3;
	}

	int i = mode;
	for ( ; i > 0; --i)
	{
		const vidmode_t * pVm = &r_vidModes[i];
		if (pVm->width >= desktopWidth || pVm->height >= desktopHeight)
		{
			continue;
		}

		*width = pVm->width;
		*height = pVm->height;
		return i;
	}

	if (i == 0)
	{
		*width = 640;
		*height = 480;
		return 3;
	}

	return i;
}


void R_ListDisplayMode_f( void )
{
	Com_Printf( "\n" );
	for (int i = 0; i < s_numVidModes; ++i)
	{
		Com_Printf( "%s \n", r_vidModes[i].description);
	}
	Com_Printf( "\n" );
}

void win_InitDisplayModel(void)
{
	Cmd_AddCommand("printDisplayModes", R_ListDisplayMode_f);
}

void win_EndDisplayModel(void)
{
	Cmd_RemoveCommand("printDisplayModes");
}