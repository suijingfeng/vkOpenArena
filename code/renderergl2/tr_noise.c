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

#include "tr_common.h"

#define NOISE_SIZE 256


static float s_noise_table[NOISE_SIZE];
static int s_noise_perm[NOISE_SIZE];

#define VAL( a )                s_noise_perm[ ( a ) & ( NOISE_SIZE - 1 )]
#define INDEX( x, y, z, t )     VAL( x + VAL( y + VAL( z + VAL( t ) ) ) )


static float GetNoiseValue( int x, int y, int z, int t )
{
	return s_noise_table[INDEX( x, y, z, t )];
}


void R_NoiseInit( void )
{
	int i;

	for ( i = 0; i < NOISE_SIZE; i++ )
	{
		s_noise_table[i] = ( ( rand() / ( float ) RAND_MAX ) * 2.0 - 1.0 ) ;
		s_noise_perm[i] = ( unsigned char ) ( rand() / ( float ) RAND_MAX * 255 );
	}
}


float R_NoiseGet4f( float x, float y, float z, float t )
{
	int ix = floor( x );
	int iy = floor( y );
	int iz = floor( z );
	int it = floor( t );
    
    float fx = x - ix;
	float fy = y - iy;
	float fz = z - iz;
	float ft = t - it;

    float value[2];

    int i;
	for ( i = 0; i < 2; i++ )
	{   
	    float front[4];
		front[0] = GetNoiseValue( ix, iy, iz, it + i );
		front[1] = GetNoiseValue( ix+1, iy, iz, it + i );
		front[2] = GetNoiseValue( ix, iy+1, iz, it + i );
		front[3] = GetNoiseValue( ix+1, iy+1, iz, it + i );

	    float back[4];
		back[0] = GetNoiseValue( ix, iy, iz + 1, it + i );
		back[1] = GetNoiseValue( ix+1, iy, iz + 1, it + i );
		back[2] = GetNoiseValue( ix, iy+1, iz + 1, it + i );
		back[3] = GetNoiseValue( ix+1, iy+1, iz + 1, it + i );


        float lerp1 = LERP( front[0], front[1], fx );
        float lerp2 = LERP( front[2], front[3], fx );
        float fvalue = LERP( lerp1, lerp2, fy );
		
        lerp1 = LERP( back[0], back[1], fx );
        lerp2 = LERP( back[2], back[3], fx );
        float bvalue = LERP(lerp1, lerp2, fy);

		value[i] = LERP( fvalue, bvalue, fz );
	}

	float finalvalue = LERP( value[0], value[1], ft );

	return finalvalue;
}
