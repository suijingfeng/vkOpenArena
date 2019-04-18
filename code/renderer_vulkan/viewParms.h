#ifndef VIEW_PARMS_H_
#define VIEW_PARMS_H_

#include "../qcommon/q_shared.h"

typedef struct {
	float		modelMatrix[16] QALIGN(16);
	float		axis[3][3];		// orientation in world
    float		origin[3];			// in world coordinates
	float		viewOrigin[3];		// viewParms->or.origin in local coordinates
} orientationr_t;

typedef struct {
	orientationr_t	or;
	orientationr_t	world;
	float		pvsOrigin[3];			// may be different than or.origin for portals
	qboolean	isPortal;			// true if this view is through a portal
	qboolean	isMirror;			// the portal is a mirror, invert the face culling
//	cplane_t	portalPlane;		// clip anything behind this if mirroring
	int			viewportX, viewportY, viewportWidth, viewportHeight;
	float		fovX, fovY;
	float		projectionMatrix[16] QALIGN(16);
	cplane_t	frustum[4];
	float		visBounds[2][3];
	float		zFar;
} viewParms_t;

#endif
