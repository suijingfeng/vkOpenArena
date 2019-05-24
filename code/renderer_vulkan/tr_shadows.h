#ifndef TR_SHADOWS_H_
#define TR_SHADOWS_H_


/*
============================================================

SHADOWS

============================================================
*/

void RB_ShadowTessEnd( void );
void RB_ShadowFinish( void );
void RB_ProjectionShadowDeform( float (* const xyz)[4], uint32_t nVerts );

#endif
