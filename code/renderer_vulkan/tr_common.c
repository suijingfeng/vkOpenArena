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


extern refimport_t ri;


char* R_SkipPath(char *pathname)
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


void R_StripExtension(const char *in, char *out, int destsize)
{
	const char *dot = strrchr(in, '.');
    const char *slash = strrchr(in, '/');


	if ((dot != NULL) && ( (slash == NULL) || (slash < dot) ) )
    {
        int len = dot-in+1;
        if(len <= destsize)
            destsize = len;
        else
		    ri.Printf( PRINT_WARNING, "stripExtension: dest size not enough!\n");
    }

    if(in != out)
    	strncpy(out, in, destsize-1);
	
    out[destsize-1] = '\0';
}


/*
char* getExtension( const char *name )
{
	char* dot = strrchr(name, '.');
    char* slash = strrchr(name, '/');

	if ((dot != NULL) && ((slash == NULL) || (slash < dot) ))
		return dot + 1;
	else
		return "";
}
*/


void VectorCross( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}



// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation
void R_FastNormalize1f(float v[3])
{
	// writing it this way allows gcc to recognize that rsqrt can be used
    float invLen = 1.0f / sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

 	v[0] = v[0] * invLen;
	v[1] = v[1] * invLen;
	v[2] = v[2] * invLen;
}

void FastNormalize2f( const float* v, float* out)
{
	// writing it this way allows gcc to recognize that rsqrt can be used
    float invLen = 1.0f / sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

 	out[0] = v[0] * invLen;
	out[1] = v[1] * invLen;
	out[2] = v[2] * invLen;
}


//
// common function replacements for modular renderer
// 
#ifdef USE_RENDERER_DLOPEN

void QDECL Com_Printf( const char *msg, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	ri.Printf(PRINT_ALL, "%s", text);
}

void QDECL Com_Error( int level, const char *error, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, error);
	vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri.Error(level, "%s", text);
}

inline int LongSwap(int l)
{
	unsigned char b1,b2,b3,b4;

	b1 = l & 0xff;
	b2 = (l>>8) & 0xff;
	b3 = (l>>16) & 0xff;
	b4 = (l>>24) & 0xff;

	return ((b1<<24) | (b2<<16) | (b3<<8) | b4 );
}

/*
 * Safe strncpy that ensures a trailing zero
 */
void Q_strncpyz( char *dest, const char *src, int destsize )
{
/*    
    if( !dest )
        Com_Error( ERR_FATAL, "Q_strncpyz: NULL dest" );

	if( !src )
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );

	if ( destsize < 1 )
		Com_Error(ERR_FATAL,"Q_strncpyz: destsize < 1" ); 
*/
	strncpy(dest, src, destsize-1);
    dest[destsize-1] = 0;
}

#endif


float VectorNormalize( float v[3] )
{
	// NOTE: TTimo - Apple G4 source uses double?
	float length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if ( length )
    {
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		float ilength = 1.0f / sqrtf (length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
	return length;
}


float VectorNormalize2( const float v[3], float out[3] )
{
	float length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if (length)
	{
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		float ilength = 1.0f/(float)sqrt(length);
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

int isNonCaseStringEqual(const char * s1, const char * s2)
{
    if( (s1 != NULL) && (s2 != NULL) )
    {
        int	c1;
        int c2;
        do
        {
            c1 = *s1++; 
            c2 = *s2++;

            // consider that most shader is little a - z, 
            if( (c1 >= 'A') && (c1 <= 'Z') )
            {
                c1 += 'a' - 'A';
            }
            
            // to a - z
            if( (c2 >= 'A') && (c2 <= 'Z') )
            {
                c2 +=  'a' - 'A';
            }

            if( c1 != c2 )
            {
                // string are not equal
                return 0;
            }
        } while(c1 || c2);

        return 1;
    }

    // should not compare null, this is not meaningful ...
    ri.Printf( PRINT_WARNING, "WARNING: compare NULL string. %p == %p ? \n", s1, s2 );
    return 0;
}


int isNonCaseNStringEqual(const char *s1, const char *s2, int n)
{
	int	c1, c2;

    if( s1 == NULL )
    {
        if( s2 == NULL )
             return 0;
        else
             return -1;
    }
    else if ( s2 == NULL )
    {
	    return 1;
    }

    do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point
		
		if(c1 >= 'a' && c1 <= 'z')
			c1 -= ('a' - 'A');
			
		if(c2 >= 'a' && c2 <= 'z')
			c2 -= ('a' - 'A');
		
		if(c1 != c2) 
			return c1 < c2 ? -1 : 1;

   } while (c1);

    return 0;		// strings are equal
}


char * R_Strlwr( char * const s1 )
{
    char *s = s1;
	while ( *s ) {
		*s = tolower(*s);
		s++;
	}
    return s1;
}

// use Rodrigue's rotation formula
// dir are not assumed to be unit vector
void PointRotateAroundVector(float* res, const float* vec, const float* p, const float degrees)
{
    float rad = DEG2RAD( degrees );
    float cos_th = cos( rad );
    float sin_th = sin( rad );
    float k[3];

	// writing it this way allows gcc to recognize that rsqrt can be used
    float invLen = 1.0f / sqrtf(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
 	k[0] = vec[0] * invLen;
	k[1] = vec[1] * invLen;
	k[2] = vec[2] * invLen;

    float d = (1 - cos_th) * (p[0] * k[0] + p[1] * k[1] + p[2] * k[2]);

	res[0] = sin_th * (k[1]*p[2] - k[2]*p[1]);
	res[1] = sin_th * (k[2]*p[0] - k[0]*p[2]);
	res[2] = sin_th * (k[0]*p[1] - k[1]*p[0]);

    res[0] += cos_th * p[0] + d * k[0]; 
    res[1] += cos_th * p[1] + d * k[1]; 
    res[2] += cos_th * p[2] + d * k[2]; 
}


// note: vector forward are NOT assumed to be nornalized,
// unit: nornalized of forward,
// dst: unit vector which perpendicular of forward(src) 
void VectorPerp( const float src[3], float dst[3] )
{
    float unit[3];
    
    float sqlen = src[0]*src[0] + src[1]*src[1] + src[2]*src[2];
    if(0 == sqlen)
    {
        ri.Printf( PRINT_WARNING, "MakePerpVectors: zero vertor input!\n");
        return;
    }

  	dst[1] = -src[0];
	dst[2] = src[1];
	dst[0] = src[2];
	// this rotate and negate try to make a vector not colinear with the original
    // actually can not guarantee, for example
    // forward = (1/sqrt(3), 1/sqrt(3), -1/sqrt(3)),
    // then right = (-1/sqrt(3), -1/sqrt(3), 1/sqrt(3))


    float invLen = 1.0f / sqrtf(sqlen);
    unit[0] = src[0] * invLen;
    unit[1] = src[1] * invLen;
    unit[2] = src[2] * invLen;

    
    float d = DotProduct(unit, dst);
	dst[0] -= d*unit[0];
	dst[1] -= d*unit[1];
	dst[2] -= d*unit[2];

    // normalize the result
    invLen = 1.0f / sqrtf(dst[0]*dst[0] + dst[1]*dst[1] + dst[2]*dst[2]);

    dst[0] *= invLen;
    dst[1] *= invLen;
    dst[2] *= invLen;
}

// Given a normalized forward vector, create two other perpendicular vectors
// note: vector forward are NOT assumed to be nornalized,
// after this funtion is called , forward are nornalized.
// right, up: perpendicular of forward 
float MakeTwoPerpVectors(const float forward[3], float right[3], float up[3])
{

    float sqLen = forward[0]*forward[0]+forward[1]*forward[1]+forward[2]*forward[2];
    if(sqLen)
    {
        float nf[3] = {0, 0, 1};
        float invLen = 1.0f / sqrtf(sqLen);
        nf[0] = forward[0] * invLen;
        nf[1] = forward[1] * invLen;
        nf[2] = forward[2] * invLen;

        float adjlen = DotProduct(nf, right);

        // this rotate and negate guarantees a vector
        // not colinear with the original
        right[0] = forward[2] - adjlen * nf[0];
        right[1] = -forward[0] - adjlen * nf[1];
        right[2] = forward[1] - adjlen * nf[2];


        invLen = 1.0f/sqrtf(right[0]*right[0]+right[1]*right[1]+right[2]*right[2]);
        right[0] *= invLen;
        right[1] *= invLen;
        right[2] *= invLen;

        // get the up vector with the right hand rules 
        VectorCross(right, nf, up);

        return (sqLen * invLen);
    }
    return 0;
}


/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints(vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c)
{
	vec3_t d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );

	VectorCross( d2, d1, plane );

	plane[3] = a[0]*plane[0] + a[1]*plane[1] + a[2]*plane[2];

	if((plane[0]*plane[0] + plane[1]*plane[1] + plane[2]*plane[2]) == 0)
    {
		return qfalse;
	}

	return qtrue;
}



/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist[2] = {0, 0};
	int		sides = 0, i;

	// fast axial cases
    /*
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
        else if (p->dist >= emaxs[p->type])
			return 2;
        
		return 3;
	}
    */
	// general case
	// dist[0] = dist[1] = 0;
	// >= 8: default case is original code (dist[0]=dist[1]=0)
	
    for (i=0 ; i<3; ++i)
    {
        int b = (p->signbits >> i) & 1;
        dist[ b] += p->normal[i]*emaxs[i];
        dist[!b] += p->normal[i]*emins[i];
    }


	// sides = 0;
	if (dist[0] >= p->dist)
		sides = 1;
	if (dist[1] < p->dist)
		sides |= 2;

	return sides;
}


void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs )
{
	if ( v[0] < mins[0] ) {
		mins[0] = v[0];
	}
	if ( v[0] > maxs[0]) {
		maxs[0] = v[0];
	}

	if ( v[1] < mins[1] ) {
		mins[1] = v[1];
	}
	if ( v[1] > maxs[1]) {
		maxs[1] = v[1];
	}

	if ( v[2] < mins[2] ) {
		mins[2] = v[2];
	}
	if ( v[2] > maxs[2]) {
		maxs[2] = v[2];
	}
}



void ClearBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}



qboolean SpheresIntersect(vec3_t origin1, float radius1, vec3_t origin2, float radius2)
{
	float radiusSum = radius1 + radius2;
	vec3_t diff;
	
	VectorSubtract(origin1, origin2, diff);

	if (DotProduct(diff, diff) <= radiusSum * radiusSum)
	{
		return qtrue;
	}

	return qfalse;
}

void BoundingSphereOfSpheres(vec3_t origin1, float radius1, vec3_t origin2, float radius2, vec3_t origin3, float *radius3)
{
	vec3_t diff;

	VectorScale(origin1, 0.5f, origin3);
	VectorMA(origin3, 0.5f, origin2, origin3);

	VectorSubtract(origin1, origin2, diff);
	*radius3 = VectorLen(diff) * 0.5f + MAX(radius1, radius2);
}
