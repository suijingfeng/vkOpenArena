#include "tr_globals.h"

#include "ref_import.h"
#include "tr_backend.h"
#include "tr_surface.h" // RB_AddQuadStampExt
#include "tr_noise.h" // R_NoiseGet4f
#include "RB_DeformGeometry.h" 

/*
====================================================================

DEFORMATIONS

====================================================================
*/

/*
=========================
Wiggle the normals for wavy environment mapping
=========================
*/
static void RB_CalcDeformNormals( deformStage_t * const ds, const uint32_t nVert, 
        float (* const pXYZ)[4], float (* const pNorm)[4], const float time )
{
	uint32_t i;

	for ( i = 0; i < nVert; ++i)
    {
		float scale = 0.98f;
		scale = R_NoiseGet4f( pXYZ[i][0] * scale, pXYZ[i][1] * scale, pXYZ[i][2] * scale,
			time * ds->deformationWave.frequency );
		pNorm[i][0] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 100 + pXYZ[i][0] * scale, pXYZ[i][1] * scale, pXYZ[i][2] * scale,
			time * ds->deformationWave.frequency );
		pNorm[i][1] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 200 + pXYZ[i][0] * scale, pXYZ[i][1] * scale, pXYZ[i][2] * scale,
			time * ds->deformationWave.frequency );
		pNorm[i][2] += ds->deformationWave.amplitude * scale;

		VectorNorm( pNorm[i] );
	}
}


float RB_WaveValue(enum GenFunc_T func, const float base, 
        const float Amplitude, const float phase, const float freq )
{
    // give a default value for spress warning
    // of may be used uninitialized ...
    // what about GF_NOISE ???

    float x = ( phase + freq * R_GetTessShaderTime() ) * FUNCTABLE_SIZE;
    uint32_t idx =  (uint32_t)( x ) & FUNCTABLE_MASK;

    switch ( func )
	{
	    case GF_SIN:
            return (base + Amplitude * tr.sinTable[ idx ]);
	    case GF_TRIANGLE:
            return (base + Amplitude * tr.triangleTable[ idx ]);
	    case GF_SQUARE:
            return (base + Amplitude * tr.squareTable[ idx ]);
	    case GF_SAWTOOTH:
            return (base + Amplitude * tr.sawToothTable[ idx ]);
	    case GF_INVERSE_SAWTOOTH:
            return (base + Amplitude * tr.inverseSawToothTable[ idx ]);
        case GF_NOISE:
            return (base + Amplitude * R_NoiseGet4f( 0.0f, 0.0f, 0.0f, x ));
        case GF_NONE:
        default:
			ri.Error( ERR_DROP, "RB_WaveValue called with invalid function '%d' \n", func);
            break;
	}

    return 1.0f;
}


static void RB_CalcDeformVertexes( deformStage_t *ds, const uint32_t nVert, 
        float (* const pXYZ)[4], float (* const pNorm)[4] )
{
/*
	if ( ds->deformationWave.frequency == 0 )
	{
		float scale = EvalWaveForm( &ds->deformationWave );
	    uint32_t i;
		for (i = 0; i < nVert; ++i)
		{
            float offset[3];

			VectorScale( pNorm[i], scale, offset );
			
			pXYZ[i][0] += offset[0];
			pXYZ[i][1] += offset[1];
			pXYZ[i][2] += offset[2];
		}
	}
	else
	{
*/
	    uint32_t i;
		for ( i = 0; i < nVert; ++i)
		{
			float off = ( pXYZ[i][0] + pXYZ[i][1] + pXYZ[i][2] ) * ds->deformationSpread;

			float scale = RB_WaveValue( ds->deformationWave.func, ds->deformationWave.base, 
				ds->deformationWave.amplitude,
				ds->deformationWave.phase + off,
				ds->deformationWave.frequency );
	        
            float offset[3];

			VectorScale( pNorm[i], scale, offset );
			
			pXYZ[i][0] += offset[0];
			pXYZ[i][1] += offset[1];
			pXYZ[i][2] += offset[2];
		}
//	}
}


static void RB_CalcBulgeVertexes( deformStage_t * const ds, const uint32_t nVert, 
        float (* const pXYZ)[4], const float (* const pNorm)[4],
        const float (* const pTexCoords)[2][2])
{
	// ri.Printf(PRINT_WARNING, "%d vert\n", nVert);
    uint32_t i;
    // original backEnd.refdef.time * 0.001f
	float now = R_GetRefFloatTime() * ds->bulgeSpeed ;

	for ( i = 0; i < nVert; ++i)
    {
		int off = (float)( FUNCTABLE_SIZE / (M_PI*2) ) * ( pTexCoords[i][0][0] * ds->bulgeWidth + now );

		float scale = tr.sinTable[ off & FUNCTABLE_MASK ] * ds->bulgeHeight;
			
		pXYZ[i][0] += pNorm[i][0] * scale;
		pXYZ[i][1] += pNorm[i][1] * scale;
		pXYZ[i][2] += pNorm[i][2] * scale;
	}
}


/*
======================
A deformation that can move an entire surface along a wave path
======================
*/
static void RB_CalcMoveVertexes( deformStage_t * const ds, const uint32_t nVert, float (* const pXyz)[4])
{
	float scale = RB_WaveValue( ds->deformationWave.func, ds->deformationWave.base, 
		ds->deformationWave.amplitude,
		ds->deformationWave.phase,
		ds->deformationWave.frequency );

    float offset[3];
	VectorScale( ds->moveVector, scale, offset );

    uint32_t i;

	for ( i = 0; i < nVert; ++i )
    {
		// VectorAdd( pXyz[i], offset, pXyz[i] );
        pXyz[i][0] += offset[0];
        pXyz[i][1] += offset[1];
        pXyz[i][2] += offset[2];
	}
}



static void RB_ProjectionShadowDeform( uint32_t const nVert, float (* const xyz)[4] )
{
	vec3_t	ground;
	vec3_t	light;
	vec3_t	lightDir;

	ground[0] = backEnd.or.axis[0][2];
	ground[1] = backEnd.or.axis[1][2];
	ground[2] = backEnd.or.axis[2][2];

	float groundDist = backEnd.or.origin[2] - backEnd.currentEntity->e.shadowPlane;

	VectorCopy( backEnd.currentEntity->lightDir, lightDir );
	
    float d = DotProduct( lightDir, ground );
	// don't let the shadows get too long or go negative
	if ( d < 0.5 )
    {
		VectorMA( lightDir, (0.5 - d), ground, lightDir );
		d = DotProduct( lightDir, ground );
	}
	d = 1.0 / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	uint32_t i;
	for ( i = 0; i < nVert; ++i )
    {
		float h = DotProduct( xyz[i], ground ) + groundDist;

		xyz[i][0] -= light[0] * h;
		xyz[i][1] -= light[1] * h;
		xyz[i][2] -= light[2] * h;
	}
}


static void GlobalVectorToLocal( const vec3_t in, vec3_t out )
{
	out[0] = DotProduct( in, backEnd.or.axis[0] );
	out[1] = DotProduct( in, backEnd.or.axis[1] );
	out[2] = DotProduct( in, backEnd.or.axis[2] );
}

/*
=====================
Assuming all the triangles for this shader are independant
quads, rebuild them as forward facing sprites
=====================
*/
static void AutospriteDeform( shaderCommands_t * const pTess, trRefEntity_t * const pCurEntity)
{
    int		i;

    vec3_t	left, up;
    vec3_t	leftDir, upDir;
    
    uint32_t oldVerts = pTess->numVertexes;

    if ( pTess->numVertexes & 3 )
    {
        ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd vertex count.\n", pTess->shader->name );
    }
    if ( pTess->numIndexes != ( pTess->numVertexes >> 2 ) * 6 ) {
        ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd index count. \n", pTess->shader->name );
    }

    pTess->numVertexes = 0;
    pTess->numIndexes = 0;

    if ( pCurEntity != &tr.worldEntity )
    {
        GlobalVectorToLocal( backEnd.viewParms.or.axis[1], leftDir );
        GlobalVectorToLocal( backEnd.viewParms.or.axis[2], upDir );
    }
    else
    {
        VectorCopy( backEnd.viewParms.or.axis[1], leftDir );
        VectorCopy( backEnd.viewParms.or.axis[2], upDir );
    }


    // constant normal all the way around
    float normal[3] = { -backEnd.viewParms.or.axis[0][0],
        -backEnd.viewParms.or.axis[0][1],
        -backEnd.viewParms.or.axis[0][2]};


    // ri.Printf( PRINT_WARNING, "AutospriteDeform: %d\n", oldVerts);
    float axisLength = pCurEntity->e.nonNormalizedAxes ? 
        1.0f / VectorLen( backEnd.currentEntity->e.axis[0] ) : 1.0f;
    float lr_factor = backEnd.viewParms.isMirror ? -1.0f : 1.0f;

    for ( i = 0; i < oldVerts; i+=4 )
    {
        // find the midpoint
        float origin[3] = {
            0.25f * (pTess->xyz[i][0] + pTess->xyz[i+1][0] + pTess->xyz[i+2][0] + pTess->xyz[i+3][0]),
            0.25f * (pTess->xyz[i][1] + pTess->xyz[i+1][1] + pTess->xyz[i+2][1] + pTess->xyz[i+3][1]),
            0.25f * (pTess->xyz[i][2] + pTess->xyz[i+1][2] + pTess->xyz[i+2][2] + pTess->xyz[i+3][2]) };

        float vx = pTess->xyz[i][0] - origin[0];
        float vy = pTess->xyz[i][1] - origin[1];
        float vz = pTess->xyz[i][2] - origin[2];
        
        float radius = axisLength * sqrtf(vx*vx + vy*vy + vz*vz) ;	// / sqrt(2)
        float radius_l = radius * lr_factor;
        
        VectorScale( leftDir, radius_l, left );
        VectorScale( upDir, radius, up );

        //
        // AddQuadStampExt( mid, left, up, pTess->vertexColors[i], 0.0f, 0.0f, 1.0f, 1.0f );
        // void AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, uint8_t * const color,
        // float s1, float t1, float s2, float t2 )
        // 
        uint32_t ndx0 = pTess->numVertexes;
        uint32_t ndx1 = ndx0 + 1;
        uint32_t ndx2 = ndx0 + 2;
        uint32_t ndx3 = ndx0 + 3;

        // triangle indexes for a simple quad
        pTess->indexes[ pTess->numIndexes ] = ndx0;
        pTess->indexes[ pTess->numIndexes + 1 ] = ndx1;
        pTess->indexes[ pTess->numIndexes + 2 ] = ndx3;

        pTess->indexes[ pTess->numIndexes + 3 ] = ndx3;
        pTess->indexes[ pTess->numIndexes + 4 ] = ndx1;
        pTess->indexes[ pTess->numIndexes + 5 ] = ndx2;

        pTess->numVertexes += 4;
        pTess->numIndexes += 6;


        pTess->xyz[ndx0][0] = origin[0] + left[0] + up[0];
        pTess->xyz[ndx0][1] = origin[1] + left[1] + up[1];
        pTess->xyz[ndx0][2] = origin[2] + left[2] + up[2];

        pTess->xyz[ndx1][0] = origin[0] - left[0] + up[0];
        pTess->xyz[ndx1][1] = origin[1] - left[1] + up[1];
        pTess->xyz[ndx1][2] = origin[2] - left[2] + up[2];

        pTess->xyz[ndx2][0] = origin[0] - left[0] - up[0];
        pTess->xyz[ndx2][1] = origin[1] - left[1] - up[1];
        pTess->xyz[ndx2][2] = origin[2] - left[2] - up[2];

        pTess->xyz[ndx3][0] = origin[0] + left[0] - up[0];
        pTess->xyz[ndx3][1] = origin[1] + left[1] - up[1];
        pTess->xyz[ndx3][2] = origin[2] + left[2] - up[2];


        pTess->normal[ndx0][0] = normal[0];
        pTess->normal[ndx0][1] = normal[1];
        pTess->normal[ndx0][2] = normal[2];

        pTess->normal[ndx1][0] = normal[0];
        pTess->normal[ndx1][1] = normal[1];
        pTess->normal[ndx1][2] = normal[2];

        pTess->normal[ndx2][0] = normal[0];
        pTess->normal[ndx2][1] = normal[1];
        pTess->normal[ndx2][2] = normal[2];

        pTess->normal[ndx3][0] = normal[0];
        pTess->normal[ndx3][1] = normal[1];
        pTess->normal[ndx3][2] = normal[2];

        // standard square texture coordinates
        pTess->texCoords[ndx0][0][0] = 0.0f;
        pTess->texCoords[ndx0][1][0] = 0.0f;
        pTess->texCoords[ndx0][0][1] = 0.0f;
        pTess->texCoords[ndx0][1][1] = 0.0f;

        pTess->texCoords[ndx1][0][0] = 1.0f;
        pTess->texCoords[ndx1][1][0] = 1.0f;
        pTess->texCoords[ndx1][0][1] = 0.0f;
        pTess->texCoords[ndx1][1][1] = 0.0f;

        pTess->texCoords[ndx2][0][0] = 1.0f;
        pTess->texCoords[ndx2][1][0] = 1.0f;
        pTess->texCoords[ndx2][0][1] = 1.0f;
        pTess->texCoords[ndx2][1][1] = 1.0f;

        pTess->texCoords[ndx3][0][0] = 0.0f;
        pTess->texCoords[ndx3][1][0] = 0.0f;
        pTess->texCoords[ndx3][0][1] = 1.0f;
        pTess->texCoords[ndx3][1][1] = 1.0f;

        // Vertex color need not to be change
    }
}



/*
=====================
Autosprite2Deform

Autosprite2 will pivot a rectangular quad along the center of its long axis
=====================
*/
const int edgeVerts[6][2] = {
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};


static void Autosprite2Deform( shaderCommands_t * const pTess )
{
	int		i, j, k;
	int		indexes;
	float	*xyz;
	vec3_t	forward;

	if ( pTess->numVertexes & 3 ) {
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd vertex count",
                pTess->shader->name );
	}
	if ( pTess->numIndexes != ( pTess->numVertexes >> 2 ) * 6 ) {
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd index count",
                pTess->shader->name );
	}


	if ( backEnd.currentEntity != &tr.worldEntity ) {
		GlobalVectorToLocal( backEnd.viewParms.or.axis[0], forward );
	} else {
		VectorCopy( backEnd.viewParms.or.axis[0], forward );
	}

	// this is a lot of work for two triangles...
	// we could precalculate a lot of it is an issue, but it would mess up
	// the shader abstraction
	for ( i = 0, indexes = 0 ; i < pTess->numVertexes ; i+=4, indexes+=6 )
    {
		float	lengths[2];
		int		nums[2];
		vec3_t	mid[2];
		vec3_t	major, minor;

		// find the midpoint
		xyz = pTess->xyz[i];

		// identify the two shortest edges
		nums[0] = nums[1] = 0;
		lengths[0] = lengths[1] = 999999;

		for ( j = 0 ; j < 6 ; ++j )
        {
			vec3_t	temp;

			float* v1 = xyz + 4 * edgeVerts[j][0];
			float* v2 = xyz + 4 * edgeVerts[j][1];

			VectorSubtract( v1, v2, temp );
			
			float l = DotProduct( temp, temp );
			if ( l < lengths[0] ) {
				nums[1] = nums[0];
				lengths[1] = lengths[0];
				nums[0] = j;
				lengths[0] = l;
			} else if ( l < lengths[1] ) {
				nums[1] = j;
				lengths[1] = l;
			}
		}

		for ( j = 0 ; j < 2 ; ++j )
        {
			float* v1 = xyz + 4 * edgeVerts[nums[j]][0];
			float* v2 = xyz + 4 * edgeVerts[nums[j]][1];

			mid[j][0] = 0.5f * (v1[0] + v2[0]);
			mid[j][1] = 0.5f * (v1[1] + v2[1]);
			mid[j][2] = 0.5f * (v1[2] + v2[2]);
		}

		// find the vector of the major axis
		VectorSubtract( mid[1], mid[0], major );

		// cross this with the view direction to get minor axis
		VectorCross( major, forward, minor );
		VectorNormalize( minor );
		
		// re-project the points
		for ( j = 0 ; j < 2 ; ++j )
        {
			float* v1 = xyz + 4 * edgeVerts[nums[j]][0];
			float* v2 = xyz + 4 * edgeVerts[nums[j]][1];

			float l = 0.5 * sqrt( lengths[j] );
			
			// we need to see which direction this edge
			// is used to determine direction of projection
			for ( k = 0 ; k < 5 ; k++ ) {
				if ( pTess->indexes[ indexes + k ] == i + edgeVerts[nums[j]][0]
					&& pTess->indexes[ indexes + k + 1 ] == i + edgeVerts[nums[j]][1] ) {
					break;
				}
			}

			if ( k == 5 ) {
				VectorMA( mid[j], l, minor, v1 );
				VectorMA( mid[j], -l, minor, v2 );
			} else {
				VectorMA( mid[j], -l, minor, v1 );
				VectorMA( mid[j], l, minor, v2 );
			}
		}
	}
}

/*
=============
Change a polygon into a bunch of text polygons
=============
*/
static void RB_DeformText( const char * const text, shaderCommands_t * const pTess )
{
	vec3_t	origin, width, height;
	byte	color[4];
	float	bottom, top;
	vec3_t	mid;

	height[0] = 0;
	height[1] = 0;
	height[2] = -1;
	VectorCross( pTess->normal[0], height, width );

	// find the midpoint of the box
	VectorClear( mid );
	bottom = 999999;
	top = -999999;
	for (int i = 0 ; i < 4 ; ++i )
    {
		VectorAdd( pTess->xyz[i], mid, mid );
		if ( pTess->xyz[i][2] < bottom ) {
			bottom = pTess->xyz[i][2];
		}
		if ( pTess->xyz[i][2] > top ) {
			top = pTess->xyz[i][2];
		}
	}
	VectorScale( mid, 0.25f, origin );

	// determine the individual character size
	height[0] = 0;
	height[1] = 0;
	height[2] = ( top - bottom ) * 0.5f;

	VectorScale( width, height[2] * -0.75f, width );

	// determine the starting position
	const unsigned int len = (unsigned int)strlen( text );
	VectorMA( origin, (len-1), width, origin );

	// clear the shader indexes
	pTess->numIndexes = 0;
	pTess->numVertexes = 0;

	color[0] = color[1] = color[2] = color[3] = 255;

	// draw each character
	for (unsigned int i = 0 ; i < len ; ++i )
    {
		int8_t ch = text[i];
		
		if ( ch != ' ' )
        {
			
			float frow = (ch >> 4 )* 0.0625f;
			float fcol = (ch & 15) * 0.0625f;
			float size = 0.0625f;

			RB_AddQuadStampExt( origin, width, height, color, fcol, frow, fcol + size, frow + size );
		}
		VectorMA( origin, -2, width, origin );
	}
}


void RB_DeformTessGeometry( shaderCommands_t * const pTess)
{
    const uint32_t nDeforms = pTess->shader->numDeforms;
    deformStage_t* const pDs = pTess->shader->deforms;
    
    uint32_t i;
	for ( i = 0; i < nDeforms; ++i )
    {
		//deformStage_t* ds = &pTess->shader->deforms[ i ];

		switch ( pDs[i].deformation )
        {
        case DEFORM_NONE:
            break;
		case DEFORM_NORMALS:
			RB_CalcDeformNormals( &pDs[i], pTess->numVertexes, pTess->xyz,
                    pTess->normal, pTess->shaderTime);
			break;
		case DEFORM_WAVE:
			RB_CalcDeformVertexes( &pDs[i], pTess->numVertexes, pTess->xyz, pTess->normal);
			break;
		case DEFORM_BULGE:
			RB_CalcBulgeVertexes( &pDs[i], pTess->numVertexes, pTess->xyz, pTess->normal, pTess->texCoords );
			break;
		case DEFORM_MOVE:
			RB_CalcMoveVertexes( &pDs[i], pTess->numVertexes, pTess->xyz);
			break;
		case DEFORM_PROJECTION_SHADOW:
			RB_ProjectionShadowDeform( pTess->numVertexes, pTess->xyz );
			break;
		case DEFORM_AUTOSPRITE:
			AutospriteDeform( pTess, backEnd.currentEntity);
			break;
		case DEFORM_AUTOSPRITE2:
			Autosprite2Deform( pTess );
			break;
		case DEFORM_TEXT0:
		case DEFORM_TEXT1:
		case DEFORM_TEXT2:
		case DEFORM_TEXT3:
		case DEFORM_TEXT4:
		case DEFORM_TEXT5:
		case DEFORM_TEXT6:
		case DEFORM_TEXT7:
			RB_DeformText( backEnd.refdef.text[pDs->deformation - DEFORM_TEXT0], pTess );
			break;
		}
	}
}
