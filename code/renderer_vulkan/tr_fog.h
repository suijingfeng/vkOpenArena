#ifndef TR_FOG_H_
#define TR_FOG_H_


typedef enum {
	FP_NONE,		// surface is translucent and will just be adjusted properly
	FP_EQUAL,		// surface is opaque but possibly alpha tested
	FP_LE			// surface is trnaslucent, but still needs a fog pass (fog surface)
} fogPass_t;


typedef struct {
	float	color[3];
	float	depthForOpaque;
} fogParms_t;


typedef struct {
	int			originalBrushNumber;
	float		bounds[2][3];

	unsigned char colorRGBA[4];			// in packed byte format
	float		tcScale;				// texture coordinate vector scales
	fogParms_t	parms;

	// for clipping distance in fog when outside
	int	hasSurface;
	float		surface[4];
} fog_t;

void R_InitFogTable( void );
float R_FogFactor( float s, float t );


#endif
