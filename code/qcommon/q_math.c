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
//
// q_math.c -- stateless support routines that are included in each code module

// Some of the vector functions are static inline in q_shared.h. q3asm
// doesn't understand static functions though, so we only want them in
// one file. That's what this is about.
#include "q_shared.h"

vec3_t	vec3_origin = {0,0,0};
vec3_t	axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };


vec4_t		colorBlack	= {0, 0, 0, 1};
vec4_t		colorRed	= {1, 0, 0, 1};
vec4_t		colorGreen	= {0, 1, 0, 1};
vec4_t		colorBlue	= {0, 0, 1, 1};
vec4_t		colorYellow	= {1, 1, 0, 1};
vec4_t		colorMagenta= {1, 0, 1, 1};
vec4_t		colorCyan	= {0, 1, 1, 1};
vec4_t		colorWhite	= {1, 1, 1, 1};
vec4_t		colorLtGrey	= {0.75, 0.75, 0.75, 1};
vec4_t		colorMdGrey	= {0.5, 0.5, 0.5, 1};
vec4_t		colorDkGrey	= {0.25, 0.25, 0.25, 1};

const vec4_t g_color_table[9] =
{
	{0.0, 0.0, 0.0, 1.0},
	{1.0, 0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 0.0, 1.0, 1.0},
	{0.0, 1.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
    {1.00f, 0.43f, 0.00f, 1.00f},
};

//////////////////////////////////////////////////////////////////////////
const static vec3_t bytedirs[NUMVERTEXNORMALS] =
{
{-0.525731f, 0.000000f, 0.850651f}, {-0.442863f, 0.238856f, 0.864188f}, 
{-0.295242f, 0.000000f, 0.955423f}, {-0.309017f, 0.500000f, 0.809017f}, 
{-0.162460f, 0.262866f, 0.951056f}, {0.000000f, 0.000000f, 1.000000f}, 
{0.000000f, 0.850651f, 0.525731f}, {-0.147621f, 0.716567f, 0.681718f}, 
{0.147621f, 0.716567f, 0.681718f}, {0.000000f, 0.525731f, 0.850651f}, 
{0.309017f, 0.500000f, 0.809017f}, {0.525731f, 0.000000f, 0.850651f}, 
{0.295242f, 0.000000f, 0.955423f}, {0.442863f, 0.238856f, 0.864188f}, 
{0.162460f, 0.262866f, 0.951056f}, {-0.681718f, 0.147621f, 0.716567f}, 
{-0.809017f, 0.309017f, 0.500000f},{-0.587785f, 0.425325f, 0.688191f}, 
{-0.850651f, 0.525731f, 0.000000f},{-0.864188f, 0.442863f, 0.238856f}, 
{-0.716567f, 0.681718f, 0.147621f},{-0.688191f, 0.587785f, 0.425325f}, 
{-0.500000f, 0.809017f, 0.309017f}, {-0.238856f, 0.864188f, 0.442863f}, 
{-0.425325f, 0.688191f, 0.587785f}, {-0.716567f, 0.681718f, -0.147621f}, 
{-0.500000f, 0.809017f, -0.309017f}, {-0.525731f, 0.850651f, 0.000000f}, 
{0.000000f, 0.850651f, -0.525731f}, {-0.238856f, 0.864188f, -0.442863f}, 
{0.000000f, 0.955423f, -0.295242f}, {-0.262866f, 0.951056f, -0.162460f}, 
{0.000000f, 1.000000f, 0.000000f}, {0.000000f, 0.955423f, 0.295242f}, 
{-0.262866f, 0.951056f, 0.162460f}, {0.238856f, 0.864188f, 0.442863f}, 
{0.262866f, 0.951056f, 0.162460f}, {0.500000f, 0.809017f, 0.309017f}, 
{0.238856f, 0.864188f, -0.442863f},{0.262866f, 0.951056f, -0.162460f}, 
{0.500000f, 0.809017f, -0.309017f},{0.850651f, 0.525731f, 0.000000f}, 
{0.716567f, 0.681718f, 0.147621f}, {0.716567f, 0.681718f, -0.147621f}, 
{0.525731f, 0.850651f, 0.000000f}, {0.425325f, 0.688191f, 0.587785f}, 
{0.864188f, 0.442863f, 0.238856f}, {0.688191f, 0.587785f, 0.425325f}, 
{0.809017f, 0.309017f, 0.500000f}, {0.681718f, 0.147621f, 0.716567f}, 
{0.587785f, 0.425325f, 0.688191f}, {0.955423f, 0.295242f, 0.000000f}, 
{1.000000f, 0.000000f, 0.000000f}, {0.951056f, 0.162460f, 0.262866f}, 
{0.850651f, -0.525731f, 0.000000f},{0.955423f, -0.295242f, 0.000000f}, 
{0.864188f, -0.442863f, 0.238856f}, {0.951056f, -0.162460f, 0.262866f}, 
{0.809017f, -0.309017f, 0.500000f}, {0.681718f, -0.147621f, 0.716567f}, 
{0.850651f, 0.000000f, 0.525731f}, {0.864188f, 0.442863f, -0.238856f}, 
{0.809017f, 0.309017f, -0.500000f}, {0.951056f, 0.162460f, -0.262866f}, 
{0.525731f, 0.000000f, -0.850651f}, {0.681718f, 0.147621f, -0.716567f}, 
{0.681718f, -0.147621f, -0.716567f},{0.850651f, 0.000000f, -0.525731f}, 
{0.809017f, -0.309017f, -0.500000f}, {0.864188f, -0.442863f, -0.238856f}, 
{0.951056f, -0.162460f, -0.262866f}, {0.147621f, 0.716567f, -0.681718f}, 
{0.309017f, 0.500000f, -0.809017f}, {0.425325f, 0.688191f, -0.587785f}, 
{0.442863f, 0.238856f, -0.864188f}, {0.587785f, 0.425325f, -0.688191f}, 
{0.688191f, 0.587785f, -0.425325f}, {-0.147621f, 0.716567f, -0.681718f}, 
{-0.309017f, 0.500000f, -0.809017f}, {0.000000f, 0.525731f, -0.850651f}, 
{-0.525731f, 0.000000f, -0.850651f}, {-0.442863f, 0.238856f, -0.864188f}, 
{-0.295242f, 0.000000f, -0.955423f}, {-0.162460f, 0.262866f, -0.951056f}, 
{0.000000f, 0.000000f, -1.000000f}, {0.295242f, 0.000000f, -0.955423f}, 
{0.162460f, 0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f}, 
{-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f}, 
{0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f}, 
{0.147621f, -0.716567f, -0.681718f}, {0.000000f, -0.525731f, -0.850651f}, 
{0.309017f, -0.500000f, -0.809017f}, {0.442863f, -0.238856f, -0.864188f}, 
{0.162460f, -0.262866f, -0.951056f}, {0.238856f, -0.864188f, -0.442863f}, 
{0.500000f, -0.809017f, -0.309017f}, {0.425325f, -0.688191f, -0.587785f}, 
{0.716567f, -0.681718f, -0.147621f}, {0.688191f, -0.587785f, -0.425325f}, 
{0.587785f, -0.425325f, -0.688191f}, {0.000000f, -0.955423f, -0.295242f}, 
{0.000000f, -1.000000f, 0.000000f}, {0.262866f, -0.951056f, -0.162460f}, 
{0.000000f, -0.850651f, 0.525731f}, {0.000000f, -0.955423f, 0.295242f}, 
{0.238856f, -0.864188f, 0.442863f}, {0.262866f, -0.951056f, 0.162460f}, 
{0.500000f, -0.809017f, 0.309017f}, {0.716567f, -0.681718f, 0.147621f}, 
{0.525731f, -0.850651f, 0.000000f}, {-0.238856f, -0.864188f, -0.442863f}, 
{-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f}, 
{-0.850651f, -0.525731f, 0.000000f}, {-0.716567f, -0.681718f, -0.147621f}, 
{-0.716567f, -0.681718f, 0.147621f}, {-0.525731f, -0.850651f, 0.000000f}, 
{-0.500000f, -0.809017f, 0.309017f}, {-0.238856f, -0.864188f, 0.442863f}, 
{-0.262866f, -0.951056f, 0.162460f}, {-0.864188f, -0.442863f, 0.238856f}, 
{-0.809017f, -0.309017f, 0.500000f}, {-0.688191f, -0.587785f, 0.425325f}, 
{-0.681718f, -0.147621f, 0.716567f}, {-0.442863f, -0.238856f, 0.864188f}, 
{-0.587785f, -0.425325f, 0.688191f}, {-0.309017f, -0.500000f, 0.809017f}, 
{-0.147621f, -0.716567f, 0.681718f}, {-0.425325f, -0.688191f, 0.587785f}, 
{-0.162460f, -0.262866f, 0.951056f}, {0.442863f, -0.238856f, 0.864188f}, 
{0.162460f, -0.262866f, 0.951056f}, {0.309017f, -0.500000f, 0.809017f}, 
{0.147621f, -0.716567f, 0.681718f}, {0.000000f, -0.525731f, 0.850651f}, 
{0.425325f, -0.688191f, 0.587785f}, {0.587785f, -0.425325f, 0.688191f}, 
{0.688191f, -0.587785f, 0.425325f}, {-0.955423f, 0.295242f, 0.000000f}, 
{-0.951056f, 0.162460f, 0.262866f}, {-1.000000f, 0.000000f, 0.000000f}, 
{-0.850651f, 0.000000f, 0.525731f}, {-0.955423f, -0.295242f, 0.000000f}, 
{-0.951056f, -0.162460f, 0.262866f}, {-0.864188f, 0.442863f, -0.238856f}, 
{-0.951056f, 0.162460f, -0.262866f}, {-0.809017f, 0.309017f, -0.500000f}, 
{-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f}, 
{-0.809017f, -0.309017f, -0.500000f}, {-0.681718f, 0.147621f, -0.716567f}, 
{-0.681718f, -0.147621f, -0.716567f}, {-0.850651f, 0.000000f, -0.525731f}, 
{-0.688191f, 0.587785f, -0.425325f}, {-0.587785f, 0.425325f, -0.688191f}, 
{-0.425325f, 0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f}, 
{-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f}
};


// this isn't a real cheap function to call!
int DirToByte( vec3_t dir )
{
	int	best = 0;
	float bestd = 0;
    int i;

	if ( !dir ) {
		return 0;
	}

	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
	    float d = DotProduct (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir( int b, vec3_t dir )
{
	if ( b < 0 || b >= NUMVERTEXNORMALS )
    {
		VectorCopy( vec3_origin, dir );
		return;
	}
	VectorCopy(bytedirs[b], dir);
}

//==============================================================


ID_INLINE float Q_random( int *seed )
{
	*seed = (69069 * (*seed) + 1);
	return ( *seed & 0xffff ) / (float)0x10000;
}

ID_INLINE float Q_crandom( int *seed )
{
	return 2.0 * ( Q_random( seed ) - 0.5 );
}

//=======================================================





/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/

void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point,	float degrees )
{
    float d;
    float k[3];

    float rad = DEG2RAD( degrees );
    float cos_th = cos( rad );
    float sin_th = sin( rad );


    VectorNormalize2(dir, k);

    d = (1 - cos_th) * (point[0] * k[0] + point[1] * k[1] + point[2] * k[2]);

	CrossProduct(k, point, dst);

    dst[0] *= sin_th;
    dst[1] *= sin_th;
    dst[2] *= sin_th;

    dst[0] += cos_th * point[0] + d * k[0]; 
    dst[1] += cos_th * point[1] + d * k[1]; 
    dst[2] += cos_th * point[2] + d * k[2]; 
}



void RotateAroundDirection( vec3_t axis[3], float yaw )
{
	// create an arbitrary axis[1] 
	PerpendicularVector( axis[1], axis[0] );

	// rotate it around axis[0] by yaw
	if ( yaw )
    {
		vec3_t	temp;

		VectorCopy( axis[1], temp );
		RotatePointAroundVector( axis[1], axis[0], temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( axis[0], axis[1], axis[2] );
}



void vectoangles( const vec3_t value1, vec3_t angles )
{
	float	forward;
	float	yaw, pitch;
	
	if( (value1[1] == 0) && (value1[0] == 0) )
    {
		yaw = 0;
		if( value1[2] > 0 )
			pitch = 90;
		else
			pitch = 270;
	}
	else
    {
		if ( value1[0] ) {
			yaw = ( atan2 ( value1[1], value1[0] ) * 180 / M_PI );
		}
		else if ( value1[1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrt ( value1[0]*value1[0] + value1[1]*value1[1] );
		pitch = ( atan2(value1[2], forward) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}


/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] )
{
	vec3_t	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[0], right, axis[2] );
	VectorSubtract( vec3_origin, right, axis[1] );
}


void AxisClear( vec3_t axis[3] )
{
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}


ID_INLINE void AxisCopy( vec3_t in[3], vec3_t out[3] )
{
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}



/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float	AngleSubtract( float a1, float a2 ) {
	float	a;

	a = a1 - a2;
	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}


ID_INLINE void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 )
{
	v3[0] = AngleSubtract( v1[0], v2[0] );
	v3[1] = AngleSubtract( v1[1], v2[1] );
	v3[2] = AngleSubtract( v1[2], v2[2] );
}


ID_INLINE float	AngleMod(float a)
{
	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}


//============================================================




/*
=================
RadiusFromBounds
=================
*/
ID_INLINE float RadiusFromBounds( const vec3_t mins, const vec3_t maxs )
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
    {
		float a = fabs( mins[i] );
		float b = fabs( maxs[i] );
		corner[i] = a > b ? a : b;
	}

	return VectorLength (corner);
}





ID_INLINE qboolean BoundsIntersect(const vec3_t mins, const vec3_t maxs,
		const vec3_t mins2, const vec3_t maxs2)
{
	if ( maxs[0] < mins2[0] ||
		maxs[1] < mins2[1] ||
		maxs[2] < mins2[2] ||
		mins[0] > maxs2[0] ||
		mins[1] > maxs2[1] ||
		mins[2] > maxs2[2])
	{
		return qfalse;
	}

	return qtrue;
}

ID_INLINE qboolean BoundsIntersectSphere(const vec3_t mins, const vec3_t maxs, const vec3_t origin, vec_t radius)
{
	if ( origin[0] - radius > maxs[0] ||
		origin[0] + radius < mins[0] ||
		origin[1] - radius > maxs[1] ||
		origin[1] + radius < mins[1] ||
		origin[2] - radius > maxs[2] ||
		origin[2] + radius < mins[2])
	{
		return qfalse;
	}

	return qtrue;
}

ID_INLINE qboolean BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs, const vec3_t origin)
{
	if ( origin[0] > maxs[0] ||
		origin[0] < mins[0] ||
		origin[1] > maxs[1] ||
		origin[1] < mins[1] ||
		origin[2] > maxs[2] ||
		origin[2] < mins[2])
	{
		return qfalse;
	}

	return qtrue;
}


ID_INLINE vec_t VectorNormalize( vec3_t v )
{
	// NOTE: TTimo - Apple G4 source uses double?
	float length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if ( length )
    {
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		float ilength = 1 / sqrt (length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
	return length;
}



ID_INLINE vec_t VectorNormalize2( const vec3_t v, vec3_t out)
{
	float length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if (length)
	{
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		float ilength = 1/(float)sqrt(length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	}
    else
    {
		VectorClear( out );
	}
		
	return length;
}


ID_INLINE void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


ID_INLINE vec_t _DotProduct( const vec3_t v1, const vec3_t v2 )
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

ID_INLINE void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

ID_INLINE void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

ID_INLINE void _VectorCopy( const vec3_t in, vec3_t out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

ID_INLINE void _VectorScale( const vec3_t in, vec_t scale, vec3_t out )
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

ID_INLINE void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out )
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
	out[3] = in[3]*scale;
}


ID_INLINE int Q_log2( int val )
{
	int answer = 0;
	while ( ( val>>=1 ) != 0 )
    {
		answer++;
	}
	return answer;
}



float Q_rsqrt( float number )
{
	floatint_t t;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	t.f  = number;
	t.i  = 0x5f3759df - ( t.i >> 1 );               // what the fuck?
	y  = t.f;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}





ID_INLINE void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3])
{
    const float a = in1[0][0];
    const float b = in1[1][0];
    const float c = in1[2][0];

    const float e = in1[0][1];
    const float f = in1[1][1];
    const float g = in1[2][1];

    const float h = in1[0][2];
    const float i = in1[1][2];
    const float j = in1[2][2];


	out[0][0] = a * in2[0][0] + e * in2[1][0] +	h * in2[2][0];
	out[0][1] = a * in2[0][1] + e * in2[1][1] +	h * in2[2][1];
	out[0][2] = a * in2[0][2] + e * in2[1][2] +	h * in2[2][2];
	
    out[1][0] = b * in2[0][0] + f * in2[1][0] +	i * in2[2][0];
	out[1][1] = b * in2[0][1] + f * in2[1][1] + i * in2[2][1];
	out[1][2] = b * in2[0][2] + f * in2[1][2] +	i * in2[2][2];

	out[2][0] = c * in2[0][0] + g * in2[1][0] +	j * in2[2][0];
	out[2][1] = c * in2[0][1] + g * in2[1][1] +	j * in2[2][1];
	out[2][2] = c * in2[0][2] + g * in2[1][2] +	j * in2[2][2];
}


void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	static float sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	float angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}

/*
** construct a perpendicular of dst,  assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
    float d;
    // this rotate and negate guarantees a vector not colinear with the original 
    if(src[0])
    {
        dst[0] = 0;
	    dst[1] = 1;
	    dst[2] = 0;
    }
    else if(src[1])
    {
        dst[0] = 0;
	    dst[1] = 0;
	    dst[2] = 1;
    }
    else if(src[2])
    {
        dst[0] = 1;
	    dst[1] = 0;
	    dst[2] = 0;
        return;
    }
    
    //project dst onto the src
    d = DotProduct(src, dst);
	dst[0] -= d*src[0];
	dst[1] -= d*src[1];
	dst[2] -= d*src[2];

    //normalize the result
	VectorNormalize( dst );
}

//------------------------------------------------------------------------
//

ID_INLINE int VectorCompare( const vec3_t v1, const vec3_t v2 ) {
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2]) {
		return 0;
	}			
	return 1;
}

ID_INLINE vec_t VectorLength( const vec3_t v )
{
	return (vec_t)sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

ID_INLINE vec_t VectorLengthSquared( const vec3_t v )
{
	return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

ID_INLINE vec_t Distance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return VectorLength( v );
}

ID_INLINE vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 )
{
	vec3_t	v;
	VectorSubtract (p2, p1, v);
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}




ID_INLINE void VectorInverse( vec3_t v ){
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

ID_INLINE void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

