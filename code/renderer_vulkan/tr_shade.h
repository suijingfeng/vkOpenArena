#ifndef TR_SHADE_H_
#define TR_SHADE_H_

#include "tr_shader.h"

void RB_ComputeColors(struct shaderStage_s * const pStage );
void RB_SetTessFogColor(unsigned char(* const pcolor)[4], uint32_t fnum, uint32_t nVerts);
void RB_ComputeTexCoords( shaderStage_t * const pStage );
void RB_CalcFogTexCoords( float (* const pST)[2], uint32_t );

//void	RB_CalcEnvironmentTexCoords( float *dstTexCoords );

//void	RB_CalcScrollTexCoords( const float scroll[2], float *dstTexCoords );
//void	RB_CalcRotateTexCoords( float rotSpeed, float *dstTexCoords );
//void	RB_CalcScaleTexCoords( const float scale[2], float *dstTexCoords );
//void	RB_CalcTurbulentTexCoords( const waveForm_t *wf, float *dstTexCoords );
//void	RB_CalcTransformTexCoords( const texModInfo_t *tmi, float *dstTexCoords );
//void	RB_CalcModulateColorsByFog( unsigned char *dstColors );
//void	RB_CalcModulateAlphasByFog( unsigned char *dstColors );
//void	RB_CalcModulateRGBAsByFog( unsigned char *dstColors );
//void	RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors );
//void	RB_CalcWaveColor( const waveForm_t *wf, unsigned char (*dstColors)[4] );
//void	RB_CalcAlphaFromEntity( unsigned char *dstColors );
//void	RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors );
//void	RB_CalcStretchTexCoords( const waveForm_t *wf, float *texCoords );
//void	RB_CalcColorFromEntity( unsigned char (*dstColors)[4] );
//void	RB_CalcColorFromOneMinusEntity( unsigned char (*dstColors)[4] );
//void	RB_CalcSpecularAlpha( unsigned char *alphas );
//void	RB_CalcDiffuseColor( unsigned char (*colors)[4] );

// void	RB_CalcAtlasTexCoords( const atlas_t *at, float *st );
// void RB_CalcEnvironmentTexCoordsJO( float *st );
// void RB_CalcEnvironmentCelShadeTexCoords( float *st );
// void RB_CalcCelTexCoords( float *st );
#endif
