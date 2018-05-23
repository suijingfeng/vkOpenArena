#ifndef TR_SHARE_H_
#define TR_SHARE_H_


#include<math.h>
#include<string.h>

extern const float colorBlack[4];
extern const float colorRed[4];
extern const float colorGreen[4];
extern const float colorBlue[4];
extern const float colorYellow[4];
extern const float colorMagenta[4];
extern const float colorCyan[4];
extern const float colorWhite[4];
extern const float colorLtGrey[4];
extern const float colorMdGrey[4];
extern const float colorDkGrey[4];




union uInt4bytes{
    unsigned int i;
    unsigned char uc[4];
};

union f32_u {
	float f;
	unsigned int ui;
	struct {
		unsigned int fraction:23;
		unsigned int exponent:8;
		unsigned int sign:1;
	} pack;
};

union f16_u {
	unsigned short ui;
	struct {
		unsigned int fraction:10;
		unsigned int exponent:5;
		unsigned int sign:1;
	} pack;
};



// subroutines

#ifndef SGN
#define SGN(x) (((x) >= 0) ? !!(x) : -1)
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef CLAMP
#define CLAMP(a,b,c) MIN(MAX((a),(b)),(c))
#endif

#define VectorCopy2(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1])
#define VectorSet2(v,x,y)       ((v)[0]=(x),(v)[1]=(y));

#define VectorCopy4(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define VectorSet4(v,x,y,z,w)	((v)[0]=(x),(v)[1]=(y),(v)[2]=(z),(v)[3]=(w))
#define DotProduct4(a,b)        ((a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]*(b)[2] + (a)[3]*(b)[3])
#define VectorScale4(a,b,c)     ((c)[0]=(a)[0]*(b),(c)[1]=(a)[1]*(b),(c)[2]=(a)[2]*(b),(c)[3]=(a)[3]*(b))

#define VectorCopy5(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3],(b)[4]=(a)[4])

#define OffsetByteToFloat(a)    ((float)(a) * 1.0f/127.5f - 1.0f)
#define FloatToOffsetByte(a)    (byte)((a) * 127.5f + 128.0f)
#define ByteToFloat(a)          ((float)(a) * 1.0f/255.0f)
#define FloatToByte(a)          (byte)((a) * 255.0f)



const char* getExtension( const char *name );
char* SkipPath(char *pathname);
void stripExtension(const char *in, char *out, int destsize);
float R_NoiseGet4f( float x, float y, float z, float t );
void  R_NoiseInit( void );


int NextPowerOfTwo(int in);
unsigned short FloatToHalf(float in);
float HalfToFloat(unsigned short in);

int SpheresIntersect(float origin1[3], float radius1, float origin2[3], float radius2);
void BoundingSphereOfSpheres(float origin1[3], float radius1, float origin2[3], float radius2, float origin3[3], float *radius3);


static inline int VectorCompare4(const float v1[4], const float v2[4])
{
	if( (v1[0] != v2[0]) || (v1[1] != v2[1]) || (v1[2] != v2[2]) || (v1[3] != v2[3]))
	{
		return 0;
	}
	return 1;
}

static inline int VectorCompare5(const float v1[5], const float v2[5])
{
	if(v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3] || v1[4] != v2[4])
	{
		return 0;
	}
	return 1;
}


static inline float VectorLength(const float v[3])
{
	return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static inline float VectorLength2(const float* v1, const float* v2)
{
    float tmp1 = v1[0]-v2[0];
    float tmp2 = v1[1]-v2[1];
    float tmp3 = v1[2]-v2[2];

	return sqrtf(tmp1*tmp1 + tmp2*tmp2 + tmp3*tmp3);
}

static inline void AxisClear( float axis[3][3] )
{
    memset(axis, 0, 9 * sizeof(float));
}

static inline void AxisCopy(const float in[3][3], float out[3][3])
{
    memcpy(out, in, 9 * sizeof(float));
}


// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation

static inline void FastVectorNormalize(float v[3])
{
	// writing it this way allows gcc to recognize that rsqrt can be used
    float invLen = v[0] * v[0] + v[1] * v[1] + v[2]*v[2];
    if(invLen != 0)
	{
		invLen = 1.0f / sqrtf(invLen);

		v[0] *= invLen;
		v[1] *= invLen;
		v[2] *= invLen;
	}
}


static inline void FastVectorNormalize2( const float* v, float* out)
{
	// writing it this way allows gcc to recognize that rsqrt can be used
    float invLen = 1.0f / sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

 	out[0] = v[0] * invLen;
	out[1] = v[1] * invLen;
	out[2] = v[2] * invLen;
}


static inline unsigned int ColorBytes4(float r, float g, float b, float a)
{
	union uInt4bytes cvt;

    cvt.uc[0] = r * 255;
    cvt.uc[1] = g * 255;
    cvt.uc[2] = b * 255;
    cvt.uc[3] = a * 255;

	return cvt.i;
}


static inline float NormalizeColor(const float in[3], float out[3])
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

static inline void VecCopy(const float in[3], float out[3])
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

static inline void VecCross(const float v1[3], const float v2[3], float cross[3] )
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

static inline float VecDot(const float v1[3], const float v2[3])
{
	return (v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2]);
}


static inline void VecScale(float v[3], const float s, float o[3])
{
    o[0]=v[0]*s;
    o[1]=v[1]*s;
    o[2]=v[2]*s;
}

static inline void VecSub( float a[3], float b[3], float c[3])
{
    c[0]= a[0] - b[0];
    c[1]= a[1] - b[1];
    c[2]= a[2] - b[2];
}

static inline void VecAdd( float a[3], float b[3], float c[3])
{
    c[0]= a[0] + b[0];
    c[1]= a[1] + b[1];
    c[2]= a[2] + b[2];
}


static inline void VecLerp(float a[3], float b[3], const float lerp, float c[3])
{
    float dif[3];
    dif[0] = b[0] - a[0];
    dif[1] = b[1] - a[1];
    dif[2] = b[2] - a[2];

	c[0] = a[0] + dif[0] * lerp;
	c[1] = a[1] + dif[1] * lerp;
	c[2] = a[2] + dif[2] * lerp;
}


static inline void VecMA(float v[3], float s, float b[3], float o[3])
{
    o[0] = v[0] + b[0]*s;
    o[1] = v[1] + b[1]*s;
    o[2] = v[2] + b[2]*s;
}
// use Rodrigue's rotation formula
static inline void PointRotateAroundVector(const float p[3], const float dir[3], const float degrees, float sum[3])
{
    float k[3]; 
    FastVectorNormalize2(dir, k);
   

    float rad = degrees * ( M_PI / 180.0F);
    float cos_th = cos( rad );
    float sin_th = sin( rad );
    float d = (1 - cos_th) * (p[0] * k[0] + p[1] * k[1] + p[2] * k[2]);

	VecCross(k, p, sum);


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


void MatrixMultiply4x4(const float A[16], const float B[16], float out[16]);
void Mat4Translation(const float vec[3], float out[16]);
void Mat4Transform(const float mat[16], const float v[4], float out[4]);
void Mat4Copy(const float in[16], float out[16]);
void Mat4Zero(float out[16]);
void Mat4Identity(float out[16]);
int Mat4Compare(const float a[16], const float b[16]);

void Mat4Ortho(float left, float right, float bottom, float top, float znear, float zfar, float out[16]);
void Mat4View(const float axes[3][3], const float origin[3], float out[16]);
void Mat4SimpleInverse( const float in[16], float out[16]);

void Mat4Dump(const float in[16]);
/*
================
MakeNormalVectors

Given a normalized forward vector, create two other perpendicular vectors
/perpendicular vector could be replaced by this

================
*/

static inline void MakeNormalVectors( const float forward[3], float right[3], float up[3])
{
	// this rotate and negate guarantees a vector not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];
    // actually can not guarantee,
    // assume forward = (1/sqrt(3), 1/sqrt(3), -1/sqrt(3)),
    // then right = (-1/sqrt(3), -1/sqrt(3), 1/sqrt(3))

	float d = right[0]*forward[0] + right[1]*forward[1] + right[2]*forward[2];

	right[0] -= d*forward[0];
	right[1] -= d*forward[1];
	right[2] -= d*forward[2];

    FastVectorNormalize(right);
	VecCross(forward, right, up);
}




#endif
