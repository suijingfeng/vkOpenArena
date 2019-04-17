#ifndef RB_SURFACE_H_
#define RB_SURFACE_H_

#include "tr_local.h"



void RB_CHECKOVERFLOW(uint32_t v, uint32_t i);


void RB_BeginSurface(shader_t *shader, int fogNum );
void RB_EndSurface(void);
void RB_CheckOverflow( int verts, int indexes );
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 );
void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte *color );


extern void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void *);

#endif
