
#include "tr_local.h"


extern Dx_Instance	dx;

extern float * R_GetModelViewMatPtr(void);
/*
================
R_DebugPolygon
================
*/
void R_DebugPolygon(int color, int numPoints, float *points)
{
	GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

	// draw solid shade

	qglColor3f(color & 1, (color >> 1) & 1, (color >> 2) & 1);
	qglBegin(GL_POLYGON);
	for (int i = 0; i < numPoints; i++) {
		qglVertex3fv(points + i * 3);
	}
	qglEnd();

	// draw wireframe outline
	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	qglDepthRange(0, 0);
	qglColor3f(1, 1, 1);
	qglBegin(GL_POLYGON);
	for (int i = 0; i < numPoints; i++) {
		qglVertex3fv(points + i * 3);
	}
	qglEnd();
	qglDepthRange(0, 1);


	// DX12
	if (!dx.active)
		return;
	if (numPoints < 3 || numPoints >= SHADER_MAX_VERTEXES / 2)
		return;

	// In Vulkan we don't have GL_POLYGON + GLS_POLYMODE_LINE equivalent, so we use lines to draw polygon outlines.
	// This approach has additional implication that we need to do manual backface culling to reject outlines that
	// belong to back facing polygons.
	// The code assumes that polygons are convex.

	// Backface culling.
	auto transform_to_eye_space = [](vec3_t v, vec3_t v_eye)
	{
		float* m = R_GetModelViewMatPtr();
		v_eye[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12];
		v_eye[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13];
		v_eye[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
	};
	vec3_t pa;
	vec3_t pb;
	transform_to_eye_space(&points[0], pa);
	transform_to_eye_space(&points[3], pb);
	vec3_t p;
	VectorSubtract(pb, pa, p);
	vec3_t n;
	for (int i = 2; i < numPoints; i++) {
		transform_to_eye_space(&points[3 * i], pb);
		vec3_t q;
		VectorSubtract(pb, pa, q);
		CrossProduct(q, p, n);
		if (VectorLength(n) > 1e-5)
			break;
	}
	if (DotProduct(n, pa) >= 0)
		return; // discard backfacing polygon

	// Solid shade.
	for (int i = 0; i < numPoints; i++) {
		VectorCopy(&points[3 * i], tess.xyz[i]);

		tess.svars.colors[i][0] = (color & 1) ? 255 : 0;
		tess.svars.colors[i][1] = (color & 2) ? 255 : 0;
		tess.svars.colors[i][2] = (color & 4) ? 255 : 0;
		tess.svars.colors[i][3] = 255;
	}
	tess.numVertexes = numPoints;

	tess.numIndexes = 0;
	for (int i = 1; i < numPoints - 1; i++) {
		tess.indexes[tess.numIndexes + 0] = 0;
		tess.indexes[tess.numIndexes + 1] = i;
		tess.indexes[tess.numIndexes + 2] = i + 1;
		tess.numIndexes += 3;
	}


	if (dx.active) {
		dx_bind_geometry();
		dx_shade_geometry(dx.surface_debug_pipeline_solid, false, DX_Depth_Range::normal, true, false);
	}

	// Outline.
	memset(tess.svars.colors, tr.identityLightByte, numPoints * 2 * sizeof(color4ub_t));

	for (int i = 0; i < numPoints; i++) {
		VectorCopy(&points[3 * i], tess.xyz[2 * i]);
		VectorCopy(&points[3 * ((i + 1) % numPoints)], tess.xyz[2 * i + 1]);
	}
	tess.numVertexes = numPoints * 2;
	tess.numIndexes = 0;


	if (dx.active) {
		dx_bind_geometry();
		dx_shade_geometry(dx.surface_debug_pipeline_outline, false, DX_Depth_Range::force_zero, false, true);
	}

	tess.numVertexes = 0;
}


/*
====================
R_DebugGraphics

Visualization aid for movement clipping debugging
====================
*/
void R_DebugGraphics(void)
{
	// the render thread can't make callbacks to the main thread
	R_SyncRenderThread();

	GL_Bind(tr.whiteImage);
	GL_Cull(CT_FRONT_SIDED);
	ri.CM_DrawDebugSurface(R_DebugPolygon);
}