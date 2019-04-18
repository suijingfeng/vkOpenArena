#ifndef TR_MAIN_H_
#define TR_MAIN_H_


#include "viewParms.h"
#include "trRefDef.h"


void R_RenderView( viewParms_t *parms );
void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, 
					 int *fogNum, int *dlightMap );
void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *or );
#endif
