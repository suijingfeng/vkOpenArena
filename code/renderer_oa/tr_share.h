#ifndef TR_SHARE_H_
#define TR_SHARE_H_


union Int4bytes{
    int i;
    unsigned char uc[4];
};

static ID_INLINE float VectorLength( const vec3_t v )
{
	return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}



static ID_INLINE void AxisCopy( vec3_t in[3], vec3_t out[3] )
{
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}


static ID_INLINE void FastVectorNormalize( float* v )
{
	// writing it this way allows gcc to recognize that rsqrt can be used
	float ilength = 1.0f/sqrtf( v[0] * v[0] + v[1] * v[1] + v[2]*v[2] );

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}


static ID_INLINE void FastVectorNormalize2( const float* v, float* out)
{
    // writing it this way allows gcc to recognize that rsqrt can be used
	float invLen = 1.0f/sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

 	out[0] = v[0] * invLen;
	out[1] = v[1] * invLen;
	out[2] = v[2] * invLen;
}


static ID_INLINE unsigned ColorBytes4(float r, float g, float b, float a)
{
	union Int4bytes cvt;

    cvt.uc[0] = r * 255;
    cvt.uc[1] = g * 255;
    cvt.uc[2] = b * 255;
    cvt.uc[3] = a * 255;

	return cvt.i;
}


static ID_INLINE  float NormalizeColor( const vec3_t in, vec3_t out )
{
	float max= in[0];
	if ( in[1] > max )
    {
		max = in[1];
	}
	if ( in[2] > max )
    {
		max = in[2];
	}

    out[0] = in[0] / max;
    out[1] = in[1] / max;
    out[2] = in[2] / max;
	return max;
}

/*
ID_INLINE float Norm(const float* v)
{
    // writing it this way allows gcc to recognize that rsqrt can be used
	return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}
*/

static ID_INLINE void VectorCross( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

// use Rodrigue's rotation formula
static ID_INLINE void PointRotateAroundVector( const vec3_t p, const vec3_t dir, float degrees, vec3_t sum )
{
    vec3_t k; 
    FastVectorNormalize2(dir, k);
    
    float rad = DEG2RAD( degrees );
    float cos_th = cos( rad );
    float sin_th = sin( rad );


	VectorCross(k, p, sum);
    sum[0] *= sin_th;
    sum[1] *= sin_th;
    sum[2] *= sin_th;

    sum[0] += cos_th * p[0]; 
    sum[1] += cos_th * p[1]; 
    sum[2] += cos_th * p[2]; 

    float d = (1 - cos_th)*( p[0] * k[0] + p[1] * k[1] + p[2] * k[2]);

    sum[0] += d * k[0];
    sum[1] += d * k[1];
    sum[2] += d * k[2];
}

#endif
