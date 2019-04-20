#include "trRefDef.h"
#include "viewParms.h"
#include "R_RotateForViewer.h"
#include "../renderercommon/matrix_multiplication.h"


static void R_WorldVectorToLocal (const vec3_t world, const float R[3][3], vec3_t local)
{
	local[0] = DotProduct(world, R[0]);
	local[1] = DotProduct(world, R[1]);
	local[2] = DotProduct(world, R[2]);
}

static void R_WorldPointToLocal (const vec3_t world, const orientationr_t * const pRT, vec3_t local)
{
    vec3_t delta;

    VectorSubtract( world, pRT->origin, delta );

	local[0] = DotProduct(delta, pRT->axis[0]);
	local[1] = DotProduct(delta, pRT->axis[1]);
	local[2] = DotProduct(delta, pRT->axis[2]);
}

/*
=================
typedef struct {
	float		modelMatrix[16] QALIGN(16);
	float		axis[3][3];		// orientation in world
    float		origin[3];			// in world coordinates
	float		viewOrigin[3];		// viewParms->or.origin in local coordinates
} orientationr_t;


Sets up the modelview matrix for a given viewParm

IN: tr.viewParms
OUT: tr.or
=================
*/
void R_RotateForViewer ( viewParms_t * const pViewParams, orientationr_t * const pEntityPose) 
{
    //const viewParms_t * const pViewParams = &tr.viewParms;
    // for current entity
    // orientationr_t * const pEntityPose = &tr.or;
    
    const static float s_flipMatrix[16] QALIGN(16) = {
        // convert from our coordinate system (looking down X)
        // to OpenGL's coordinate system (looking down -Z)
        0, 0, -1, 0,
        -1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 0, 1
    };
    

	float o0, o1, o2;

    pEntityPose->origin[0] = pEntityPose->origin[1] = pEntityPose->origin[2] = 0;
    Mat3x3Identity(pEntityPose->axis);


    // transform by the camera placement
	// VectorCopy( tr.viewParms.or.origin, tr.or.viewOrigin );
	// VectorCopy( tr.viewParms.or.origin, origin );

    pEntityPose->viewOrigin[0] = o0 = pViewParams->or.origin[0];
    pEntityPose->viewOrigin[1] = o1 = pViewParams->or.origin[1];
    pEntityPose->viewOrigin[2] = o2 = pViewParams->or.origin[2];

    
    float viewerMatrix[16] QALIGN(16);
	viewerMatrix[0] = pViewParams->or.axis[0][0];
	viewerMatrix[1] = pViewParams->or.axis[1][0];
	viewerMatrix[2] = pViewParams->or.axis[2][0];
	viewerMatrix[3] = 0;

	viewerMatrix[4] = pViewParams->or.axis[0][1];
	viewerMatrix[5] = pViewParams->or.axis[1][1];
	viewerMatrix[6] = pViewParams->or.axis[2][1];
	viewerMatrix[7] = 0;

	viewerMatrix[8] = pViewParams->or.axis[0][2];
	viewerMatrix[9] = pViewParams->or.axis[1][2];
	viewerMatrix[10] = pViewParams->or.axis[2][2];
	viewerMatrix[11] = 0;

	viewerMatrix[12] = - o0 * viewerMatrix[0] - o1 * viewerMatrix[4] - o2 * viewerMatrix[8];
	viewerMatrix[13] = - o0 * viewerMatrix[1] - o1 * viewerMatrix[5] - o2 * viewerMatrix[9];
	viewerMatrix[14] = - o0 * viewerMatrix[2] - o1 * viewerMatrix[6] - o2 * viewerMatrix[10];
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	MatrixMultiply4x4_SSE( viewerMatrix, s_flipMatrix, pEntityPose->modelMatrix );

	pViewParams->world = *pEntityPose;
}


/*
=================
R_RotateForEntity

Generates an orientation for an entity and viewParms
Does NOT produce any GL calls
Called by both the front end and the back end

typedef struct {
	float		modelMatrix[16] QALIGN(16);
	float		axis[3][3];		// orientation in world
    float		origin[3];		// in world coordinates
	float		viewOrigin[3];	// viewParms->or.origin in local coordinates
} orientationr_t;

=================
*/
void R_RotateForEntity(const trRefEntity_t* const ent, const viewParms_t* const viewParms, orientationr_t* const or)
{

	if ( ent->e.reType != RT_MODEL )
    {
		*or = viewParms->world;
		return;
	}

	//VectorCopy( ent->e.origin, or->origin );
	//VectorCopy( ent->e.axis[0], or->axis[0] );
	//VectorCopy( ent->e.axis[1], or->axis[1] );
	//VectorCopy( ent->e.axis[2], or->axis[2] );
    memcpy(or->origin, ent->e.origin, 12);
    memcpy(or->axis, ent->e.axis, 36);

	float glMatrix[16] QALIGN(16);

	glMatrix[0] = or->axis[0][0];
	glMatrix[1] = or->axis[0][1];
	glMatrix[2] = or->axis[0][2];
	glMatrix[3] = 0;

    glMatrix[4] = or->axis[1][0];
	glMatrix[5] = or->axis[1][1];
	glMatrix[6] = or->axis[1][2];
	glMatrix[7] = 0;
    
    glMatrix[8] = or->axis[2][0];
	glMatrix[9] = or->axis[2][1];
	glMatrix[10] = or->axis[2][2];
	glMatrix[11] = 0;

	glMatrix[12] = or->origin[0];
	glMatrix[13] = or->origin[1];
	glMatrix[14] = or->origin[2];
	glMatrix[15] = 1;

	MatrixMultiply4x4_SSE( glMatrix, viewParms->world.modelMatrix, or->modelMatrix );

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
    
    R_WorldPointToLocal(viewParms->or.origin, or, or->viewOrigin);
    
    if ( ent->e.nonNormalizedAxes )
    {
         if ( ent->e.nonNormalizedAxes )
        {
            const float * v = ent->e.axis[0];
            float axisLength = v[0] * v[0] + v[0] * v[0] + v[2] * v[2]; 
            if ( axisLength ) {
                axisLength = 1.0f / sqrtf(axisLength);
            }

            or->viewOrigin[0] *= axisLength;
            or->viewOrigin[1] *= axisLength;
            or->viewOrigin[2] *= axisLength;
        }
    }
/*  
    vec3_t delta;

	VectorSubtract( viewParms->or.origin, or->origin, delta );

    R_WorldVectorToLocal(delta, or->axis, or->viewOrigin);
    
	// compensate for scale in the axes if necessary
    float axisLength = 1.0f;
	if ( ent->e.nonNormalizedAxes )
    {
		axisLength = VectorLength( ent->e.axis[0] );
		if ( axisLength ) {
			axisLength = 1.0f / axisLength;
		}
	}

	or->viewOrigin[0] = DotProduct( delta, or->axis[0] ) * axisLength;
	or->viewOrigin[1] = DotProduct( delta, or->axis[1] ) * axisLength;
	or->viewOrigin[2] = DotProduct( delta, or->axis[2] ) * axisLength;

*/
    // printMat1x3f("viewOrigin", or->viewOrigin);
    // printMat4x4f("modelMatrix", or->modelMatrix);
}
