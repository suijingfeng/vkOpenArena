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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
/*
** WIN_GAMMA.C
*/
#include <assert.h>
#include "win_public.h"
#include "../client/client.h"


static unsigned short s_oldHardwareGamma[3][256];
static qboolean deviceSupportsGamma = qfalse;
/*
** Determines if the underlying hardware supports the Win32 gamma correction API.
*/
qboolean win_checkHardwareGamma( void )
{

	HDC hDC = GetDC( GetDesktopWindow() );
	
	// GetDeviceGammaRamp function gets the gamma ramp
	// on direct color display boards having drivers 
	// that support downloadable gamma ramps in hardware.

	deviceSupportsGamma = (qboolean) GetDeviceGammaRamp( hDC, s_oldHardwareGamma );
	
	ReleaseDC( GetDesktopWindow(), hDC );

	if ( deviceSupportsGamma )
	{
		Com_Printf(" Device support gamma ramp. \n");
		//
		// do a sanity check on the gamma values
		//
		if ( ( HIBYTE( s_oldHardwareGamma[0][255] ) <= HIBYTE( s_oldHardwareGamma[0][0] ) ) ||
				( HIBYTE( s_oldHardwareGamma[1][255] ) <= HIBYTE( s_oldHardwareGamma[1][0] ) ) ||
				( HIBYTE( s_oldHardwareGamma[2][255] ) <= HIBYTE( s_oldHardwareGamma[2][0] ) ) )
		{
			deviceSupportsGamma = qfalse;
			Com_Printf( "WARNING: device has broken gamma support, generated gamma.dat\n" );
		}

		//
		// make sure that we didn't have a prior crash in the game, and if so we need to
		// restore the gamma values to at least a linear value
		//
		if ( ( HIBYTE( s_oldHardwareGamma[0][181] ) == 255 ) )
		{
			int g;

			Com_Printf( "WARNING: suspicious gamma tables, using linear ramp for restoration\n" );

			for ( g = 0; g < 255; g++ )
			{
				s_oldHardwareGamma[0][g] = g << 8;
				s_oldHardwareGamma[1][g] = g << 8;
				s_oldHardwareGamma[2][g] = g << 8;
			}
		}
	}
	return deviceSupportsGamma;
}

/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	unsigned short table[3][256];
	int	ret;

	if ( !deviceSupportsGamma )
	{
		return;
	}

	for (int i = 0; i < 256; ++i )
	{
		table[0][i] = ( ( ( unsigned short ) red[i] ) << 8 ) | red[i];
		table[1][i] = ( ( ( unsigned short ) green[i] ) << 8 ) | green[i];
		table[2][i] = ( ( ( unsigned short ) blue[i] ) << 8 ) | blue[i];
	}

	// Win2K puts this odd restriction on gamma ramps...
	Com_DPrintf( "performing W2K gamma clamp.\n" );
	for (int j = 0 ; j < 3 ; ++j )
	{
		for ( int i = 0 ; i < 128 ; ++i )
		{
			int tmp = (128 + i) << 8;
			if ( table[j][i] > tmp )
			{
				table[j][i] = tmp;
			}
		}
		if ( table[j][127] > 254<<8 )
		{
			table[j][127] = 254<<8;
		}
	}
	
	// enforce constantly increasing
	for (int j = 0 ; j < 3 ; ++j )
	{
		for (int i = 1 ; i < 256 ; ++i )
		{
			if ( table[j][i] < table[j][i-1] )
			{
				table[j][i] = table[j][i-1];
			}
		}
	}

    HDC hdc = GetDC(NULL);
	ret = SetDeviceGammaRamp( hdc, table );
	if ( !ret ) {
		Com_Printf( " SetDeviceGammaRamp failed.\n" );
	}
    ReleaseDC(NULL, hdc);
}



void win_restoreGamma( void )
{
	if ( deviceSupportsGamma )
	{
		// Retrieves a handle to the desktop window. 
		// The desktop window covers the entire screen. 
		// The desktop window is the area on top of which other windows are painted.
		// The return value is a handle to the desktop window.
		//
		// The GetDC function retrieves a handle to a device context (DC) 
		// for the client area of a specified window or for the entire screen.
		// You can use the returned handle in subsequent GDI functions to draw in the DC.
		// The device context is an opaque data structure, whose values are used internally by GDI.
		HDC hDC = GetDC( GetDesktopWindow() );
		// The SetDeviceGammaRamp function sets the gamma ramp
		// on direct color display boards having drivers that 
		// support downloadable gamma ramps in hardware.
		//
		// LPVOID lpRamp: Pointer to a buffer containing the gamma ramp to be set.
		// The gamma ramp is specified in three arrays of 256 WORD elements each, 
		// which contain the mapping between RGB values in the frame buffer and 
		// digital-analog-converter (DAC ) values. The sequence of the arrays is
		// red, green, blue. The RGB values must be stored in the most significant
		// bits of each WORD to increase DAC independence.

		// Direct color display modes do not use color lookup tables and are usually
		// 16, 24, or 32 bit. Not all direct color video boards support loadable gamma
		// ramps. SetDeviceGammaRamp succeeds only for devices with drivers that support
		// downloadable gamma ramps in hardware.
		SetDeviceGammaRamp( hDC, s_oldHardwareGamma );
		ReleaseDC( GetDesktopWindow(), hDC );
	}
}

