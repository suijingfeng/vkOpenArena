#ifndef R_PORTAL_H_
#define R_PORTAL_H_

#include "tr_globals.h"
#include "tr_cvar.h"
#include "tr_shader.h"
#include "tr_surface.h"

qboolean R_MirrorViewBySurface (drawSurf_t * drawSurf, int entityNum);
void R_RotateForViewer ( viewParms_t * const pViewParams, orientationr_t * const pEntityPose); 
#endif
