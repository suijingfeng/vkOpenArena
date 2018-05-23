/*
===========================================================================
Copyright (C) 2010 James Canete (use.less01@gmail.com)

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
// tr_subs.c - common function replacements for modular renderer

#include "tr_shared.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>


const float colorBlack[4]	= {0, 0, 0, 1};
const float	colorRed[4]	    = {1, 0, 0, 1};
const float	colorGreen[4]	= {0, 1, 0, 1};
const float	colorBlue[4]	= {0, 0, 1, 1};
const float	colorYellow[4]	= {1, 1, 0, 1};
const float	colorMagenta[4] = {1, 0, 1, 1};
const float	colorCyan[4]	= {0, 1, 1, 1};
const float	colorWhite[4]	= {1, 1, 1, 1};
const float	colorLtGrey[4]	= {0.75, 0.75, 0.75, 1};
const float	colorMdGrey[4]	= {0.5, 0.5, 0.5, 1};
const float	colorDkGrey[4]	= {0.25, 0.25, 0.25, 1};


#define NOISE_SIZE 256
#define NOISE_MASK ( NOISE_SIZE - 1 )


static float s_noise_table[NOISE_SIZE];
static int s_noise_perm[NOISE_SIZE];

#define VAL( a )                s_noise_perm[ ( a ) & ( NOISE_MASK )]
#define INDEX( x, y, z, t )     VAL( x + VAL( y + VAL( z + VAL( t ) ) ) )
#define LERP( a, b, w )     ( ( a ) * ( 1.0f - ( w ) ) + ( b ) * ( w ) )





char* SkipPath(char *pathname)
{
	char *last = pathname;
    char c;
    do{
        c = *pathname;
    	if (c == '/')
		    last = pathname+1;
        pathname++;
    }while(c);

	return last;
}


void stripExtension(const char *in, char *out, int destsize)
{
	const char *dot = strrchr(in, '.');
    const char *slash = strrchr(in, '/');


	if ((dot != NULL) && ( (slash < dot) || (slash == NULL) ) )
    {
        int len = dot-in+1;
        if(len <= destsize)
            destsize = len;
        else
		    fprintf( stderr, "stripExtension: dest size not enough!\n");
    }

    if(in != out)
    	strncpy(out, in, destsize-1);
	
    out[destsize-1] = '\0';
}


const char* getExtension( const char *name )
{
	const char* dot = strrchr(name, '.');
    const char* slash = strrchr(name, '/');

	if ((dot != NULL) && ((slash == NULL) || (slash < dot) ))
		return dot + 1;
	else
		return "";
}


static inline float GetNoiseValue( int x, int y, int z, int t )
{
	int index = INDEX( ( int ) x, ( int ) y, ( int ) z, ( int ) t );

	return s_noise_table[index];
}



//
////////////////////////  noise 
//
void R_NoiseInit( void )
{
	int i;

	for ( i = 0; i < NOISE_SIZE; i++ )
	{
		s_noise_table[i] = ( float ) ( ( ( rand() / ( float ) RAND_MAX ) * 2.0 - 1.0 ) );
		s_noise_perm[i] = ( unsigned char ) ( rand() / ( float ) RAND_MAX * 255 );
	}
}


float R_NoiseGet4f( float x, float y, float z, float t )
{
	int ix = ( int ) floor( x );
	int iy = ( int ) floor( y );
	int iz = ( int ) floor( z );
	int it = ( int ) floor( t );
    
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

/////// extra math.h //////

unsigned short int FloatToHalf(float in)
{
	union f32_u f32;
	union f16_u f16;

	f32.f = in;

	f16.pack.exponent = CLAMP((int)(f32.pack.exponent) - 112, 0, 31);
	f16.pack.fraction = f32.pack.fraction >> 13;
	f16.pack.sign     = f32.pack.sign;

	return f16.ui;
}

float HalfToFloat(unsigned short int in)
{
	union f32_u f32;
	union f16_u f16;

	f16.ui = in;

	f32.pack.exponent = (int)(f16.pack.exponent) + 112;
	f32.pack.fraction = f16.pack.fraction << 13;
	f32.pack.sign = f16.pack.sign;

	return f32.f;
}

int NextPowerOfTwo(int in)
{
	int out;

	for (out = 1; out < in; out <<= 1)
		;

	return out;
}


////////////////// matrix and linear algerba //////////////////////
//void Matrix4Multiply(const float a[16], const float b[16], float out[16]);
//note: out = A x B
void MatrixMultiply4x4(const float A[16], const float B[16], float out[16])
{
    out[0] = A[0]*B[0] + A[1]*B[4] + A[2]*B[8] + A[3]*B[12];
    out[1] = A[0]*B[1] + A[1]*B[5] + A[2]*B[9] + A[3]*B[13];
    out[2] = A[0]*B[2] + A[1]*B[6] + A[2]*B[10] + A[3]*B[14];
    out[3] = A[0]*B[3] + A[1]*B[7] + A[2]*B[11] + A[3]*B[15];

    out[4] = A[4]*B[0] + A[5]*B[4] + A[6]*B[8] + A[7]*B[12];
    out[5] = A[4]*B[1] + A[5]*B[5] + A[6]*B[9] + A[7]*B[13];
    out[6] = A[4]*B[2] + A[5]*B[6] + A[6]*B[10] + A[7]*B[14];
    out[7] = A[4]*B[3] + A[5]*B[7] + A[6]*B[11] + A[7]*B[15];

    out[8] = A[8]*B[0] + A[9]*B[4] + A[10]*B[8] + A[11]*B[12];
    out[9] = A[8]*B[1] + A[9]*B[5] + A[10]*B[9] + A[11]*B[13];
    out[10] = A[8]*B[2] + A[9]*B[6] + A[10]*B[10] + A[11]*B[14];
    out[11] = A[8]*B[3] + A[9]*B[7] + A[10]*B[11] + A[11]*B[15];

    out[12] = A[12]*B[0] + A[13]*B[4] + A[14]*B[8] + A[15]*B[12];
    out[13] = A[12]*B[1] + A[13]*B[5] + A[14]*B[9] + A[15]*B[13];
    out[14] = A[12]*B[2] + A[13]*B[6] + A[14]*B[10] + A[15]*B[14];
    out[15] = A[12]*B[3] + A[13]*B[7] + A[14]*B[11] + A[15]*B[15];
}

void Mat4Transform(const float mat[16], const float v[4], float out[4])
{
	out[0] = mat[0] * v[0] + mat[4] * v[1] + mat[ 8] * v[2] + mat[12] * v[3];
	out[1] = mat[1] * v[0] + mat[5] * v[1] + mat[ 9] * v[2] + mat[13] * v[3];
	out[2] = mat[2] * v[0] + mat[6] * v[1] + mat[10] * v[2] + mat[14] * v[3];
	out[3] = mat[3] * v[0] + mat[7] * v[1] + mat[11] * v[2] + mat[15] * v[3];
}

void Mat4Translation(const float vec[3], float out[16])
{
	out[ 0] = 1.0f; out[ 4] = 0.0f; out[ 8] = 0.0f; out[12] = vec[0];
	out[ 1] = 0.0f; out[ 5] = 1.0f; out[ 9] = 0.0f; out[13] = vec[1];
	out[ 2] = 0.0f; out[ 6] = 0.0f; out[10] = 1.0f; out[14] = vec[2];
	out[ 3] = 0.0f; out[ 7] = 0.0f; out[11] = 0.0f; out[15] = 1.0f;
}


void Mat4Copy( const float in[16], float out[16] )
{
    /*
	out[ 0] = in[ 0]; out[ 4] = in[ 4]; out[ 8] = in[ 8]; out[12] = in[12]; 
	out[ 1] = in[ 1]; out[ 5] = in[ 5]; out[ 9] = in[ 9]; out[13] = in[13]; 
	out[ 2] = in[ 2]; out[ 6] = in[ 6]; out[10] = in[10]; out[14] = in[14]; 
	out[ 3] = in[ 3]; out[ 7] = in[ 7]; out[11] = in[11]; out[15] = in[15];
    */
    memcpy(out, in, 16*4);
}


void Mat4Zero(float out[16])
{
	out[ 0] = 0.0f; out[ 4] = 0.0f; out[ 8] = 0.0f; out[12] = 0.0f;
	out[ 1] = 0.0f; out[ 5] = 0.0f; out[ 9] = 0.0f; out[13] = 0.0f;
	out[ 2] = 0.0f; out[ 6] = 0.0f; out[10] = 0.0f; out[14] = 0.0f;
	out[ 3] = 0.0f; out[ 7] = 0.0f; out[11] = 0.0f; out[15] = 0.0f;
}


void Mat4Identity(float out[16])
{
	out[ 0] = 1.0f; out[ 4] = 0.0f; out[ 8] = 0.0f; out[12] = 0.0f;
	out[ 1] = 0.0f; out[ 5] = 1.0f; out[ 9] = 0.0f; out[13] = 0.0f;
	out[ 2] = 0.0f; out[ 6] = 0.0f; out[10] = 1.0f; out[14] = 0.0f;
	out[ 3] = 0.0f; out[ 7] = 0.0f; out[11] = 0.0f; out[15] = 1.0f;
}


int Mat4Compare(const float a[16], const float b[16])
{
	if( (a[0]!=b[0])   || (a[1]!=b[1])   || (a[2]!=b[2])   || (a[3]!=b[3])   ||
        (a[4]!=b[4])   || (a[5]!=b[5])   || (a[6]!=b[6])   || (a[7]!=b[7])   ||
        (a[8]!=b[8])   || (a[9]!=b[9])   || (a[10]!=b[10]) || (a[11]!=b[11]) ||
        (a[12]!=b[12]) || (a[13]!=b[13]) || (a[14]!=b[14]) || (a[15] != b[15]) )
    {       
        return 1;
    }
    
    return 0;
}


void Mat4Ortho(float left, float right, float bottom, float top, float znear, float zfar, float out[16])
{
	out[ 0] = 2.0f / (right - left); out[ 4] = 0.0f;                  out[ 8] = 0.0f;                  out[12] = -(right + left) / (right - left);
	out[ 1] = 0.0f;                  out[ 5] = 2.0f / (top - bottom); out[ 9] = 0.0f;                  out[13] = -(top + bottom) / (top - bottom);
	out[ 2] = 0.0f;                  out[ 6] = 0.0f;                  out[10] = 2.0f / (zfar - znear); out[14] = -(zfar + znear) / (zfar - znear);
	out[ 3] = 0.0f;                  out[ 7] = 0.0f;                  out[11] = 0.0f;                  out[15] = 1.0f;
}


void Mat4View(const float axes[3][3], const float origin[3], float out[16])
{
	out[0]  = axes[0][0];
	out[1]  = axes[1][0];
	out[2]  = axes[2][0];
	out[3]  = 0;

	out[4]  = axes[0][1];
	out[5]  = axes[1][1];
	out[6]  = axes[2][1];
	out[7]  = 0;

	out[8]  = axes[0][2];
	out[9]  = axes[1][2];
	out[10] = axes[2][2];
	out[11] = 0;

	out[12] = -VecDot(origin, axes[0]);
	out[13] = -VecDot(origin, axes[1]);
	out[14] = -VecDot(origin, axes[2]);
	out[15] = 1;
}



void Mat4SimpleInverse( const float in[16], float out[16])
{
	float v[3];
	float invSqrLen;
 
	VecCopy(in + 0, v);
	invSqrLen = 1.0f / VecDot(v, v);
    VecScale(v, invSqrLen, v);
	out[ 0] = v[0]; out[ 4] = v[1]; out[ 8] = v[2]; out[12] = -VecDot(v, &in[12]);

	VecCopy(in + 4, v);
	invSqrLen = 1.0f / VecDot(v, v);
    VecScale(v, invSqrLen, v);
	out[ 1] = v[0]; out[ 5] = v[1]; out[ 9] = v[2]; out[13] = -VecDot(v, &in[12]);

	VecCopy(in + 8, v);
	invSqrLen = 1.0f / VecDot(v, v);
    VecScale(v, invSqrLen, v);
	out[ 2] = v[0]; out[ 6] = v[1]; out[10] = v[2]; out[14] = -VecDot(v, &in[12]);

	out[ 3] = 0.0f; out[ 7] = 0.0f; out[11] = 0.0f; out[15] = 1.0f;
}


void Mat4Dump( const float in[16] )
{
	printf("%3.5f %3.5f %3.5f %3.5f\n", in[ 0], in[ 4], in[ 8], in[12]);
	printf("%3.5f %3.5f %3.5f %3.5f\n", in[ 1], in[ 5], in[ 9], in[13]);
	printf("%3.5f %3.5f %3.5f %3.5f\n", in[ 2], in[ 6], in[10], in[14]);
	printf("%3.5f %3.5f %3.5f %3.5f\n", in[ 3], in[ 7], in[11], in[15]);
}


int SpheresIntersect(float origin1[3], float radius1, float origin2[3], float radius2)
{
	float radiusSum = radius1 + radius2;
	float diff[3];
	
	VecSub(origin1, origin2, diff);

	if (VecDot(diff, diff) <= radiusSum * radiusSum)
	{
		return 1;
	}

	return 0;
}


void BoundingSphereOfSpheres(float origin1[3], float radius1, float origin2[3], float radius2, float origin3[3], float *radius3)
{
	float diff[3];

	VecScale(origin1, 0.5f, origin3);
	VecMA(origin3, 0.5f, origin2, origin3);

	VecSub(origin1, origin2, diff);
	*radius3 = VectorLength(diff) * 0.5f + MAX(radius1, radius2);
}
