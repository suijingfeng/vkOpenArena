#include "local.h"


extern glwstate_t glw_state;
////////////////////////////////////////////////////

/*****************************************************************************/

qboolean BuildGammaRampTable( unsigned char *red, unsigned char *green, unsigned char *blue, int gammaRampSize, unsigned short table[3][4096] )
{
	int i, j;
	int shift;

	switch ( gammaRampSize )
	{
		case 256: shift = 0; break;
		case 512: shift = 1; break;
		case 1024: shift = 2; break;
		case 2048: shift = 3; break;
		case 4096: shift = 4; break;
		default:
			Com_Printf( "Unsupported gamma ramp size: %d\n", gammaRampSize );
		return qfalse;
	};
	
	int m = gammaRampSize / 256;
	int m1 = 256 / m;

	for ( i = 0; i < 256; i++ ) {
		for ( j = 0; j < m; j++ ) {
			table[0][i*m+j] = (unsigned short)(red[i] << 8)   | (m1 * j) | ( red[i] >> shift );
			table[1][i*m+j] = (unsigned short)(green[i] << 8) | (m1 * j) | ( green[i] >> shift );
			table[2][i*m+j] = (unsigned short)(blue[i] << 8)  | (m1 * j) | ( blue[i] >> shift );
		}
	}

	// enforce constantly increasing
	for ( j = 0 ; j < 3 ; j++ ) {
		for ( i = 1 ; i < gammaRampSize ; i++ ) {
			if ( table[j][i] < table[j][i-1] ) {
				table[j][i] = table[j][i-1];
			}
		}
	}

	return qtrue;
}


void InitGammaImpl( glconfig_t *config )
{
    config->deviceSupportsGamma = qfalse;

	if ( glw_state.randr_gamma )
	{
		Com_Printf( "...using xrandr gamma extension\n" );
		config->deviceSupportsGamma = qtrue;
		return;
	}

	if ( glw_state.vidmode_gamma )
	{
		Com_Printf( "...using vidmode gamma extension\n" );
		config->deviceSupportsGamma = qtrue;
		return;
	}
}


/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void SetGammaImpl( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	if ( glw_state.randr_gamma )
	{
		RandR_SetGamma( red, green, blue );
		return;
	}
	
	if ( glw_state.vidmode_gamma )
	{
		VidMode_SetGamma( red, green, blue );
		return;
	}
}

