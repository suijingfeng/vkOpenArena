#ifndef TR_SHARE_H_
#define TR_SHARE_H_

extern const vec4_t	colorBlack;
extern const vec4_t	colorRed;
extern const vec4_t	colorGreen;
extern const vec4_t	colorBlue;
extern const vec4_t	colorYellow;
extern const vec4_t	colorMagenta;
extern const vec4_t	colorCyan;
extern const vec4_t	colorWhite;
extern const vec4_t	colorLtGrey;
extern const vec4_t	colorMdGrey;
extern const vec4_t	colorDkGrey;




union uInt4bytes{
    int i;
    unsigned char uc[4];
};

static ID_INLINE float VectorLength( const vec3_t v )
{
	return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static ID_INLINE float VectorLength2( const float* v1, const float* v2)
{
    float tmp1 = v1[0]-v2[0];
    float tmp2 = v1[1]-v2[1];
    float tmp3 = v1[2]-v2[2];

	return sqrtf(tmp1*tmp1 + tmp2*tmp2 + tmp3*tmp3);
}


static ID_INLINE void AxisCopy( vec3_t in[3], vec3_t out[3] )
{
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}


// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation

static ID_INLINE void FastVectorNormalize( float* v )
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


static ID_INLINE void FastVectorNormalize2( const float* v, float* out)
{
	// writing it this way allows gcc to recognize that rsqrt can be used
    float invLen = 1.0f / sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

 	out[0] = v[0] * invLen;
	out[1] = v[1] * invLen;
	out[2] = v[2] * invLen;
}


static ID_INLINE unsigned int ColorBytes4(float r, float g, float b, float a)
{
	union uInt4bytes cvt;

    cvt.uc[0] = r * 255;
    cvt.uc[1] = g * 255;
    cvt.uc[2] = b * 255;
    cvt.uc[3] = a * 255;

	return cvt.i;
}


static ID_INLINE float NormalizeColor( const vec3_t in, vec3_t out )
{
	float max = in[0];
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


static ID_INLINE void VectorCross( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

// use Rodrigue's rotation formula
static ID_INLINE void PointRotateAroundVector( const vec3_t p, const vec3_t dir, const float degrees, float* sum )
{
    vec3_t k; 
    FastVectorNormalize2(dir, k);
    
    float rad = DEG2RAD( degrees );
    float cos_th = cos( rad );
    float sin_th = sin( rad );
    float d = (1 - cos_th) * (p[0] * k[0] + p[1] * k[1] + p[2] * k[2]);

	VectorCross(k, p, sum);

    sum[0] *= sin_th;
    sum[1] *= sin_th;
    sum[2] *= sin_th;

    sum[0] += cos_th * p[0]; 
    sum[1] += cos_th * p[1]; 
    sum[2] += cos_th * p[2]; 

    sum[0] += d * k[0];
    sum[1] += d * k[1];
    sum[2] += d * k[2];
}



//void Matrix4Multiply(const float a[16], const float b[16], float out[16]);
//note: out = A X B
static ID_INLINE void MatrixMultiply4x4(const float* A, const float* B, float* out)
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


/*
================
MakeNormalVectors

Given a normalized forward vector, create two other perpendicular vectors
/perpendicular vector could be replaced by this

================
*/

static ID_INLINE void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up)
{
	// this rotate and negate guarantees a vector not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];
    // actually can not guarantee,
    // assume forward = (1/sqrt(3), 1/sqrt(3), -1/sqrt(3)),
    // then right = (-1/sqrt(3), -1/sqrt(3), 1/sqrt(3))

	float d = DotProduct(right, forward);

	right[0] -= d*forward[0];
	right[1] -= d*forward[1];
	right[2] -= d*forward[2];

    FastVectorNormalize(right);
	CrossProduct(forward, right, up);
}




/*
static void VectorPerp(const vec3_t src, vec3_t dst1, vec3_t dst2 )
{
	int	pos = 0;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec = {0.0f, 0.0f, 0.0f};

	// find the smallest magnitude axially aligned vector
	for (i = 0; i < 3; i++ )
	{
        float len = fabs( src[i] );
		if ( len < minelem )
		{
			pos = i;
			minelem = len;
		}
	}
	//tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	//project the point onto the plane defined by src
	//float d = -DotProduct( tempvec, src );
	VectorMA( tempvec, -src[pos], src, dst1 );

    //normalize the result
	VectorNormalize( dst1 );

	CrossProduct(src, dst1, dst2);
}
*/

#endif
