#ifndef R_Rotate_For_Viewer_H_
#define R_Rotate_For_Viewer_H_

#include "trRefDef.h"
#include "viewParms.h"

void R_RotateForViewer( viewParms_t * const pViewParams, orientationr_t * const pEntityPose);
void R_RotateForEntity(const trRefEntity_t* const ent, const viewParms_t* const viewParms, orientationr_t* const or);

#endif
