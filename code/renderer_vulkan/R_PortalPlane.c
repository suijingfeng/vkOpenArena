#include "R_PortalPlane.h"
#include "tr_backend.h"
#include "tr_common.h"
// void R_TransformPlane(const float R[3][3], const float T[3], struct rplane_s* pDstPlane);


struct rplane_s {
	float normal[3];
	float dist;
};

static struct rplane_s g_portalPlane;		// clip anything behind this if mirroring

/*
inline static float DotProduct( const float v1[3], const float v2[3] )
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}
*/

void R_SetupPortalPlane(const float axis[3][3], const float origin[3])
{
	// VectorSubtract( ORIGIN, pCamera->axis[0], g_portalPlane.normal );
    // g_portalPlane.dist = DotProduct( pCamera->origin, g_portalPlane.normal );

    g_portalPlane.normal[0] = - axis[0][0];
    g_portalPlane.normal[1] = - axis[0][1];
    g_portalPlane.normal[2] = - axis[0][2];
    g_portalPlane.dist = - origin[0] * axis[0][0]
                         - origin[1] * axis[0][1]
                         - origin[2] * axis[0][2];
}


void R_TransformPlane(const float R[3][3], const float T[3], struct rplane_s* pDstPlane)
{
    pDstPlane->normal[0] = DotProduct (R[0], g_portalPlane.normal);
    pDstPlane->normal[1] = DotProduct (R[1], g_portalPlane.normal);
    pDstPlane->normal[2] = DotProduct (R[2], g_portalPlane.normal);
    pDstPlane->dist = DotProduct (T, g_portalPlane.normal) - g_portalPlane.dist;
}


void R_SetPushConstForPortal(float* const pPush)
{
    pPush[0] = backEnd.or.modelMatrix[0];
    pPush[1] = backEnd.or.modelMatrix[4];
    pPush[2] = backEnd.or.modelMatrix[8];
    pPush[3] = backEnd.or.modelMatrix[12];

    pPush[4] = backEnd.or.modelMatrix[1];
    pPush[5] = backEnd.or.modelMatrix[5];
    pPush[6] = backEnd.or.modelMatrix[9];
    pPush[7] = backEnd.or.modelMatrix[13];

    pPush[8] = backEnd.or.modelMatrix[2];
    pPush[9] = backEnd.or.modelMatrix[6];
    pPush[10] = backEnd.or.modelMatrix[10];
    pPush[11] = backEnd.or.modelMatrix[14];

    // Clipping plane in eye coordinates.
    struct rplane_s eye_plane;

    R_TransformPlane(backEnd.viewParms.or.axis, backEnd.viewParms.or.origin, &eye_plane);

    // Apply s_flipMatrix to be in the same coordinate system as pPush.

    pPush[12] = -eye_plane.normal[1];
    pPush[13] =  eye_plane.normal[2];
    pPush[14] = -eye_plane.normal[0];
    pPush[15] =  eye_plane.dist;
}
