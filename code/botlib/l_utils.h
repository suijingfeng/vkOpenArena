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

/*****************************************************************************
 * name:		l_util.h
 *
 * desc:		utils
 *
 * $Archive: /source/code/botlib/l_util.h $
 *
 *****************************************************************************/

#ifndef L_UTILS_H_
#define L_UTILS_H_

#define Vector2Angles(v,a)		vectoangles(v,a)
#ifndef MAX_PATH
#define MAX_PATH				MAX_QPATH
#endif
#define Maximum(x,y)			(x > y ? x : y)
#define Minimum(x,y)			(x < y ? x : y)


static ID_INLINE float BE_VectorNormalize( vec3_t v )
{
	float length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if ( length != 0)
    {
		float invLen = 1.0f / sqrtf(length);
		v[0] *= invLen;
		v[1] *= invLen;
		v[2] *= invLen;
        return length*invLen;
	}
		
	return length;
}

static ID_INLINE void VectorNormalize( float* v )
{
	// writing it this way allows gcc to recognize that rsqrt can be used
    float invLen = v[0] * v[0] + v[1] * v[1] + v[2]*v[2];
    if(invLen == 0)
        return;

	invLen = 1.0f / sqrtf(invLen);

	v[0] *= invLen;
	v[1] *= invLen;
	v[2] *= invLen;
}

static ID_INLINE void VectorNormalize2( const float* v, float* out)
{
    // writing it this way allows gcc to recognize that rsqrt can be used
	float invLen = 1.0f/sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

 	out[0] = v[0] * invLen;
	out[1] = v[1] * invLen;
	out[2] = v[2] * invLen;
}


static ID_INLINE float VectorLength( const vec3_t v )
{
	return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

#endif
