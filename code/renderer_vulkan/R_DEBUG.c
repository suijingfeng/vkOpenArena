#include "tr_globals.h"
#include "../renderercommon/ref_import.h"
#include "vk_shade_geometry.h"
#include "vk_instance.h"
#include "vk_image.h"
#include "vk_pipelines.h"
#include "tr_cvar.h"


void R_DebugPolygon( int color, int numPoints, float *points )
{

	if (numPoints < 3 || numPoints >= SHADER_MAX_VERTEXES/2)
		return;
    int i;
	// In Vulkan we don't have GL_POLYGON + GLS_POLYMODE_LINE equivalent, 
    // so we use lines to draw polygon outlines.This approach has additional
    // implication that we need to do manual backface culling to reject outlines
    // that belong to back facing polygons. The code assumes that polygons are convex.

	// Backface culling.
    float pa[3], pb[3];
//	transform_to_eye_space(&points[0], pa);
    
    const float* m = getptr_modelview_matrix();
    

    pa[0] = m[0]*points[0] + m[4]*points[1] + m[8 ]*points[2] + points[12];
    pa[1] = m[1]*points[0] + m[5]*points[1] + m[9 ]*points[2] + points[13];
    pa[2] = m[2]*points[0] + m[6]*points[1] + m[10]*points[2] + points[14];
	
    
//   transform_to_eye_space(&points[3], pb);
    {
        float *v = &points[3];
        float *v_eye = pb;
        v_eye[0] = m[0]*v[0] + m[4]*v[1] + m[8 ]*v[2] + m[12];
		v_eye[1] = m[1]*v[0] + m[5]*v[1] + m[9 ]*v[2] + m[13];
		v_eye[2] = m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14];
    }
	float p[3];
	VectorSubtract(pb, pa, p);
	float n[3];
	for (i = 2; i < numPoints; i++)
    {
		//transform_to_eye_space(&points[3*i], pb);
        {
            float *v = &points[3*i];
            float *v_eye = pb;

            v_eye[0] = m[0]*v[0] + m[4]*v[1] + m[8 ]*v[2] + m[12];
		    v_eye[1] = m[1]*v[0] + m[5]*v[1] + m[9 ]*v[2] + m[13];
		    v_eye[2] = m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14];
        }

		float q[3];
		VectorSubtract(pb, pa, q);
		CrossProduct(q, p, n);
		if (VectorLength(n) > 1e-5)
			break;
	}
	if (DotProduct(n, pa) >= 0)
		return; // discard backfacing polygon

	// Solid shade.
	for (i = 0; i < numPoints; i++)
    {
		VectorCopy(&points[3*i], tess.xyz[i]);

		tess.svars.colors[i][0] = (color&1) ? 255 : 0;
		tess.svars.colors[i][1] = (color&2) ? 255 : 0;
		tess.svars.colors[i][2] = (color&4) ? 255 : 0;
		tess.svars.colors[i][3] = 255;
	}
	tess.numVertexes = numPoints;

	tess.numIndexes = 0;
	for (i = 1; i < numPoints - 1; i++)
    {
		tess.indexes[tess.numIndexes + 0] = 0;
		tess.indexes[tess.numIndexes + 1] = i;
		tess.indexes[tess.numIndexes + 2] = i + 1;
		tess.numIndexes += 3;
	}



    vk_bind_geometry();
    vk_shade_geometry(g_stdPipelines.surface_debug_pipeline_solid, VK_FALSE, DEPTH_RANGE_NORMAL, VK_TRUE);


	// Outline.
	memset(tess.svars.colors, tr.identityLightByte, numPoints * 2 * sizeof(color4ub_t));

	for (i = 0; i < numPoints; i++)
    {
		VectorCopy(&points[3*i], tess.xyz[2*i]);
		VectorCopy(&points[3*((i + 1) % numPoints)], tess.xyz[2*i + 1]);
	}
	tess.numVertexes = numPoints * 2;
	tess.numIndexes = 0;


    vk_bind_geometry();
    vk_shade_geometry(g_stdPipelines.surface_debug_pipeline_outline, VK_FALSE, DEPTH_RANGE_ZERO, VK_FALSE);
	
    tess.numVertexes = 0;
}

/*
====================
R_DebugGraphics

Visualization aid for movement clipping debugging
====================
*/
void R_DebugGraphics( void )
{
	// the render thread can't make callbacks to the main thread
	if ( tr.registered ) {
		R_IssueRenderCommands( qfalse );
	}

	updateCurDescriptor( tr.whiteImage->descriptor_set, 0);
	ri.CM_DrawDebugSurface( R_DebugPolygon );
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever was there.
This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/

void RB_ShowImages(void)
{

    backEnd.projection2D = qtrue;

	const float black[4] = {0, 0, 0, 1};
	vk_clearColorAttachments(black);
    
    uint32_t i;
	for (i = 0 ; i < tr.numImages ; i++)
    {
		image_t* image = tr.images[i];

		float w = glConfig.vidWidth / 20;
		float h = glConfig.vidHeight / 15;
		float x = i % 20 * w;
		float y = i / 20 * h;


		memset( tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4 );

		tess.numIndexes = 6;
		tess.numVertexes = 4;

		tess.indexes[0] = 0;
		tess.indexes[1] = 1;
		tess.indexes[2] = 2;
		tess.indexes[3] = 0;
		tess.indexes[4] = 2;
		tess.indexes[5] = 3;

		tess.xyz[0][0] = x;
		tess.xyz[0][1] = y;
		tess.svars.texcoords[0][0][0] = 0;
		tess.svars.texcoords[0][0][1] = 0;

		tess.xyz[1][0] = x + w;
		tess.xyz[1][1] = y;
		tess.svars.texcoords[0][1][0] = 1;
		tess.svars.texcoords[0][1][1] = 0;

		tess.xyz[2][0] = x + w;
		tess.xyz[2][1] = y + h;
		tess.svars.texcoords[0][2][0] = 1;
		tess.svars.texcoords[0][2][1] = 1;

		tess.xyz[3][0] = x;
		tess.xyz[3][1] = y + h;
		tess.svars.texcoords[0][3][0] = 0;
		tess.svars.texcoords[0][3][1] = 1;
		
        
        updateCurDescriptor( image->descriptor_set, 0);
        vk_bind_geometry();
        vk_shade_geometry(g_stdPipelines.images_debug_pipeline, VK_FALSE, DEPTH_RANGE_NORMAL, VK_TRUE);

	}
	tess.numIndexes = 0;
	tess.numVertexes = 0;
    
    backEnd.projection2D = qfalse;
}

/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
void DrawTris (shaderCommands_t *input)
{
	updateCurDescriptor( tr.whiteImage->descriptor_set, 0);

	// VULKAN

    memset(tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4 );
    VkPipeline pipeline = backEnd.viewParms.isMirror ? g_stdPipelines.tris_mirror_debug_pipeline : g_stdPipelines.tris_debug_pipeline;
    vk_shade_geometry(pipeline, VK_FALSE, DEPTH_RANGE_ZERO, VK_FALSE);

}


/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
void DrawNormals (shaderCommands_t *input)
{
	// VULKAN

    vec4_t xyz[SHADER_MAX_VERTEXES];
    memcpy(xyz, tess.xyz, tess.numVertexes * sizeof(vec4_t));
    memset(tess.svars.colors, tr.identityLightByte, SHADER_MAX_VERTEXES * sizeof(color4ub_t));

    int numVertexes = tess.numVertexes;
    int i = 0;
    while (i < numVertexes)
    {
        int count = numVertexes - i;
        if (count >= SHADER_MAX_VERTEXES/2 - 1)
            count = SHADER_MAX_VERTEXES/2 - 1;

        int k;
        for (k = 0; k < count; k++)
        {
            VectorCopy(xyz[i + k], tess.xyz[2*k]);
            VectorMA(xyz[i + k], 2, input->normal[i + k], tess.xyz[2*k + 1]);
        }
        tess.numVertexes = 2 * count;
        tess.numIndexes = 0;


        vk_bind_geometry();
        vk_shade_geometry(g_stdPipelines.normals_debug_pipeline, VK_FALSE, DEPTH_RANGE_ZERO, VK_TRUE);

        i += count;
    }

}
